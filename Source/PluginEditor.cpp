#include "PluginEditor.h"

//==============================================================================
VintageGuitarSuiteAudioProcessorEditor::VintageGuitarSuiteAudioProcessorEditor (
    VintageGuitarSuiteAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setupKnob (inputKnob,  inputLabel,  "INPUT");
    setupKnob (outputKnob, outputLabel, "OUTPUT");

    // Connect knobs to the parameters
    inputAttachment  = std::make_unique<SliderAttachment> (audioProcessor.apvts, "inputGain",  inputKnob);
    outputAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "outputGain", outputKnob);

    setSize (420, 260);
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
    slider.setColour (juce::Slider::thumbColourId,               juce::Colour (0xfff2ead3));
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colour (0xfff2ead3));
    label.setFont (juce::Font (15.0f, juce::Font::bold));
    addAndMakeVisible (label);
}

//==============================================================================
void VintageGuitarSuiteAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Black-panel style background
    g.fillAll (juce::Colour (0xff161616));

    // Thin silver strip at the top
    g.setColour (juce::Colour (0xffb8b8b8));
    g.fillRect (0, 0, getWidth(), 4);

    g.setColour (juce::Colour (0xfff2ead3));
    g.setFont (juce::Font (22.0f, juce::Font::bold));
    g.drawText ("Vintage Guitar Suite", 0, 14, getWidth(), 30, juce::Justification::centred);

    g.setColour (juce::Colour (0xff8a8a8a));
    g.setFont (juce::Font (12.0f));
    g.drawText ("Phase 1", 0, 42, getWidth(), 16, juce::Justification::centred);
}

void VintageGuitarSuiteAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (50); // title space

    auto left  = area.removeFromLeft (area.getWidth() / 2);
    auto right = area;

    inputLabel.setBounds  (left.removeFromTop (24));
    inputKnob.setBounds   (left.reduced (20, 0));

    outputLabel.setBounds (right.removeFromTop (24));
    outputKnob.setBounds  (right.reduced (20, 0));
}
