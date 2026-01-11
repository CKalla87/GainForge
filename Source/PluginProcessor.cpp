/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// AmpEmulator Implementation
//==============================================================================

GainForgeAudioProcessor::AmpEmulator::AmpEmulator()
{
    // Initialize smoothed values
    smoothedGain.reset (44100.0, 0.05);
    smoothedBass.reset (44100.0, 0.05);
    smoothedMid.reset (44100.0, 0.05);
    smoothedTreble.reset (44100.0, 0.05);
    smoothedPresence.reset (44100.0, 0.05);
    smoothedMaster.reset (44100.0, 0.05);
    smoothedDrive.reset (44100.0, 0.05);
    smoothedRectifierMode.reset (44100.0, 0.1);
    rectifierSagState = 0.0f;
}

void GainForgeAudioProcessor::AmpEmulator::prepare (double sampleRate, int maxBlockSize)
{
    currentSampleRate = sampleRate;
    
    // Prepare filters
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (maxBlockSize);
    spec.numChannels = 1;
    
    bassFilter.prepare (spec);
    midFilter.prepare (spec);
    trebleFilter.prepare (spec);
    presenceFilter.prepare (spec);
    
    // Reset smoothed values (prevents loud pops on load - default parameters are now 0.0)
    smoothedGain.reset (sampleRate, 0.05);
    smoothedBass.reset (sampleRate, 0.05);
    smoothedMid.reset (sampleRate, 0.05);
    smoothedTreble.reset (sampleRate, 0.05);
    smoothedPresence.reset (sampleRate, 0.05);
    smoothedMaster.reset (sampleRate, 0.05);
    smoothedDrive.reset (sampleRate, 0.05);
    smoothedRectifierMode.reset (sampleRate, 0.1);
    rectifierSagState = 0.0f;
    
    // Initialize filters with safe defaults (EQ at neutral, gain-related at 0.0)
    updateFilters (0.5f, 0.5f, 0.5f, 0.5f);
}

void GainForgeAudioProcessor::AmpEmulator::reset()
{
    bassFilter.reset();
    midFilter.reset();
    trebleFilter.reset();
    presenceFilter.reset();
}

void GainForgeAudioProcessor::AmpEmulator::updateFilters (float bass, float mid, float treble, float presence)
{
    // Mesa Boogie Triple Rectifier tone stack frequencies (authentic values)
    // Bass: Low shelf at 80Hz - very powerful low end
    auto bassCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
        currentSampleRate, 80.0, 0.707, 
        juce::jmap (bass, 0.12f, 4.2f) // 0.0 -> 0.12x, 1.0 -> 4.2x (massive bass boost capability)
    );
    *bassFilter.coefficients = *bassCoeffs;
    
    // Mid: Peaking at 800Hz with wider Q for classic scooped mids (Rectifier signature)
    // Rectifier mids can go very low (scooped) - this is the key to the Rectifier sound
    auto midCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        currentSampleRate, 800.0, 0.65, // Wider Q (0.65) for more pronounced scoop
        juce::jmap (mid, 0.08f, 2.4f) // 0.0 -> 0.08x (very scooped), 1.0 -> 2.4x
    );
    *midFilter.coefficients = *midCoeffs;
    
    // Treble: High shelf at 2500Hz - bright and cutting
    auto trebleCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        currentSampleRate, 2500.0, 0.707,
        juce::jmap (treble, 0.18f, 2.8f) // 0.0 -> 0.18x, 1.0 -> 2.8x (bright)
    );
    *trebleFilter.coefficients = *trebleCoeffs;
    
    // Presence: High shelf at 5500Hz - articulation and high-end clarity
    auto presenceCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        currentSampleRate, 5500.0, 0.707,
        juce::jmap (presence, 0.15f, 2.6f) // 0.0 -> 0.15x, 1.0 -> 2.6x
    );
    *presenceFilter.coefficients = *presenceCoeffs;
}

float GainForgeAudioProcessor::AmpEmulator::applyPreampStage (float input, float stageGain, int stageNumber)
{
    // Triple Rectifier cascading preamp stages - tight, aggressive saturation
    // Each stage progressively adds more saturation and compression
    float output = input * stageGain;
    
    // Rectifier preamp stages: aggressive, tight saturation with harmonic content
    // Later stages are more compressed and aggressive
    float saturationAmount = 1.0f + stageNumber * 0.35f;
    
    // Asymmetric tube saturation (Rectifier style - tight and aggressive)
    // More compression on positive cycle, creates characteristic Rectifier grind
    if (output > 0.0f)
        output = std::tanh (output * saturationAmount * 1.8f) * 0.58f; // Tighter, more aggressive
    else
        output = std::tanh (output * saturationAmount * 1.4f) * 0.65f;
    
    return output;
}

float GainForgeAudioProcessor::AmpEmulator::applyRectifierSaturation (float input, float drive, float rectifierMode)
{
    // Triple Rectifier rectification: Silicon Diode (tight) vs Tube Rectifier (saggy)
    // Silicon Diode mode (0.0): Tighter, faster attack, more aggressive
    // Tube Rectifier mode (1.0): Softer attack, more sag, vintage feel
    
    float driven = input * (1.0f + drive * 14.0f); // High gain range
    
    if (rectifierMode < 0.5f) // Silicon Diode mode - tight and aggressive
    {
        // Tighter, harder clipping - signature Rectifier tightness
        driven = std::tanh (driven * 3.0f) * 0.48f; // More aggressive clipping
    }
    else // Tube Rectifier mode - saggy and compressed
    {
        // Simulate rectifier sag (voltage drop under load - characteristic tube rectifier behavior)
        float sagAmount = std::abs (driven) * 0.18f;
        rectifierSagState = rectifierSagState * 0.94f + sagAmount * 0.06f; // More sag response
        driven *= (1.0f - rectifierSagState * 0.35f); // More voltage sag effect
        
        // Softer tube rectifier saturation - more vintage feel
        driven = std::tanh (driven * 2.2f) * 0.52f;
    }
    
    return driven;
}

void GainForgeAudioProcessor::AmpEmulator::processBlock (juce::AudioBuffer<float>& buffer, 
                                                          float gain, float bass, float mid, float treble, 
                                                          float presence, float master, float drive, float rectifierMode,
                                                          float voice, float mode)
{
    if (buffer.getNumSamples() == 0)
        return;
    
    // Update smoothed values
    smoothedGain.setTargetValue (gain);
    smoothedBass.setTargetValue (bass);
    smoothedMid.setTargetValue (mid);
    smoothedTreble.setTargetValue (treble);
    smoothedPresence.setTargetValue (presence);
    smoothedMaster.setTargetValue (master);
    smoothedDrive.setTargetValue (drive);
    smoothedRectifierMode.setTargetValue (rectifierMode);
    
    // Update filter coefficients at the start of the block
    updateFilters (bass, mid, treble, presence);
    
    // Create DSP audio block
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    
    // Process each sample for gain, drive, and master (these need per-sample smoothing)
    auto* channelData = buffer.getWritePointer (0);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float input = channelData[sample];
        
        // Triple Rectifier gain staging: Multiple cascading preamp stages
        float currentGain = smoothedGain.getNextValue();
        // Increased gain range: 0.3x to 20x (more headroom for high gain)
        float gainAmount = 0.3f + currentGain * 19.7f;
        
        // Stage 1: Initial gain boost
        input *= gainAmount * 0.4f;
        input = applyPreampStage (input, 1.0f, 1);
        
        // Stage 2: Second gain stage
        input *= gainAmount * 0.6f;
        input = applyPreampStage (input, 1.0f, 2);
        
        // Stage 3: Third gain stage (high gain)
        input *= gainAmount * 0.8f;
        input = applyPreampStage (input, 1.0f, 3);
        
        // Stage 4: Final preamp stage
        input *= gainAmount;
        input = applyPreampStage (input, 1.0f, 4);
        
        // Apply rectifier saturation (after preamp, before tone stack)
        float currentDrive = smoothedDrive.getNextValue();
        float currentRectifierMode = smoothedRectifierMode.getNextValue();
        input = applyRectifierSaturation (input, currentDrive, currentRectifierMode);
        
        // Apply Voice control (Raw/Mid/Mod) - Triple Rectifier channel voicing
        // Voice: 0.0 = Raw (aggressive, tight, less compression), 
        //        0.5 = Mid (balanced, classic Rectifier), 
        //        1.0 = Mod (smooth, modern, more compression)
        if (voice < 0.25f) // Raw - aggressive, tight, less compressed
        {
            // More aggressive, tighter saturation - less compression
            input = std::tanh (input * 2.1f) * 0.62f; // Tighter, more aggressive
        }
        else if (voice < 0.75f) // Mid - balanced, classic Rectifier sound
        {
            // Balanced Rectifier tone - slight smoothing
            input = std::tanh (input * 1.65f) * 0.68f;
        }
        else // Mod - smooth, modern, more compressed
        {
            // Smoother, more compressed - modern high-gain sound
            input = std::tanh (input * 1.5f) * 0.72f; // More compression, smoother
        }
        
        // Apply Mode control (Cln/Cru/Mod) - Triple Rectifier mode selection
        // Mode: 0.0 = Cln (clean, minimal saturation), 
        //       0.5 = Cru (crunch, moderate gain), 
        //       1.0 = Mod (modern high gain, maximum saturation)
        if (mode < 0.25f) // Cln - clean, minimal saturation
        {
            // Clean mode - reduce gain significantly, minimal saturation
            input *= 0.35f; // Much less gain
            input = std::tanh (input * 1.0f) * 0.9f; // Very light saturation
        }
        else if (mode < 0.75f) // Cru - crunch, moderate gain
        {
            // Crunch mode - moderate gain boost, classic crunch
            input *= 1.4f; // Moderate gain boost
            input = std::tanh (input * 1.6f) * 0.7f; // Classic crunch saturation
        }
        else // Mod - modern high gain, maximum saturation
        {
            // Modern mode - high gain, aggressive saturation (Triple Rectifier high gain)
            input *= 1.8f; // High gain boost
            input = std::tanh (input * 2.3f) * 0.58f; // Aggressive, tight saturation
        }
        
        channelData[sample] = input;
    }
    
    // Apply tone stack filters (block processing) - positioned after preamp in Rectifier
    bassFilter.process (context);
    midFilter.process (context);
    trebleFilter.process (context);
    presenceFilter.process (context);
    
    // Apply master volume (per-sample for smoothing)
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float currentMaster = smoothedMaster.getNextValue();
        channelData[sample] *= (0.15f + currentMaster * 11.85f); // 0.15x to 12x (Rectifier master)
        
        // Final soft clipping to prevent harsh digital distortion
        channelData[sample] = juce::jlimit (-0.95f, 0.95f, channelData[sample]);
    }
}

//==============================================================================
// AudioProcessor Implementation
//==============================================================================

GainForgeAudioProcessor::GainForgeAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // Get parameter pointers
    gainParam = apvts.getRawParameterValue("GAIN");
    bassParam = apvts.getRawParameterValue("BASS");
    midParam = apvts.getRawParameterValue("MID");
    trebleParam = apvts.getRawParameterValue("TREBLE");
    presenceParam = apvts.getRawParameterValue("PRESENCE");
    masterParam = apvts.getRawParameterValue("MASTER");
    driveParam = apvts.getRawParameterValue("DRIVE");
    rectifierModeParam = apvts.getRawParameterValue("RECTIFIER_MODE");
    voiceParam = apvts.getRawParameterValue("VOICE");
    modeParam = apvts.getRawParameterValue("MODE");
    bypassParam = apvts.getRawParameterValue("BYPASS");
}

GainForgeAudioProcessor::~GainForgeAudioProcessor()
{
}

//==============================================================================
const juce::String GainForgeAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GainForgeAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool GainForgeAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool GainForgeAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double GainForgeAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GainForgeAudioProcessor::getNumPrograms()
{
    return 1;
}

int GainForgeAudioProcessor::getCurrentProgram()
{
    return 0;
}

void GainForgeAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String GainForgeAudioProcessor::getProgramName (int index)
{
    return {};
}

void GainForgeAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void GainForgeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    for (int channel = 0; channel < 2; ++channel)
    {
        ampEmulator[channel].prepare (sampleRate, samplesPerBlock);
    }
}

void GainForgeAudioProcessor::releaseResources()
{
    for (int channel = 0; channel < 2; ++channel)
    {
        ampEmulator[channel].reset();
    }
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool GainForgeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void GainForgeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Check bypass state - if bypassed, pass audio through unchanged
    bool bypassed = bypassParam && bypassParam->load() > 0.5f;
    if (bypassed)
        return; // Pass audio through unchanged

    // Get parameter values
    float gain = gainParam->load();
    float bass = bassParam->load();
    float mid = midParam->load();
    float treble = trebleParam->load();
    float presence = presenceParam->load();
    float master = masterParam->load();
    float drive = driveParam->load();
    float rectifierMode = rectifierModeParam->load() > 0.5f ? 1.0f : 0.0f; // Convert bool to float
    
    // Get voice and mode parameters (AudioParameterChoice returns normalized 0.0-1.0)
    float voice = voiceParam ? voiceParam->load() : 0.5f; // Default to Mid if not found
    float mode = modeParam ? modeParam->load() : 1.0f;    // Default to Mod if not found

    // Process each channel
    for (int channel = 0; channel < totalNumInputChannels && channel < 2; ++channel)
    {
        // Create a single-channel buffer for processing
        juce::AudioBuffer<float> singleChannelBuffer (1, buffer.getNumSamples());
        singleChannelBuffer.copyFrom (0, 0, buffer, channel, 0, buffer.getNumSamples());
        
        // Process the channel with amp emulator
        ampEmulator[channel].processBlock (singleChannelBuffer, gain, bass, mid, treble, presence, master, drive, rectifierMode, voice, mode);
        
        // Copy processed audio back to main buffer
        buffer.copyFrom (channel, 0, singleChannelBuffer, 0, 0, buffer.getNumSamples());
    }
}

//==============================================================================
bool GainForgeAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* GainForgeAudioProcessor::createEditor()
{
    return new GainForgeAudioProcessorEditor (*this);
}

//==============================================================================
void GainForgeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void GainForgeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// Parameter layout creation
juce::AudioProcessorValueTreeState::ParameterLayout GainForgeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Gain: 0 to 100% - start at 0.0 for safe loading (professional plugin practice)
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("GAIN", 1), "Gain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.0f, "%" // Start at 0.0 to prevent loud pops on load
    ));

    // Bass: 0 to 100%
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("BASS", 1), "Bass",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.5f, "%"
    ));

    // Mid: 0 to 100%
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("MID", 1), "Mid",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.5f, "%"
    ));

    // Treble: 0 to 100%
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("TREBLE", 1), "Treble",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.5f, "%"
    ));

    // Presence: 0 to 100%
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("PRESENCE", 1), "Presence",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.5f, "%"
    ));

    // Master: 0 to 100% - start at 0.0 for safe loading (professional plugin practice)
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("MASTER", 1), "Master",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.0f, "%" // Start at 0.0 to prevent loud pops on load
    ));

    // Drive: 0 to 100%
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("DRIVE", 1), "Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.3f, "%"
    ));

    // Rectifier Mode: 0 = Silicon Diode (tighter), 1 = Tube Rectifier (saggy)
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID ("RECTIFIER_MODE", 1), "Rectifier Mode",
        false, // Default to Silicon (false = 0)
        "" // false = Silicon, true = Tube
    ));

    // Voice: 3-position (Raw/Mid/Mod) - normalized 0.0, 0.5, 1.0
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID ("VOICE", 1), "Voice",
        juce::StringArray { "Raw", "Mid", "Mod" },
        1 // Default to Mid
    ));

    // Mode: 3-position (Cln/Cru/Mod) - normalized 0.0, 0.5, 1.0
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID ("MODE", 1), "Mode",
        juce::StringArray { "Cln", "Cru", "Mod" },
        2 // Default to Mod
    ));

    // Bypass: Toggle plugin on/off
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID ("BYPASS", 1), "Bypass",
        false // Default to not bypassed (plugin on)
    ));

    return { params.begin(), params.end() };
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GainForgeAudioProcessor();
}

