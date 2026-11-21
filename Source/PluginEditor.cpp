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

    lastFolder = juce::File::getSpecialLocation (juce::File::userDesktopDirectory);

    refreshAmpInfo();
    startTimerHz (4); // keeps the model name in sync (e.g. after a project loads)

    setSize (640, 300);
}

//==============================================================================
void VintageGuitarSuiteAudioProcessorEditor::setupKnob (juce::Slider& slider,
                                                        juce::Label& label,
                                                        const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    slider.setTextValueSuffix (" dB");
    slider.setDoubleClickReturnValue (true, 0.0); // double-click = 0 dB
    slider.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colour (0xffd8c9a3));
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff3a3a3a));
    slider.setColour (juce::Slider::thumbColourId,               cream);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, cream);
    label.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    addAndMakeVisible (label);
}

//==============================================================================
void VintageGuitarSuiteAudioProcessorEditor::chooseNamFile()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Choose a NAM model", lastFolder, "*.nam");

    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
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
    g.drawText ("Phase 2 - NAM Amp", 0, 42, getWidth(), 16, juce::Justification::centred);

    // Amp panel
    g.setColour (tolex);
    g.fillRoundedRectangle (ampArea.toFloat(), 6.0f);
    g.setColour (silver.withAlpha (0.6f));
    g.drawRoundedRectangle (ampArea.toFloat().reduced (0.5f), 6.0f, 1.0f);

    g.setColour (cream);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText ("AMP", ampArea.withHeight (30), juce::Justification::centred);
}

void VintageGuitarSuiteAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (50); // title space

    auto left  = area.removeFromLeft (150);
    auto right = area.removeFromRight (150);
    ampArea    = area.reduced (10, 0);

    inputLabel.setBounds  (left.removeFromTop (24));
    inputKnob.setBounds   (left.reduced (10, 0));

    outputLabel.setBounds (right.removeFromTop (24));
    outputKnob.setBounds  (right.reduced (10, 0));

    // Amp section contents
    auto amp = ampArea.reduced (15);
    amp.removeFromTop (25); // "AMP" title

    modelNameLabel.setBounds (amp.removeFromTop (30));
    amp.removeFromTop (8);

    auto buttonRow = amp.removeFromTop (30);
    clearButton.setBounds (buttonRow.removeFromRight (30));
    buttonRow.removeFromRight (6);
    loadButton.setBounds (buttonRow);

    amp.removeFromTop (10);
    bypassButton.setBounds (amp.removeFromTop (26).withSizeKeepingCentre (90, 26));

    warningLabel.setBounds (amp.removeFromBottom (36));
}
