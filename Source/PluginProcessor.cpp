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
    updateFilters (0.5f, 0.5f, 0.5f, 0.5f, 0.5f);
}

void GainForgeAudioProcessor::AmpEmulator::reset()
{
    bassFilter.reset();
    midFilter.reset();
    trebleFilter.reset();
    presenceFilter.reset();
}

void GainForgeAudioProcessor::AmpEmulator::updateFilters (float bass, float mid, float treble, float presence, float mode)
{
    if (mode < 0.25f)
    {
        // Fender Deluxe Reverb-inspired clean stack: scooped mids, airy top, rich lows
        auto bassCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
            currentSampleRate, 90.0, 0.707,
            juce::jmap (bass, 0.40f, 2.8f)
        );
        *bassFilter.coefficients = *bassCoeffs;
        
        auto midCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            currentSampleRate, 450.0, 0.75,
            juce::jmap (mid, 0.25f, 1.6f)
        );
        *midFilter.coefficients = *midCoeffs;
        
        auto trebleCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            currentSampleRate, 3500.0, 0.707,
            juce::jmap (treble, 0.35f, 2.2f)
        );
        *trebleFilter.coefficients = *trebleCoeffs;
        
        auto presenceCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            currentSampleRate, 5200.0, 0.707,
            juce::jmap (presence, 0.30f, 2.3f)
        );
        *presenceFilter.coefficients = *presenceCoeffs;
    }
    else
    {
        // JCM800 2203-inspired tone stack (tighter low end, forward mids)
        auto bassCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
            currentSampleRate, 100.0, 0.707,
            juce::jmap (bass, 0.35f, 2.6f)
        );
        *bassFilter.coefficients = *bassCoeffs;
        
        auto midCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            currentSampleRate, 650.0, 0.85,
            juce::jmap (mid, 0.30f, 2.8f)
        );
        *midFilter.coefficients = *midCoeffs;
        
        auto trebleCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            currentSampleRate, 3200.0, 0.707,
            juce::jmap (treble, 0.30f, 2.4f)
        );
        *trebleFilter.coefficients = *trebleCoeffs;
        
        auto presenceCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            currentSampleRate, 4000.0, 0.707,
            juce::jmap (presence, 0.35f, 2.2f)
        );
        *presenceFilter.coefficients = *presenceCoeffs;
    }
}

float GainForgeAudioProcessor::AmpEmulator::applyPreampStage (float input, float stageGain, int stageNumber)
{
    // JCM800-style preamp staging: more bite, less compression
    float output = input * stageGain;
    
    float saturationAmount = 0.8f + stageNumber * 0.18f;
    
    // Asymmetric saturation with a harder knee for classic JCM800 bite
    if (output > 0.0f)
        output = std::tanh (output * saturationAmount * 1.9f) * 0.68f;
    else
        output = std::tanh (output * saturationAmount * 1.3f) * 0.78f;
    
    return output;
}

float GainForgeAudioProcessor::AmpEmulator::applyRectifierSaturation (float input, float drive, float rectifierMode)
{
    // JCM800-inspired power section feel:
    // keep it tight, less sag, and slightly crunchy when pushed.
    float driven = input * (1.0f + drive * 5.0f);
    
    if (rectifierMode < 0.5f) // Tight mode
    {
        driven = std::tanh (driven * 2.4f) * 0.70f;
    }
    else // Slight sag mode
    {
        float sagAmount = std::abs (driven) * 0.06f;
        rectifierSagState = rectifierSagState * 0.97f + sagAmount * 0.03f;
        driven *= (1.0f - rectifierSagState * 0.14f);
        driven = std::tanh (driven * 1.9f) * 0.72f;
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
    updateFilters (bass, mid, treble, presence, mode);
    
    // Create DSP audio block
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    
    // Process each sample for gain, drive, and master (these need per-sample smoothing)
    auto* channelData = buffer.getWritePointer (0);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float input = channelData[sample];
        float currentMode = mode; // Use current mode value
        
        // Apply Mode control EARLY - Clean mode bypasses most saturation
        if (currentMode < 0.25f) // Cln - richer clean, Fender-like body
        {
            // Clean mode - more headroom with gentle even-order warmth
            float currentGain = smoothedGain.getNextValue();
            float gainAmount = 0.7f + currentGain * 2.0f; // 0.7x to 2.7x
            float driven = input * gainAmount;
            
            // Subtle even-order enrichment without obvious breakup
            float shaped = driven - 0.12f * driven * driven * driven;
            float edge = std::tanh (driven * 0.9f) * 0.85f;
            input = (0.85f * shaped) + (0.15f * edge);
            // Bypass all other processing stages for clean sound
        }
        else
        {
            // Crunch and Modern modes - apply full preamp processing
            float currentGain = smoothedGain.getNextValue();
            // JCM800 gain range: medium overall gain, more dynamic
            float gainAmount = 1.0f + currentGain * 7.0f;
            
            // Stage 1: Initial gain boost
            input *= gainAmount * 0.33f;
            input = applyPreampStage (input, 1.0f, 1);
            
            // Stage 2: Second gain stage
            input *= gainAmount * 0.42f;
            input = applyPreampStage (input, 1.0f, 2);
            
            // Stage 3: Third gain stage (high gain)
            input *= gainAmount * 0.50f;
            input = applyPreampStage (input, 1.0f, 3);
            
            // Stage 4: Final preamp stage
            input *= gainAmount * 0.58f;
            input = applyPreampStage (input, 1.0f, 4);
            
            // Apply rectifier saturation (after preamp, before tone stack)
            float currentDrive = smoothedDrive.getNextValue();
            float currentRectifierMode = smoothedRectifierMode.getNextValue();
            input = applyRectifierSaturation (input, currentDrive, currentRectifierMode);
            
            // Voice control repurposed for JCM800-like bite vs smoothness
            // Voice: 0.0 = Bright/Tight, 0.5 = Classic, 1.0 = Smooth
            if (voice < 0.25f) // Bright/Tight
            {
                input = std::tanh (input * 1.7f) * 0.72f;
            }
            else if (voice < 0.75f) // Classic
            {
                input = std::tanh (input * 1.4f) * 0.78f;
            }
            else // Smooth
            {
                input = std::tanh (input * 1.25f) * 0.82f;
            }
            
            // Apply Mode control for Crunch vs Modern
            if (currentMode < 0.75f) // Cru - crunch, classic JCM800
            {
                input *= 1.10f;
                input = std::tanh (input * 1.45f) * 0.76f;
            }
            else // Mod - hotter, but still British
            {
                input *= 1.22f;
                input = std::tanh (input * 1.80f) * 0.74f;
            }
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
        
        // Final soft clipping to prevent harsh digital distortion (gentler)
        channelData[sample] = juce::jlimit (-0.98f, 0.98f, channelData[sample]);
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
    // Effect: allow mono and stereo; require input and output layouts to match
    auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
   #if ! JucePlugin_IsSynth
    if (layouts.getMainInputChannelSet() != out)
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

    // Gain: 0 to 100% - start at 0 to avoid loud load
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("GAIN", 1), "Gain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.0f, "%" // Start at 0 on load
    ));

    // Bass: 0 to 100% - tighter low end
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("BASS", 1), "Bass",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.35f, "%"
    ));

    // Mid: 0 to 100% - forward British mids
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("MID", 1), "Mid",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.70f, "%"
    ));

    // Treble: 0 to 100% - bright but not harsh
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("TREBLE", 1), "Treble",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.60f, "%"
    ));

    // Presence: 0 to 100% - classic Marshall bite
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("PRESENCE", 1), "Presence",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.55f, "%"
    ));

    // Master: 0 to 100% - very low default level
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("MASTER", 1), "Master",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.05f, "%" // Lower output on load
    ));

    // Drive: 0 to 100% - very low default drive
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID ("DRIVE", 1), "Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.12f, "%"
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
        0 // Default to Raw (brighter/tighter)
    ));

    // Mode: 3-position (Cln/Cru/Mod) - normalized 0.0, 0.5, 1.0
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID ("MODE", 1), "Mode",
        juce::StringArray { "Cln", "Cru", "Mod" },
        1 // Default to Cru (classic crunch)
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

