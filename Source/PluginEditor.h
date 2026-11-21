#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Phase 2 editor: INPUT knob | AMP section (NAM loader) | OUTPUT knob
//==============================================================================
class VintageGuitarSuiteAudioProcessorEditor : public juce::AudioProcessorEditor,
                                               private juce::Timer
{
public:
    explicit VintageGuitarSuiteAudioProcessorEditor (VintageGuitarSuiteAudioProcessor&);
    ~VintageGuitarSuiteAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& text,
                    float labelSize = 15.0f, int textBoxWidth = 80,
                    const juce::String& suffix = " dB", double doubleClickValue = 0.0);
    void chooseNamFile();
    void refreshAmpInfo();
    void timerCallback() override;

    VintageGuitarSuiteAudioProcessor& audioProcessor;

    // Gain knobs
    juce::Slider inputKnob, outputKnob;
    juce::Label  inputLabel, outputLabel;

    // Gate section
    juce::Rectangle<int> gateArea, gateLedArea;
    juce::ToggleButton gateButton { "ON" };
    juce::Slider gateThresholdKnob, gateReleaseKnob;
    juce::Label  gateThresholdLabel, gateReleaseLabel;
    bool gateLedOn = false;

    // Amp section
    juce::Rectangle<int> ampArea;
    juce::TextButton loadButton  { "LOAD NAM..." };
    juce::TextButton clearButton { "X" };
    juce::ToggleButton bypassButton { "Bypass" };
    juce::Label modelNameLabel, warningLabel;

    // Amp knobs: GAIN, BASS, MIDDLE, TREBLE, VOLUME
    static constexpr int numAmpKnobs = 5;
    std::array<juce::Slider, numAmpKnobs> ampKnobs;
    std::array<juce::Label,  numAmpKnobs> ampKnobLabels;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::File lastFolder;

    // Attachments must be declared AFTER the controls (destroyed first)
    std::unique_ptr<SliderAttachment> inputAttachment, outputAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment, gateOnAttachment;
    std::unique_ptr<SliderAttachment> gateThresholdAttachment, gateReleaseAttachment;
    std::array<std::unique_ptr<SliderAttachment>, numAmpKnobs> ampKnobAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VintageGuitarSuiteAudioProcessorEditor)
};
