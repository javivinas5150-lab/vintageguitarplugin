#include "RigPanel.h"
#include "JeminiLookAndFeel.h"
#include "Realism.h"
#include "Skins.h"

using LnF = JeminiLookAndFeel;

namespace
{
    const juce::Colour warnAmber        (0xffe0a030);
    const juce::Colour stripFace        (0xff17171a);

    // "1.2 kHz" / "850 Hz" / "380 ms" / "-6.0 dB"
    juce::String formatValue (double v, int decimals, const juce::String& unit)
    {
        if (unit == "Hz" && v >= 1000.0)
            return juce::String (v / 1000.0, 1) + " kHz";

        return juce::String (v, decimals) + (unit.isEmpty() ? juce::String() : " " + unit);
    }

    // Deterministic noise texture (tolex / grille fibres)
    juce::Image makeNoise (int w, int h, juce::Colour base, float amount, int seed)
    {
        juce::Image img (juce::Image::ARGB, w, h, true);
        juce::Random rnd (seed);
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);

        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                const float n = (rnd.nextFloat() - 0.5f) * amount;
                bd.setPixelColour (x, y, n > 0 ? base.brighter (n) : base.darker (-n));
            }

        return img;
    }

    // Vintage "wheat" basket-weave grille cloth: thick horizontal tan threads,
    // thinner vertical threads going over/under them, dark backing between.
    // 'scale' = texture pixels per design pixel (render above 1 for crispness).
    juce::Image makeWheatGrille (int w, int h, float scale, int seed, const Skin& sk)
    {
        juce::Random rnd (seed);
        const float pitchY = 6.2f * scale, pitchX = 12.8f * scale;
        const float thickH = 1.75f * scale, thickV = 1.35f * scale;

        // Thread positions with slight irregularity, and per-thread brightness
        std::vector<float> hy, hb, vx, vb;
        for (float y = -pitchY; y < (float) h + pitchY * 2.0f; y += pitchY)
        {
            hy.push_back (y + (rnd.nextFloat() - 0.5f) * 0.25f * pitchY);
            hb.push_back (0.82f + rnd.nextFloat() * 0.3f);
        }
        for (float x = -pitchX; x < (float) w + pitchX * 2.0f; x += pitchX)
        {
            vx.push_back (x + (rnd.nextFloat() - 0.5f) * 0.18f * pitchX);
            vb.push_back (0.75f + rnd.nextFloat() * 0.3f);
        }

        // Slow brightness drift along each horizontal thread (yarn thickness)
        realism::Field drift (w, (int) hy.size());
        drift.randomise (rnd);
        drift.blurH (juce::roundToInt (6.0f * scale));
        drift.normalise();

        const juce::Colour light = sk.grilleLight, mid = sk.grilleMid, shadowCol = sk.grilleShadow, backing = sk.grilleBacking;

        juce::Image img (juce::Image::RGB, w, h, false);
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);

        auto mix = [] (juce::Colour a, juce::Colour b, float t) { return a.interpolatedWith (b, juce::jlimit (0.0f, 1.0f, t)); };

        for (int y = 0; y < h; ++y)
        {
            // nearest horizontal thread
            int k = juce::jlimit (0, (int) hy.size() - 2, (int) (((float) y + pitchY) / pitchY));
            while (k > 0 && hy[(size_t) k] > (float) y) --k;
            const int kNear = (std::abs ((float) y - hy[(size_t) k]) < std::abs ((float) y - hy[(size_t) k + 1])) ? k : k + 1;
            const float dyH = (float) y - hy[(size_t) kNear];
            const float tGap = juce::jlimit (0.0f, 1.0f, ((float) y - hy[(size_t) k]) / (hy[(size_t) k + 1] - hy[(size_t) k]));

            for (int x = 0; x < w; ++x)
            {
                int j = juce::jlimit (0, (int) vx.size() - 2, (int) (((float) x + pitchX) / pitchX));
                while (j > 0 && vx[(size_t) j] > (float) x) --j;
                const int jNear = (std::abs ((float) x - vx[(size_t) j]) < std::abs ((float) x - vx[(size_t) j + 1])) ? j : j + 1;
                const float dxV = (float) x - vx[(size_t) jNear];

                // Gap: dark backing, deepest just under each horizontal thread
                const float shadowAmt = std::exp (-std::pow ((tGap - 0.5f) / 0.17f, 2.0f));
                auto col = mix (backing, shadowCol, 0.15f + 0.8f * shadowAmt);

                // Horizontal thread: round yarn, lit from above
                const float ph = std::exp (-std::pow (dyH / thickH, 2.0f));
                const float litH = juce::jlimit (0.0f, 1.0f, 0.55f - 0.45f * dyH / thickH);
                const float bh = hb[(size_t) kNear] + drift.at (x, kNear) * 0.08f + (rnd.nextFloat() - 0.5f) * 0.2f;
                const auto hCol = mix (mid, light, litH).withMultipliedBrightness (bh);

                // Vertical thread: thinner, slightly darker, lit from the left
                const float pv = std::exp (-std::pow (dxV / thickV, 2.0f));
                const float litV = juce::jlimit (0.0f, 1.0f, 0.5f - 0.4f * dxV / thickV);
                const float bv = vb[(size_t) jNear] + (rnd.nextFloat() - 0.5f) * 0.12f;
                const auto vCol = mix (mid, light, litV).withMultipliedBrightness (bv * 1.05f);

                // Over/under: alternate which thread is on top at each crossing
                const bool verticalOnTop = ((kNear + jNear) & 1) != 0;
                if (verticalOnTop)
                {
                    col = mix (col, hCol, ph * (1.0f - 0.7f * pv));
                    col = mix (col, vCol, pv);
                }
                else
                {
                    col = mix (col, vCol, pv * 0.9f);
                    col = mix (col, hCol, ph);
                    if (ph > 0.3f && pv > 0.3f)   // vertical dives under: small shadow on its sides
                        col = col.darker (0.15f * pv);
                }

                bd.setPixelColour (x, y, col);
            }
        }

        return img;
    }

    // Woven grille cloth (tan / silver threads)
    juce::Image makeGrille (int w, int h, juce::Colour light, juce::Colour dark, int seed)
    {
        juce::Image img (juce::Image::ARGB, w, h, true);
        juce::Graphics g (img);
        g.fillAll (dark);

        juce::Random rnd (seed);
        constexpr float pitch = 4.0f;

        // Diagonal weave in both directions
        for (float d = -(float) h; d < (float) w; d += pitch)
        {
            g.setColour (light.withAlpha (0.55f + rnd.nextFloat() * 0.25f));
            g.drawLine (d, 0.0f, d + (float) h, (float) h, 1.6f);
        }
        for (float d = 0.0f; d < (float) (w + h); d += pitch)
        {
            g.setColour (light.darker (0.35f).withAlpha (0.45f + rnd.nextFloat() * 0.2f));
            g.drawLine (d, 0.0f, d - (float) h, (float) h, 1.2f);
        }

        // Subtle vignette
        juce::ColourGradient shade (juce::Colours::transparentBlack, (float) w * 0.5f, (float) h * 0.4f,
                                    juce::Colours::black.withAlpha (0.45f), 0.0f, (float) h, true);
        g.setGradientFill (shade);
        g.fillAll();
        return img;
    }
}

//==============================================================================
RigPanel::RigPanel (VintageGuitarSuiteAudioProcessor& p)
    : audioProcessor (p)
{
    const auto cream = LnF::cream, ink = LnF::ink;

    // --- VIBE pedal ---
    setupKnob (vibeSpeed,     "vibeSpeed",     "SPEED",     "pedal", ValueBox::none, 1, "Hz", cream);
    setupKnob (vibeIntensity, "vibeIntensity", "INTENSITY", "pedal", ValueBox::none, 0, "%",  cream);
    setupToggle (vibeOn,   vibeOnAttachment,   "vibeOn",   "footswitch", cream);
    setupToggle (vibeMode, vibeModeAttachment, "vibeMode", "modeSwitch", cream);

    // --- GOLD DRIVE pedal ---
    setupKnob (odGain,   "odGain",   "GAIN",   "pedal", ValueBox::none, 1, {}, ink);
    setupKnob (odTreble, "odTreble", "TREBLE", "pedal", ValueBox::none, 1, {}, ink);
    setupKnob (odOutput, "odOutput", "OUTPUT", "pedal", ValueBox::none, 1, {}, ink);
    setupToggle (odOn, odOnAttachment, "odOn", "footswitch", ink);

    // --- NAM DRIVE (alternative pedal in the same slot) ---
    setupKnob (pnamInput,  "pnamInput",  "INPUT",  "pedal", ValueBox::none, 1, "dB", cream);
    setupKnob (pnamTone,   "pnamTone",   "TONE",   "pedal", ValueBox::none, 1, "dB", cream);
    setupKnob (pnamOutput, "pnamOutput", "OUTPUT", "pedal", ValueBox::none, 1, "dB", cream);
    setupToggle (odType, odTypeAttachment, "odType", "modeSwitch", ink);
    odType.onStateChange = [this] { updateDriveType(); };

    setupTextButton (loadPedalButton);
    setupTextButton (clearPedalButton);
    loadPedalButton.onClick  = [this] { choosePedalFile(); };
    clearPedalButton.onClick = [this] { audioProcessor.clearPedalModel(); refreshPedalInfo(); };
    clearPedalButton.setTooltip ("Unload pedal model");
    loadPedalButton.setTooltip ("Load a NAM capture of a drive pedal");

    pedalNameLabel.setJustificationType (juce::Justification::centredLeft);
    pedalNameLabel.setFont (LnF::display (15.0f));
    pedalNameLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8fe0a0));
    pedalNameLabel.setMinimumHorizontalScale (0.5f);
    pedalNameLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (pedalNameLabel);

    for (auto* b : { &vibeOn, &odOn })
        b->onStateChange = [this] { repaint (vibeLedArea.expanded (8)); repaint (odLedArea.expanded (8)); };

    // --- AMP ---
    const std::array<const char*, 5> ampIds   { "ampGain", "ampBass", "ampMiddle", "ampTreble", "ampVolume" };
    const std::array<const char*, 5> ampNames { "GAIN", "BASS", "MIDDLE", "TREBLE", "VOLUME" };
    for (size_t i = 0; i < ampKnobs.size(); ++i)
        setupKnob (ampKnobs[i], ampIds[i], ampNames[i], "blackface", ValueBox::none, 1, "dB", cream);

    setupTextButton (loadNamButton);
    setupTextButton (clearNamButton);
    loadNamButton.onClick  = [this] { chooseNamFile(); };
    clearNamButton.onClick = [this] { audioProcessor.clearNamModel(); refreshAmpInfo(); };
    clearNamButton.setTooltip ("Unload model");
    setupToggle (ampBypass, ampBypassAttachment, "ampBypass", {}, cream);
    ampBypass.onStateChange = [this] { repaint (ampArea); };

    modelNameLabel.setJustificationType (juce::Justification::centred);
    modelNameLabel.setFont (LnF::condensed (19.0f));
    modelNameLabel.setColour (juce::Label::textColourId, ink);
    modelNameLabel.setMinimumHorizontalScale (0.6f);
    addAndMakeVisible (modelNameLabel);

    warningLabel.setJustificationType (juce::Justification::centred);
    warningLabel.setFont (LnF::condensed (12.0f));
    warningLabel.setColour (juce::Label::textColourId, warnAmber);
    addAndMakeVisible (warningLabel);

    // --- RACK ---
    const std::array<const char*, 3> pageNames { "DELAY", "REVERB", "IR VERB" };
    for (int i = 0; i < 3; ++i)
    {
        auto& b = rackSelect[(size_t) i];
        b.setButtonText (pageNames[(size_t) i]);
        b.getProperties().set ("style", "rackSelect");
        b.onClick = [this, i] { setRackPage (i); };
        addAndMakeVisible (b);
    }

    setupToggle (dlyOn,       dlyOnAttachment,       "dlyOn",       {}, cream);
    setupToggle (dlyPingPong, dlyPingPongAttachment, "dlyPingPong", {}, cream);
    setupToggle (revOn,       revOnAttachment,       "revOn",       {}, cream);
    setupToggle (irvOn,       irvOnAttachment,       "irvOn",       {}, cream);
    for (auto* b : { &dlyOn, &revOn, &irvOn })
        b->onStateChange = [this] { repaint (rackArea); };

    struct Def { const char* id; const char* name; int decimals; const char* unit; };
    const std::array<Def, 5> dlyDefs {{ { "dlyTime", "TIME", 0, "ms" }, { "dlyFeedback", "FEEDBACK", 0, "%" },
                                        { "dlyTone", "TONE", 0, "Hz" }, { "dlyMod", "MOD", 0, "%" }, { "dlyMix", "MIX", 0, "%" } }};
    const std::array<Def, 6> revDefs {{ { "revDecay", "DECAY", 1, "s" }, { "revSize", "SIZE", 0, "%" },
                                        { "revPreDelay", "PRE-DELAY", 0, "ms" }, { "revTone", "TONE", 0, "Hz" },
                                        { "revMod", "MOD", 0, "%" }, { "revMix", "MIX", 0, "%" } }};
    const std::array<Def, 5> irvDefs {{ { "irvMix", "MIX", 0, "%" }, { "irvPreDelay", "PRE-DELAY", 0, "ms" },
                                        { "irvLowCut", "LOW CUT", 0, "Hz" }, { "irvHighCut", "HIGH CUT", 0, "Hz" },
                                        { "irvLevel", "LEVEL", 1, "dB" } }};

    for (size_t i = 0; i < dlyKnobs.size(); ++i) setupKnob (dlyKnobs[i], dlyDefs[i].id, dlyDefs[i].name, "rack", ValueBox::below, dlyDefs[i].decimals, dlyDefs[i].unit, cream);
    for (size_t i = 0; i < revKnobs.size(); ++i) setupKnob (revKnobs[i], revDefs[i].id, revDefs[i].name, "rack", ValueBox::below, revDefs[i].decimals, revDefs[i].unit, cream);
    for (size_t i = 0; i < irvKnobs.size(); ++i) setupKnob (irvKnobs[i], irvDefs[i].id, irvDefs[i].name, "rack", ValueBox::below, irvDefs[i].decimals, irvDefs[i].unit, cream);

    setupTextButton (loadIRVButton);
    setupTextButton (clearIRVButton);
    loadIRVButton.onClick  = [this] { chooseIRVerbFile(); };
    clearIRVButton.onClick = [this] { audioProcessor.clearIRVerb(); refreshIRVerbInfo(); };
    clearIRVButton.setTooltip ("Unload IR");

    // --- BOTTOM STRIP ---
    setupTextButton (loadCabButton);
    setupTextButton (clearCabButton);
    loadCabButton.onClick  = [this] { chooseIRFile(); };
    clearCabButton.onClick = [this] { audioProcessor.clearCabIR(); refreshCabInfo(); };
    clearCabButton.setTooltip ("Unload IR");
    setupToggle (cabBypass, cabBypassAttachment, "cabBypass", {}, cream);

    cabNameLabel.setJustificationType (juce::Justification::centredLeft);
    cabNameLabel.setFont (LnF::condensed (18.0f));
    cabNameLabel.setColour (juce::Label::textColourId, cream);
    cabNameLabel.setMinimumHorizontalScale (0.6f);
    addAndMakeVisible (cabNameLabel);

    setupKnob (lowCut,  "cabLowCut",  "LOW CUT",  "pedal", ValueBox::below, 0, "Hz", cream);
    setupKnob (highCut, "cabHighCut", "HIGH CUT", "pedal", ValueBox::below, 0, "Hz", cream);
    setupKnob (cabLevelKnob, "cabLevel", "LEVEL", "pedal", ValueBox::below, 1, "dB", cream);
    cabBypass.onStateChange = [this] { repaint (cabLedArea.expanded (10)); };

    for (int i = 0; i < 2; ++i)
    {
        auto& b = inputButtons[(size_t) i];
        b.setButtonText (juce::String (i + 1));
        b.getProperties().set ("style", "inputSelect");
        b.setTooltip ("Guitar on input " + juce::String (i + 1) + " of your interface");
        b.onClick = [this, i] { audioProcessor.setInputChannel (i); };
        addAndMakeVisible (b);
    }

    setupToggle (gateOn, gateOnAttachment, "gateOn", {}, cream);
    gateOn.onStateChange = [this] { repaint (gateLedArea.expanded (10)); };
    setupKnob (gateThreshold, "gateThreshold", "THRESHOLD", "pedal", ValueBox::below, 1, "dB", cream);
    setupKnob (gateRelease,   "gateRelease",   "RELEASE",   "pedal", ValueBox::below, 0, "ms", cream);

    setupKnob (input,  "inputGain",  "INPUT",  "pedal", ValueBox::below, 1, "dB", cream);
    setupKnob (output, "outputGain", "OUTPUT", "pedal", ValueBox::below, 1, "dB", cream);

    lastNamFolder = lastCabFolder = lastIRVFolder = lastPedalFolder
        = juce::File::getSpecialLocation (juce::File::userDesktopDirectory);

    // --- Title bar: skin selector + tuner ---
    for (int i = 0; i < (int) skins::all().size(); ++i)
        skinBox.addItem (skins::all()[(size_t) i].name.toUpperCase(), i + 1);
    skinBox.setTooltip ("Choose the look of the rig");
    skinBox.onChange = [this] { applySkin (skinBox.getSelectedItemIndex()); };
    addAndMakeVisible (skinBox);

    tunerButton.setClickingTogglesState (true);
    tunerButton.getProperties().set ("style", "tunerButton");
    tunerButton.setTooltip ("Chromatic tuner (mutes the output)");
    tunerAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "tunerOn", tunerButton);
    addAndMakeVisible (tunerButton);

    addChildComponent (tunerOverlay);

    // --- Recorder (standalone only) ---
    recButton.getProperties().set ("style", "recButton");
    recButton.setTooltip ("Record the processed sound to a WAV file");
    recButton.onClick = [this] { toggleRecording(); };
    folderButton.getProperties().set ("style", "presetButton");
    folderButton.setTooltip ("Show the recordings folder");
    folderButton.onClick = []
    {
        const auto folder = Recorder::getRecordingsFolder();
        folder.createDirectory();
        folder.startAsProcess();
    };
    if (audioProcessor.isStandalone())
    {
        addAndMakeVisible (recButton);
        addAndMakeVisible (folderButton);
        recStatus = "READY  -  records exactly what you hear (24-bit WAV)";
    }

    // --- Presets ---
    for (auto* b : { &presetPrev, &presetNext, &presetSave })
    {
        b->getProperties().set ("style", "presetButton");
        addAndMakeVisible (*b);
    }
    presetPrev.setTooltip ("Previous preset");
    presetNext.setTooltip ("Next preset");
    presetSave.setTooltip ("Save the rig in the current preset slot");

    auto step = [this] (int dir)
    {
        auto& bank = audioProcessor.getPresets();
        int slot = bank.getCurrent();
        for (int n = 0; n < PresetManager::numSlots; ++n)      // skip empty slots
        {
            slot = (slot + dir + PresetManager::numSlots) % PresetManager::numSlots;
            if (bank.load (slot)) break;
        }
        refreshPresetDisplay();
    };
    presetPrev.onClick = [step] { step (-1); };
    presetNext.onClick = [step] { step (+1); };
    presetSave.onClick = [this]
    {
        auto& bank = audioProcessor.getPresets();
        int slot = bank.getCurrent();
        if (slot < 0)
            for (int i = 0; i < PresetManager::numSlots && slot < 0; ++i)
                if (bank.isEmpty (i)) slot = i;
        askNameAndStore (juce::jmax (0, slot));
    };

    skinIndex = juce::jlimit (0, (int) skins::all().size() - 1, audioProcessor.getSkin());
    skinBox.setSelectedItemIndex (skinIndex, juce::dontSendNotification);
    buildTextures();
    setRackPage (0);

    refreshAmpInfo();
    refreshCabInfo();
    refreshIRVerbInfo();
    updateDriveType();
    syncTuner();
    refreshPresetDisplay();
    shownInput = audioProcessor.getInputChannel();
    for (int i = 0; i < 2; ++i)
        inputButtons[(size_t) i].setToggleState (i == shownInput, juce::dontSendNotification);
    startTimerHz (30);

    setSize (designWidth, designHeight);
}

//==============================================================================
void RigPanel::setupKnob (Knob& k, const juce::String& paramId, const juce::String& name, const juce::String& style,
                          ValueBox box, int decimals, const juce::String& unit, juce::Colour labelColour)
{
    auto& s = k.slider;
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.getProperties().set ("style", style);

    if (box == ValueBox::below)
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 20);
    else
    {
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setPopupDisplayEnabled (true, true, this);   // value bubble while turning
    }

    s.setColour (juce::Slider::textBoxTextColourId, labelColour);
    addAndMakeVisible (s);

    k.attachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, paramId, s);

    // Tidy value text (set after the attachment, which installs its own)
    s.textFromValueFunction = [decimals, unit] (double v) { return formatValue (v, decimals, unit); };
    s.valueFromTextFunction = [] (const juce::String& t)
    {
        const auto v = t.retainCharacters ("0123456789.-").getDoubleValue();
        return t.containsIgnoreCase ("k") ? v * 1000.0 : v;
    };
    s.updateText();

    // Double-click returns to the parameter's default value
    if (auto* param = audioProcessor.apvts.getParameter (paramId))
        s.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));

    k.label.setText (name, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (LnF::condensed (17.0f));
    k.label.setColour (juce::Label::textColourId, labelColour);
    k.label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (k.label);
}

void RigPanel::setupToggle (juce::ToggleButton& b, std::unique_ptr<ButtonAttachment>& att, const juce::String& paramId,
                            const juce::String& style, juce::Colour textColour)
{
    if (style.isNotEmpty())
        b.getProperties().set ("style", style);

    b.setColour (juce::ToggleButton::textColourId, textColour);
    addAndMakeVisible (b);
    att = std::make_unique<ButtonAttachment> (audioProcessor.apvts, paramId, b);
}

void RigPanel::setupTextButton (juce::TextButton& b)
{
    addAndMakeVisible (b);
}

//==============================================================================
void RigPanel::setRackPage (int page)
{
    rackPage = juce::jlimit (0, 2, page);

    for (int i = 0; i < 3; ++i)
        rackSelect[(size_t) i].setToggleState (i == rackPage, juce::dontSendNotification);

    auto show = [] (bool visible, std::initializer_list<juce::Component*> comps)
    {
        for (auto* c : comps)
            c->setVisible (visible);
    };

    show (rackPage == 0, { &dlyOn, &dlyPingPong });
    show (rackPage == 1, { &revOn });
    show (rackPage == 2, { &irvOn, &loadIRVButton, &clearIRVButton });

    for (auto& k : dlyKnobs) show (rackPage == 0, { &k.slider, &k.label });
    for (auto& k : revKnobs) show (rackPage == 1, { &k.slider, &k.label });
    for (auto& k : irvKnobs) show (rackPage == 2, { &k.slider, &k.label });

    repaint (rackArea);
}

juce::String RigPanel::getDisplayText() const
{
    auto value = [this] (const char* id) { return audioProcessor.apvts.getRawParameterValue (id)->load(); };

    switch (rackPage)
    {
        case 0:  return "1 DELAY  " + juce::String (juce::roundToInt (value ("dlyTime"))) + "ms";
        case 1:  return "2 REVERB " + juce::String (value ("revDecay"), 1) + "s";
        default:
        {
            const auto& irv = audioProcessor.getIRVerb();
            return "3 " + (irv.hasIR() ? irv.getIRName().toUpperCase() : juce::String ("NO IR"));
        }
    }
}

//==============================================================================
void RigPanel::chooseNamFile()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Choose a NAM model", lastNamFolder, "*.nam");

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();
        if (file == juce::File())
            return;

        lastNamFolder = file.getParentDirectory();

        if (const auto error = audioProcessor.loadNamModel (file); error.isNotEmpty())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, JucePlugin_Name, error);

        refreshAmpInfo();
    });
}

void RigPanel::choosePedalFile()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Choose a NAM drive pedal capture", lastPedalFolder, "*.nam");

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();
        if (file == juce::File())
            return;

        lastPedalFolder = file.getParentDirectory();

        if (const auto error = audioProcessor.loadPedalModel (file); error.isNotEmpty())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, JucePlugin_Name, error);

        refreshPedalInfo();
    });
}

void RigPanel::refreshPedalInfo()
{
    const auto& pedal = audioProcessor.getPedalNam();
    const auto name = pedal.hasModel() ? pedal.getModelName().toUpperCase() : juce::String ("NO MODEL");

    if (pedalNameLabel.getText() != name)
        pedalNameLabel.setText (name, juce::dontSendNotification);

    clearPedalButton.setEnabled (pedal.hasModel());
}

void RigPanel::updateDriveType()
{
    namPedalShown = odType.getToggleState();

    for (auto* k : { &odGain, &odTreble, &odOutput })
    {
        k->slider.setVisible (! namPedalShown);
        k->label.setVisible (! namPedalShown);
    }

    for (auto* k : { &pnamInput, &pnamTone, &pnamOutput })
    {
        k->slider.setVisible (namPedalShown);
        k->label.setVisible (namPedalShown);
    }

    for (auto* c : std::initializer_list<juce::Component*> { &loadPedalButton, &clearPedalButton, &pedalNameLabel })
        c->setVisible (namPedalShown);

    refreshPedalInfo();
    repaint (odArea.expanded (4));
}

void RigPanel::chooseIRFile()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Choose a cabinet IR", lastCabFolder, CabIR::getSupportedWildcard());

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();
        if (file == juce::File())
            return;

        lastCabFolder = file.getParentDirectory();

        if (const auto error = audioProcessor.loadCabIR (file); error.isNotEmpty())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, JucePlugin_Name, error);

        refreshCabInfo();
    });
}

void RigPanel::chooseIRVerbFile()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Choose a reverb / delay IR", lastIRVFolder, IRVerb::getSupportedWildcard());

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();
        if (file == juce::File())
            return;

        lastIRVFolder = file.getParentDirectory();

        if (const auto error = audioProcessor.loadIRVerb (file); error.isNotEmpty())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, JucePlugin_Name, error);

        refreshIRVerbInfo();
    });
}

void RigPanel::refreshAmpInfo()
{
    const auto& amp = audioProcessor.getNamAmp();

    const auto name = amp.hasModel() ? amp.getModelName() : juce::String ("NO MODEL LOADED");
    if (modelNameLabel.getText() != name)
    {
        modelNameLabel.setText (name, juce::dontSendNotification);
        repaint (ampArea);
    }

    clearNamButton.setEnabled (amp.hasModel());

    juce::String warning;
    if (amp.hasSampleRateMismatch())
        warning = "Model is " + juce::String (amp.getModelSampleRate() / 1000.0, 1)
                + " kHz - set the project to the same rate for the correct tone";

    if (warningLabel.getText() != warning)
        warningLabel.setText (warning, juce::dontSendNotification);
}

void RigPanel::refreshCabInfo()
{
    const auto& cab = audioProcessor.getCabIR();
    const auto name = cab.hasIR() ? cab.getIRName() : juce::String ("No IR loaded");

    if (cabNameLabel.getText() != name)
    {
        cabNameLabel.setText (name, juce::dontSendNotification);
        repaint (cabLedArea.expanded (10));
    }

    clearCabButton.setEnabled (cab.hasIR());
}

void RigPanel::refreshIRVerbInfo()
{
    clearIRVButton.setEnabled (audioProcessor.getIRVerb().hasIR());
}

//==============================================================================
void RigPanel::timerCallback()
{
    // Gate LED
    const bool gateLed = audioProcessor.isGateClosing();
    if (gateLed != gateLedLit)
    {
        gateLedLit = gateLed;
        repaint (gateLedArea.expanded (8));
    }

    // Output meter with smooth fall-back
    const float decay = 0.85f;
    meterL = juce::jmax (audioProcessor.takeOutputPeak (0), meterL * decay);
    meterR = juce::jmax (audioProcessor.takeOutputPeak (1), meterR * decay);
    repaint (meterArea);

    // Rack display
    if (const auto text = getDisplayText(); text != lastDisplayText)
    {
        lastDisplayText = text;
        repaint (rackDisplayArea);
    }

    refreshAmpInfo();
    refreshPedalInfo();
    refreshCabInfo();
    refreshIRVerbInfo();

    syncTuner();

    if (++presetTick >= 10) { presetTick = 0; refreshPresetDisplay(); }

    // Recording clock (and blinking lamp)
    if (audioProcessor.isStandalone())
    {
        auto& rec = audioProcessor.getRecorder();
        const double secs = rec.isRecording() ? rec.getSeconds() : -1.0;
        if (rec.isRecording() || secs != shownRecSeconds)
        {
            shownRecSeconds = secs;
            repaint (recTimeArea.expanded (4));
            recButton.repaint();
        }
    }

    // Input signal LED (green = signal, red = clipping) and input 1/2 buttons
    const float inPeak = audioProcessor.takeInputPeak();
    inputLevel = juce::jmax (inPeak, inputLevel * 0.8f);
    const bool lit = inputLevel > 0.01f, clip = inputLevel > 0.95f;
    if (lit != inputLedLit || clip != inputClip)
    {
        inputLedLit = lit; inputClip = clip;
        repaint (inputLedArea.expanded (10));
    }

    if (const int sel = audioProcessor.getInputChannel(); sel != shownInput)
    {
        shownInput = sel;
        for (int i = 0; i < 2; ++i)
            inputButtons[(size_t) i].setToggleState (i == sel, juce::dontSendNotification);
        repaint (ampPanelArea);
    }
}

//==============================================================================
// RECORDER
//==============================================================================
void RigPanel::toggleRecording()
{
    auto& rec = audioProcessor.getRecorder();

    if (rec.isRecording())
    {
        rec.stop();
        recStatus = "SAVED:  " + rec.getFile().getFileName();
    }
    else if (rec.start (Recorder::makeNewFileName()))
    {
        recStatus = "RECORDING  >  " + rec.getFile().getFileName();
    }
    else
    {
        recStatus = "CANNOT RECORD  -  start the audio in Options > Audio/MIDI Settings";
    }

    recButton.setToggleState (rec.isRecording(), juce::dontSendNotification);
    repaint (footerBar);
}

//==============================================================================
// PRESETS
//==============================================================================
void RigPanel::refreshPresetDisplay()
{
    auto& bank = audioProcessor.getPresets();
    const int cur = bank.getCurrent();
    const auto text = cur < 0 ? juce::String ("--  NO PRESET")
                              : juce::String (cur + 1).paddedLeft ('0', 2) + "  " + bank.getName (cur) + (bank.isModified() ? " *" : "");
    if (text != presetText)
    {
        presetText = text;
        repaint (presetDisplayArea.expanded (4));
    }
}

void RigPanel::showPresetMenu()
{
    auto& bank = audioProcessor.getPresets();
    const int cur = bank.getCurrent();

    juce::PopupMenu m, saveTo;
    m.addSectionHeader ("PRESETS");
    for (int i = 0; i < PresetManager::numSlots; ++i)
    {
        const auto label = juce::String (i + 1).paddedLeft ('0', 2) + "   " + bank.getName (i);
        m.addItem (i + 1, label, ! bank.isEmpty (i), i == cur);
        saveTo.addItem (200 + i, label);
    }
    m.addSeparator();
    m.addItem (100, cur >= 0 ? "Save to " + juce::String (cur + 1).paddedLeft ('0', 2) + "..." : juce::String ("Save..."));
    m.addSubMenu ("Save to slot", saveTo);
    m.addItem (300, "Rename current...", cur >= 0 && ! bank.isEmpty (cur));
    m.addItem (301, "Clear current", cur >= 0 && ! bank.isEmpty (cur));

    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (presetDisplayArea)),
                     [this, safe = juce::Component::SafePointer<RigPanel> (this)] (int r)
    {
        if (safe == nullptr || r == 0) return;
        auto& b = audioProcessor.getPresets();
        const int current = b.getCurrent();

        if (r >= 1 && r <= PresetManager::numSlots)       b.load (r - 1);
        else if (r == 100)                                 presetSave.onClick();
        else if (r >= 200 && r < 200 + PresetManager::numSlots) askNameAndStore (r - 200);
        else if (r == 301 && current >= 0)                 b.clear (current);
        else if (r == 300 && current >= 0)
        {
            auto* w = new juce::AlertWindow ("Rename preset", "New name for preset " + juce::String (current + 1) + ":",
                                             juce::MessageBoxIconType::NoIcon);
            w->addTextEditor ("name", b.getName (current));
            w->addButton ("Rename", 1, juce::KeyPress (juce::KeyPress::returnKey));
            w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
            w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w, current] (int res)
            {
                if (res == 1) audioProcessor.getPresets().rename (current, w->getTextEditorContents ("name"));
                refreshPresetDisplay();
            }), true);
        }
        refreshPresetDisplay();
    });
}

void RigPanel::askNameAndStore (int slot)
{
    auto& bank = audioProcessor.getPresets();
    const auto suggested = bank.isEmpty (slot) ? juce::String() : bank.getName (slot);

    auto* w = new juce::AlertWindow ("Save preset " + juce::String (slot + 1).paddedLeft ('0', 2),
                                     bank.isEmpty (slot) ? juce::String ("Name for the new preset:")
                                                         : "This replaces \"" + bank.getName (slot) + "\". Name:",
                                     juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", suggested);
    w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w, slot] (int res)
    {
        if (res == 1)
            audioProcessor.getPresets().store (slot, w->getTextEditorContents ("name"));
        refreshPresetDisplay();
    }), true);
}

juce::Point<float> RigPanel::jackCentre (int index) const
{
    return { (float) ampPanelArea.getX() + 36.0f + (float) index * 46.0f, (float) ampPanelArea.getY() + 56.0f };
}

void RigPanel::mouseDown (const juce::MouseEvent& e)
{
    if (presetDisplayArea.contains (e.getPosition()))
    {
        showPresetMenu();
        return;
    }

    for (int j = 0; j < 2; ++j)
        if (e.position.getDistanceFrom (jackCentre (j)) < 15.0f)
            audioProcessor.setInputChannel (j);
}

void RigPanel::mouseMove (const juce::MouseEvent& e)
{
    const bool overJack = e.position.getDistanceFrom (jackCentre (0)) < 15.0f
                       || e.position.getDistanceFrom (jackCentre (1)) < 15.0f
                       || presetDisplayArea.contains (e.getPosition());
    setMouseCursor (overJack ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

// Tuner overlay follows the parameter (button, host automation, CLOSE)
void RigPanel::syncTuner()
{
    if (const bool tuning = audioProcessor.isTunerOn(); tuning != tunerOverlay.isVisible())
    {
        tunerOverlay.setAccent (skin().display);
        tunerOverlay.setVisible (tuning);
        if (tuning) tunerOverlay.toFront (false);
    }
}

//==============================================================================
void RigPanel::buildTextures()
{
    texturesSkin = skinIndex;
    const auto& sk = skin();
    tolex       = sk.covering == Skin::Covering::tweed ? realism::makeTweed (256, sk.covering1, sk.covering2, 7)
                                                       : realism::makeTolex (192, sk.covering1, 7);
    ampGrille   = makeWheatGrille (896 * 2, 112 * 2, 2.0f, 11, sk);
    cabGrille   = makeWheatGrille (500 * 2, 158 * 2, 2.0f, 13, sk);
    brushedRack = realism::makeBrushed (512, 256, sk.rackFace, 5.0f, 21);
    brushedDark = realism::makeBrushed (512, 256, sk.subPanel, 4.0f, 23);
    brushedAlu  = realism::makeBrushed (256, 256, juce::Colour (0xff8d9198), 9.0f, 25);
    brushedPanel = realism::makeBrushed (512, 128, sk.panelTop.interpolatedWith (sk.panelBottom, 0.4f), 7.0f, 33);
    flakes      = realism::makeSpeckle (320, 240, 0.18f, 0.16f, 27);
    paper       = realism::makeSpeckle (256, 128, 0.35f, 0.10f, 29);
    wall        = realism::makeTolex (256, sk.wall, 31);
}

const Skin& RigPanel::skin() const { return skins::get (skinIndex); }

void RigPanel::applySkin (int index)
{
    skinIndex = juce::jlimit (0, (int) skins::all().size() - 1, index);
    audioProcessor.setSkin (skinIndex);
    const auto& sk = skin();

    if (auto* lf = dynamic_cast<JeminiLookAndFeel*> (&getLookAndFeel()))
    {
        lf->ampKnobCap     = sk.knobCap;
        lf->ampKnobNumbers = sk.knobNumbers;
        lf->rackAccent     = sk.rackAccent;
    }

    for (auto& k : ampKnobs)
        k.label.setColour (juce::Label::textColourId, sk.panelText);
    for (auto* k : { &vibeSpeed, &vibeIntensity })
        k->label.setColour (juce::Label::textColourId, sk.vibeText);
    for (auto* k : { &odGain, &odTreble, &odOutput })
        k->label.setColour (juce::Label::textColourId, sk.goldText);
    for (auto* k : { &pnamInput, &pnamTone, &pnamOutput })
        k->label.setColour (juce::Label::textColourId, sk.namText);

    if (skinBox.getSelectedItemIndex() != skinIndex)
        skinBox.setSelectedItemIndex (skinIndex, juce::dontSendNotification);

    if (texturesSkin != skinIndex)
    {
        buildTextures();
        buildBackdrop();
    }
    repaint();
}

//==============================================================================
// LAYOUT (design size 1280 x 720)
//==============================================================================
void RigPanel::resized()
{
    titleBar   = { 0, 0, designWidth, 48 };
    skinBox.setBounds (66, 11, 150, 26);
    tunerButton.setBounds (designWidth - 128, 10, 112, 28);
    footerBar     = { 0, 720, designWidth, 42 };
    recButton.setBounds (22, 725, 32, 32);
    recTimeArea   = { 64, 727, 118, 28 };
    recStatusArea = { 196, 727, 900, 28 };
    folderButton.setBounds (designWidth - 150, 728, 132, 26);

    presetPrev.setBounds (844, 11, 26, 26);
    presetDisplayArea = { 874, 10, 178, 28 };
    presetNext.setBounds (1056, 11, 26, 26);
    presetSave.setBounds (1088, 11, 54, 26);
    tunerOverlay.setBounds (getLocalBounds());
    vibeArea   = { 14, 58, 300, 220 };
    odArea     = { 14, 288, 300, 220 };
    ampArea    = { 326, 58, 940, 272 };
    rackArea   = { 326, 340, 940, 168 };
    bottomArea = { 14, 518, 1252, 190 };

    // ---- VIBE pedal ----
    {
        const auto a = vibeArea;
        const int knobSize = 74;
        vibeSpeed.slider.setBounds     (a.getX() + 90 - knobSize / 2,  a.getY() + 62, knobSize, knobSize);
        vibeIntensity.slider.setBounds (a.getX() + 210 - knobSize / 2, a.getY() + 62, knobSize, knobSize);
        vibeSpeed.label.setBounds      (a.getX() + 40,  a.getY() + 136, 100, 22);
        vibeIntensity.label.setBounds  (a.getX() + 160, a.getY() + 136, 100, 22);

        vibeMode.setBounds (a.getX() + 90 - 20, a.getY() + 178, 40, 24);
        vibeLedArea = juce::Rectangle<int> (12, 12).withCentre ({ a.getX() + 210, a.getY() + 166 });
        vibeOn.setBounds (juce::Rectangle<int> (40, 40).withCentre ({ a.getX() + 210, a.getY() + 194 }));
    }

    // ---- GOLD DRIVE pedal ----
    {
        const auto a = odArea;
        const int knobSize = 66;
        const std::array<Knob*, 3> knobs { &odGain, &odTreble, &odOutput };
        const std::array<int, 3> xs { 58, 150, 242 };
        for (size_t i = 0; i < 3; ++i)
        {
            knobs[i]->slider.setBounds (a.getX() + xs[i] - knobSize / 2, a.getY() + 58, knobSize, knobSize);
            knobs[i]->label.setBounds  (a.getX() + xs[i] - 45, a.getY() + 124, 90, 22);
        }
        odLedArea = juce::Rectangle<int> (12, 12).withCentre ({ a.getX() + 150, a.getY() + 160 });
        odOn.setBounds (juce::Rectangle<int> (40, 40).withCentre ({ a.getX() + 150, a.getY() + 192 }));

        // GOLD / NAM selector (bottom-left, like the Vibe's mode switch)
        odType.setBounds (a.getX() + 58 - 20, a.getY() + 182, 40, 24);

        // NAM DRIVE: model window under the title, smaller knobs below it
        pedalWindowArea = { a.getX() + 22, a.getY() + 52, a.getWidth() - 44, 26 };
        auto win = pedalWindowArea.reduced (3, 2);
        clearPedalButton.setBounds (win.removeFromRight (22));
        win.removeFromRight (4);
        loadPedalButton.setBounds (win.removeFromRight (52));
        win.removeFromRight (6);
        pedalNameLabel.setBounds (win.withTrimmedLeft (6));

        const int namKnob = 54;
        const std::array<Knob*, 3> namKnobs { &pnamInput, &pnamTone, &pnamOutput };
        for (size_t i = 0; i < 3; ++i)
        {
            namKnobs[i]->slider.setBounds (a.getX() + xs[i] - namKnob / 2, a.getY() + 81, namKnob, namKnob);
            namKnobs[i]->label.setBounds  (a.getX() + xs[i] - 45, a.getY() + 133, 90, 19);
        }
    }

    // ---- AMP HEAD ----
    {
        const auto a = ampArea;
        ampPanelArea  = { a.getX() + 22, a.getY() + 34, a.getWidth() - 44, 104 };
        ampGrilleArea = { a.getX() + 22, a.getY() + 146, a.getWidth() - 44, 112 };

        const int knobSize = 72, firstX = ampPanelArea.getX() + 190, spacing = 96;
        for (size_t i = 0; i < ampKnobs.size(); ++i)
        {
            const int cx = firstX + (int) i * spacing;
            ampKnobs[i].slider.setBounds (cx - knobSize / 2, ampPanelArea.getY() + 4, knobSize, knobSize);
            ampKnobs[i].label.setBounds  (cx - 45, ampPanelArea.getY() + 76, 90, 24);
        }

        jewelArea = juce::Rectangle<int> (26, 26).withCentre ({ ampPanelArea.getRight() - 34, ampPanelArea.getCentreY() });

        // Model tag on the grille (right side)
        modelTagArea = juce::Rectangle<int> (330, 80).withPosition (ampGrilleArea.getRight() - 346, ampGrilleArea.getY() + 16);
        auto tag = modelTagArea.reduced (10, 8);
        modelNameLabel.setBounds (tag.removeFromTop (24));
        tag.removeFromTop (6);
        auto row = tag.removeFromTop (26);
        loadNamButton.setBounds (row.removeFromLeft (120));
        row.removeFromLeft (6);
        clearNamButton.setBounds (row.removeFromLeft (28));
        row.removeFromLeft (16);
        ampBypass.setBounds (row.removeFromLeft (110));

        warningLabel.setBounds (ampGrilleArea.getX() + 10, ampGrilleArea.getBottom() - 20, ampGrilleArea.getWidth() - 20, 16);
    }

    // ---- RACK ----
    {
        const auto a = rackArea;
        rackDisplayArea  = { a.getX() + 196, a.getY() + 30, 236, 66 };
        rackListArea     = { a.getX() + 448, a.getY() + 24, 106, 96 };
        rackControlsArea = { a.getX() + 566, a.getY() + 12, 352, 132 };

        for (int i = 0; i < 3; ++i)
        {
            const auto rowArea = juce::Rectangle<int> (rackListArea.getX(), rackListArea.getY() + i * 28, rackListArea.getWidth(), 24);
            rackLedAreas[(size_t) i] = juce::Rectangle<int> (10, 10).withCentre ({ rowArea.getX() + 6, rowArea.getCentreY() });
            rackSelect[(size_t) i].setBounds (rowArea.withTrimmedLeft (18));
        }

        // Header row of the controls: ON (+ PING-PONG / LOAD IR + X)
        const auto head = juce::Rectangle<int> (rackControlsArea.getX(), rackControlsArea.getY(), rackControlsArea.getWidth(), 24);
        for (auto* b : { &dlyOn, &revOn, &irvOn })
            b->setBounds (head.withWidth (60));
        dlyPingPong.setBounds (head.withTrimmedLeft (76).withWidth (110));
        loadIRVButton.setBounds (head.withTrimmedLeft (76).withWidth (100));
        clearIRVButton.setBounds (head.withTrimmedLeft (182).withWidth (26));

        // Knobs: up to 6 across
        auto layoutKnobs = [this] (auto& knobs)
        {
            const auto area = rackControlsArea.withTrimmedTop (28);
            const int w = area.getWidth() / 6;
            for (size_t i = 0; i < knobs.size(); ++i)
            {
                const auto col = juce::Rectangle<int> (area.getX() + (int) i * w, area.getY(), w, area.getHeight());
                knobs[i].label.setBounds (col.withHeight (20));
                knobs[i].slider.setBounds (col.withTrimmedTop (20));
            }
        };
        layoutKnobs (dlyKnobs);
        layoutKnobs (revKnobs);
        layoutKnobs (irvKnobs);
    }

    // ---- BOTTOM STRIP ----
    {
        const auto a = bottomArea;
        cabGrilleArea = { a.getX() + 20, a.getY() + 16, 450, a.getHeight() - 32 };
        cabFileArea   = juce::Rectangle<int> (400, 34).withCentre ({ cabGrilleArea.getCentreX(), cabGrilleArea.getY() + 62 });
        cabLedArea    = juce::Rectangle<int> (12, 12).withCentre ({ cabGrilleArea.getCentreX() - 70, cabGrilleArea.getY() + 22 });

        auto file = cabFileArea.reduced (6, 4);
        clearCabButton.setBounds (file.removeFromRight (26));
        file.removeFromRight (6);
        loadCabButton.setBounds (file.removeFromRight (96));
        file.removeFromRight (8);
        cabNameLabel.setBounds (file.withTrimmedLeft (6));
        cabBypass.setBounds (cabGrilleArea.getCentreX() - 50, cabFileArea.getBottom() + 10, 100, 24);

        int x = cabGrilleArea.getRight() + 16;
        const int top = a.getY() + 16, h = a.getHeight() - 32;
        cabToneArea = { x, top, 230, h };  x += 230 + 12;
        gateArea    = { x, top, 230, h };  x += 230 + 12;
        inputArea   = { x, top, 110, h };  x += 110 + 12;
        outputArea  = { x, top, a.getRight() - 16 - x, h };

        auto placeKnob = [] (Knob& k, juce::Rectangle<int> col)
        {
            k.label.setBounds (col.removeFromTop (22));
            k.slider.setBounds (col.reduced (4, 0));
        };

        auto tone = cabToneArea.withTrimmedTop (34).reduced (6, 6);
        const int third = tone.getWidth() / 3;
        placeKnob (lowCut,  tone.removeFromLeft (third));
        placeKnob (highCut, tone.removeFromLeft (third));
        placeKnob (cabLevelKnob, tone);

        auto gate = gateArea.withTrimmedTop (34).reduced (6, 6);
        auto gateHead = juce::Rectangle<int> (gateArea.getX() + 12, gateArea.getY() + 6, 70, 22);
        gateOn.setBounds (gateHead);
        gateLedArea = juce::Rectangle<int> (12, 12).withCentre ({ gateArea.getRight() - 22, gateArea.getY() + 17 });
        placeKnob (gateThreshold, gate.removeFromLeft (gate.getWidth() / 2));
        placeKnob (gateRelease, gate);

        placeKnob (input, inputArea.withTrimmedTop (34).reduced (6, 6));
        inputLedArea = juce::Rectangle<int> (10, 10).withCentre ({ inputArea.getX() + 16, inputArea.getY() + 16 });
        inputButtons[1].setBounds (inputArea.getRight() - 30, inputArea.getY() + 6, 24, 20);
        inputButtons[0].setBounds (inputArea.getRight() - 56, inputArea.getY() + 6, 24, 20);

        auto out = outputArea.withTrimmedTop (34).reduced (6, 6);
        meterArea = out.removeFromRight (26).withTrimmedTop (20).withTrimmedBottom (20);
        placeKnob (output, out);
    }

    buildBackdrop();
}

//==============================================================================
// DRAWING
//==============================================================================
namespace
{
    // Fill a rounded rectangle with a tiled image texture
    void fillTextured (juce::Graphics& g, juce::Rectangle<float> r, float corner, const juce::Image& tex,
                       float opacity = 1.0f)
    {
        juce::FillType fill (tex, juce::AffineTransform::translation (r.getX(), r.getY()));
        fill.setOpacity (opacity);
        g.setFillType (fill);
        g.fillRoundedRectangle (r, corner);
    }
}

namespace
{
    // Rubbed / chipped finish along the edges of a rounded box: lighter scuffs
    // where hands and cases touch it. Deterministic per box (seeded by position).
    void edgeWear (juce::Graphics& g, juce::Rectangle<float> r, float corner, juce::Colour wearColour,
                   float amount, int count)
    {
        juce::Random rnd ((juce::int64) (r.getX() * 31.0f + r.getY() * 17.0f + r.getWidth()));
        juce::Path outline;
        outline.addRoundedRectangle (r.reduced (2.0f), corner);
        const float len = outline.getLength();

        for (int i = 0; i < count; ++i)
        {
            const float at = rnd.nextFloat() * len;
            const auto p = outline.getPointAlongPath (at);
            const auto q = outline.getPointAlongPath (juce::jmin (len, at + 3.0f + rnd.nextFloat() * 14.0f));
            const float inward = rnd.nextFloat() * 2.5f;
            const auto c = r.getCentre();
            const auto shift = (c - p) * (inward / juce::jmax (1.0f, p.getDistanceFrom (c)));
            g.setColour (wearColour.withAlpha (amount * (0.3f + rnd.nextFloat() * 0.7f)));
            g.drawLine (juce::Line<float> (p + shift, q + shift), 0.8f + rnd.nextFloat() * 1.6f);
        }

        // a few chips: small bright metal flecks
        for (int i = 0; i < count / 6; ++i)
        {
            const auto p = outline.getPointAlongPath (rnd.nextFloat() * len);
            const float sz = 0.8f + rnd.nextFloat() * 2.0f;
            g.setColour (juce::Colour (0xffd6d8dc).withAlpha (amount * (0.6f + rnd.nextFloat() * 0.4f)));
            g.fillEllipse (juce::Rectangle<float> (sz * 1.6f, sz).withCentre (p));
        }
    }

    // Fine hairline scratches on a flat surface
    void scratches (juce::Graphics& g, juce::Rectangle<float> r, int count, float alpha, int seed)
    {
        juce::Random rnd (seed);
        for (int i = 0; i < count; ++i)
        {
            const auto p = juce::Point<float> (r.getX() + rnd.nextFloat() * r.getWidth(), r.getY() + rnd.nextFloat() * r.getHeight());
            const float a = rnd.nextFloat() * juce::MathConstants<float>::pi;
            const float l = 6.0f + rnd.nextFloat() * 26.0f;
            g.setColour (juce::Colours::white.withAlpha (alpha * (0.4f + rnd.nextFloat() * 0.6f)));
            g.drawLine (p.x, p.y, p.x + std::cos (a) * l, p.y + std::sin (a) * l, 0.6f);
        }
    }
}

// Background wall, title bar and the soft shadows every unit casts on it.
// Drawn once at design size; paint() just blits it.
void RigPanel::buildBackdrop()
{
    backdrop = juce::Image (juce::Image::RGB, designWidth, designHeight, true);
    juce::Graphics g (backdrop);

    // Dark acoustic wall with a warm spotlight from above
    g.setFillType (juce::FillType (wall, juce::AffineTransform()));
    g.fillAll();
    const auto& sk = skin();
    juce::ColourGradient spot (sk.spotlight, (float) designWidth * 0.45f, 40.0f,
                               juce::Colour (0x00000000), (float) designWidth * 0.45f, (float) designHeight * 0.95f, true);
    g.setGradientFill (spot);
    g.fillAll();
    g.fillAll (juce::Colours::black.withAlpha (0.3f));

    // Title bar: dark walnut-ish header with a lit edge
    {
        const auto bar = titleBar.toFloat();
        juce::ColourGradient wood (sk.woodTop, 0.0f, 0.0f, sk.woodBottom, 0.0f, bar.getBottom(), false);
        g.setGradientFill (wood);
        g.fillRect (bar);
        juce::Random rnd (41);
        for (int i = 0; i < 70; ++i)   // wood grain
        {
            const float y = rnd.nextFloat() * bar.getHeight();
            g.setColour ((rnd.nextBool() ? juce::Colours::black : sk.woodGrain).withAlpha (0.04f + rnd.nextFloat() * 0.05f));
            juce::Path grain;
            grain.startNewSubPath (0.0f, y);
            for (float x = 0.0f; x <= (float) designWidth; x += 160.0f)
                grain.lineTo (x, y + std::sin (x * 0.004f + (float) i) * 2.0f);
            g.strokePath (grain, juce::PathStrokeType (0.6f + rnd.nextFloat()));
        }
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.fillRect (bar.withTop (bar.getBottom() - 2.0f));
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.fillRect (bar.withHeight (1.0f));
        realism::dropShadow (g, bar.withTop (bar.getBottom() - 4.0f), 0.0f, 10, { 0, 4 }, 0.6f);

        // Engraved brushed-aluminium nameplate, screwed to the wood
        const auto plate = juce::Rectangle<float> (380.0f, 34.0f).withCentre (bar.getCentre());
        realism::dropShadow (g, plate, 4.0f, 6, { 0, 3 }, 0.8f);
        g.setFillType (juce::FillType (brushedAlu, juce::AffineTransform::translation (plate.getX(), plate.getY())));
        g.fillRoundedRectangle (plate, 4.0f);
        juce::ColourGradient sheenGrad (juce::Colours::white.withAlpha (0.28f), plate.getX(), plate.getY(),
                                        juce::Colours::black.withAlpha (0.18f), plate.getX(), plate.getBottom(), false);
        sheenGrad.addColour (0.5, juce::Colours::white.withAlpha (0.04f));
        g.setGradientFill (sheenGrad);
        g.fillRoundedRectangle (plate, 4.0f);
        realism::bevel (g, plate, 4.0f, 1.5f, 0.7f, 0.6f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (plate, 4.0f, 0.8f);

        // engraving: dark cut with a light lower lip
        const auto textArea = plate.reduced (34.0f, 0.0f).translated (0.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        LnF::drawSlantedLogo (g, "JEMINI GUITAR RIG", textArea.translated (0.0f, 1.0f), 24.0f, juce::Justification::centred);
        g.setColour (juce::Colour (0xff17181b));
        LnF::drawSlantedLogo (g, "JEMINI GUITAR RIG", textArea, 24.0f, juce::Justification::centred);

        for (auto p : { juce::Point<float> (plate.getX() + 14.0f, plate.getCentreY()), juce::Point<float> (plate.getRight() - 14.0f, plate.getCentreY()) })
        {
            realism::softShadowEllipse (g, p, 4.5f, 2.0f, { 0.6f, 1.2f }, 0.45f);
            realism::spunMetal (g, p, 4.5f, juce::Colour (0xffb8bcc2), 55.0f);
            g.setColour (juce::Colours::black.withAlpha (0.65f));
            g.drawLine (p.x - 3.0f, p.y + 1.0f, p.x + 3.0f, p.y - 1.0f, 1.1f);
        }
    }

    // Footer: the same walnut, lit from above (its top edge catches the light)
    {
        const auto bar = footerBar.toFloat();
        juce::ColourGradient wood (sk.woodTop, 0.0f, bar.getY(), sk.woodBottom, 0.0f, bar.getBottom(), false);
        g.setGradientFill (wood);
        g.fillRect (bar);
        juce::Random rnd (43);
        for (int i = 0; i < 60; ++i)
        {
            const float y = bar.getY() + rnd.nextFloat() * bar.getHeight();
            g.setColour ((rnd.nextBool() ? juce::Colours::black : sk.woodGrain).withAlpha (0.04f + rnd.nextFloat() * 0.05f));
            juce::Path grain;
            grain.startNewSubPath (0.0f, y);
            for (float x = 0.0f; x <= (float) designWidth; x += 160.0f)
                grain.lineTo (x, y + std::sin (x * 0.004f + (float) i) * 2.0f);
            g.strokePath (grain, juce::PathStrokeType (0.6f + rnd.nextFloat()));
        }
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.fillRect (bar.withHeight (2.0f));
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.fillRect (bar.withY (bar.getY() + 2.0f).withHeight (1.0f));

        if (audioProcessor.isStandalone())
        {
            // recessed LCD windows for the clock and the status line
            for (auto r : { recTimeArea.toFloat(), recStatusArea.toFloat() })
            {
                g.setColour (juce::Colour (0xff050506));
                g.fillRoundedRectangle (r, 4.0f);
                juce::ColourGradient glassGrad (sk.displayGlass, r.getX(), r.getY(), sk.displayGlass.darker (0.8f), r.getX(), r.getBottom(), false);
                g.setGradientFill (glassGrad);
                g.fillRoundedRectangle (r.reduced (2.0f), 3.0f);
                realism::innerShadow (g, r.reduced (2.0f), 3.0f, 4.0f, 0.9f);
                g.setColour (juce::Colours::white.withAlpha (0.14f));
                g.drawRoundedRectangle (r.translated (0.0f, 1.0f), 4.0f, 0.7f);
            }
        }
        else
        {
            g.setColour (sk.titleText.withAlpha (0.35f));
            g.setFont (LnF::condensed (14.0f));
            g.drawText ("JEMINI GUITAR RIG   -   NAM AMP  /  PEDALS  /  STUDIO EFFECTS", bar, juce::Justification::centred, false);
        }
    }

    // Soft shadows: a wide ambient one plus a tight contact one per unit
    auto castShadow = [&g] (juce::Rectangle<int> r, float corner)
    {
        realism::dropShadow (g, r.toFloat(), corner, 26, { 4, 14 }, 0.75f);
        realism::dropShadow (g, r.toFloat(), corner, 5,  { 1, 3 },  0.85f);
    };
    castShadow (vibeArea.withHeight (vibeArea.getHeight() + 7), 12.0f);
    castShadow (odArea.withHeight (odArea.getHeight() + 7), 12.0f);
    castShadow (ampArea.withTrimmedTop (14), 10.0f);
    castShadow (rackArea, 4.0f);
    castShadow (bottomArea, 8.0f);

    // Parts sitting on top of the units: label, badge, control sub-panels
    mountedShadows = juce::Image (juce::Image::ARGB, designWidth, designHeight, true);
    juce::Graphics ms (mountedShadows);
    realism::dropShadow (ms, modelTagArea.toFloat(), 6.0f, 6, { 2, 4 }, 0.7f);
    realism::dropShadow (ms, juce::Rectangle<float> (170.0f, 28.0f).withCentre ({ cabGrilleArea.toFloat().getCentreX(), (float) cabGrilleArea.getY() + 22.0f }),
                         5.0f, 5, { 1, 3 }, 0.7f);
    for (auto r : { cabToneArea, gateArea, inputArea, outputArea })
        realism::dropShadow (ms, r.toFloat(), 6.0f, 6, { 0, 3 }, 0.75f);
}

// Shadows of parts mounted on the units (drawn over the unit bodies, also cached)
void RigPanel::drawMountedShadows (juce::Graphics& g, juce::Rectangle<int> within) const
{
    juce::Graphics::ScopedSaveState s (g);
    g.reduceClipRegion (within);
    g.drawImageAt (mountedShadows, 0, 0);
}

void RigPanel::paint (juce::Graphics& g)
{
    g.drawImageAt (backdrop, 0, 0);

    const auto& sk = skin();
    g.setColour (sk.titleText.withAlpha (0.7f));
    g.setFont (LnF::condensed (15.0f));
    g.drawText ("SKIN", juce::Rectangle<int> (18, 11, 44, 26), juce::Justification::centredRight, false);

    // Preset display: recessed LCD window in the rack display colour
    {
        const auto d = presetDisplayArea.toFloat();
        g.setColour (juce::Colour (0xff050506));
        g.fillRoundedRectangle (d, 4.0f);
        juce::ColourGradient glass (sk.displayGlass, d.getX(), d.getY(), sk.displayGlass.darker (0.8f), d.getX(), d.getBottom(), false);
        g.setGradientFill (glass);
        g.fillRoundedRectangle (d.reduced (2.0f), 3.0f);
        realism::innerShadow (g, d.reduced (2.0f), 3.0f, 4.0f, 0.9f);
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.drawRoundedRectangle (d.translated (0.0f, 1.0f), 4.0f, 0.7f);

        const auto t = d.reduced (9.0f, 2.0f);
        g.setFont (LnF::display (16.0f));
        g.setColour (sk.display.withAlpha (0.25f));
        g.drawText (presetText, t.translated (0.0f, 1.0f), juce::Justification::centredLeft, true);
        g.setColour (sk.display.brighter (0.2f));
        g.drawText (presetText, t, juce::Justification::centredLeft, true);
        realism::sheen (g, d.reduced (2.0f), 3.0f, 0.08f);
    }

    // Recorder clock + status (standalone)
    if (audioProcessor.isStandalone())
    {
        auto& rec = audioProcessor.getRecorder();
        const bool recording = rec.isRecording();
        const double secs = recording ? rec.getSeconds() : 0.0;
        const int mins = (int) (secs / 60.0);
        const auto clock = juce::String (mins).paddedLeft ('0', 2) + ":" + juce::String (std::fmod (secs, 60.0), 1).paddedLeft ('0', 4);

        const auto lcd = recTimeArea.toFloat().reduced (8.0f, 2.0f);
        const auto colour = recording ? juce::Colour (0xffff4a3a) : sk.display.withAlpha (0.55f);
        g.setFont (LnF::display (18.0f));
        g.setColour (colour.withAlpha (0.25f));
        g.drawText (clock, lcd.translated (0.0f, 1.0f), juce::Justification::centred, false);
        g.setColour (colour);
        g.drawText (clock, lcd, juce::Justification::centred, false);

        g.setFont (LnF::display (15.0f));
        g.setColour (recording ? juce::Colour (0xffff4a3a) : sk.display.withAlpha (0.8f));
        g.drawText (recStatus, recStatusArea.toFloat().reduced (12.0f, 2.0f), juce::Justification::centredLeft, true);

        realism::sheen (g, recTimeArea.toFloat().reduced (2.0f), 3.0f, 0.08f);
        realism::sheen (g, recStatusArea.toFloat().reduced (2.0f), 3.0f, 0.06f);
    }

    // ---- Pedals ----
    drawPedal (g, vibeArea.toFloat(), sk.vibeTop, sk.vibeBottom);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    LnF::drawSlantedLogo (g, "VIBE", vibeArea.withHeight (60).translated (1, 8).toFloat(), 34.0f, juce::Justification::centred);
    g.setColour (sk.vibeText);
    LnF::drawSlantedLogo (g, "VIBE", vibeArea.withHeight (60).translated (0, 6).toFloat(), 34.0f, juce::Justification::centred);

    g.setFont (LnF::condensed (14.0f));
    g.drawText ("CHORUS",  juce::Rectangle<int> (vibeArea.getX() + 14, vibeArea.getY() + 182, 52, 16), juce::Justification::centredRight, false);
    g.drawText ("VIBRATO", juce::Rectangle<int> (vibeArea.getX() + 114, vibeArea.getY() + 182, 56, 16), juce::Justification::centredLeft, false);
    drawLed (g, vibeLedArea, vibeOn.getToggleState(), LnF::redLed);

    if (! namPedalShown)
    {
        drawPedal (g, odArea.toFloat(), sk.goldTop, sk.goldBottom);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        LnF::drawSlantedLogo (g, "GOLD DRIVE", odArea.withHeight (54).translated (1, 7).toFloat(), 30.0f, juce::Justification::centred);
        g.setColour (sk.goldText);
        LnF::drawSlantedLogo (g, "GOLD DRIVE", odArea.withHeight (54).translated (0, 6).toFloat(), 30.0f, juce::Justification::centred);
    }
    else
    {
        drawPedal (g, odArea.toFloat(), sk.namTop, sk.namBottom);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        LnF::drawSlantedLogo (g, "NAM DRIVE", odArea.withHeight (50).translated (1, 6).toFloat(), 28.0f, juce::Justification::centred);
        g.setColour (sk.namText);
        LnF::drawSlantedLogo (g, "NAM DRIVE", odArea.withHeight (50).translated (0, 4).toFloat(), 28.0f, juce::Justification::centred);

        // Model window: dark recessed LCD-style strip
        const auto win = pedalWindowArea.toFloat();
        g.setColour (juce::Colour (0xff070908));
        g.fillRoundedRectangle (win, 4.0f);
        realism::innerShadow (g, win, 4.0f, 5.0f, 0.9f);
        g.setColour (juce::Colours::white.withAlpha (0.16f));
        g.drawRoundedRectangle (win.translated (0.0f, 1.0f), 4.0f, 0.7f);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawRoundedRectangle (win, 4.0f, 1.0f);
    }

    // GOLD / NAM selector labels
    {
        const auto labelColour = namPedalShown ? sk.namText : sk.goldText;
        const auto sw = odType.getBounds();
        g.setFont (LnF::condensed (14.0f));
        g.setColour (labelColour.withAlpha (namPedalShown ? 0.55f : 1.0f));
        g.drawText ("GOLD", juce::Rectangle<int> (sw.getX() - 40, sw.getY(), 38, sw.getHeight()), juce::Justification::centredRight, false);
        g.setColour (labelColour.withAlpha (namPedalShown ? 1.0f : 0.55f));
        g.drawText ("NAM", juce::Rectangle<int> (sw.getRight() + 3, sw.getY(), 38, sw.getHeight()), juce::Justification::centredLeft, false);
    }
    drawLed (g, odLedArea, odOn.getToggleState(), LnF::redLed);

    // ---- Amp, rack, bottom strip ----
    drawAmpHead (g);
    drawRack (g);
    drawBottomStrip (g);
}

// Room lighting over everything (controls included): soft vignette
void RigPanel::paintOverChildren (juce::Graphics& g)
{
    juce::ColourGradient vignette (juce::Colours::transparentBlack, (float) designWidth * 0.5f, (float) designHeight * 0.42f,
                                   juce::Colours::black.withAlpha (0.38f), 0.0f, (float) designHeight * 1.05f, true);
    vignette.addColour (0.55, juce::Colours::transparentBlack);
    g.setGradientFill (vignette);
    g.fillRect (getLocalBounds().withTrimmedTop (titleBar.getBottom()).withBottom (footerBar.getY()));
}

//------------------------------------------------------------------------------
void RigPanel::drawPedal (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour top, juce::Colour bottom) const
{
    constexpr float corner = 12.0f;

    // Side wall of the die-cast box (we look at it slightly from the front)
    {
        const auto side = r.translated (0.0f, 7.0f);
        juce::ColourGradient sideShade (bottom.darker (0.35f), side.getX(), side.getBottom() - 10.0f,
                                        bottom.darker (1.4f), side.getX(), side.getBottom(), false);
        g.setGradientFill (sideShade);
        g.fillRoundedRectangle (side, corner);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawHorizontalLine ((int) side.getBottom() - 2, side.getX() + corner, side.getRight() - corner);
    }

    // Die-cast enclosure: rounded edges catch the light, flat top is lit from above
    juce::ColourGradient body (top.brighter (0.12f), r.getX(), r.getY(), bottom, r.getX() + r.getWidth() * 0.3f, r.getBottom(), false);
    body.addColour (0.55, top.interpolatedWith (bottom, 0.5f));
    g.setGradientFill (body);
    g.fillRoundedRectangle (r, corner);

    // Metallic flake / powder coat
    fillTextured (g, r, corner, flakes);

    // Broad soft reflection on the top-left
    juce::ColourGradient glow (juce::Colours::white.withAlpha (0.16f), r.getX() + r.getWidth() * 0.25f, r.getY() + r.getHeight() * 0.1f,
                               juce::Colours::transparentWhite, r.getX() + r.getWidth() * 0.9f, r.getY() + r.getHeight() * 0.8f, true);
    g.setGradientFill (glow);
    g.fillRoundedRectangle (r, corner);

    // Rounded die-cast edge: bright rim on top, dark rim at the bottom
    realism::bevel (g, r, corner, 5.0f, 0.38f, 0.55f);
    realism::bevel (g, r.reduced (5.0f), corner - 5.0f, 1.5f, 0.0f, 0.0f);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawRoundedRectangle (r.reduced (5.0f), corner - 5.0f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.0f);

    // Gig wear: rubbed edges, chips and hairline scratches
    edgeWear (g, r, corner, juce::Colours::white, 0.22f, 90);
    scratches (g, r.reduced (16.0f), 14, 0.07f, (int) r.getY());

    // Bottom-plate screws
    for (auto p : { r.getTopLeft().translated (13, 13), r.getTopRight().translated (-13, 13),
                    r.getBottomLeft().translated (13, -13), r.getBottomRight().translated (-13, -13) })
        drawScrew (g, p, 5.0f);
}

void RigPanel::drawScrew (juce::Graphics& g, juce::Point<float> c, float radius) const
{
    realism::softShadowEllipse (g, c, radius, radius * 0.5f, { 0.6f, 1.4f }, 0.35f);

    juce::ColourGradient head (juce::Colour (0xfff2f3f5), c.translated (-radius, -radius),
                               juce::Colour (0xff4e5258), c.translated (radius * 0.8f, radius), false);
    g.setGradientFill (head);
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c));
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c), 0.6f);

    // Phillips recess: dark cut with a lit lower edge
    for (auto offs : { juce::Point<float> (0.0f, 0.0f), juce::Point<float> (0.5f, 0.6f) })
    {
        g.setColour (offs.x == 0.0f ? juce::Colours::black.withAlpha (0.7f) : juce::Colours::white.withAlpha (0.35f));
        const auto cc = c + offs;
        g.drawLine (cc.x - radius * 0.6f, cc.y, cc.x + radius * 0.6f, cc.y, offs.x == 0.0f ? 1.3f : 0.5f);
        g.drawLine (cc.x, cc.y - radius * 0.6f, cc.x, cc.y + radius * 0.6f, offs.x == 0.0f ? 1.3f : 0.5f);
    }
}

void RigPanel::drawLed (juce::Graphics& g, juce::Rectangle<int> area, bool on, juce::Colour colour) const
{
    const auto r = area.toFloat();
    const auto c = r.getCentre();
    const float rad = r.getWidth() * 0.5f;

    // Light spill on the panel
    if (on)
    {
        juce::ColourGradient spill (colour.withAlpha (0.55f), c, colour.withAlpha (0.0f), c.translated (rad * 4.0f, 0.0f), true);
        g.setGradientFill (spill);
        g.fillEllipse (r.expanded (rad * 3.0f));
    }

    // Chrome bezel
    const auto bez = r.expanded (rad * 0.45f);
    g.setGradientFill (realism::chrome (bez));
    g.fillEllipse (bez);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (bez, 0.7f);

    // Lens: hot core when lit, deep tinted plastic when off
    juce::ColourGradient lens (on ? colour.brighter (0.9f) : colour.darker (2.2f), c.translated (-rad * 0.2f, -rad * 0.2f),
                               on ? colour.darker (0.2f) : colour.darker (4.0f), c.translated (rad, rad), true);
    g.setGradientFill (lens);
    g.fillEllipse (r);
    if (on)
    {
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.fillEllipse (juce::Rectangle<float> (rad * 0.7f, rad * 0.7f).withCentre (c));
    }
    realism::specular (g, c, rad, on ? 0.7f : 0.45f);
}

//------------------------------------------------------------------------------
void RigPanel::drawAmpHead (juce::Graphics& g) const
{
    const auto& sk = skin();
    const auto a = ampArea.toFloat();

    // Leather strap handle with chrome end brackets
    {
        const auto strap = juce::Rectangle<float> (230.0f, 15.0f).withCentre ({ a.getCentreX(), a.getY() + 7.0f });
        juce::ColourGradient leather (juce::Colour (0xff3a3532), strap.getX(), strap.getY(),
                                      juce::Colour (0xff0a0909), strap.getX(), strap.getBottom(), false);
        g.setGradientFill (leather);
        g.fillRoundedRectangle (strap, 7.0f);
        fillTextured (g, strap, 7.0f, tolex, 0.5f);
        g.setColour (juce::Colour (0x55d8cfc0));
        const float dashes[] = { 3.0f, 3.0f };
        g.drawDashedLine (juce::Line<float> (strap.getX() + 12.0f, strap.getY() + 3.5f, strap.getRight() - 12.0f, strap.getY() + 3.5f), dashes, 2, 0.8f);
        g.drawDashedLine (juce::Line<float> (strap.getX() + 12.0f, strap.getBottom() - 3.5f, strap.getRight() - 12.0f, strap.getBottom() - 3.5f), dashes, 2, 0.8f);
        for (float x : { strap.getX() - 14.0f, strap.getRight() - 4.0f })
        {
            const auto bracket = juce::Rectangle<float> (x, strap.getY() - 1.0f, 18.0f, 18.0f);
            g.setGradientFill (realism::chrome (bracket));
            g.fillRoundedRectangle (bracket, 4.0f);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.drawRoundedRectangle (bracket, 4.0f, 0.8f);
        }
    }

    // Tolex-covered head
    const auto cab = a.withTrimmedTop (14.0f);
    fillTextured (g, cab, 10.0f, tolex);
    juce::ColourGradient light (juce::Colours::white.withAlpha (0.07f), cab.getX(), cab.getY(),
                                juce::Colours::black.withAlpha (0.25f), cab.getX(), cab.getBottom(), false);
    g.setGradientFill (light);
    g.fillRoundedRectangle (cab, 10.0f);
    realism::bevel (g, cab, 10.0f, 4.0f, 0.22f, 0.6f);
    edgeWear (g, cab, 10.0f, juce::Colours::white, 0.09f, 260);

    // Chrome corner protectors with rivets
    for (int i = 0; i < 4; ++i)
    {
        const bool right = (i & 1) != 0, bottom = (i & 2) != 0;
        constexpr float len = 30.0f;

        // Curved-triangle cap for the top-left corner, mirrored for the others
        juce::Path capShape;
        capShape.startNewSubPath (0.0f, len);
        capShape.lineTo (0.0f, 10.0f);
        capShape.quadraticTo (0.0f, 0.0f, 10.0f, 0.0f);
        capShape.lineTo (len, 0.0f);
        capShape.quadraticTo (len * 0.42f, len * 0.42f, 0.0f, len);
        capShape.closeSubPath();

        const auto t = juce::AffineTransform::scale (right ? -1.0f : 1.0f, bottom ? -1.0f : 1.0f)
                           .translated (right ? cab.getRight() : cab.getX(), bottom ? cab.getBottom() : cab.getY());
        capShape.applyTransform (t);
        const auto cb = capShape.getBounds();

        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillPath (capShape, juce::AffineTransform::translation (1.0f, 2.5f));
        juce::ColourGradient metal (juce::Colour (0xfff4f5f7), cb.getX(), cb.getY(),
                                    juce::Colour (0xff5d6168), cb.getRight(), cb.getBottom(), false);
        metal.addColour (0.45, juce::Colour (0xffc4c8ce));
        metal.addColour (0.55, juce::Colour (0xff7d8188));
        g.setGradientFill (metal);
        g.fillPath (capShape);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.strokePath (capShape, juce::PathStrokeType (0.8f));

        const auto rivet = juce::Point<float> (right ? cab.getRight() - 8.0f : cab.getX() + 8.0f,
                                               bottom ? cab.getBottom() - 8.0f : cab.getY() + 8.0f);
        drawScrew (g, rivet, 2.6f);
    }

    // --- Control panel (black panel with chrome trim, slightly recessed) ---
    const auto panel = ampPanelArea.toFloat();
    if (sk.brushedPanel)
    {
        fillTextured (g, panel, 0.0f, brushedPanel);
        juce::ColourGradient tint (juce::Colours::white.withAlpha (0.18f), panel.getX(), panel.getY(),
                                   juce::Colours::black.withAlpha (0.22f), panel.getX(), panel.getBottom(), false);
        g.setGradientFill (tint);
        g.fillRect (panel);
    }
    else
    {
        juce::ColourGradient face (sk.panelTop, panel.getTopLeft(), sk.panelBottom, panel.getBottomLeft(), false);
        g.setGradientFill (face);
        g.fillRect (panel);
    }
    realism::sheen (g, panel, 0.0f, 0.05f);
    realism::innerShadow (g, panel, 0.0f, 8.0f, 0.9f);
    g.setGradientFill (realism::chrome (panel));
    g.drawRect (panel, 2.5f);
    g.setColour (sk.panelTrim.withAlpha (0.45f));
    g.drawRect (panel.reduced (6.0f), 1.0f);
    for (auto p : { panel.getTopLeft().translated (12, 12), panel.getTopRight().translated (-12, 12),
                    panel.getBottomLeft().translated (12, -12), panel.getBottomRight().translated (-12, -12) })
        drawScrew (g, p, 3.2f);

    // Channel label + input jacks (hex nut, chrome ring, dark socket)
    g.setColour (sk.panelText);
    g.setFont (LnF::condensed (18.0f));
    g.drawText ("NAM", juce::Rectangle<float> (panel.getX() + 22.0f, panel.getY() + 12.0f, 100.0f, 18.0f),
                juce::Justification::centredLeft, false);
    const int selectedInput = audioProcessor.getInputChannel();
    for (int j = 0; j < 2; ++j)
    {
        const auto jc = jackCentre (j);
        realism::softShadowEllipse (g, jc, 11.0f, 4.0f, { 1.0f, 2.0f }, 0.4f);
        realism::spunMetal (g, jc, 12.0f, juce::Colour (0xffa9adb3), 55.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (juce::Rectangle<float> (24.0f, 24.0f).withCentre (jc), 0.8f);
        juce::ColourGradient hole (juce::Colour (0xff000000), jc.translated (0, -3), juce::Colour (0xff2a2a2e), jc.translated (0, 5), false);
        g.setGradientFill (hole);
        g.fillEllipse (juce::Rectangle<float> (11.0f, 11.0f).withCentre (jc));
    }
    // Guitar cable plugged into the selected input
    {
        const auto jc = jackCentre (selectedInput);
        juce::Path cable;
        cable.startNewSubPath (jc.translated (0.0f, 8.0f));
        cable.cubicTo (jc.translated (0.0f, 22.0f), jc.translated (6.0f, 30.0f), { jc.x + 4.0f, panel.getBottom() - 2.0f });
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.strokePath (cable, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                      juce::AffineTransform::translation (3.0f, 3.0f));
        g.setColour (juce::Colour (0xff0e0e10));
        g.strokePath (cable, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.strokePath (cable, juce::PathStrokeType (1.2f), juce::AffineTransform::translation (-1.5f, 0.0f));

        realism::softShadowEllipse (g, jc, 10.0f, 5.0f, { 2.0f, 3.0f }, 0.55f);
        realism::spunMetal (g, jc, 10.0f, juce::Colour (0xffd2d5da), 70.0f);         // chrome sleeve
        juce::ColourGradient boot (juce::Colour (0xff3a3a3e), jc.translated (-4.0f, -4.0f),
                                   juce::Colour (0xff050506), jc.translated (5.0f, 5.0f), true);
        g.setGradientFill (boot);
        g.fillEllipse (juce::Rectangle<float> (13.0f, 13.0f).withCentre (jc));       // rubber boot
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        for (float rr : { 3.0f, 5.0f })
            g.drawEllipse (juce::Rectangle<float> (rr * 2.0f, rr * 2.0f).withCentre (jc), 0.7f);
        realism::specular (g, jc, 6.5f, 0.35f);
    }

    g.setFont (LnF::condensed (11.0f));
    for (int j = 0; j < 2; ++j)
    {
        g.setColour (sk.panelText.withAlpha (j == selectedInput ? 1.0f : 0.5f));
        g.drawText (juce::String (j + 1), juce::Rectangle<float> (20.0f, 14.0f).withCentre (jackCentre (j).translated (j == 0 ? -17.0f : 17.0f, 16.0f)),
                    juce::Justification::centred, false);
    }

    // Divider before the logo plate
    const float logoX = panel.getX() + 190.0f + 4.0f * 96.0f + 60.0f;
    g.setColour (sk.panelTrim.withAlpha (0.55f));
    g.drawVerticalLine ((int) logoX, panel.getY() + 8.0f, panel.getBottom() - 8.0f);

    // Panel logo (silk-screened) + model line
    g.setColour (sk.logo);
    LnF::drawSlantedLogo (g, "JEMINI", juce::Rectangle<float> (logoX + 22.0f, panel.getY() + 14.0f, 220.0f, 46.0f),
                          34.0f, juce::Justification::centredLeft);
    g.setColour (sk.panelText);
    g.setFont (LnF::condensed (15.0f));
    g.drawText (sk.ampModelLine, juce::Rectangle<float> (logoX + 30.0f, panel.getY() + 64.0f, 200.0f, 18.0f),
                juce::Justification::centredLeft, false);

    // Jewel pilot light: faceted red glass in a chrome bezel
    const bool ampLive = audioProcessor.getNamAmp().hasModel() && ! ampBypass.getToggleState();
    const auto jewel = jewelArea.toFloat();
    const auto jc = jewel.getCentre();
    const float jr = jewel.getWidth() * 0.5f;
    if (ampLive)
    {
        juce::ColourGradient spill (LnF::redLed.withAlpha (0.6f), jc, LnF::redLed.withAlpha (0.0f), jc.translated (jr * 3.2f, 0.0f), true);
        g.setGradientFill (spill);
        g.fillEllipse (jewel.expanded (jr * 2.2f));
    }
    realism::softShadowEllipse (g, jc, jr + 4.0f, 5.0f, { 1.0f, 3.0f }, 0.45f);
    realism::spunMetal (g, jc, jr + 5.0f, juce::Colour (0xffb4b8be), 60.0f);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawEllipse (jewel.expanded (5.0f), 0.8f);
    g.drawEllipse (jewel.expanded (0.5f), 1.2f);

    juce::ColourGradient glass (ampLive ? juce::Colour (0xffffc0a8) : juce::Colour (0xff7a1d16), jc.translated (-jr * 0.25f, -jr * 0.3f),
                                ampLive ? juce::Colour (0xffb01208) : juce::Colour (0xff250604), jc.translated (jr, jr), true);
    g.setGradientFill (glass);
    g.fillEllipse (jewel);
    {
        // Facets: alternating light / dark triangles radiating from the centre
        juce::Graphics::ScopedSaveState s (g);
        juce::Path clip;
        clip.addEllipse (jewel);
        g.reduceClipRegion (clip);
        for (int i = 0; i < 12; ++i)
        {
            juce::Path facet;
            const float a0 = (float) i / 12.0f * juce::MathConstants<float>::twoPi;
            facet.addPieSegment (jewel.reduced (jr * 0.25f), a0, a0 + juce::MathConstants<float>::twoPi / 12.0f, 0.0f);
            g.setColour ((i % 2 == 0 ? juce::Colours::white : juce::Colours::black).withAlpha (ampLive ? 0.08f : 0.11f));
            g.fillPath (facet);
        }
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.drawEllipse (jewel.reduced (jr * 0.25f), 0.8f);
    }
    realism::specular (g, jc, jr, 0.75f);

    // --- Grille cloth behind white piping ---
    const auto grille = ampGrilleArea.toFloat();
    g.drawImage (ampGrille, grille, juce::RectanglePlacement::stretchToFit);

    // Warm valve glow behind the cloth while the amp is live
    if (ampLive)
    {
        juce::ColourGradient valves (juce::Colour (0x55ff8a2a), grille.getX() + grille.getWidth() * 0.55f, grille.getCentreY() + 10.0f,
                                     juce::Colour (0x00ff6a10), grille.getX() + grille.getWidth() * 0.55f, grille.getY() - 40.0f, true);
        g.setGradientFill (valves);
        g.fillRect (grille);
    }
    realism::innerShadow (g, grille, 0.0f, 10.0f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRect (grille.expanded (3.0f), 1.0f);
    juce::ColourGradient piping (sk.piping.brighter (0.15f), grille.getX(), grille.getY(),
                                 sk.piping.darker (0.45f), grille.getX(), grille.getBottom(), false);
    g.setGradientFill (piping);
    g.drawRect (grille.expanded (2.0f), 2.0f);

    // Chrome logo on the grille: cast shadow, dark edge, then a chrome fill
    const auto logoArea = juce::Rectangle<float> (grille.getX() + 30.0f, grille.getY() + 18.0f, 300.0f, 80.0f);
    for (int i = 4; i >= 1; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.16f));
        LnF::drawSlantedLogo (g, "JEMINI", logoArea.translated (1.0f + (float) i, 2.0f + (float) i * 1.4f), 52.0f, juce::Justification::centredLeft);
    }
    g.setColour (juce::Colour (0xff2a2c30));
    LnF::drawSlantedLogo (g, "JEMINI", logoArea.translated (0.8f, 1.2f), 52.0f, juce::Justification::centredLeft);
    if (sk.grilleLight.getPerceivedBrightness() > 0.8f)
    {
        // Light grille: dark enamel badge lettering with a chrome edge
        g.setColour (juce::Colour (0xffe8ecf0));
        LnF::drawSlantedLogo (g, "JEMINI", logoArea.translated (-0.8f, -1.0f), 52.0f, juce::Justification::centredLeft);
        juce::ColourGradient enamel (sk.logo.brighter (0.6f), logoArea.getX(), logoArea.getY() + 18.0f,
                                     sk.logo.darker (0.6f), logoArea.getX(), logoArea.getBottom() - 18.0f, false);
        g.setGradientFill (enamel);
    }
    else
    {
        g.setGradientFill (realism::chrome (logoArea.reduced (0.0f, 18.0f)));
    }
    LnF::drawSlantedLogo (g, "JEMINI", logoArea, 52.0f, juce::Justification::centredLeft);

    // Model tag: cream paper label riveted to the grille
    drawMountedShadows (g, ampArea);
    const auto tag = modelTagArea.toFloat();
    juce::ColourGradient card (juce::Colour (0xfff0e6cc), tag.getX(), tag.getY(), juce::Colour (0xffd9caa6), tag.getX(), tag.getBottom(), false);
    g.setGradientFill (card);
    g.fillRoundedRectangle (tag, 6.0f);
    fillTextured (g, tag, 6.0f, paper);
    realism::bevel (g, tag, 6.0f, 1.5f, 0.6f, 0.35f);
    g.setColour (juce::Colour (0xff6f6450));
    g.drawRoundedRectangle (tag.reduced (0.5f), 6.0f, 1.0f);
    const auto well = tag.withTrimmedTop (34.0f).reduced (4.0f);
    g.setColour (juce::Colour (0xff1c1c1f));
    g.fillRoundedRectangle (well, 4.0f);
    realism::innerShadow (g, well, 4.0f, 5.0f, 0.9f);
    for (auto p : { tag.getTopLeft().translated (9, 9), tag.getTopRight().translated (-9, 9) })
        drawScrew (g, p, 3.0f);
}

//------------------------------------------------------------------------------
void RigPanel::drawRack (juce::Graphics& g) const
{
    const auto& sk = skin();
    const auto a = rackArea.toFloat();

    // Rack ears: brushed aluminium with oval mounting slots
    fillTextured (g, a, 4.0f, brushedAlu);
    juce::ColourGradient earLight (juce::Colours::white.withAlpha (0.18f), a.getX(), a.getY(),
                                   juce::Colours::black.withAlpha (0.35f), a.getX(), a.getBottom(), false);
    g.setGradientFill (earLight);
    g.fillRoundedRectangle (a, 4.0f);
    realism::bevel (g, a, 4.0f, 2.0f, 0.5f, 0.6f);

    for (auto x : { a.getX() + 12.0f, a.getRight() - 12.0f })
        for (auto y : { a.getY() + 24.0f, a.getBottom() - 24.0f })
        {
            const auto slot = juce::Rectangle<float> (14.0f, 22.0f).withCentre ({ x, y });
            g.setColour (juce::Colour (0xff050506));
            g.fillRoundedRectangle (slot, 7.0f);
            realism::bevel (g, slot, 7.0f, 1.2f, 0.0f, 0.0f);
            g.setColour (juce::Colours::white.withAlpha (0.25f));
            g.drawRoundedRectangle (slot.translated (0.0f, 0.8f), 7.0f, 0.7f);
            drawScrew (g, { x, y + 2.0f }, 5.5f);
        }

    // Chrome rack handles on both ears
    for (auto x : { a.getX() + 12.0f, a.getRight() - 12.0f })
    {
        const float y0 = a.getY() + 48.0f, y1 = a.getBottom() - 48.0f;
        const auto bar = juce::Rectangle<float> (x - 5.0f, y0, 10.0f, y1 - y0);

        // cast shadow on the ear (handle stands off the panel)
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (bar.translated (4.0f, 7.0f).expanded (1.0f, 0.0f), 5.0f);

        // round bar: cylindrical chrome shading across its width
        juce::ColourGradient cyl (juce::Colour (0xff5e636a), bar.getX(), 0.0f, juce::Colour (0xff3a3e44), bar.getRight(), 0.0f, false);
        cyl.addColour (0.25, juce::Colour (0xfff6f7f9));
        cyl.addColour (0.45, juce::Colour (0xffb5bac1));
        cyl.addColour (0.70, juce::Colour (0xff7d8289));
        g.setGradientFill (cyl);
        g.fillRoundedRectangle (bar, 5.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (bar, 5.0f, 0.7f);

        // mounting posts
        for (auto y : { y0 + 2.0f, y1 - 2.0f })
        {
            realism::softShadowEllipse (g, { x, y }, 6.5f, 3.0f, { 1.5f, 3.0f }, 0.45f);
            realism::spunMetal (g, { x, y }, 7.0f, juce::Colour (0xffb8bcc2), 60.0f);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.drawEllipse (juce::Rectangle<float> (14.0f, 14.0f).withCentre ({ x, y }), 0.7f);
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre ({ x, y }));
        }
    }

    // Anodised, brushed front panel
    const auto face = a.reduced (24.0f, 4.0f);
    fillTextured (g, face, 0.0f, brushedRack);
    juce::ColourGradient faceLight (juce::Colours::white.withAlpha (0.06f), face.getX(), face.getY(),
                                    juce::Colours::black.withAlpha (0.30f), face.getX(), face.getBottom(), false);
    g.setGradientFill (faceLight);
    g.fillRect (face);
    realism::bevel (g, face, 0.0f, 2.0f, 0.3f, 0.7f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRect (face.expanded (0.5f), 1.0f);

    // Logo block
    const auto logo = juce::Rectangle<float> (face.getX() + 16.0f, face.getY() + 16.0f, 160.0f, 120.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.setFont (LnF::condensed (40.0f));
    g.drawText ("JGR-41", logo.withHeight (48.0f).translated (0.0f, 1.5f), juce::Justification::centredLeft, false);
    g.setColour (sk.rackTitle);
    g.drawText ("JGR-41", logo.withHeight (48.0f), juce::Justification::centredLeft, false);
    g.setColour (sk.rackAccent);
    g.setFont (LnF::condensed (16.0f));
    g.drawText ("DIGITAL DELAY",  logo.withTrimmedTop (52.0f).withHeight (18.0f), juce::Justification::centredLeft, false);
    g.drawText ("REVERB PROCESSOR", logo.withTrimmedTop (70.0f).withHeight (18.0f), juce::Justification::centredLeft, false);

    // Power LED: lit while any of the rack effects is switched on
    {
        const bool rackOn = dlyOn.getToggleState() || revOn.getToggleState() || irvOn.getToggleState();
        const auto led = juce::Rectangle<int> (10, 10).withCentre ({ (int) logo.getX() + 6, (int) logo.getY() + 106 });
        drawLed (g, led, rackOn, juce::Colour (0xff40ff70));
        g.setColour (LnF::silver.withAlpha (0.8f));
        g.setFont (LnF::condensed (13.0f));
        g.drawText ("POWER", juce::Rectangle<float> (logo.getX() + 18.0f, logo.getY() + 98.0f, 60.0f, 16.0f), juce::Justification::centredLeft, false);
    }

    // Display: recessed bezel, smoked red glass, glowing segments, reflection
    const auto disp = rackDisplayArea.toFloat();
    g.setColour (juce::Colour (0xff050506));
    g.fillRoundedRectangle (disp.expanded (3.0f), 6.0f);
    realism::bevel (g, disp.expanded (3.0f), 6.0f, 2.0f, 0.0f, 0.0f);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawRoundedRectangle (disp.expanded (3.0f).translated (0.0f, 1.0f), 6.0f, 0.8f);

    const auto glassArea = disp.reduced (6.0f);
    juce::ColourGradient smoke (sk.displayGlass, glassArea.getX(), glassArea.getY(),
                                sk.displayGlass.darker (0.8f), glassArea.getX(), glassArea.getBottom(), false);
    g.setGradientFill (smoke);
    g.fillRoundedRectangle (glassArea, 2.0f);

    const auto text = getDisplayText();
    const auto textArea = disp.reduced (14.0f, 8.0f);
    g.setFont (LnF::display (26.0f));
    const auto displayRed = sk.display;
    for (auto [dx, dy, al] : { std::tuple { -1.5f, 0.0f, 0.18f }, std::tuple { 1.5f, 0.0f, 0.18f },
                               std::tuple { 0.0f, -1.5f, 0.18f }, std::tuple { 0.0f, 1.5f, 0.18f } })
    {
        g.setColour (displayRed.withAlpha (al));            // bloom
        g.drawText (text, textArea.translated (dx, dy), juce::Justification::centredLeft, true);
    }
    g.setColour (displayRed.brighter (0.25f));
    g.drawText (text, textArea, juce::Justification::centredLeft, true);

    {
        juce::Graphics::ScopedSaveState s (g);                // faint scan lines
        g.reduceClipRegion (glassArea.toNearestInt());
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        for (float y = glassArea.getY(); y < glassArea.getBottom(); y += 2.0f)
            g.drawHorizontalLine ((int) y, glassArea.getX(), glassArea.getRight());
    }
    realism::innerShadow (g, glassArea, 2.0f, 6.0f, 0.9f);
    realism::sheen (g, glassArea, 2.0f, 0.10f);

    // Effect list LEDs (lit = effect ON)
    const std::array<bool, 3> states { dlyOn.getToggleState(), revOn.getToggleState(), irvOn.getToggleState() };
    for (size_t i = 0; i < 3; ++i)
        drawLed (g, rackLedAreas[i], states[i], LnF::redLed);

    // Selected-page marker
    const auto sel = rackSelect[(size_t) rackPage].getBounds().toFloat();
    const auto marker = juce::Rectangle<float> (sel.getRight() + 2.0f, sel.getCentreY() - 6.0f, 3.0f, 12.0f);
    g.setColour (sk.rackAccent.withAlpha (0.3f));
    g.fillRoundedRectangle (marker.expanded (2.5f), 3.0f);
    g.setColour (sk.rackAccent);
    g.fillRect (marker);

    // Engraved divider
    const int dx = rackControlsArea.getX() - 8;
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawVerticalLine (dx, face.getY() + 10.0f, face.getBottom() - 26.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawVerticalLine (dx + 1, face.getY() + 10.0f, face.getBottom() - 26.0f);

    // Blue stripe (printed, slight gloss)
    const auto stripe = juce::Rectangle<float> (disp.getX(), face.getBottom() - 18.0f, face.getRight() - disp.getX() - 10.0f, 9.0f);
    const auto stripeColour = sk.rackAccent.withMultipliedSaturation (1.1f).withMultipliedBrightness (0.8f);
    juce::ColourGradient blue (stripeColour.brighter (0.35f), stripe.getX(), stripe.getY(), stripeColour.darker (0.4f), stripe.getX(), stripe.getBottom(), false);
    g.setGradientFill (blue);
    g.fillRect (stripe);
}

//------------------------------------------------------------------------------
void RigPanel::drawSubPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title,
                             juce::Justification titleJust) const
{
    const auto f = r.toFloat();
    fillTextured (g, f, 6.0f, brushedDark);
    juce::ColourGradient light (juce::Colours::white.withAlpha (0.05f), f.getX(), f.getY(),
                                juce::Colours::black.withAlpha (0.35f), f.getX(), f.getBottom(), false);
    g.setGradientFill (light);
    g.fillRoundedRectangle (f, 6.0f);
    realism::bevel (g, f, 6.0f, 2.0f, 0.28f, 0.7f);

    // Title in an engraved strip
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.setFont (LnF::condensed (19.0f));
    const auto titleArea = titleJust == juce::Justification::centred ? r.withHeight (30) : r.withHeight (30).withTrimmedLeft (28);
    g.drawText (title, titleArea.translated (0, 1), titleJust, false);
    g.setColour (LnF::cream);
    g.drawText (title, titleArea, titleJust, false);
}

void RigPanel::drawBottomStrip (juce::Graphics& g) const
{
    const auto& sk = skin();
    const auto a = bottomArea.toFloat();

    fillTextured (g, a, 8.0f, tolex);
    juce::ColourGradient light (juce::Colours::white.withAlpha (0.06f), a.getX(), a.getY(),
                                juce::Colours::black.withAlpha (0.25f), a.getX(), a.getBottom(), false);
    g.setGradientFill (light);
    g.fillRoundedRectangle (a, 8.0f);
    realism::bevel (g, a, 8.0f, 4.0f, 0.22f, 0.6f);
    edgeWear (g, a, 8.0f, juce::Colours::white, 0.09f, 260);

    for (auto p : { a.getTopLeft().translated (10, 10), a.getTopRight().translated (-10, 10),
                    a.getBottomLeft().translated (10, -10), a.getBottomRight().translated (-10, -10) })
        drawScrew (g, p, 5.0f);

    // Cabinet grille (recessed, with piping)
    const auto grille = cabGrilleArea.toFloat();
    g.drawImage (cabGrille, grille, juce::RectanglePlacement::stretchToFit);

    // Two speakers faintly visible through the cloth (2x12)
    for (float cx : { grille.getX() + grille.getWidth() * 0.27f, grille.getX() + grille.getWidth() * 0.73f })
    {
        const auto c = juce::Point<float> (cx, grille.getCentreY() + 6.0f);
        juce::ColourGradient cone (juce::Colours::black.withAlpha (0.45f), c,
                                   juce::Colours::black.withAlpha (0.0f), c.translated (0.0f, 76.0f), true);
        cone.addColour (0.8, juce::Colours::black.withAlpha (0.2f));
        g.setGradientFill (cone);
        g.fillEllipse (juce::Rectangle<float> (152.0f, 152.0f).withCentre (c));
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.drawEllipse (juce::Rectangle<float> (40.0f, 40.0f).withCentre (c), 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.drawEllipse (juce::Rectangle<float> (150.0f, 150.0f).withCentre (c.translated (-1.0f, -1.5f)), 2.0f);
    }

    realism::innerShadow (g, grille, 0.0f, 10.0f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRect (grille.expanded (3.0f), 1.0f);
    juce::ColourGradient piping (sk.piping.brighter (0.15f), grille.getX(), grille.getY(),
                                 sk.piping.darker (0.45f), grille.getX(), grille.getBottom(), false);
    g.setGradientFill (piping);
    g.drawRect (grille.expanded (2.0f), 2.0f);

    drawMountedShadows (g, bottomArea);

    // Title plate: small engraved metal badge
    const auto plate = juce::Rectangle<float> (170.0f, 28.0f).withCentre ({ grille.getCentreX(), grille.getY() + 22.0f });
    fillTextured (g, plate, 5.0f, brushedDark);
    realism::bevel (g, plate, 5.0f, 1.5f, 0.35f, 0.6f);
    g.setColour (LnF::cream);
    g.setFont (LnF::condensed (19.0f));
    g.drawText ("CABINET / IR", plate, juce::Justification::centred, false);

    // IR file field (dark recessed window)
    const auto file = cabFileArea.toFloat();
    g.setColour (juce::Colour (0xf00c0c0e));
    g.fillRoundedRectangle (file, 5.0f);
    realism::innerShadow (g, file, 5.0f, 6.0f, 0.9f);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawRoundedRectangle (file.translated (0.0f, 1.0f), 5.0f, 0.8f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (file, 5.0f, 1.0f);

    // Sub-panels
    drawSubPanel (g, cabToneArea, "CAB TONE");
    drawSubPanel (g, gateArea,    "NOISE GATE");
    drawSubPanel (g, inputArea,   "IN", juce::Justification::centredLeft);
    drawSubPanel (g, outputArea,  "OUT");

    // Gate: off = dark, on + open = green, closing on noise = red
    drawLed (g, gateLedArea, gateOn.getToggleState(), gateLedLit ? LnF::redLed : juce::Colour (0xff40ff70));

    // Cabinet power: lit when an IR is loaded and the cab is not bypassed
    drawLed (g, cabLedArea, audioProcessor.getCabIR().hasIR() && ! cabBypass.getToggleState(), juce::Colour (0xff40ff70));

    // Input signal: green with signal, red when clipping
    drawLed (g, inputLedArea, inputLedLit, inputClip ? LnF::redLed : juce::Colour (0xff40ff70));

    // Output meter: segments behind a dark window
    const auto m = meterArea.toFloat();
    g.setColour (juce::Colour (0xff050506));
    g.fillRoundedRectangle (m.expanded (3.0f), 3.0f);
    realism::bevel (g, m.expanded (3.0f), 3.0f, 1.2f, 0.0f, 0.0f);
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawRoundedRectangle (m.expanded (3.0f).translated (0.0f, 1.0f), 3.0f, 0.7f);

    constexpr int segments = 14;
    const float segH = m.getHeight() / (float) segments;
    const std::array<float, 2> levels { meterL, meterR };

    for (int ch = 0; ch < 2; ++ch)
    {
        const float db = juce::Decibels::gainToDecibels (levels[(size_t) ch], -60.0f);
        const int lit = juce::roundToInt (juce::jmap (db, -60.0f, 0.0f, 0.0f, (float) segments));
        const auto col = m.withWidth (m.getWidth() * 0.4f).translated ((float) ch * m.getWidth() * 0.55f, 0.0f);

        for (int s = 0; s < segments; ++s)
        {
            const auto seg = juce::Rectangle<float> (col.getX(), col.getBottom() - (float) (s + 1) * segH + 1.0f, col.getWidth(), segH - 2.0f);
            const auto colour = s >= segments - 2 ? juce::Colour (0xffff3020)
                              : s >= segments - 5 ? juce::Colour (0xffffd020)
                                                  : juce::Colour (0xff30e040);
            if (s < lit)
            {
                g.setColour (colour.withAlpha (0.35f));
                g.fillRect (seg.expanded (1.5f));
                g.setColour (colour.brighter (0.2f));
            }
            else
            {
                g.setColour (colour.darker (3.0f).withAlpha (0.55f));
            }
            g.fillRect (seg);
        }
    }
}
