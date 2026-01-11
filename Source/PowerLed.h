#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Power LED component with toggle functionality.
    Clickable LED indicator that can be toggled on/off.
*/
class PowerLed : public juce::Component
{
public:
    void setOn(bool b) { on = b; repaint(); }
    bool isOn() const { return on; }

    std::function<void(bool)> onToggle;

    void mouseUp(const juce::MouseEvent&) override
    {
        on = !on;
        repaint();
        if (onToggle) onToggle(on);
    }

    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced(2.0f);
        auto c = r.getCentre();
        const float rad = juce::jmin(r.getWidth(), r.getHeight()) * 0.5f;

        // --- Strong circular glow (no square clipping) ---
        if (on)
        {
            // glow fills most of the component
            auto glowArea = r.reduced(rad * 0.10f);

            juce::ColourGradient glow(
                juce::Colour::fromRGB(255, 60, 60).withAlpha(0.55f),  // stronger center
                glowArea.getCentreX(), glowArea.getCentreY(),
                juce::Colour::fromRGB(255, 60, 60).withAlpha(0.0f),   // fade out
                glowArea.getCentreX() + glowArea.getWidth() * 0.55f,
                glowArea.getCentreY(),
                true // radial
            );

            g.setGradientFill(glow);
            g.fillEllipse(glowArea);
        }

        // --- LED core (NO outer ring/bezel) ---
        auto core = r.reduced(rad * 0.42f); // smaller dot inside glow

        const juce::Colour ledCol = on
            ? juce::Colour::fromRGB(255, 85, 85)
            : juce::Colour::fromRGB(90, 20, 20);

        // soft "bulb" gradient
        juce::ColourGradient bulb(
            juce::Colours::white.withAlpha(on ? 0.35f : 0.12f),
            core.getX(), core.getY(),
            ledCol,
            core.getRight(), core.getBottom(),
            false
        );

        g.setGradientFill(bulb);
        g.fillEllipse(core);

        // highlight speck (gives it that glassy LED look)
        g.setColour(juce::Colours::white.withAlpha(on ? 0.28f : 0.10f));
        auto hi = core.withSizeKeepingCentre(core.getWidth() * 0.55f, core.getHeight() * 0.40f)
                      .translated(-core.getWidth() * 0.10f, -core.getHeight() * 0.12f);
        g.fillEllipse(hi);
    }

private:
    bool on = true;
};

