#pragma once

#include <JuceHeader.h>
#include "ThreePositionToggle.h"

//==============================================================================
/**
    Attachment class to connect ThreeWayToggle to AudioProcessorValueTreeState.
    Works with AudioParameterChoice - maps 3-position toggle (0, 1, 2) to choice index.
*/
class ThreePositionToggleAttachment : public juce::AudioProcessorValueTreeState::Listener
{
public:
    ThreePositionToggleAttachment (juce::AudioProcessorValueTreeState& apvts,
                                   const juce::String& paramID,
                                   ThreeWayToggle& toggleToControl)
        : toggle (toggleToControl), param (apvts.getParameter (paramID)), apvtsRef (apvts), paramIDStr (paramID)
    {
        if (param == nullptr)
        {
            jassertfalse; // Parameter not found!
            return;
        }

        // When user clicks toggle -> update parameter
        toggle.onChange = [this] (int newPos) { userChanged (newPos); };

        // Listen for parameter changes
        apvtsRef.addParameterListener (paramIDStr, this);

        // Initialize toggle from parameter
        setFromParameter();
    }

    ~ThreePositionToggleAttachment() override
    {
        apvtsRef.removeParameterListener (paramIDStr, this);
    }

    void parameterChanged (const juce::String&, float) override
    {
        if (param != nullptr)
            setFromParameter();
    }

private:
    void setFromParameter()
    {
        if (param == nullptr)
            return;

        juce::ScopedValueSetter<bool> svs (ignoreCallbacks, true);

        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*> (param))
        {
            // AudioParameterChoice stores index directly
            const int idx = choiceParam->getIndex();
            toggle.setPosition (juce::jlimit (0, 2, idx));
        }
        else
        {
            // Fallback: treat as normalized float (0.0, 0.5, 1.0)
            const float normalized = param->getValue();
            const int idx = juce::jlimit (0, 2, (int) std::round (normalized * 2.0f));
            toggle.setPosition (idx);
        }
    }

    void userChanged (int newPos)
    {
        if (ignoreCallbacks || param == nullptr)
            return;

        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*> (param))
        {
            // Set choice index directly
            choiceParam->beginChangeGesture();
            choiceParam->setValueNotifyingHost (choiceParam->convertTo0to1 (newPos));
            choiceParam->endChangeGesture();
        }
        else
        {
            // Fallback: set normalized value
            param->beginChangeGesture();
            param->setValueNotifyingHost ((float) newPos / 2.0f); // 0, 0.5, 1.0
            param->endChangeGesture();
        }
    }

    ThreeWayToggle& toggle;
    juce::RangedAudioParameter* param;
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramIDStr;
    bool ignoreCallbacks = false;
};

