#pragma once
#include <JuceHeader.h>

class KnobImageLNF : public juce::LookAndFeel_V4
{
public:
    KnobImageLNF(juce::Image knobImage, float pointerAngleRadians)
        : knob(std::move(knobImage)), pointerAngleInPng(pointerAngleRadians) {}

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/,
                          juce::Slider& slider) override
    {
        if (!knob.isValid())
            return;

        // Stable destination square
        auto area = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(6.0f);
        const float s = juce::jmin(area.getWidth(), area.getHeight());
        auto dst = juce::Rectangle<float>(0, 0, s, s).withCentre(area.getCentre());

        // Image inset (keeps ring tight)
        auto knobRect = dst.reduced(dst.getWidth() * 0.02f);

        // =========================
        // TRACE (no dot, glowing cap)
        // =========================

        // Trace starts at TOP
        const float traceStart = -juce::MathConstants<float>::halfPi;
        const float traceSweep = juce::MathConstants<float>::twoPi * 0.72f;
        const float traceCurrent = traceStart + sliderPos * traceSweep;

        // Color per knob
        const auto name = slider.getName().toLowerCase();
        const bool isRed = (name.contains("gain") || name.contains("master"));
        const juce::Colour traceColour = isRed
            ? juce::Colour::fromRGB(255, 60, 60)   // red
            : juce::Colour::fromRGB(60, 160, 255); // blue

        // Geometry
        const float thickness = dst.getWidth() * 0.045f;
        const float knobRadius = knobRect.getWidth() * 0.5f;
        const float r = knobRadius + thickness * 0.5f + dst.getWidth() * 0.003f;

        const float cx = dst.getCentreX();
        const float cy = dst.getCentreY();

        // Draw trace ONLY if moved
        if (sliderPos > 0.0001f)
        {
            juce::Path arc;
            arc.addCentredArc(cx, cy, r, r, 0.0f, traceStart, traceCurrent, true);

            // --- Subtle glow behind the trace ---
            g.setColour(traceColour.withAlpha(0.35f));
            g.strokePath(arc, juce::PathStrokeType(thickness * 1.9f,
                                                   juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

            // --- Main trace (rounded cap = visual "dot") ---
            g.setColour(traceColour.withAlpha(0.95f));
            g.strokePath(arc, juce::PathStrokeType(thickness,
                                                   juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
        }

        // =====================================
        // 2) KNOB ROTATION (white pointer rule)
        // =====================================
        // Pointer should START at bottom-left at min.
        // Bottom-left ≈ 225° = 5*pi/4
        const float desiredPointerAtMin = juce::MathConstants<float>::pi * 1.25f; // bottom-left
        const float knobAngleOffset = desiredPointerAtMin - pointerAngleInPng;

        // Make the knob rotate through the SAME sweep as the trace (feels consistent)
        const float rot = knobAngleOffset + (traceStart + sliderPos * traceSweep);

        juce::Graphics::ScopedSaveState ss(g);
        g.addTransform(juce::AffineTransform::rotation(rot, knobRect.getCentreX(), knobRect.getCentreY()));

        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImage(knob, knobRect);
    }

private:
    juce::Image knob;
    float pointerAngleInPng = 0.0f;
};
