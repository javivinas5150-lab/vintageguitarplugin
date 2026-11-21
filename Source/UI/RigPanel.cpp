#include "RigPanel.h"
#include "JeminiLookAndFeel.h"

using LnF = JeminiLookAndFeel;

namespace
{
    const juce::Colour vibePurpleTop    (0xff4a3a62);
    const juce::Colour vibePurpleBottom (0xff241a33);
    const juce::Colour goldTop          (0xffe4c46e);
    const juce::Colour goldBottom       (0xffa9822f);
    const juce::Colour rackFace         (0xff1b2030);
    const juce::Colour rackBlue         (0xff1f5fd0);
    const juce::Colour displayRed       (0xffff3a2a);
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
    ampBypass.onStateChange = [this] { repaint (jewelArea.expanded (10)); };

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

    setupToggle (gateOn, gateOnAttachment, "gateOn", {}, cream);
    setupKnob (gateThreshold, "gateThreshold", "THRESHOLD", "pedal", ValueBox::below, 1, "dB", cream);
    setupKnob (gateRelease,   "gateRelease",   "RELEASE",   "pedal", ValueBox::below, 0, "ms", cream);

    setupKnob (input,  "inputGain",  "INPUT",  "pedal", ValueBox::below, 1, "dB", cream);
    setupKnob (output, "outputGain", "OUTPUT", "pedal", ValueBox::below, 1, "dB", cream);

    lastNamFolder = lastCabFolder = lastIRVFolder
        = juce::File::getSpecialLocation (juce::File::userDesktopDirectory);

    buildTextures();
    setRackPage (0);

    refreshAmpInfo();
    refreshCabInfo();
    refreshIRVerbInfo();
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
        repaint (jewelArea.expanded (10));
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
        cabNameLabel.setText (name, juce::dontSendNotification);

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
    refreshCabInfo();
    refreshIRVerbInfo();
}

//==============================================================================
void RigPanel::buildTextures()
{
    tolex     = makeNoise (160, 160, juce::Colour (0xff17171a), 0.6f, 7);
    ampGrille = makeGrille (900, 140, juce::Colour (0xffcbbf9f), juce::Colour (0xff2a2620), 11);
    cabGrille = makeGrille (560, 170, juce::Colour (0xffcbbf9f), juce::Colour (0xff2a2620), 13);
}

//==============================================================================
// LAYOUT (design size 1280 x 720)
//==============================================================================
void RigPanel::resized()
{
    titleBar   = { 0, 0, designWidth, 48 };
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
        cabGrilleArea = { a.getX() + 20, a.getY() + 16, 500, a.getHeight() - 32 };
        cabFileArea   = juce::Rectangle<int> (420, 34).withCentre ({ cabGrilleArea.getCentreX(), cabGrilleArea.getY() + 62 });

        auto file = cabFileArea.reduced (6, 4);
        clearCabButton.setBounds (file.removeFromRight (26));
        file.removeFromRight (6);
        loadCabButton.setBounds (file.removeFromRight (96));
        file.removeFromRight (8);
        cabNameLabel.setBounds (file.withTrimmedLeft (6));
        cabBypass.setBounds (cabGrilleArea.getCentreX() - 50, cabFileArea.getBottom() + 10, 100, 24);

        int x = cabGrilleArea.getRight() + 16;
        const int top = a.getY() + 16, h = a.getHeight() - 32;
        cabToneArea = { x, top, 170, h };  x += 170 + 12;
        gateArea    = { x, top, 230, h };  x += 230 + 12;
        inputArea   = { x, top, 110, h };  x += 110 + 12;
        outputArea  = { x, top, a.getRight() - 16 - x, h };

        auto placeKnob = [] (Knob& k, juce::Rectangle<int> col)
        {
            k.label.setBounds (col.removeFromTop (22));
            k.slider.setBounds (col.reduced (4, 0));
        };

        auto tone = cabToneArea.withTrimmedTop (34).reduced (6, 6);
        placeKnob (lowCut,  tone.removeFromLeft (tone.getWidth() / 2));
        placeKnob (highCut, tone);

        auto gate = gateArea.withTrimmedTop (34).reduced (6, 6);
        auto gateHead = juce::Rectangle<int> (gateArea.getX() + 12, gateArea.getY() + 6, 70, 22);
        gateOn.setBounds (gateHead);
        gateLedArea = juce::Rectangle<int> (12, 12).withCentre ({ gateArea.getRight() - 22, gateArea.getY() + 17 });
        placeKnob (gateThreshold, gate.removeFromLeft (gate.getWidth() / 2));
        placeKnob (gateRelease, gate);

        placeKnob (input, inputArea.withTrimmedTop (34).reduced (6, 6));

        auto out = outputArea.withTrimmedTop (34).reduced (6, 6);
        meterArea = out.removeFromRight (26).withTrimmedTop (20).withTrimmedBottom (20);
        placeKnob (output, out);
    }
}

//==============================================================================
// DRAWING
//==============================================================================
void RigPanel::paint (juce::Graphics& g)
{
    // Background: dark stage
    juce::ColourGradient bg (juce::Colour (0xff201914), 0.0f, 0.0f, juce::Colour (0xff0b0908), 0.0f, (float) designHeight, false);
    g.setGradientFill (bg);
    g.fillAll();

    // Title bar
    {
        juce::ColourGradient bar (juce::Colour (0xff2a221b), 0.0f, 0.0f, juce::Colour (0xff120e0b), 0.0f, (float) titleBar.getBottom(), false);
        g.setGradientFill (bar);
        g.fillRect (titleBar);
        g.setColour (juce::Colours::black);
        g.drawHorizontalLine (titleBar.getBottom() - 1, 0.0f, (float) designWidth);
        g.setColour (juce::Colour (0x22ffffff));
        g.drawHorizontalLine (titleBar.getBottom() - 2, 0.0f, (float) designWidth);

        g.setColour (juce::Colour (0xffe9d9b0));
        LnF::drawSlantedLogo (g, "JEMINI GUITAR RIG", titleBar.toFloat(), 26.0f, juce::Justification::centred);
    }

    // ---- Pedals ----
    drawPedal (g, vibeArea.toFloat(), vibePurpleTop, vibePurpleBottom);
    g.setColour (LnF::cream);
    g.setFont (LnF::script (44.0f));
    g.drawText ("Vibe", vibeArea.withHeight (60).translated (0, 6), juce::Justification::centred, false);

    g.setFont (LnF::condensed (14.0f));
    g.drawText ("CHORUS",  juce::Rectangle<int> (vibeArea.getX() + 14, vibeArea.getY() + 182, 52, 16), juce::Justification::centredRight, false);
    g.drawText ("VIBRATO", juce::Rectangle<int> (vibeArea.getX() + 114, vibeArea.getY() + 182, 56, 16), juce::Justification::centredLeft, false);
    drawLed (g, vibeLedArea, vibeOn.getToggleState(), LnF::redLed);

    drawPedal (g, odArea.toFloat(), goldTop, goldBottom);
    g.setColour (LnF::ink);
    g.setFont (LnF::script (42.0f));
    g.drawText ("Gold Drive", odArea.withHeight (54).translated (0, 4), juce::Justification::centred, false);
    drawLed (g, odLedArea, odOn.getToggleState(), LnF::redLed);

    // ---- Amp, rack, bottom strip ----
    drawAmpHead (g);
    drawRack (g);
    drawBottomStrip (g);
}

//------------------------------------------------------------------------------
void RigPanel::drawPedal (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour top, juce::Colour bottom) const
{
    // Shadow
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (r.translated (3.0f, 5.0f), 12.0f);

    // Enclosure
    juce::ColourGradient body (top, r.getTopLeft(), bottom, r.getBottomRight(), false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (r, 12.0f);

    // Bevel
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawRoundedRectangle (r.reduced (1.5f), 11.0f, 1.5f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (r, 12.0f, 1.5f);

    // Corner screws
    for (auto p : { r.getTopLeft().translated (13, 13), r.getTopRight().translated (-13, 13),
                    r.getBottomLeft().translated (13, -13), r.getBottomRight().translated (-13, -13) })
        drawScrew (g, p, 5.0f);
}

void RigPanel::drawScrew (juce::Graphics& g, juce::Point<float> c, float radius) const
{
    juce::ColourGradient head (juce::Colour (0xffe8eaed), c.translated (-radius, -radius),
                               juce::Colour (0xff5b5f66), c.translated (radius, radius), false);
    g.setGradientFill (head);
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c));
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawLine (c.x - radius * 0.65f, c.y, c.x + radius * 0.65f, c.y, 1.2f);
    g.drawLine (c.x, c.y - radius * 0.65f, c.x, c.y + radius * 0.65f, 1.2f);
}

void RigPanel::drawLed (juce::Graphics& g, juce::Rectangle<int> area, bool on, juce::Colour colour) const
{
    const auto r = area.toFloat();
    if (on)
    {
        g.setColour (colour.withAlpha (0.35f));
        g.fillEllipse (r.expanded (5.0f));
    }

    juce::ColourGradient lens (on ? colour.brighter (0.6f) : colour.darker (2.5f), r.getCentre().translated (-1.5f, -1.5f),
                               on ? colour : colour.darker (3.5f), r.getBottomRight(), true);
    g.setGradientFill (lens);
    g.fillEllipse (r);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawEllipse (r, 1.0f);
}

//------------------------------------------------------------------------------
void RigPanel::drawAmpHead (juce::Graphics& g) const
{
    const auto a = ampArea.toFloat();

    // Leather handle on top
    const auto handle = juce::Rectangle<float> (240.0f, 16.0f).withCentre ({ a.getCentreX(), a.getY() + 6.0f });
    g.setColour (juce::Colour (0xff0d0d0e));
    g.fillRoundedRectangle (handle, 6.0f);
    g.setColour (juce::Colours::white.withAlpha (0.1f));
    g.drawRoundedRectangle (handle, 6.0f, 1.0f);

    // Tolex-covered head
    const auto cab = a.withTrimmedTop (14.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (cab.translated (3.0f, 5.0f), 10.0f);
    g.setFillType (juce::FillType (tolex, juce::AffineTransform()));
    g.fillRoundedRectangle (cab, 10.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (cab.reduced (1.0f), 10.0f, 1.2f);

    // Metal corners
    g.setColour (juce::Colour (0xff8f8b80));
    for (auto p : { cab.getTopLeft(), cab.getTopRight().translated (-18, 0), cab.getBottomLeft().translated (0, -18),
                    cab.getBottomRight().translated (-18, -18) })
        g.fillRoundedRectangle (juce::Rectangle<float> (p.x, p.y, 18.0f, 18.0f), 4.0f);

    // --- Control panel (black panel with silver trim) ---
    const auto panel = ampPanelArea.toFloat();
    juce::ColourGradient face (juce::Colour (0xff1c1c1f), panel.getTopLeft(), juce::Colour (0xff070708), panel.getBottomLeft(), false);
    g.setGradientFill (face);
    g.fillRect (panel);
    g.setColour (juce::Colour (0xffb9b4a6));
    g.drawRect (panel, 2.0f);
    g.setColour (juce::Colour (0xffb9b4a6).withAlpha (0.4f));
    g.drawRect (panel.reduced (5.0f), 1.0f);

    // Channel label + input jacks
    g.setColour (LnF::cream);
    g.setFont (LnF::condensed (18.0f));
    g.drawText ("NAM", juce::Rectangle<float> (panel.getX() + 22.0f, panel.getY() + 12.0f, 100.0f, 18.0f),
                juce::Justification::centredLeft, false);
    for (int j = 0; j < 2; ++j)
    {
        const auto jc = juce::Point<float> (panel.getX() + 36.0f + (float) j * 46.0f, panel.getY() + 56.0f);
        juce::ColourGradient nut (juce::Colour (0xffd8dade), jc.translated (-9, -9), juce::Colour (0xff4e5157), jc.translated (9, 9), false);
        g.setGradientFill (nut);
        g.fillEllipse (juce::Rectangle<float> (22.0f, 22.0f).withCentre (jc));
        g.setColour (juce::Colours::black);
        g.fillEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre (jc));
    }
    g.setColour (LnF::cream.withAlpha (0.8f));
    g.setFont (LnF::condensed (11.0f));
    g.drawText ("1        2", juce::Rectangle<float> (panel.getX() + 26.0f, panel.getY() + 72.0f, 80.0f, 14.0f),
                juce::Justification::centred, false);

    // Divider before the logo plate
    const float logoX = panel.getX() + 190.0f + 4.0f * 96.0f + 60.0f;
    g.setColour (juce::Colour (0xffb9b4a6).withAlpha (0.5f));
    g.drawVerticalLine ((int) logoX, panel.getY() + 8.0f, panel.getBottom() - 8.0f);

    // Script logo + model plate
    g.setColour (LnF::cream);
    LnF::drawSlantedLogo (g, "JEMINI", juce::Rectangle<float> (logoX + 22.0f, panel.getY() + 14.0f, 220.0f, 46.0f),
                          34.0f, juce::Justification::centredLeft);
    g.setFont (LnF::condensed (15.0f));
    g.drawText ("BLACK PANEL  '65", juce::Rectangle<float> (logoX + 30.0f, panel.getY() + 64.0f, 200.0f, 18.0f),
                juce::Justification::centredLeft, false);

    // Red jewel pilot light: lit when the amp is active (model loaded, not bypassed)
    const bool ampLive = audioProcessor.getNamAmp().hasModel() && ! ampBypass.getToggleState();
    const auto jewel = jewelArea.toFloat();
    if (ampLive)
    {
        g.setColour (LnF::redLed.withAlpha (0.35f));
        g.fillEllipse (jewel.expanded (8.0f));
    }
    juce::ColourGradient glass (ampLive ? juce::Colour (0xffffb0a0) : juce::Colour (0xff6a1a14), jewel.getCentre().translated (-4, -5),
                                ampLive ? juce::Colour (0xffd01a10) : juce::Colour (0xff2a0806), jewel.getBottomRight(), true);
    g.setGradientFill (glass);
    g.fillEllipse (jewel);
    g.setColour (juce::Colour (0xffc9ccd1));
    g.drawEllipse (jewel.expanded (2.0f), 3.0f);

    // --- Grille cloth ---
    const auto grille = ampGrilleArea.toFloat();
    g.drawImage (ampGrille, grille, juce::RectanglePlacement::stretchToFit);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawRect (grille, 2.0f);

    // Chrome script logo on the grille
    const auto logoArea = juce::Rectangle<float> (grille.getX() + 30.0f, grille.getY() + 18.0f, 300.0f, 80.0f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    LnF::drawSlantedLogo (g, "JEMINI", logoArea.translated (3.0f, 4.0f), 52.0f, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xffe6e8eb));
    LnF::drawSlantedLogo (g, "JEMINI", logoArea, 52.0f, juce::Justification::centredLeft);

    // Model tag (cream label plate)
    const auto tag = modelTagArea.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (tag.translated (2.0f, 3.0f), 6.0f);
    g.setColour (juce::Colour (0xffe8dcc0));
    g.fillRoundedRectangle (tag, 6.0f);
    g.setColour (juce::Colour (0xff6f6450));
    g.drawRoundedRectangle (tag.reduced (1.0f), 6.0f, 1.0f);
    g.setColour (juce::Colour (0xff1c1c1f));
    g.fillRoundedRectangle (tag.withTrimmedTop (34.0f).reduced (4.0f), 4.0f);
}

//------------------------------------------------------------------------------
void RigPanel::drawRack (juce::Graphics& g) const
{
    const auto a = rackArea.toFloat();

    // Ears + face
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (a.translated (3.0f, 5.0f), 4.0f);

    juce::ColourGradient ears (juce::Colour (0xff3b3e45), a.getTopLeft(), juce::Colour (0xff1a1c20), a.getBottomLeft(), false);
    g.setGradientFill (ears);
    g.fillRoundedRectangle (a, 4.0f);

    const auto face = a.reduced (24.0f, 4.0f);
    juce::ColourGradient brushed (rackFace.brighter (0.15f), face.getTopLeft(), rackFace.darker (0.3f), face.getBottomLeft(), false);
    g.setGradientFill (brushed);
    g.fillRect (face);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    for (float y = face.getY(); y < face.getBottom(); y += 3.0f)
        g.drawHorizontalLine ((int) y, face.getX(), face.getRight());

    for (auto x : { a.getX() + 12.0f, a.getRight() - 12.0f })
        for (auto y : { a.getY() + 24.0f, a.getBottom() - 24.0f })
            drawScrew (g, { x, y }, 6.0f);

    // Logo block
    const auto logo = juce::Rectangle<float> (face.getX() + 16.0f, face.getY() + 16.0f, 160.0f, 120.0f);
    g.setColour (juce::Colours::white);
    g.setFont (LnF::condensed (40.0f));
    g.drawText ("JGR-41", logo.withHeight (48.0f), juce::Justification::centredLeft, false);
    g.setColour (LnF::blueLed);
    g.setFont (LnF::condensed (16.0f));
    g.drawText ("DIGITAL DELAY",  logo.withTrimmedTop (52.0f).withHeight (18.0f), juce::Justification::centredLeft, false);
    g.drawText ("REVERB PROCESSOR", logo.withTrimmedTop (70.0f).withHeight (18.0f), juce::Justification::centredLeft, false);

    // Display window (red text on black glass)
    const auto disp = rackDisplayArea.toFloat();
    g.setColour (juce::Colour (0xff050505));
    g.fillRoundedRectangle (disp, 4.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (disp.expanded (2.0f), 5.0f, 3.0f);
    juce::ColourGradient glass (juce::Colour (0xff1a0806), disp.getTopLeft(), juce::Colour (0xff070202), disp.getBottomLeft(), false);
    g.setGradientFill (glass);
    g.fillRoundedRectangle (disp.reduced (6.0f), 2.0f);

    const auto text = getDisplayText();
    g.setColour (displayRed.withAlpha (0.25f));
    g.setFont (LnF::display (26.0f));
    g.drawText (text, disp.reduced (14.0f, 8.0f).translated (0.0f, 1.0f), juce::Justification::centredLeft, true);
    g.setColour (displayRed);
    g.drawText (text, disp.reduced (14.0f, 8.0f), juce::Justification::centredLeft, true);

    // Effect list LEDs (lit = effect ON)
    const std::array<bool, 3> states { dlyOn.getToggleState(), revOn.getToggleState(), irvOn.getToggleState() };
    for (size_t i = 0; i < 3; ++i)
        drawLed (g, rackLedAreas[i], states[i], LnF::redLed);

    // Selected-page marker
    g.setColour (LnF::blueLed);
    const auto sel = rackSelect[(size_t) rackPage].getBounds().toFloat();
    g.fillRect (sel.getRight() + 2.0f, sel.getCentreY() - 6.0f, 3.0f, 12.0f);

    // Divider
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawVerticalLine (rackControlsArea.getX() - 8, face.getY() + 10.0f, face.getBottom() - 26.0f);

    // Blue stripe
    g.setColour (rackBlue);
    g.fillRect (juce::Rectangle<float> (disp.getX(), face.getBottom() - 18.0f, face.getRight() - disp.getX() - 10.0f, 9.0f));
}

//------------------------------------------------------------------------------
void RigPanel::drawSubPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title) const
{
    const auto f = r.toFloat();
    juce::ColourGradient face (juce::Colour (0xff222226), f.getTopLeft(), juce::Colour (0xff101012), f.getBottomLeft(), false);
    g.setGradientFill (face);
    g.fillRoundedRectangle (f, 6.0f);
    g.setColour (juce::Colours::white.withAlpha (0.14f));
    g.drawRoundedRectangle (f.reduced (0.5f), 6.0f, 1.0f);

    g.setColour (LnF::cream);
    g.setFont (LnF::condensed (19.0f));
    g.drawText (title, r.withHeight (30), juce::Justification::centred, false);
}

void RigPanel::drawBottomStrip (juce::Graphics& g) const
{
    const auto a = bottomArea.toFloat();

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (a.translated (3.0f, 5.0f), 8.0f);
    g.setFillType (juce::FillType (tolex, juce::AffineTransform()));
    g.fillRoundedRectangle (a, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (a.reduced (1.0f), 8.0f, 1.2f);

    for (auto p : { a.getTopLeft().translated (10, 10), a.getTopRight().translated (-10, 10),
                    a.getBottomLeft().translated (10, -10), a.getBottomRight().translated (-10, -10) })
        drawScrew (g, p, 5.0f);

    // Cabinet grille + title plate
    const auto grille = cabGrilleArea.toFloat();
    g.drawImage (cabGrille, grille, juce::RectanglePlacement::stretchToFit);

    // Two speakers faintly visible through the cloth (2x12)
    for (float cx : { grille.getX() + grille.getWidth() * 0.27f, grille.getX() + grille.getWidth() * 0.73f })
    {
        const auto c = juce::Point<float> (cx, grille.getCentreY() + 6.0f);
        juce::ColourGradient cone (juce::Colours::black.withAlpha (0.42f), c,
                                   juce::Colours::black.withAlpha (0.0f), c.translated (0.0f, 74.0f), true);
        g.setGradientFill (cone);
        g.fillEllipse (juce::Rectangle<float> (150.0f, 150.0f).withCentre (c));
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.drawEllipse (juce::Rectangle<float> (40.0f, 40.0f).withCentre (c), 3.0f);
    }

    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawRect (grille, 2.0f);

    const auto plate = juce::Rectangle<float> (170.0f, 28.0f).withCentre ({ grille.getCentreX(), grille.getY() + 22.0f });
    g.setColour (juce::Colour (0xff1c1c1f));
    g.fillRoundedRectangle (plate, 5.0f);
    g.setColour (LnF::cream);
    g.setFont (LnF::condensed (19.0f));
    g.drawText ("CABINET / IR", plate, juce::Justification::centred, false);

    // IR file field
    const auto file = cabFileArea.toFloat();
    g.setColour (juce::Colour (0xee101012));
    g.fillRoundedRectangle (file, 5.0f);
    g.setColour (LnF::silver.withAlpha (0.4f));
    g.drawRoundedRectangle (file, 5.0f, 1.0f);

    // Sub-panels
    drawSubPanel (g, cabToneArea, "CAB TONE");
    drawSubPanel (g, gateArea,    "NOISE GATE");
    drawSubPanel (g, inputArea,   "IN");
    drawSubPanel (g, outputArea,  "OUT");

    drawLed (g, gateLedArea, gateLedLit, LnF::redLed);

    // Output meter (green -> yellow -> red segments)
    const auto m = meterArea.toFloat();
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
            g.setColour (s < lit ? colour : colour.darker (3.0f).withAlpha (0.5f));
            g.fillRect (seg);
        }
    }
}
