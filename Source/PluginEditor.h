#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/JeminiLookAndFeel.h"
#include "UI/RigPanel.h"

//==============================================================================
// The plugin window. It holds the RigPanel (all the controls, drawn at a fixed
// design size of 1180x740) and scales it to whatever size the window is.
// The window can be resized from the bottom-right corner (aspect ratio locked);
// the chosen size is remembered with the project.
//==============================================================================
class VintageGuitarSuiteAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VintageGuitarSuiteAudioProcessorEditor (VintageGuitarSuiteAudioProcessor&);
    ~VintageGuitarSuiteAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VintageGuitarSuiteAudioProcessor& audioProcessor;

    JeminiLookAndFeel lookAndFeel;   // must outlive the panel
    RigPanel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VintageGuitarSuiteAudioProcessorEditor)
};
