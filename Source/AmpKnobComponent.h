#pragma once

#include <JuceHeader.h>
#include "FilmstripLookAndFeel.h"
#include "TextUtilities.h"

//==============================================================================
/**
    Custom rotary knob component for amp-style controls.
    Displays label, numeric value, and uses FilmstripLookAndFeel for rendering.
*/
class AmpKnobComponent : public juce::Component
{
public:
    AmpKnobComponent (const juce::String& labelText, FilmstripLookAndFeel& lnfToUse)
        : label (labelText), lookAndFeel (lnfToUse), isValid (true)
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setLookAndFeel (&lookAndFeel);
        slider.setName (labelText);

        // Range: 0.0 to 10.0 with 0.1 steps
        slider.setRange (0.0, 10.0, 0.1);

        // Name label
        nameLabel.setText (label, juce::dontSendNotification);
        nameLabel.setJustificationType (juce::Justification::centred);
        nameLabel.setFont (juce::Font (11.0f, juce::Font::bold));
        nameLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (0x99, 0x99, 0x99));

        // Value label
        valueLabel.setJustificationType (juce::Justification::centred);
        valueLabel.setFont (juce::Font (13.0f, juce::Font::plain));
        valueLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (0xf5, 0xf5, 0xf5));

        // Update value label when slider changes
        slider.onValueChange = [this]
        {
            valueLabel.setText (oneDecimal (slider.getValue()), juce::dontSendNotification);
        };
        slider.onValueChange(); // Initialize

        addAndMakeVisible (slider);
        addAndMakeVisible (valueLabel);
        addAndMakeVisible (nameLabel);
    }

    ~AmpKnobComponent() override
    {
        isValid = false;
        slider.setLookAndFeel (nullptr);
        slider.onValueChange = nullptr;
    }

    juce::Slider& getSlider() { return slider; }
    const juce::Slider& getSlider() const { return slider; }
    
    // Update value label manually (useful when onValueChange is overwritten)
    void updateValueLabel()
    {
        valueLabel.setText (oneDecimal (slider.getValue()), juce::dontSendNotification);
    }

    void resized() override
    {
        // Guard against resizing during destruction
        if (!isValid.load())
            return;
        
        auto r = getLocalBounds().toFloat();

        // Reserve bottom area for value + label so the knob face stays circular
        const float labelAreaH = r.getHeight() * 0.32f;
        auto knobArea = r.removeFromTop (r.getHeight() - labelAreaH);

        // Force a perfect square for the knob face
        const float s = juce::jmin (knobArea.getWidth(), knobArea.getHeight());
        auto knobFaceArea = juce::Rectangle<float> (0, 0, s, s).withCentre (knobArea.getCentre());
        
        // Set slider bounds to the square knob face area
        slider.setBounds (knobFaceArea.toNearestInt());
        
        // Bottom: labels - use the reserved label area
        const float nameLabelHeight = labelAreaH * 0.4f;
        const float valueLabelHeight = labelAreaH * 0.6f;
        
        auto labelArea = r;
        auto valueArea = labelArea.removeFromBottom (valueLabelHeight);
        auto nameArea = labelArea;
        
        nameLabel.setBounds (nameArea.toNearestInt());
        valueLabel.setBounds (valueArea.toNearestInt());
    }

    void paint (juce::Graphics& g) override
    {
        // Guard against painting during destruction
        if (!isValid.load())
            return;
        
        auto r = getLocalBounds().toFloat();

        // Reserve bottom area for value + label so the knob face stays circular
        const float labelAreaH = r.getHeight() * 0.32f;
        auto knobArea = r.removeFromTop (r.getHeight() - labelAreaH);

        // Force a perfect square for the knob face
        const float s = juce::jmin (knobArea.getWidth(), knobArea.getHeight());
        auto knobFaceArea = juce::Rectangle<float> (0, 0, s, s).withCentre (knobArea.getCentre());
        
        // No circle border - knob image is drawn directly by LookAndFeel
        // Let JUCE handle default painting - we only customize the slider via LookAndFeel
        Component::paint (g);
    }

private:
    // Helper to format value to one decimal place
    static juce::String oneDecimal (double v) { return TextUtilities::oneDecimal (v); }

    juce::String label;
    FilmstripLookAndFeel& lookAndFeel;
    std::atomic<bool> isValid { true };

    juce::Slider slider;
    juce::Label nameLabel;
    juce::Label valueLabel;
};

