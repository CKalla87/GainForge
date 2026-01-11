#pragma once

#include <JuceHeader.h>
#include <cmath>

//==============================================================================
/**
    Utility functions for text rendering with letter-spacing (tracking)
    and number formatting.
*/
namespace TextUtilities
{
    /**
        Draw text with custom letter-spacing (tracking).
        
        @param g            Graphics context to draw into
        @param text         Text string to draw
        @param area         Rectangle area to draw text within
        @param font         Font to use
        @param colour       Text colour
        @param trackingPx   Extra pixels between letters (letter-spacing)
        @param just         Justification flags
    */
    static void drawTrackedText (juce::Graphics& g,
                                 const juce::String& text,
                                 juce::Rectangle<float> area,
                                 juce::Font font,
                                 juce::Colour colour,
                                 float trackingPx,
                                 juce::Justification just = juce::Justification::centred)
    {
        // Validate inputs
        if (text.isEmpty() || !area.isFinite() || area.getWidth() <= 0.0f || area.getHeight() <= 0.0f)
            return;

        g.setColour (colour);
        g.setFont (font);

        // Build glyphs with manual tracking
        juce::GlyphArrangement ga;
        ga.addLineOfText (font, text, 0.0f, 0.0f);

        // Apply tracking by shifting each glyph after the first
        // (simple approach that looks like Figma letter-spacing)
        // In JUCE 8, we use moveRangeOfGlyphs to shift individual glyphs
        const int numGlyphs = ga.getNumGlyphs();
        
        // Start from glyph 1 (skip the first one) and move each by increasing offset
        for (int i = 1; i < numGlyphs; ++i)
        {
            const float offset = trackingPx * static_cast<float> (i);
            // Move just this one glyph by the accumulated tracking offset
            ga.moveRangeOfGlyphs (i, 1, offset, 0.0f);
        }

        // Now position the arrangement inside the area
        auto bounds = ga.getBoundingBox (0, -1, true);
        
        // Validate bounds
        if (!bounds.isFinite() || bounds.getWidth() <= 0.0f || bounds.getHeight() <= 0.0f)
            return;

        float x = area.getX();
        float y = area.getY();

        if (just.testFlags (juce::Justification::horizontallyCentred))
            x = area.getCentreX() - bounds.getWidth() * 0.5f;
        else if (just.testFlags (juce::Justification::right))
            x = area.getRight() - bounds.getWidth();

        if (just.testFlags (juce::Justification::verticallyCentred))
            y = area.getCentreY() - bounds.getHeight() * 0.5f;
        else if (just.testFlags (juce::Justification::bottom))
            y = area.getBottom() - bounds.getHeight();

        // Validate final position before drawing
        if (std::isfinite (x) && std::isfinite (y))
        {
            ga.draw (g, juce::AffineTransform::translation (x - bounds.getX(), y - bounds.getY()));
        }
    }

    /**
        Format a number to one decimal place.
        
        @param v    Value to format
        @return     String representation with one decimal place (e.g., "0.0", "7.5")
    */
    static juce::String oneDecimal (double v)
    {
        return juce::String (v, 1); // ensures "0.0"
    }
}

