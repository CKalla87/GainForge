#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Three-way toggle switch component.
    Cycles through positions 0->1->2 on click.
    Displays three option labels at top, housing with lever in middle, label at bottom.
*/
class ThreeWayToggle : public juce::Component
{
public:
    ThreeWayToggle (juce::String labelText,
                    juce::String left, juce::String mid, juce::String right)
        : label (std::move (labelText))
    {
        options[0] = std::move (left);
        options[1] = std::move (mid);
        options[2] = std::move (right);
        setInterceptsMouseClicks (true, false);
    }

    void setPosition (int newPos)
    {
        position = juce::jlimit (0, 2, newPos);
        repaint();
    }

    int getPosition() const { return position; }

    std::function<void (int)> onChange;

    void mouseDown (const juce::MouseEvent&) override
    {
        position = (position + 1) % 3;
        repaint();
        if (onChange) onChange (position);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();

        // Layout: top labels, housing, bottom label
        auto bottomLabelArea = r.removeFromBottom (r.getHeight() * 0.22f);
        r.removeFromBottom (r.getHeight() * 0.02f); // small gap - reduced to bring label closer to switch
        auto housingArea = r.removeFromBottom (r.getHeight() * 0.55f);
        auto topLabelsArea = r; // remaining

        // Background (transparent; your panel image is behind)
        g.setColour (juce::Colours::transparentBlack);

        // --- TOP OPTION LABELS (RAW/MID/MOD etc) ---
        {
            const float h = topLabelsArea.getHeight();
            const float fontPx = juce::jlimit (11.0f, 18.0f, h * 0.60f); // BIGGER
            g.setFont (juce::Font (fontPx, juce::Font::bold));

            auto thirds = topLabelsArea;
            auto leftA  = thirds.removeFromLeft (thirds.getWidth() / 3.0f);
            auto midA   = thirds.removeFromLeft (thirds.getWidth() / 2.0f);
            auto rightA = thirds;

            auto drawOpt = [&] (juce::Rectangle<float> area, const juce::String& text, bool active)
            {
                g.setColour (active ? juce::Colour::fromRGB (220, 38, 38) : juce::Colour::fromRGB (120, 120, 120));
                g.drawFittedText (text, area.toNearestInt(), juce::Justification::centred, 1);
            };

            drawOpt (leftA,  options[0], position == 0);
            drawOpt (midA,   options[1], position == 1);
            drawOpt (rightA, options[2], position == 2);
        }

        // --- HOUSING ---
        {
            auto box = housingArea.reduced (housingArea.getWidth() * 0.08f,
                                            housingArea.getHeight() * 0.18f);

            g.setColour (juce::Colour::fromRGBA (0, 0, 0, 110));
            g.fillRoundedRectangle (box, 10.0f);

            g.setColour (juce::Colour::fromRGBA (255, 255, 255, 30));
            g.drawRoundedRectangle (box, 10.0f, 1.5f);

            // slider/lever
            auto slot = box.reduced (box.getWidth() * 0.08f, box.getHeight() * 0.12f);

            const float thirdW = slot.getWidth() / 3.0f;
            auto knob = juce::Rectangle<float> (
                slot.getX() + thirdW * (float) position,
                slot.getY(),
                thirdW,
                slot.getHeight()
            ).reduced (thirdW * 0.15f, 0.0f);

            g.setColour (juce::Colour::fromRGB (70, 70, 70));
            g.fillRoundedRectangle (knob, 6.0f);

            g.setColour (juce::Colour::fromRGBA (255, 255, 255, 35));
            g.drawRoundedRectangle (knob, 6.0f, 1.2f);

            // grip lines
            g.setColour (juce::Colour::fromRGBA (255, 255, 255, 22));
            for (int i = 0; i < 5; ++i)
            {
                const float y = knob.getY() + knob.getHeight() * (0.25f + i * 0.12f);
                g.drawLine (knob.getX() + knob.getWidth() * 0.25f, y,
                           knob.getRight() - knob.getWidth() * 0.25f, y, 1.0f);
            }
        }

        // --- BOTTOM LABEL (VOICE / MODE) ---
        {
            const float h = bottomLabelArea.getHeight();
            const float fontPx = juce::jlimit (14.0f, 22.0f, h * 0.70f); // BIGGER
            g.setFont (juce::Font (fontPx, juce::Font::bold));

            g.setColour (juce::Colour::fromRGB (140, 140, 140));
            g.drawFittedText (label, bottomLabelArea.toNearestInt(), juce::Justification::centred, 1);
        }
    }

private:
    juce::String label;
    juce::String options[3];
    int position = 1;
};
