#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Phase 1 editor: two knobs (Input / Output)
//==============================================================================
class VintageGuitarSuiteAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VintageGuitarSuiteAudioProcessorEditor (VintageGuitarSuiteAudioProcessor&);
    ~VintageGuitarSuiteAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& text);

    VintageGuitarSuiteAudioProcessor& audioProcessor;

    juce::Slider inputKnob, outputKnob;
    juce::Label  inputLabel, outputLabel;

    // Attachments must be declared AFTER the sliders (destroyed first)
    std::unique_ptr<SliderAttachment> inputAttachment, outputAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VintageGuitarSuiteAudioProcessorEditor)
};
