#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Custom LookAndFeel for rendering knobs from a filmstrip image.
    Simplified version that takes the filmstrip in the constructor.
*/
class FilmstripKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FilmstripKnobLookAndFeel(const juce::Image& strip, int numFrames)
        : filmstrip(strip), frames(numFrames) {}

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        if (!filmstrip.isValid() || frames <= 0)
        {
            juce::LookAndFeel_V4::drawRotarySlider(g, x, y, w, h, sliderPos,
                                                  rotaryStartAngle, rotaryEndAngle, slider);
            return;
        }

        const int frameW = filmstrip.getWidth();
        const int frameH = filmstrip.getHeight() / frames;

        int frameIndex = juce::jlimit(0, frames - 1, (int)std::round(sliderPos * (frames - 1)));

        juce::Rectangle<int> dest(x, y, w, h);

        // source rect for the chosen frame
        juce::Rectangle<int> src(0, frameIndex * frameH, frameW, frameH);

        g.drawImage(filmstrip, dest.toFloat(), src.toFloat(), false);
    }

private:
    juce::Image filmstrip;
    int frames = 0;
};


