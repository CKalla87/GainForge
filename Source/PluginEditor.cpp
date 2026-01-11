/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
GainForgeAudioProcessorEditor::GainForgeAudioProcessorEditor (GainForgeAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    loadPanelBackground();

    // Fixed (non-resizable) editor size to match the design screenshot.
    // Screenshot is Retina 2x: 2306x1298 -> points: 1153x649
    setResizable (false, false);
    setSize (1153, 649);

    // Optional: if you added an aspect-ratio constrainer earlier, you can leave it,
    // but it won't matter when resizable is false.

    // Try to load knob filmstrip from BinaryData (if available)
    // If not available, FilmstripLookAndFeel will use fallback metallic rendering
    #ifdef BinaryData_knob_strip_png
    knobFilmstrip = juce::ImageFileFormat::loadFrom (BinaryData::knob_strip_png,
                                                      BinaryData::knob_strip_pngSize);
    if (knobFilmstrip.isValid())
    {
        redKnobLnf.setFilmstrip (knobFilmstrip, 128, true);
        blueKnobLnf.setFilmstrip (knobFilmstrip, 128, true);
    }
    #endif

    // Create knobs - GAIN and MASTER use red LookAndFeel, others use blue
    // Create them first, then add to editor to avoid JUCE accessing them during construction
    gainKnob     = std::make_unique<AmpKnobComponent> ("GAIN",     redKnobLnf);
    bassKnob     = std::make_unique<AmpKnobComponent> ("BASS",     blueKnobLnf);
    midKnob      = std::make_unique<AmpKnobComponent> ("MID",      blueKnobLnf);
    trebleKnob   = std::make_unique<AmpKnobComponent> ("TREBLE",   blueKnobLnf);
    presenceKnob = std::make_unique<AmpKnobComponent> ("PRESENCE", blueKnobLnf);
    masterKnob   = std::make_unique<AmpKnobComponent> ("MASTER",   redKnobLnf);

    // Create toggles
    voiceToggle = std::make_unique<ThreeWayToggle> ("VOICE", "RAW", "MID", "MOD");
    addAndMakeVisible (*voiceToggle);

    modeToggle = std::make_unique<ThreeWayToggle> ("MODE", "CLN", "CRU", "MOD");
    addAndMakeVisible (*modeToggle);

    // Add components but keep them hidden until fully initialized
    // This prevents JUCE 8 from calling resized()/paint() during construction
    for (auto* knob : { gainKnob.get(), bassKnob.get(), midKnob.get(),
                        trebleKnob.get(), presenceKnob.get(), masterKnob.get() })
    {
        if (knob)
        {
            addChildComponent (knob); // Add but don't make visible yet
            knob->setVisible (false); // Explicitly hide
        }
    }

    // Create APVTS attachments
    // Map knob values (0-10) to parameter values (0-1.0)
    auto& apvts = audioProcessor.apvts;

    // Helper to set up knob attachment with value mapping
    auto setupKnobAttachment = [&] (HiddenSlider& hs, const juce::String& paramID, AmpKnobComponent& knobComp)
    {
        // Check parameter exists
        auto* param = apvts.getParameter (paramID);
        if (param == nullptr)
        {
            jassertfalse; // Parameter not found!
            return;
        }

        hs.knobComponent = &knobComp;
        hs.slider->setRange (0.0, 1.0, 0.001);
        
        // Create attachment first - this will sync hidden slider with parameter
        try
        {
            hs.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, paramID, *hs.slider);
        }
        catch (...)
        {
            jassertfalse; // Failed to create attachment
            return;
        }

        // Now set up bidirectional sync callbacks
        // Use safe pointer captures - store pointers in the struct for validation
        juce::Slider* hiddenSliderPtr = hs.slider.get();
        AmpKnobComponent* knobPtr = &knobComp;
        
        // Sync knob (0-10) -> hidden slider (0-1.0) AND update value label
        // Capture by value to avoid dangling references
        knobComp.getSlider().onValueChange = [hiddenSliderPtr, knobPtr, &hs, &apvts, paramID]
        {
            // Validate pointers are still valid
            if (hiddenSliderPtr != nullptr && knobPtr != nullptr && !hs.isInitializing.load())
            {
                const float knobValue = (float) knobPtr->getSlider().getValue();
                const float normalized = knobValue / 10.0f;
                
                // Update the parameter directly (this triggers the attachment)
                if (auto* param = apvts.getParameter (paramID))
                {
                    param->setValueNotifyingHost (normalized);
                }
                
                // Also update hidden slider to keep it in sync
                hiddenSliderPtr->setValue (normalized, juce::dontSendNotification);
                
                // Update the value label (since we overwrote the original callback)
                knobPtr->updateValueLabel();
            }
        };

        // Sync hidden slider -> knob (when parameter changes externally)
        hs.slider->onValueChange = [hiddenSliderPtr, knobPtr, &hs]
        {
            // Validate pointers are still valid
            if (hiddenSliderPtr != nullptr && knobPtr != nullptr && !hs.isInitializing.load())
            {
                const float knobValue = (float) hiddenSliderPtr->getValue() * 10.0f;
                knobPtr->getSlider().setValue (knobValue, juce::dontSendNotification);
                
                // Update the value label after syncing the knob
                knobPtr->updateValueLabel();
            }
        };

        // Initialize: sync knob to current parameter value
        // The attachment already synced the hidden slider, so we just need to sync the knob
        hs.isInitializing.store (true);
        const float paramValue = param->getValue();
        const float knobValue = paramValue * 10.0f;
        knobComp.getSlider().setValue (knobValue, juce::dontSendNotification);
        knobComp.updateValueLabel(); // Update value label after setting initial value
        hs.isInitializing.store (false);
    };

    setupKnobAttachment (hiddenSliders[0], "GAIN", *gainKnob);
    setupKnobAttachment (hiddenSliders[1], "BASS", *bassKnob);
    setupKnobAttachment (hiddenSliders[2], "MID", *midKnob);
    setupKnobAttachment (hiddenSliders[3], "TREBLE", *trebleKnob);
    setupKnobAttachment (hiddenSliders[4], "PRESENCE", *presenceKnob);
    setupKnobAttachment (hiddenSliders[5], "MASTER", *masterKnob);

    // Create toggle attachments
    voiceAttachment = std::make_unique<ThreePositionToggleAttachment>(
        apvts, "VOICE", *voiceToggle);
    modeAttachment = std::make_unique<ThreePositionToggleAttachment>(
        apvts, "MODE", *modeToggle);
    
    // Mark as fully initialized - NOW JUCE can safely call resized()
    isFullyInitialized.store (true);
    
    // Now make all components visible and trigger layout
    for (auto* knob : { gainKnob.get(), bassKnob.get(), midKnob.get(),
                        trebleKnob.get(), presenceKnob.get(), masterKnob.get() })
    {
        if (knob)
            knob->setVisible (true);
    }
    
    // Trigger initial layout now that everything is ready
    resized();
}

GainForgeAudioProcessorEditor::~GainForgeAudioProcessorEditor()
{
    // Mark editor as invalid FIRST to prevent any operations
    isEditorValid.store (false);
    isFullyInitialized.store (false);
    
    // Clear all callbacks FIRST before removing components
    for (auto& hs : hiddenSliders)
    {
        if (hs.slider)
        {
            hs.slider->onValueChange = nullptr;
            hs.slider->setLookAndFeel (nullptr);
        }
        hs.knobComponent = nullptr; // Clear pointer
        hs.attachment.reset(); // Explicitly reset attachment
    }
    
    // Clear LookAndFeel references BEFORE removing components
    // Red knobs (GAIN, MASTER)
    if (gainKnob)
    {
        gainKnob->getSlider().setLookAndFeel (nullptr);
        gainKnob->getSlider().onValueChange = nullptr;
    }
    if (masterKnob)
    {
        masterKnob->getSlider().setLookAndFeel (nullptr);
        masterKnob->getSlider().onValueChange = nullptr;
    }
    
    // Blue knobs (BASS, MID, TREBLE, PRESENCE)
    if (bassKnob)
    {
        bassKnob->getSlider().setLookAndFeel (nullptr);
        bassKnob->getSlider().onValueChange = nullptr;
    }
    if (midKnob)
    {
        midKnob->getSlider().setLookAndFeel (nullptr);
        midKnob->getSlider().onValueChange = nullptr;
    }
    if (trebleKnob)
    {
        trebleKnob->getSlider().setLookAndFeel (nullptr);
        trebleKnob->getSlider().onValueChange = nullptr;
    }
    if (presenceKnob)
    {
        presenceKnob->getSlider().setLookAndFeel (nullptr);
        presenceKnob->getSlider().onValueChange = nullptr;
    }
    
    // Clear toggle callbacks
    if (voiceToggle)
        voiceToggle->onChange = nullptr;
    if (modeToggle)
        modeToggle->onChange = nullptr;
    
    // Reset all attachments BEFORE removing components
    gainAttachment.reset();
    bassAttachment.reset();
    midAttachment.reset();
    trebleAttachment.reset();
    presenceAttachment.reset();
    masterAttachment.reset();
    voiceAttachment.reset();
    modeAttachment.reset();
    
    // NOW remove components from parent (this will trigger their destruction)
    removeAllChildren();
    
    // Reset all component pointers
    gainKnob.reset();
    bassKnob.reset();
    midKnob.reset();
    trebleKnob.reset();
    presenceKnob.reset();
    masterKnob.reset();
    voiceToggle.reset();
    modeToggle.reset();
    
    // Clear images
    panelImage = {};
    knobFilmstrip = {};
}

//==============================================================================
// GAINFORGE Panel Handling
//==============================================================================
void GainForgeAudioProcessorEditor::loadPanelBackground()
{
    panelImage = {};

    // Preferred: BinaryData (after adding panel_bg.png in Projucer)
   #if defined(BinaryData_h) || defined(BinaryData_H) || 1
    if (BinaryData::panel_bg_pngSize > 0)
    {
        panelImage = juce::ImageFileFormat::loadFrom(
            BinaryData::panel_bg_png,
            BinaryData::panel_bg_pngSize
        );
    }
   #endif

    // Temporary fallback: load from disk (DEV ONLY)
    if (! panelImage.isValid())
    {
        const juce::StringArray candidates =
        {
            "Assets/panel_bg.png",
            "Resources/panel_bg.png",
            "../Assets/panel_bg.png",
            "../Resources/panel_bg.png"
        };

        for (auto& path : candidates)
        {
            auto f = juce::File::getCurrentWorkingDirectory().getChildFile(path);
            if (f.existsAsFile())
            {
                panelImage = juce::ImageFileFormat::loadFrom(f);
                if (panelImage.isValid())
                    break;
            }
        }
    }
}

//==============================================================================
static void drawImageCover (juce::Graphics& g, const juce::Image& img, juce::Rectangle<float> dest)
{
    if (!img.isValid())
        return;

    const float iw = (float) img.getWidth();
    const float ih = (float) img.getHeight();
    const float dw = dest.getWidth();
    const float dh = dest.getHeight();

    const float scale = juce::jmax (dw / iw, dh / ih);

    const float sw = dw / scale;
    const float sh = dh / scale;

    const float sx = (iw - sw) * 0.5f;
    const float sy = (ih - sh) * 0.5f;

    g.drawImage (img,
                 dest.getX(), dest.getY(), dw, dh,
                 sx, sy, sw, sh);
}

static juce::Rectangle<float> computePanelContainRect (const juce::Image& img,
                                                        juce::Rectangle<float> dest)
{
    if (!img.isValid())
        return dest;

    const float iw = (float) img.getWidth();
    const float ih = (float) img.getHeight();
    const float dw = dest.getWidth();
    const float dh = dest.getHeight();

    // CONTAIN: show entire image, no cropping
    const float scale = juce::jmin (dw / iw, dh / ih);

    const float rw = iw * scale;
    const float rh = ih * scale;

    return juce::Rectangle<float> (0, 0, rw, rh).withCentre (dest.getCentre());
}

static juce::Rectangle<float> drawPanelCover (juce::Graphics& g,
                                               const juce::Image& img,
                                               juce::Rectangle<float> dest)
{
    if (!img.isValid())
        return dest;

    const float iw = (float) img.getWidth();
    const float ih = (float) img.getHeight();
    const float dw = dest.getWidth();
    const float dh = dest.getHeight();

    // cover: fill dest completely, crop source if needed (no distortion)
    const float scale = juce::jmax (dw / iw, dh / ih);

    const float sw = dw / scale;
    const float sh = dh / scale;

    const float sx = (iw - sw) * 0.5f;
    const float sy = (ih - sh) * 0.5f;

    g.drawImage (img,
                 dest.getX(), dest.getY(), dw, dh,
                 sx, sy, sw, sh);

    return dest; // panel fills the entire editor
}

static juce::Rectangle<float> drawPanelCoverPreferCropSides (juce::Graphics& g,
                                                              const juce::Image& img,
                                                              juce::Rectangle<float> dest,
                                                              float maxTopBottomCropPct = 0.04f) // 4% max crop top/bottom
{
    if (!img.isValid())
        return dest;

    const float iw = (float) img.getWidth();
    const float ih = (float) img.getHeight();
    const float dw = dest.getWidth();
    const float dh = dest.getHeight();

    // Ideal cover scale
    float scale = juce::jmax (dw / iw, dh / ih);

    float sw = dw / scale;
    float sh = dh / scale;

    // If cover would crop too much vertically, reduce scale so we crop more on sides instead.
    const float maxCropY = ih * maxTopBottomCropPct;
    const float cropY = (ih - sh) * 0.5f;

    if (cropY > maxCropY)
    {
        // Use scale based on height (so sh == ih - 2*maxCropY)
        sh = ih - 2.0f * maxCropY;
        scale = dh / sh;
        sw = dw / scale;
    }

    const float sx = (iw - sw) * 0.5f;
    const float sy = (ih - sh) * 0.5f;

    g.drawImage (img,
                 dest.getX(), dest.getY(), dw, dh,
                 sx, sy, sw, sh);

    return dest; // Panel fills the entire editor
}

//==============================================================================
void GainForgeAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Guard against painting during destruction
    if (!isEditorValid.load())
        return;

    g.fillAll (juce::Colours::black);

    if (panelImage.isValid())
        g.drawImage (panelImage, getLocalBounds().toFloat());

    // ---- Title & subtitle (moved down) ----
    auto bounds = getLocalBounds().toFloat();

    // Push the whole title block down by ~12% of height
    const float titleTopPad = bounds.getHeight() * 0.12f;

    auto titleBlock = bounds;
    titleBlock.removeFromTop (titleTopPad);

    // Title block height
    const float titleBlockH = bounds.getHeight() * 0.20f;
    titleBlock = titleBlock.withHeight (titleBlockH);

    // Main title
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (titleBlockH * 0.42f, juce::Font::bold));
    g.drawFittedText ("GAINFORGE",
                      titleBlock.removeFromTop (titleBlockH * 0.60f).toNearestInt(),
                      juce::Justification::centred, 1);

    // Subtitle (a bit lower)
    g.setColour (juce::Colour::fromRGB (220, 38, 38));
    g.setFont (juce::Font (titleBlockH * 0.16f, juce::Font::bold));
    g.drawFittedText ("HIGH GAIN AMPLIFIER",
                      titleBlock.toNearestInt(),
                      juce::Justification::centred, 1);
}

void GainForgeAudioProcessorEditor::resized()
{
    // Guard against resizing during destruction or before initialization
    if (!isEditorValid.load() || !isFullyInitialized.load())
        return;

    // Validate components exist
    if (!gainKnob || !bassKnob || !midKnob || !trebleKnob || !presenceKnob || !masterKnob ||
        !voiceToggle || !modeToggle)
        return;

    // Panel fills window (cover mode with limited top/bottom crop) - same as paint()
    panelDrawArea = getLocalBounds().toFloat(); // drawPanelCoverPreferCropSides fills entire bounds
    auto p = panelDrawArea;

    // Inner safe area inside the metal rails (tune these once if needed)
    auto safe = p.withTrimmedLeft  (p.getWidth()  * 0.08f)
                 .withTrimmedRight (p.getWidth()  * 0.08f)
                 .withTrimmedTop   (p.getHeight() * 0.30f)  // title zone
                 .withTrimmedBottom (p.getHeight() * 0.14f); // bottom rail

    // --- KNOBS: smaller and guaranteed square face area ---
    const float gapX = safe.getWidth() * 0.040f;
    const int cols = 6;
    const float colW = (safe.getWidth() - gapX * (cols - 1)) / cols;

    // smaller than before
    float knobFace = safe.getHeight() * 0.36f;        // was ~0.42
    knobFace = juce::jlimit (110.0f, 160.0f, knobFace);

    // give a dedicated label area below the knob face
    const float valueLabelH = knobFace * 0.42f;       // space for value + label
    const float knobRowH = knobFace + valueLabelH;

    const float knobY = safe.getY() + (safe.getHeight() * 0.44f) - (knobRowH * 0.5f);

    auto setKnobBounds = [&] (juce::Component& c, int index)
    {
        const float colX = safe.getX() + index * (colW + gapX);
        const float cx   = colX + colW * 0.5f;
        const float x    = cx - knobFace * 0.5f;

        // Component bounds include knob + labels, but knob face will be square at top
        c.setBounds ((int) x, (int) knobY, (int) knobFace, (int) knobRowH);
    };

    setKnobBounds (*gainKnob, 0);
    setKnobBounds (*bassKnob, 1);
    setKnobBounds (*midKnob, 2);
    setKnobBounds (*trebleKnob, 3);
    setKnobBounds (*presenceKnob, 4);
    setKnobBounds (*masterKnob, 5);

    // --- Push toggles DOWN a bit to avoid crowding knob labels ---
    const float toggleW = colW * 1.05f;
    const float toggleH = knobFace * 0.55f;

    // spacing between knob row and toggle row
    const float extraGap = getHeight() * 0.035f; // tweak: 0.03–0.05

    // Place toggles lower (but still above the bottom rail)
    float togglesY = safe.getBottom() - toggleH - (safe.getHeight() * 0.01f);

    // Now nudge them DOWN by extraGap, but clamp so they don't go past safe area
    togglesY = juce::jmin (togglesY + extraGap, safe.getBottom() - toggleH);

    auto setToggle = [&] (juce::Component& c, int colIndex)
    {
        const float colX = safe.getX() + colIndex * (colW + gapX);
        const float x = colX + (colW - toggleW) * 0.5f;
        c.setBounds ((int) x, (int) togglesY, (int) toggleW, (int) toggleH);
    };

    setToggle (*voiceToggle, 1);
    setToggle (*modeToggle,  4);
}
