/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GainForgeAudioProcessorEditor::GainForgeAudioProcessorEditor (GainForgeAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Set editor size - wider for amp head design
    setSize (1000, 700);

    // Setup sliders
    setupSlider (gainSlider, gainLabel, "Gain");
    setupSlider (bassSlider, bassLabel, "Bass");
    setupSlider (midSlider, midLabel, "Mid");
    setupSlider (trebleSlider, trebleLabel, "Treble");
    setupSlider (presenceSlider, presenceLabel, "Presence");
    setupSlider (masterSlider, masterLabel, "Master");
    setupSlider (driveSlider, driveLabel, "Drive");

    // Title label
    titleLabel.setText ("GAINFORGE", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (56.0f, juce::Font::bold));
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, metalOrange);
    addAndMakeVisible (&titleLabel);
    
    // Rectifier mode button - styled like an amp switch
    rectifierButton.setButtonText ("Silicon Rectifier");
    rectifierButton.setClickingTogglesState (true);
    rectifierButton.setColour (juce::TextButton::buttonColourId, metalDark);
    rectifierButton.setColour (juce::TextButton::buttonOnColourId, metalOrange);
    rectifierButton.setColour (juce::TextButton::textColourOffId, metalText);
    rectifierButton.setColour (juce::TextButton::textColourOnId, metalBlack);
    rectifierButton.setColour (juce::TextButton::buttonColourId, metalSilver.darker (0.3f));
    addAndMakeVisible (&rectifierButton);
    rectifierAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "RECTIFIER_MODE", rectifierButton);
    
    // Update button text when state changes
    rectifierButton.onStateChange = [this]()
    {
        rectifierButton.setButtonText (rectifierButton.getToggleState() ? "Tube Rectifier" : "Silicon Rectifier");
    };
    
    // Hide the separate label since button text includes it now
    rectifierLabel.setText ("", juce::dontSendNotification);
    rectifierLabel.setJustificationType (juce::Justification::centred);
    rectifierLabel.setFont (juce::Font (14.0f, juce::Font::bold));
    rectifierLabel.setColour (juce::Label::textColourId, metalText);
    addAndMakeVisible (&rectifierLabel);
}

GainForgeAudioProcessorEditor::~GainForgeAudioProcessorEditor()
{
}

void GainForgeAudioProcessorEditor::setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& labelText)
{
    // Configure slider - no text box, we'll draw custom knobs
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled (true, false, this);
    
    // Amp-style knob colors - silver/chrome knobs with orange indicator
    slider.setColour (juce::Slider::rotarySliderFillColourId, metalOrange);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, metalSilver);
    slider.setColour (juce::Slider::thumbColourId, metalOrange);
    slider.setColour (juce::Slider::trackColourId, metalDark);
    slider.setColour (juce::Slider::backgroundColourId, metalDark);
    addAndMakeVisible (&slider);

    // Configure label - smaller, positioned below knob
    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (14.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, metalText);
    addAndMakeVisible (&label);

    // Attach to parameters
    if (labelText == "Gain")
    {
        gainSlider.setTextValueSuffix ("%");
        gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "GAIN", gainSlider);
    }
    else if (labelText == "Bass")
    {
        bassSlider.setTextValueSuffix ("%");
        bassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "BASS", bassSlider);
    }
    else if (labelText == "Mid")
    {
        midSlider.setTextValueSuffix ("%");
        midAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "MID", midSlider);
    }
    else if (labelText == "Treble")
    {
        trebleSlider.setTextValueSuffix ("%");
        trebleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "TREBLE", trebleSlider);
    }
    else if (labelText == "Presence")
    {
        presenceSlider.setTextValueSuffix ("%");
        presenceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "PRESENCE", presenceSlider);
    }
    else if (labelText == "Master")
    {
        masterSlider.setTextValueSuffix ("%");
        masterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "MASTER", masterSlider);
    }
    else if (labelText == "Drive")
    {
        driveSlider.setTextValueSuffix ("%");
        driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "DRIVE", driveSlider);
    }
}

//==============================================================================
void GainForgeAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Dark background
    g.fillAll (metalBlack);
    
    // Draw the amp head
    drawAmpHead (g);
}

void GainForgeAudioProcessorEditor::drawAmpHead (juce::Graphics& g)
{
    const int ampWidth = getWidth() - 40;
    const int ampHeight = getHeight() - 40;
    const int ampX = 20;
    const int ampY = 20;
    
    juce::Rectangle<int> ampBounds (ampX, ampY, ampWidth, ampHeight);
    
    // Draw main chassis
    drawAmpChassis (g, ampBounds);
    
    // Draw grille area (top portion)
    const int grilleHeight = ampHeight / 3;
    juce::Rectangle<int> grilleBounds (ampX + 20, ampY + 20, ampWidth - 40, grilleHeight);
    drawGrille (g, grilleBounds);
    
    // Draw logo inside grille area
    g.setColour (metalOrange);
    g.setFont (juce::Font (42.0f, juce::Font::bold));
    g.drawText ("GAINFORGE", grilleBounds.getX(), grilleBounds.getY() + grilleBounds.getHeight() / 2 - 25, 
               grilleBounds.getWidth(), 50,
               juce::Justification::centred, false);
    
    // Draw control panel area (below grille)
    const int controlPanelY = ampY + grilleHeight + 40;
    const int controlPanelHeight = ampHeight - grilleHeight - 60;
    
    // Draw control panel background
    g.setColour (metalDark);
    g.fillRoundedRectangle ((float)(ampX + 30), (float)controlPanelY, 
                           (float)(ampWidth - 60), (float)controlPanelHeight, 5.0f);
    
    // Draw panel border
    g.setColour (metalOrange.withAlpha (0.5f));
    g.drawRoundedRectangle ((float)(ampX + 30), (float)controlPanelY, 
                           (float)(ampWidth - 60), (float)controlPanelHeight, 5.0f, 2.0f);
    
    // Draw section dividers
    g.setColour (metalGray.withAlpha (0.3f));
    int divider1X = ampX + ampWidth / 3;
    int divider2X = ampX + 2 * ampWidth / 3;
    g.drawLine ((float)divider1X, (float)(controlPanelY + 10), 
                (float)divider1X, (float)(controlPanelY + controlPanelHeight - 10), 1.0f);
    g.drawLine ((float)divider2X, (float)(controlPanelY + 10), 
                (float)divider2X, (float)(controlPanelY + controlPanelHeight - 10), 1.0f);
    
    // Draw section labels
    g.setFont (juce::Font (12.0f, juce::Font::bold));
    g.setColour (metalGray);
    g.drawText ("GAIN", ampX + 50, controlPanelY + 10, 150, 20,
               juce::Justification::centred, false);
    g.drawText ("TONE", divider1X - 75, controlPanelY + 10, 150, 20,
               juce::Justification::centred, false);
    g.drawText ("OUTPUT", divider2X - 75, controlPanelY + 10, 150, 20,
               juce::Justification::centred, false);
}

void GainForgeAudioProcessorEditor::drawGrille (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Draw grille background
    g.setColour (metalBlack);
    g.fillRoundedRectangle (bounds.toFloat(), 3.0f);
    
    // Draw grille pattern (horizontal lines)
    g.setColour (metalGray.withAlpha (0.4f));
    const int lineSpacing = 8;
    for (int y = bounds.getY() + 5; y < bounds.getBottom() - 5; y += lineSpacing)
    {
        g.drawLine ((float)(bounds.getX() + 5), (float)y, 
                    (float)(bounds.getRight() - 5), (float)y, 1.5f);
    }
    
    // Draw vertical grille supports
    g.setColour (metalGray.withAlpha (0.6f));
    const int supportSpacing = 60;
    for (int x = bounds.getX() + 20; x < bounds.getRight() - 20; x += supportSpacing)
    {
        g.drawLine ((float)x, (float)(bounds.getY() + 5), 
                    (float)x, (float)(bounds.getBottom() - 5), 2.0f);
    }
    
    // Draw grille border
    g.setColour (metalOrange.withAlpha (0.6f));
    g.drawRoundedRectangle (bounds.toFloat(), 3.0f, 2.0f);
}

void GainForgeAudioProcessorEditor::drawAmpChassis (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Draw main chassis with gradient for 3D effect
    juce::ColourGradient gradient (metalDark.brighter (0.1f), (float)bounds.getX(), (float)bounds.getY(),
                                   metalDark.darker (0.2f), (float)bounds.getX(), (float)bounds.getBottom(),
                                   false);
    g.setGradientFill (gradient);
    g.fillRoundedRectangle (bounds.toFloat(), 8.0f);
    
    // Draw chassis border
    g.setColour (metalOrange.withAlpha (0.7f));
    g.drawRoundedRectangle (bounds.toFloat(), 8.0f, 3.0f);
    
    // Draw corner reinforcements
    const int cornerSize = 15;
    g.setColour (metalOrange.withAlpha (0.5f));
    
    // Top corners
    g.drawLine ((float)bounds.getX(), (float)bounds.getY(), 
                (float)(bounds.getX() + cornerSize), (float)bounds.getY(), 2.0f);
    g.drawLine ((float)bounds.getX(), (float)bounds.getY(), 
                (float)bounds.getX(), (float)(bounds.getY() + cornerSize), 2.0f);
    
    g.drawLine ((float)(bounds.getRight() - cornerSize), (float)bounds.getY(), 
                (float)bounds.getRight(), (float)bounds.getY(), 2.0f);
    g.drawLine ((float)bounds.getRight(), (float)bounds.getY(), 
                (float)bounds.getRight(), (float)(bounds.getY() + cornerSize), 2.0f);
    
    // Bottom corners
    g.drawLine ((float)bounds.getX(), (float)(bounds.getBottom() - cornerSize), 
                (float)bounds.getX(), (float)bounds.getBottom(), 2.0f);
    g.drawLine ((float)bounds.getX(), (float)bounds.getBottom(), 
                (float)(bounds.getX() + cornerSize), (float)bounds.getBottom(), 2.0f);
    
    g.drawLine ((float)(bounds.getRight() - cornerSize), (float)bounds.getBottom(), 
                (float)bounds.getRight(), (float)bounds.getBottom(), 2.0f);
    g.drawLine ((float)bounds.getRight(), (float)(bounds.getBottom() - cornerSize), 
                (float)bounds.getRight(), (float)bounds.getBottom(), 2.0f);
}

void GainForgeAudioProcessorEditor::drawKnob (juce::Graphics& g, juce::Rectangle<int> bounds, float value, const juce::String& label)
{
    // This function is available for future custom knob rendering
    // Currently using JUCE's built-in rotary slider rendering
    juce::ignoreUnused (g, bounds, value, label);
}

void GainForgeAudioProcessorEditor::resized()
{
    const int ampWidth = getWidth() - 40;
    const int ampHeight = getHeight() - 40;
    const int ampX = 20;
    const int ampY = 20;
    const int grilleHeight = ampHeight / 3;
    const int controlPanelY = ampY + grilleHeight + 40;
    const int controlPanelHeight = ampHeight - grilleHeight - 60;
    
    const int knobSize = 80;
    const int labelHeight = 20;
    const int knobSpacing = 100;
    
    // Calculate control panel bounds
    const int panelX = ampX + 30;
    const int panelWidth = ampWidth - 60;
    const int panelStartY = controlPanelY + 40;
    
    // GAIN section (left third)
    const int gainSectionX = panelX + 30;
    const int gainSectionCenterY = panelStartY + (controlPanelHeight - 100) / 2;
    
    gainSlider.setBounds (gainSectionX, gainSectionCenterY - knobSize / 2, knobSize, knobSize);
    gainLabel.setBounds (gainSectionX, gainSectionCenterY + knobSize / 2 + 5, knobSize, labelHeight);
    
    driveSlider.setBounds (gainSectionX + knobSpacing, gainSectionCenterY - knobSize / 2, knobSize, knobSize);
    driveLabel.setBounds (gainSectionX + knobSpacing, gainSectionCenterY + knobSize / 2 + 5, knobSize, labelHeight);
    
    // TONE section (middle third)
    const int toneSectionX = panelX + panelWidth / 3 + 20;
    const int toneRow1Y = panelStartY + 30;
    const int toneRow2Y = panelStartY + 150;
    
    bassSlider.setBounds (toneSectionX, toneRow1Y, knobSize, knobSize);
    bassLabel.setBounds (toneSectionX, toneRow1Y + knobSize + 5, knobSize, labelHeight);
    
    midSlider.setBounds (toneSectionX + knobSpacing, toneRow1Y, knobSize, knobSize);
    midLabel.setBounds (toneSectionX + knobSpacing, toneRow1Y + knobSize + 5, knobSize, labelHeight);
    
    trebleSlider.setBounds (toneSectionX, toneRow2Y, knobSize, knobSize);
    trebleLabel.setBounds (toneSectionX, toneRow2Y + knobSize + 5, knobSize, labelHeight);
    
    presenceSlider.setBounds (toneSectionX + knobSpacing, toneRow2Y, knobSize, knobSize);
    presenceLabel.setBounds (toneSectionX + knobSpacing, toneRow2Y + knobSize + 5, knobSize, labelHeight);
    
    // OUTPUT section (right third)
    const int outputSectionX = panelX + 2 * panelWidth / 3 + 20;
    const int outputCenterY = panelStartY + (controlPanelHeight - 100) / 2;
    
    masterSlider.setBounds (outputSectionX, outputCenterY - knobSize / 2, knobSize, knobSize);
    masterLabel.setBounds (outputSectionX, outputCenterY + knobSize / 2 + 5, knobSize, labelHeight);
    
    // Rectifier mode button (below master) - wider to fit text
    const int buttonWidth = 120;
    const int buttonHeight = 35;
    rectifierButton.setBounds (outputSectionX, outputCenterY + knobSize / 2 + 30, buttonWidth, buttonHeight);
    rectifierLabel.setBounds (0, 0, 0, 0); // Hide label since button has text
    
    // Hide title label (we draw it in paint)
    titleLabel.setBounds (0, 0, 0, 0);
}

