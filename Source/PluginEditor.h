/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "FilmstripLookAndFeel.h"
#include "AmpKnobComponent.h"
#include "ThreePositionToggle.h"
#include "ThreePositionToggleAttachment.h"
#include "ImageWithFallback.h"
#include "TextUtilities.h"

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
    //==============================================================================
    // GAINFORGE Panel Handling
    void loadPanelBackground();
    GainForgeAudioProcessor& audioProcessor;
    std::atomic<bool> isEditorValid { true }; // Guard against operations during destruction
    std::atomic<bool> isFullyInitialized { false }; // Guard against resized() during construction

    // Look and feel for knobs (red for GAIN/MASTER, blue for others)
    FilmstripLookAndFeel redKnobLnf;
    FilmstripLookAndFeel blueKnobLnf;
    juce::Image knobFilmstrip; // Optional: load from BinaryData if available
    juce::Image panelImage; // Background panel image
    juce::Rectangle<float> panelDrawArea;

    // Knob components (0-10 range, will map to 0-1.0 for parameters)
    std::unique_ptr<AmpKnobComponent> gainKnob;
    std::unique_ptr<AmpKnobComponent> bassKnob;
    std::unique_ptr<AmpKnobComponent> midKnob;
    std::unique_ptr<AmpKnobComponent> trebleKnob;
    std::unique_ptr<AmpKnobComponent> presenceKnob;
    std::unique_ptr<AmpKnobComponent> masterKnob;

    // Toggle switches
    std::unique_ptr<ThreeWayToggle> voiceToggle;
    std::unique_ptr<ThreeWayToggle> modeToggle;

    // APVTS attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> midAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> trebleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> presenceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterAttachment;

    std::unique_ptr<ThreePositionToggleAttachment> voiceAttachment;
    std::unique_ptr<ThreePositionToggleAttachment> modeAttachment;

    // Hidden sliders for parameter binding (parameters use 0-1.0, knobs use 0-10)
    struct HiddenSlider
    {
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        AmpKnobComponent* knobComponent = nullptr; // Store pointer for lambda access
        std::atomic<bool> isInitializing { false }; // Flag to prevent callback loops during init

        HiddenSlider()
            : slider (std::make_unique<juce::Slider>())
        {
        }

        ~HiddenSlider()
        {
            // Clear callbacks to prevent accessing deleted objects
            if (slider)
                slider->onValueChange = nullptr;
            if (knobComponent)
                knobComponent->getSlider().onValueChange = nullptr;
        }
        
        // Default move operations work fine with unique_ptr
        HiddenSlider (HiddenSlider&&) noexcept = default;
        HiddenSlider& operator= (HiddenSlider&&) noexcept = default;
        
        // Can't copy because of unique_ptr
        HiddenSlider (const HiddenSlider&) = delete;
        HiddenSlider& operator= (const HiddenSlider&) = delete;
    };

    // Use array - objects are constructed in place
    std::array<HiddenSlider, 6> hiddenSliders;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainForgeAudioProcessorEditor)
};
