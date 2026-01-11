#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Custom LookAndFeel for rendering knobs from a filmstrip image.
    Supports both vertical and horizontal filmstrips.
*/
class FilmstripLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FilmstripLookAndFeel() = default;
    ~FilmstripLookAndFeel() override = default;

    void setFilmstrip (juce::Image strip, int frames, bool vertical = true)
    {
        filmstrip = strip;
        numFrames = juce::jmax (1, frames);
        isVertical = vertical;
    }

    bool hasFilmstrip() const { return filmstrip.isValid() && numFrames > 0; }

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        juce::ignoreUnused (rotaryStartAngle, rotaryEndAngle);

        // CRITICAL: Validate slider is valid before accessing it
        // Try to access a safe method to check if slider is valid
        try
        {
            // This will throw if slider is invalid
            auto test = slider.getRange();
            juce::ignoreUnused (test);
        }
        catch (...)
        {
            return; // Slider is invalid, don't draw
        }

        // CRITICAL: Validate ALL parameters before ANY operations
        // Check for invalid memory addresses or corrupted values
        if (width <= 0 || height <= 0 || width > 10000 || height > 10000 ||
            x < -10000 || y < -10000 || x > 10000 || y > 10000)
            return;
            
        // Validate sliderPosProportional
        if (sliderPosProportional != sliderPosProportional || 
            sliderPosProportional < -1.0f || sliderPosProportional > 2.0f)
            sliderPosProportional = 0.0f;

        if (hasFilmstrip())
        {
            // Use filmstrip
            const int frameIndex = juce::jlimit (0, numFrames - 1,
                                                 (int) std::round (sliderPosProportional * (numFrames - 1)));

            const int frameW = isVertical ? filmstrip.getWidth() : filmstrip.getWidth() / numFrames;
            const int frameH = isVertical ? filmstrip.getHeight() / numFrames : filmstrip.getHeight();

            // Safety check: ensure frame dimensions are valid
            if (frameW <= 0 || frameH <= 0)
            {
                drawMetallicKnob (g, x, y, width, height, sliderPosProportional, slider);
                return;
            }

            // Calculate source rectangle values manually - NEVER call Rectangle methods
            const int srcX = isVertical ? 0 : (frameIndex * frameW);
            const int srcY = isVertical ? (frameIndex * frameH) : 0;
            
            // Validate source coordinates
            if (srcX >= 0 && srcY >= 0 && frameW > 0 && frameH > 0 &&
                srcX < filmstrip.getWidth() && srcY < filmstrip.getHeight())
            {
                g.drawImage (filmstrip,
                             x, y, width, height,
                             srcX, srcY, frameW, frameH,
                             false);
            }
            else
            {
                drawMetallicKnob (g, x, y, width, height, sliderPosProportional, slider);
            }
        }
        else
        {
            // Fallback: draw a simple metallic knob
            drawMetallicKnob (g, x, y, width, height, sliderPosProportional, slider);
        }
    }

private:
    void drawMetallicKnob (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, juce::Slider& slider)
    {
        // Safety check: ensure valid dimensions
        if (width <= 0 || height <= 0 || x < -10000 || y < -10000 || 
            x > 10000 || y > 10000 || width > 10000 || height > 10000)
            return;

        // Calculate all values manually - NEVER call Rectangle methods
        const float fx = (float)x;
        const float fy = (float)y;
        const float fw = (float)width;
        const float fh = (float)height;
        const float reduction = 2.0f;
        
        const float rX = fx + reduction;
        const float rY = fy + reduction;
        const float rW = fw - (reduction * 2.0f);
        const float rH = fh - (reduction * 2.0f);
        
        // Safety check: ensure reduced dimensions are valid
        if (rW <= 0.0f || rH <= 0.0f)
            return;

        const float centreX = rX + rW * 0.5f;
        const float centreY = rY + rH * 0.5f;
        const float radius = juce::jmin (rW, rH) * 0.5f;
        
        // Safety check: ensure radius is valid
        if (radius <= 0.0f || centreX != centreX || centreY != centreY)
            return;

        // Check if this knob should have red accent
        const bool hasRedAccent = slider.getName().containsIgnoreCase ("Gain") ||
                                  slider.getName().containsIgnoreCase ("Master");

        // Create Rectangle only when needed for drawing - calculate all values first
        juce::Rectangle<float> outerRect (rX, rY, rW, rH);
        
        // Outer ring - dark metal
        g.setColour (juce::Colour::fromRGB (0x2a, 0x2a, 0x2a));
        g.fillEllipse (outerRect);

        // Inner gradient - metallic shine (calculate reduced rect manually)
        const float innerReduction = 3.0f;
        const float innerX = rX + innerReduction;
        const float innerY = rY + innerReduction;
        const float innerW = rW - (innerReduction * 2.0f);
        const float innerH = rH - (innerReduction * 2.0f);
        
        if (innerW > 0.0f && innerH > 0.0f)
        {
            juce::ColourGradient gradient (juce::Colour::fromRGB (0x6a, 0x6a, 0x6a),
                                          centreX - radius * 0.3f, centreY - radius * 0.3f,
                                          juce::Colour::fromRGB (0x1a, 0x1a, 0x1a),
                                          centreX + radius * 0.3f, centreY + radius * 0.3f,
                                          false);
            g.setGradientFill (gradient);
            juce::Rectangle<float> innerRect (innerX, innerY, innerW, innerH);
            g.fillEllipse (innerRect);
        }

        // Highlight (calculate manually)
        const float highlightReduction = radius * 0.4f;
        const float highlightX = centreX - radius + highlightReduction;
        const float highlightY = centreY - radius + highlightReduction;
        const float highlightSize = (radius - highlightReduction) * 2.0f;
        
        if (highlightSize > 0.0f)
        {
            g.setColour (juce::Colours::white.withAlpha (0.3f));
            g.fillEllipse (highlightX, highlightY, highlightSize, highlightSize);
        }

        // Indicator line
        const float angle = juce::MathConstants<float>::pi * 1.25f + 
                           (sliderPosProportional * juce::MathConstants<float>::pi * 1.5f);
        const float lineLength = radius * 0.7f;
        const float lineThickness = 3.0f;

        juce::Colour indicatorColour = hasRedAccent 
            ? juce::Colour::fromRGB (220, 38, 38)  // Red accent
            : juce::Colour::fromRGB (0xff, 0x66, 0x00);  // Orange

        g.setColour (indicatorColour);
        g.drawLine (centreX, centreY,
                   centreX + std::cos (angle) * lineLength,
                   centreY + std::sin (angle) * lineLength,
                   lineThickness);

        // Center dot
        g.setColour (juce::Colours::black);
        g.fillEllipse (centreX - 3.0f, centreY - 3.0f, 6.0f, 6.0f);
    }

    juce::Image filmstrip;
    int numFrames = 128;
    bool isVertical = true;
};

