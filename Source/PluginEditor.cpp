#include "PluginEditor.h"

namespace
{
    const juce::Colour panelBlack  (0xff161616);
    const juce::Colour cream       (0xfff2ead3);
    const juce::Colour tolex       (0xff222222);
    const juce::Colour silver      (0xffb8b8b8);
    const juce::Colour dimGrey     (0xff8a8a8a);
    const juce::Colour warnAmber   (0xffe0a030);
}

//==============================================================================
VintageGuitarSuiteAudioProcessorEditor::VintageGuitarSuiteAudioProcessorEditor (
    VintageGuitarSuiteAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setupKnob (inputKnob,  inputLabel,  "INPUT");
    setupKnob (outputKnob, outputLabel, "OUTPUT");

    // --- Amp section ---
    loadButton.onClick = [this] { chooseNamFile(); };
    loadButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff3a3a3a));
    loadButton.setColour (juce::TextButton::textColourOffId, cream);
    addAndMakeVisible (loadButton);

    clearButton.setTooltip ("Unload model");
    clearButton.onClick = [this]
    {
        audioProcessor.clearNamModel();
        refreshAmpInfo();
    };
    clearButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff3a3a3a));
    clearButton.setColour (juce::TextButton::textColourOffId, cream);
    addAndMakeVisible (clearButton);

    bypassButton.setColour (juce::ToggleButton::textColourId, cream);
    bypassButton.setColour (juce::ToggleButton::tickColourId, cream);
    addAndMakeVisible (bypassButton);

    modelNameLabel.setJustificationType (juce::Justification::centred);
    modelNameLabel.setColour (juce::Label::textColourId, cream);
    modelNameLabel.setFont (juce::FontOptions (15.0f));
    addAndMakeVisible (modelNameLabel);

    warningLabel.setJustificationType (juce::Justification::centred);
    warningLabel.setColour (juce::Label::textColourId, warnAmber);
    warningLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (warningLabel);

    // Connect controls to the parameters
    inputAttachment  = std::make_unique<SliderAttachment> (audioProcessor.apvts, "inputGain",  inputKnob);
    outputAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "outputGain", outputKnob);
    bypassAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "ampBypass",  bypassButton);

    // Amp knobs
    const std::array<const char*, numAmpKnobs> ampNames { "GAIN", "BASS", "MIDDLE", "TREBLE", "VOLUME" };
    const std::array<const char*, numAmpKnobs> ampIds   { "ampGain", "ampBass", "ampMiddle", "ampTreble", "ampVolume" };

    for (int i = 0; i < numAmpKnobs; ++i)
    {
        setupKnob (ampKnobs[(size_t) i], ampKnobLabels[(size_t) i], ampNames[(size_t) i], 12.0f, 62);
        ampKnobAttachments[(size_t) i] = std::make_unique<SliderAttachment> (audioProcessor.apvts,
                                                                             ampIds[(size_t) i],
                                                                             ampKnobs[(size_t) i]);
    }

    // Gate
    gateButton.setColour (juce::ToggleButton::textColourId, cream);
    gateButton.setColour (juce::ToggleButton::tickColourId, cream);
    addAndMakeVisible (gateButton);

    setupKnob (gateThresholdKnob, gateThresholdLabel, "THRESH",  12.0f, 62, " dB", -60.0);
    setupKnob (gateReleaseKnob,   gateReleaseLabel,   "RELEASE", 12.0f, 62, " ms", 100.0);

    gateOnAttachment        = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "gateOn",        gateButton);
    gateThresholdAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "gateThreshold", gateThresholdKnob);
    gateReleaseAttachment   = std::make_unique<SliderAttachment> (audioProcessor.apvts, "gateRelease",   gateReleaseKnob);

    // Cab
    for (auto* b : { &loadIRButton, &clearIRButton })
    {
        b->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff3a3a3a));
        b->setColour (juce::TextButton::textColourOffId, cream);
        addAndMakeVisible (*b);
    }

    loadIRButton.onClick = [this] { chooseIRFile(); };
    clearIRButton.setTooltip ("Unload IR");
    clearIRButton.onClick = [this]
    {
        audioProcessor.clearCabIR();
        refreshCabInfo();
    };

    cabBypassButton.setColour (juce::ToggleButton::textColourId, cream);
    cabBypassButton.setColour (juce::ToggleButton::tickColourId, cream);
    addAndMakeVisible (cabBypassButton);

    irNameLabel.setJustificationType (juce::Justification::centred);
    irNameLabel.setColour (juce::Label::textColourId, cream);
    irNameLabel.setFont (juce::FontOptions (13.0f));
    irNameLabel.setMinimumHorizontalScale (0.6f);
    addAndMakeVisible (irNameLabel);

    setupKnob (lowCutKnob,  lowCutLabel,  "LOW CUT",  12.0f, 66, " Hz", 20.0);
    setupKnob (highCutKnob, highCutLabel, "HIGH CUT", 12.0f, 66, " Hz", 20000.0);

    cabBypassAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "cabBypass",  cabBypassButton);
    lowCutAttachment    = std::make_unique<SliderAttachment> (audioProcessor.apvts, "cabLowCut",  lowCutKnob);
    highCutAttachment   = std::make_unique<SliderAttachment> (audioProcessor.apvts, "cabHighCut", highCutKnob);

    // --- Pedals ---
    for (auto* b : { &vibeOnButton, &odOnButton })
    {
        b->setColour (juce::ToggleButton::textColourId, cream);
        b->setColour (juce::ToggleButton::tickColourId, cream);
        b->onStateChange = [this] { repaint (vibeLedArea.expanded (2)); repaint (odLedArea.expanded (2)); };
        addAndMakeVisible (*b);
    }

    setupKnob (vibeSpeedKnob,     vibeSpeedLabel,     "SPEED",     12.0f, 62, " Hz", 3.0);
    setupKnob (vibeIntensityKnob, vibeIntensityLabel, "INTENSITY", 12.0f, 62, " %",  70.0);

    vibeModeBox.addItemList ({ "Chorus", "Vibrato" }, 1);
    addAndMakeVisible (vibeModeBox);

    setupKnob (odGainKnob,   odGainLabel,   "GAIN",   12.0f, 50, {}, 3.0);
    setupKnob (odTrebleKnob, odTrebleLabel, "TREBLE", 12.0f, 50, {}, 5.0);
    setupKnob (odOutputKnob, odOutputLabel, "OUTPUT", 12.0f, 50, {}, 5.0);

    // Dark lettering on the gold pedal
    const juce::Colour ink (0xff2a2010);
    odOnButton.setColour (juce::ToggleButton::textColourId, ink);
    odOnButton.setColour (juce::ToggleButton::tickColourId, ink);
    odOnButton.setColour (juce::ToggleButton::tickDisabledColourId, ink.withAlpha (0.6f));

    for (auto* l : { &odGainLabel, &odTrebleLabel, &odOutputLabel })
        l->setColour (juce::Label::textColourId, ink);

    for (auto* k : { &odGainKnob, &odTrebleKnob, &odOutputKnob })
    {
        k->setColour (juce::Slider::rotarySliderFillColourId,    ink);
        k->setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff8a6f2e));
        k->setColour (juce::Slider::thumbColourId,               juce::Colour (0xfff5e6b8));
        k->setColour (juce::Slider::textBoxTextColourId,         ink);
        k->setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    }

    vibeOnAttachment        = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "vibeOn",        vibeOnButton);
    vibeSpeedAttachment     = std::make_unique<SliderAttachment> (audioProcessor.apvts, "vibeSpeed",     vibeSpeedKnob);
    vibeIntensityAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "vibeIntensity", vibeIntensityKnob);
    vibeModeAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
                                  audioProcessor.apvts, "vibeMode", vibeModeBox);

    odOnAttachment     = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "odOn",     odOnButton);
    odGainAttachment   = std::make_unique<SliderAttachment> (audioProcessor.apvts, "odGain",   odGainKnob);
    odTrebleAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "odTreble", odTrebleKnob);
    odOutputAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "odOutput", odOutputKnob);

    // --- FX rack ---
    for (auto* b : { &dlyOnButton, &dlyPingPongButton, &revOnButton })
    {
        b->setColour (juce::ToggleButton::textColourId, cream);
        b->setColour (juce::ToggleButton::tickColourId, cream);
        addAndMakeVisible (*b);
    }
    dlyOnButton.onStateChange = [this] { repaint (dlyLedArea.expanded (4)); };
    revOnButton.onStateChange = [this] { repaint (revLedArea.expanded (4)); };

    struct KnobDef { const char* name; const char* id; const char* suffix; double def; int decimals; };

    const std::array<KnobDef, numDlyKnobs> dlyDefs {{
        { "TIME",     "dlyTime",     " ms", 380.0,  0 },
        { "FEEDBACK", "dlyFeedback", " %",  35.0,   0 },
        { "TONE",     "dlyTone",     " Hz", 6000.0, 0 },
        { "MOD",      "dlyMod",      " %",  20.0,   0 },
        { "MIX",      "dlyMix",      " %",  25.0,   0 } }};

    const std::array<KnobDef, numRevKnobs> revDefs {{
        { "DECAY",     "revDecay",    " s",  2.5,    1 },
        { "SIZE",      "revSize",     " %",  60.0,   0 },
        { "PRE-DELAY", "revPreDelay", " ms", 20.0,   0 },
        { "TONE",      "revTone",     " Hz", 6000.0, 0 },
        { "MOD",       "revMod",      " %",  40.0,   0 },
        { "MIX",       "revMix",      " %",  20.0,   0 } }};

    for (size_t i = 0; i < (size_t) numDlyKnobs; ++i)
    {
        setupKnob (dlyKnobs[i], dlyLabels[i], dlyDefs[i].name, 11.0f, 70, dlyDefs[i].suffix, dlyDefs[i].def);
        dlyAttachments[i] = std::make_unique<SliderAttachment> (audioProcessor.apvts, dlyDefs[i].id, dlyKnobs[i]);
        dlyKnobs[i].setNumDecimalPlacesToDisplay (dlyDefs[i].decimals);
    }

    for (size_t i = 0; i < (size_t) numRevKnobs; ++i)
    {
        setupKnob (revKnobs[i], revLabels[i], revDefs[i].name, 11.0f, 70, revDefs[i].suffix, revDefs[i].def);
        revAttachments[i] = std::make_unique<SliderAttachment> (audioProcessor.apvts, revDefs[i].id, revKnobs[i]);
        revKnobs[i].setNumDecimalPlacesToDisplay (revDefs[i].decimals);
    }

    dlyOnAttachment       = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "dlyOn",       dlyOnButton);
    dlyPingPongAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "dlyPingPong", dlyPingPongButton);
    revOnAttachment       = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "revOn",       revOnButton);

    // Tidy number display on the other knobs
    for (auto* k : { &odGainKnob, &odTrebleKnob, &odOutputKnob })
        k->setNumDecimalPlacesToDisplay (1);
    vibeSpeedKnob.setNumDecimalPlacesToDisplay (1);
    vibeIntensityKnob.setNumDecimalPlacesToDisplay (0);
    lowCutKnob.setNumDecimalPlacesToDisplay (0);
    highCutKnob.setNumDecimalPlacesToDisplay (0);
    gateReleaseKnob.setNumDecimalPlacesToDisplay (0);

    lastFolder   = juce::File::getSpecialLocation (juce::File::userDesktopDirectory);
    lastIRFolder = lastFolder;

    refreshAmpInfo();
    refreshCabInfo();
    startTimerHz (15); // gate LED + keeps the model / IR names in sync

    setSize (1180, 720);
}

//==============================================================================
void VintageGuitarSuiteAudioProcessorEditor::chooseIRFile()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Choose a cabinet IR", lastIRFolder,
                                                       CabIR::getSupportedWildcard());

    const auto openFlags = juce::FileBrowserComponent::openMode
                         | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync (openFlags, [this] (const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();

        if (file == juce::File())
            return; // cancelled

        lastIRFolder = file.getParentDirectory();

        const auto error = audioProcessor.loadCabIR (file);

        if (error.isNotEmpty())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                    "Vintage Guitar Suite", error);
        refreshCabInfo();
    });
}

void VintageGuitarSuiteAudioProcessorEditor::refreshCabInfo()
{
    const auto& cab = audioProcessor.getCabIR();

    const auto name = cab.hasIR() ? cab.getIRName() : juce::String ("No IR loaded");
    if (irNameLabel.getText() != name)
        irNameLabel.setText (name, juce::dontSendNotification);

    clearIRButton.setEnabled (cab.hasIR());
}

//==============================================================================
void VintageGuitarSuiteAudioProcessorEditor::timerCallback()
{
    const bool led = audioProcessor.isGateClosing();

    if (led != gateLedOn)
    {
        gateLedOn = led;
        repaint (gateLedArea.expanded (2));
    }

    refreshAmpInfo();
    refreshCabInfo();
}

//==============================================================================
void VintageGuitarSuiteAudioProcessorEditor::setupKnob (juce::Slider& slider,
                                                        juce::Label& label,
                                                        const juce::String& text,
                                                        float labelSize,
                                                        int textBoxWidth,
                                                        const juce::String& suffix,
                                                        double doubleClickValue)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, textBoxWidth, 20);
    slider.setTextValueSuffix (suffix);
    slider.setDoubleClickReturnValue (true, doubleClickValue); // double-click = default
    slider.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colour (0xffd8c9a3));
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff3a3a3a));
    slider.setColour (juce::Slider::thumbColourId,               cream);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, cream);
    label.setFont (juce::FontOptions (labelSize, juce::Font::bold));
    addAndMakeVisible (label);
}

//==============================================================================
void VintageGuitarSuiteAudioProcessorEditor::chooseNamFile()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Choose a NAM model", lastFolder, "*.nam");

    const auto openFlags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync (openFlags, [this] (const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();

        if (file == juce::File())
            return; // cancelled

        lastFolder = file.getParentDirectory();

        const auto error = audioProcessor.loadNamModel (file);

        if (error.isNotEmpty())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                    "Vintage Guitar Suite", error);
        refreshAmpInfo();
    });
}

void VintageGuitarSuiteAudioProcessorEditor::refreshAmpInfo()
{
    const auto& amp = audioProcessor.getNamAmp();

    const auto name = amp.hasModel() ? amp.getModelName() : juce::String ("No model loaded");
    if (modelNameLabel.getText() != name)
        modelNameLabel.setText (name, juce::dontSendNotification);

    clearButton.setEnabled (amp.hasModel());

    juce::String warning;
    if (amp.hasSampleRateMismatch())
        warning = "Model is " + juce::String (amp.getModelSampleRate() / 1000.0, 1)
                + " kHz - set your project to the same rate for the correct tone";

    if (warningLabel.getText() != warning)
        warningLabel.setText (warning, juce::dontSendNotification);
}

//==============================================================================
void VintageGuitarSuiteAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Black-panel style background
    g.fillAll (panelBlack);

    // Thin silver strip at the top
    g.setColour (silver);
    g.fillRect (0, 0, getWidth(), 4);

    g.setColour (cream);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("Vintage Guitar Suite", 0, 14, getWidth(), 30, juce::Justification::centred);

    g.setColour (dimGrey);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("Vibe  -  Gold Drive  -  Gate  -  NAM Amp  -  Cab IR  -  Delay  -  Reverb", 0, 42, getWidth(), 16,
                juce::Justification::centred);

    // --- FX rack unit (PCM style): black faceplate, rack ears, display ---
    {
        const auto rack = fxArea.toFloat();
        juce::ColourGradient face (juce::Colour (0xff26272b), rack.getTopLeft(),
                                   juce::Colour (0xff141518), rack.getBottomLeft(), false);
        g.setGradientFill (face);
        g.fillRoundedRectangle (rack, 4.0f);
        g.setColour (silver.withAlpha (0.7f));
        g.drawRoundedRectangle (rack.reduced (0.5f), 4.0f, 1.0f);

        // Rack ears with screw holes
        for (auto ear : { rack.withWidth (22.0f), rack.withLeft (rack.getRight() - 22.0f) })
        {
            g.setColour (juce::Colour (0xff3a3b40));
            g.fillRect (ear);
            g.setColour (juce::Colours::black);
            for (float y : { ear.getY() + 18.0f, ear.getBottom() - 26.0f })
                g.fillRoundedRectangle (ear.getCentreX() - 4.0f, y, 8.0f, 12.0f, 3.0f);
        }

        // Display window
        const auto disp = fxDisplayArea.toFloat();
        g.setColour (juce::Colour (0xff0b1418));
        g.fillRoundedRectangle (disp, 3.0f);
        g.setColour (juce::Colour (0xff57d6ff));
        g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::bold));
        g.drawText ("VGS-PCM", disp, juce::Justification::centred);

        // Section titles and divider
        g.setColour (cream);
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        g.drawText ("DELAY",  dlyArea.withHeight (24).withWidth (60), juce::Justification::centredLeft);
        g.drawText ("REVERB", revArea.withHeight (24).withWidth (70), juce::Justification::centredLeft);

        g.setColour (silver.withAlpha (0.25f));
        g.drawVerticalLine ((dlyArea.getRight() + revArea.getX()) / 2, rack.getY() + 10.0f, rack.getBottom() - 10.0f);
    }

    // --- Pedals ---
    const auto drawLed = [&g] (juce::Rectangle<int> area, bool on)
    {
        const auto r = area.toFloat();
        g.setColour (on ? juce::Colour (0xffff3b2a) : juce::Colour (0xff3a1a16));
        g.fillEllipse (r);
        if (on)
        {
            g.setColour (juce::Colour (0x55ff3b2a));
            g.fillEllipse (r.expanded (3.0f));
        }
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (r, 1.0f);
    };

    // VIBE: dark enclosure with chrome edge
    g.setColour (juce::Colour (0xff2a2a30));
    g.fillRoundedRectangle (vibeArea.toFloat(), 10.0f);
    g.setColour (silver);
    g.drawRoundedRectangle (vibeArea.toFloat().reduced (1.0f), 10.0f, 2.0f);
    g.setColour (cream);
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawText ("VIBE", vibeArea.withHeight (34), juce::Justification::centred);
    drawLed (vibeLedArea, vibeOnButton.getToggleState());

    // GOLD DRIVE: gold enclosure, dark lettering
    juce::ColourGradient gold (juce::Colour (0xffe2c26a), odArea.getTopLeft().toFloat(),
                               juce::Colour (0xffb08a38), odArea.getBottomRight().toFloat(), false);
    g.setGradientFill (gold);
    g.fillRoundedRectangle (odArea.toFloat(), 10.0f);
    g.setColour (juce::Colour (0xff7a5f24));
    g.drawRoundedRectangle (odArea.toFloat().reduced (1.0f), 10.0f, 2.0f);
    g.setColour (juce::Colour (0xff2a2010));
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawText ("GOLD DRIVE", odArea.withHeight (34), juce::Justification::centred);
    drawLed (odLedArea, odOnButton.getToggleState());
    drawLed (dlyLedArea, dlyOnButton.getToggleState());
    drawLed (revLedArea, revOnButton.getToggleState());

    // Gate, amp and cab panels
    for (auto [panel, title] : { std::pair { gateArea, "GATE" },
                                 std::pair { ampArea,  "AMP" },
                                 std::pair { cabArea,  "CAB" } })
    {
        g.setColour (tolex);
        g.fillRoundedRectangle (panel.toFloat(), 6.0f);
        g.setColour (silver.withAlpha (0.6f));
        g.drawRoundedRectangle (panel.toFloat().reduced (0.5f), 6.0f, 1.0f);

        g.setColour (cream);
        g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        g.drawText (title, panel.withHeight (30), juce::Justification::centred);
    }

    // Gate LED: red while the gate is closing / silencing
    const auto led = gateLedArea.toFloat();
    g.setColour (gateLedOn ? juce::Colour (0xffe03020) : juce::Colour (0xff3a1a16));
    g.fillEllipse (led);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (led, 1.0f);
}

void VintageGuitarSuiteAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (45); // title space

    // --- Pedal row (centred): VIBE | GOLD DRIVE ---
    auto pedalRow = area.removeFromTop (180);
    area.removeFromTop (10);

    // --- FX rack at the bottom: DELAY | REVERB ---
    fxArea = area.removeFromBottom (165);
    area.removeFromBottom (10);
    {
        auto rack = fxArea.reduced (30, 10);   // inside the rack ears

        auto header = rack.removeFromTop (24);
        fxDisplayArea = header.removeFromRight (130).withSizeKeepingCentre (120, 22);

        auto half = rack.getWidth() / 2;
        dlyArea = rack.removeFromLeft (half).reduced (8, 0);
        revArea = rack.reduced (8, 0);

        // Titles sit in the header row; ON + LED (and PING-PONG) to their right
        dlyArea = dlyArea.withTop (header.getY()).withBottom (rack.getBottom());
        revArea = revArea.withTop (header.getY()).withBottom (rack.getBottom());

        auto dHead = dlyArea.withHeight (24).withTrimmedLeft (60);
        dlyOnButton.setBounds (dHead.removeFromLeft (52));
        dlyLedArea = dHead.removeFromLeft (16).withSizeKeepingCentre (12, 12);
        dHead.removeFromLeft (14);
        dlyPingPongButton.setBounds (dHead.removeFromLeft (110));

        auto rHead = revArea.withHeight (24).withTrimmedLeft (70);
        revOnButton.setBounds (rHead.removeFromLeft (52));
        revLedArea = rHead.removeFromLeft (16).withSizeKeepingCentre (12, 12);

        auto layoutKnobs = [] (juce::Rectangle<int> r, auto& knobs, auto& labels)
        {
            r.removeFromTop (28);
            const int w = r.getWidth() / (int) knobs.size();
            for (size_t i = 0; i < knobs.size(); ++i)
            {
                auto col = r.removeFromLeft (w);
                labels[i].setBounds (col.removeFromTop (15));
                knobs[i].setBounds (col);
            }
        };

        layoutKnobs (dlyArea, dlyKnobs, dlyLabels);
        layoutKnobs (revArea, revKnobs, revLabels);
    }

    constexpr int pedalWidth = 320, pedalGap = 40;
    auto pedals = pedalRow.withSizeKeepingCentre (pedalWidth * 2 + pedalGap, pedalRow.getHeight());
    vibeArea = pedals.removeFromLeft (pedalWidth);
    pedals.removeFromLeft (pedalGap);
    odArea = pedals;

    // Vibe contents
    {
        auto p = vibeArea.reduced (14);
        p.removeFromTop (28); // title

        // Mode selector on the left, ON + LED on the right
        auto onRow = p.removeFromTop (26);
        vibeLedArea = onRow.removeFromRight (16).withSizeKeepingCentre (14, 14);
        vibeOnButton.setBounds (onRow.removeFromRight (60));
        vibeModeBox.setBounds (onRow.removeFromLeft (110).withSizeKeepingCentre (110, 24));
        p.removeFromTop (4);

        auto speedCol = p.removeFromLeft (p.getWidth() / 2);
        vibeSpeedLabel.setBounds     (speedCol.removeFromTop (16));
        vibeSpeedKnob.setBounds      (speedCol);
        vibeIntensityLabel.setBounds (p.removeFromTop (16));
        vibeIntensityKnob.setBounds  (p);
    }

    // Gold Drive contents
    {
        auto p = odArea.reduced (14);
        p.removeFromTop (28); // title

        auto onRow = p.removeFromTop (26);
        odLedArea = onRow.removeFromRight (16).withSizeKeepingCentre (14, 14);
        odOnButton.setBounds (onRow.removeFromRight (60));
        p.removeFromTop (4);

        const int colW = p.getWidth() / 3;
        juce::Slider* knobs[] = { &odGainKnob, &odTrebleKnob, &odOutputKnob };
        juce::Label*  labels[] = { &odGainLabel, &odTrebleLabel, &odOutputLabel };

        for (int i = 0; i < 3; ++i)
        {
            auto col = p.removeFromLeft (colW);
            labels[i]->setBounds (col.removeFromTop (16));
            knobs[i]->setBounds (col);
        }
    }

    // --- Rack row ---
    auto left  = area.removeFromLeft (130);
    auto right = area.removeFromRight (130);
    gateArea   = area.removeFromLeft (170).reduced (10, 0);
    cabArea    = area.removeFromRight (220).reduced (10, 0);
    ampArea    = area.reduced (10, 0);

    // Cab section: IR name, LOAD IR + X, Bypass, then LOW CUT | HIGH CUT
    auto cab = cabArea.reduced (12);
    cab.removeFromTop (25); // "CAB" title

    irNameLabel.setBounds (cab.removeFromTop (26));
    cab.removeFromTop (4);

    auto irRow = cab.removeFromTop (28);
    clearIRButton.setBounds (irRow.removeFromRight (28));
    irRow.removeFromRight (6);
    loadIRButton.setBounds (irRow);

    cab.removeFromTop (6);
    cabBypassButton.setBounds (cab.removeFromTop (26).withSizeKeepingCentre (90, 26));
    cab.removeFromTop (6);

    auto lowCol  = cab.removeFromLeft (cab.getWidth() / 2);
    auto highCol = cab;
    lowCutLabel.setBounds  (lowCol.removeFromTop (18));
    lowCutKnob.setBounds   (lowCol.reduced (2, 0));
    highCutLabel.setBounds (highCol.removeFromTop (18));
    highCutKnob.setBounds  (highCol.reduced (2, 0));

    inputLabel.setBounds  (left.removeFromTop (24));
    inputKnob.setBounds   (left.reduced (5, 0));

    // Gate section: ON + LED, then THRESH and RELEASE stacked
    auto gate = gateArea.reduced (12);
    gate.removeFromTop (25); // "GATE" title

    auto gateRow = gate.removeFromTop (26);
    gateLedArea = gateRow.removeFromRight (14).withSizeKeepingCentre (14, 14);
    gateButton.setBounds (gateRow.withSizeKeepingCentre (60, 26));
    gate.removeFromTop (6);

    const int knobHeight = gate.getHeight() / 2;
    auto threshArea  = gate.removeFromTop (knobHeight);
    auto releaseArea = gate;

    gateThresholdLabel.setBounds (threshArea.removeFromTop (16));
    gateThresholdKnob.setBounds  (threshArea);
    gateReleaseLabel.setBounds   (releaseArea.removeFromTop (16));
    gateReleaseKnob.setBounds    (releaseArea);

    outputLabel.setBounds (right.removeFromTop (24));
    outputKnob.setBounds  (right.reduced (5, 0));

    // Amp section contents
    auto amp = ampArea.reduced (15);
    amp.removeFromTop (25); // "AMP" title

    modelNameLabel.setBounds (amp.removeFromTop (26));
    amp.removeFromTop (4);

    // Load / unload / bypass on one row
    auto buttonRow = amp.removeFromTop (28).withSizeKeepingCentre (juce::jmin (amp.getWidth(), 360), 28);
    bypassButton.setBounds (buttonRow.removeFromRight (90));
    buttonRow.removeFromRight (8);
    clearButton.setBounds (buttonRow.removeFromRight (28));
    buttonRow.removeFromRight (6);
    loadButton.setBounds (buttonRow);

    warningLabel.setBounds (amp.removeFromBottom (20));

    // GAIN  BASS  MIDDLE  TREBLE  VOLUME
    amp.removeFromTop (10);
    const int knobWidth = amp.getWidth() / numAmpKnobs;

    for (int i = 0; i < numAmpKnobs; ++i)
    {
        auto col = amp.removeFromLeft (knobWidth);
        ampKnobLabels[(size_t) i].setBounds (col.removeFromTop (18));
        ampKnobs[(size_t) i].setBounds (col.reduced (4, 0));
    }
}
