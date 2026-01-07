/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class GainForgeAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    GainForgeAudioProcessorEditor (GainForgeAudioProcessor&);
    ~GainForgeAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    GainForgeAudioProcessor& audioProcessor;

    // Metal-themed colors
    juce::Colour metalBlack = juce::Colour::fromFloatRGBA (0.05f, 0.05f, 0.05f, 1.0f);
    juce::Colour metalDark = juce::Colour::fromFloatRGBA (0.15f, 0.15f, 0.15f, 1.0f);
    juce::Colour metalSilver = juce::Colour::fromFloatRGBA (0.7f, 0.7f, 0.7f, 1.0f);
    juce::Colour metalOrange = juce::Colour::fromFloatRGBA (1.0f, 0.4f, 0.0f, 1.0f);
    juce::Colour metalRed = juce::Colour::fromFloatRGBA (0.8f, 0.2f, 0.1f, 1.0f);
    juce::Colour metalGray = juce::Colour::fromFloatRGBA (0.3f, 0.3f, 0.3f, 1.0f);
    juce::Colour metalText = juce::Colour::fromFloatRGBA (0.9f, 0.9f, 0.9f, 1.0f);

    // Controls
    juce::Slider gainSlider;
    juce::Slider bassSlider;
    juce::Slider midSlider;
    juce::Slider trebleSlider;
    juce::Slider presenceSlider;
    juce::Slider masterSlider;
    juce::Slider driveSlider;
    
    juce::Label gainLabel;
    juce::Label bassLabel;
    juce::Label midLabel;
    juce::Label trebleLabel;
    juce::Label presenceLabel;
    juce::Label masterLabel;
    juce::Label driveLabel;
    juce::Label titleLabel;
    juce::Label rectifierLabel;
    
    // Rectifier mode toggle button
    juce::TextButton rectifierButton;
    
    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> midAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> trebleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> presenceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> rectifierAttachment;
    
    void setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& labelText);
    void drawAmpHead (juce::Graphics& g);
    void drawGrille (juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawKnob (juce::Graphics& g, juce::Rectangle<int> bounds, float value, const juce::String& label);
    void drawAmpChassis (juce::Graphics& g, juce::Rectangle<int> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainForgeAudioProcessorEditor)
};

