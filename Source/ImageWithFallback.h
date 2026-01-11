#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Image component with fallback rendering.
    If image fails to load or is invalid, renders a fallback "error image" icon.
*/
class ImageWithFallback : public juce::Component
{
public:
    ImageWithFallback()
    {
        setInterceptsMouseClicks (false, false);
    }

    void setImage (const juce::Image& newImage)
    {
        image = newImage;
        repaint();
    }

    void setImage (juce::Image&& newImage)
    {
        image = std::move (newImage);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        // Get bounds and immediately validate
        auto bounds = getLocalBounds();
        if (bounds.getWidth() <= 0 || bounds.getHeight() <= 0)
            return;
            
        auto r = bounds.toFloat();

        // Safety check: ensure bounds are valid after conversion
        if (r.getWidth() <= 0.0f || r.getHeight() <= 0.0f || !r.isFinite())
            return;

        if (image.isValid())
        {
            // Draw the image, maintaining aspect ratio
            g.drawImageWithin (image, r.getX(), r.getY(), r.getWidth(), r.getHeight(),
                              juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize,
                              false);
        }
        else
        {
            // Fallback: draw error icon
            drawErrorIcon (g, r);
        }
    }

private:
    void drawErrorIcon (juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // Safety check: ensure bounds are valid
        if (bounds.getWidth() <= 0.0f || bounds.getHeight() <= 0.0f)
            return;

        // Draw a simple "broken image" icon
        g.setColour (juce::Colour::fromRGB (0x40, 0x40, 0x40));
        g.fillRoundedRectangle (bounds, 4.0f);

        g.setColour (juce::Colour::fromRGB (0x80, 0x80, 0x80));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

        // Draw "X" icon
        const float margin = juce::jmax (1.0f, bounds.getWidth() * 0.2f);
        auto iconBounds = bounds.reduced (margin);

        // Ensure icon bounds are valid
        if (iconBounds.getWidth() > 0.0f && iconBounds.getHeight() > 0.0f)
        {
            g.setColour (juce::Colour::fromRGB (0xcc, 0xcc, 0xcc));
            const float lineThickness = 3.0f;
            g.drawLine (iconBounds.getTopLeft().getX(), iconBounds.getTopLeft().getY(),
                       iconBounds.getBottomRight().getX(), iconBounds.getBottomRight().getY(), lineThickness);
            g.drawLine (iconBounds.getTopRight().getX(), iconBounds.getTopRight().getY(),
                       iconBounds.getBottomLeft().getX(), iconBounds.getBottomLeft().getY(), lineThickness);
        }
    }

    juce::Image image;
};

