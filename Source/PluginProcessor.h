/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class GainForgeAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    GainForgeAudioProcessor();
    ~GainForgeAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Parameter management
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout() };

    // Amp emulator parameters
    std::atomic<float>* gainParam = nullptr;
    std::atomic<float>* bassParam = nullptr;
    std::atomic<float>* midParam = nullptr;
    std::atomic<float>* trebleParam = nullptr;
    std::atomic<float>* presenceParam = nullptr;
    std::atomic<float>* masterParam = nullptr;
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* rectifierModeParam = nullptr; // 0.0 = Silicon, 1.0 = Tube
    std::atomic<float>* voiceParam = nullptr; // 0.0 = Raw, 0.5 = Mid, 1.0 = Mod
    std::atomic<float>* modeParam = nullptr;  // 0.0 = Cln, 0.5 = Cru, 1.0 = Mod
    std::atomic<float>* bypassParam = nullptr; // 0.0 = not bypassed (on), 1.0 = bypassed (off)

private:
    //==============================================================================
    // Amp emulator implementation
    class AmpEmulator
    {
    public:
        AmpEmulator();
        void prepare (double sampleRate, int maxBlockSize);
        void reset();
        void processBlock (juce::AudioBuffer<float>& buffer, 
                          float gain, float bass, float mid, float treble, 
                          float presence, float master, float drive, float rectifierMode,
                          float voice, float mode);
        
    private:
        // Tone stack filters
        juce::dsp::IIR::Filter<float> bassFilter;
        juce::dsp::IIR::Filter<float> midFilter;
        juce::dsp::IIR::Filter<float> trebleFilter;
        juce::dsp::IIR::Filter<float> presenceFilter;
        
        
        // Smoothing for parameter changes
        juce::LinearSmoothedValue<float> smoothedGain;
        juce::LinearSmoothedValue<float> smoothedBass;
        juce::LinearSmoothedValue<float> smoothedMid;
        juce::LinearSmoothedValue<float> smoothedTreble;
        juce::LinearSmoothedValue<float> smoothedPresence;
        juce::LinearSmoothedValue<float> smoothedMaster;
        juce::LinearSmoothedValue<float> smoothedDrive;
        juce::LinearSmoothedValue<float> smoothedRectifierMode;
        
        // Rectifier sag simulation (for tube mode)
        float rectifierSagState = 0.0f;
        
        double currentSampleRate = 44100.0;
        
        void updateFilters (float bass, float mid, float treble, float presence);
        float applyRectifierSaturation (float input, float drive, float rectifierMode);
        float applyPreampStage (float input, float stageGain, int stageNumber);
    };
    
    AmpEmulator ampEmulator[2]; // One per channel (stereo)
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainForgeAudioProcessor)
};

