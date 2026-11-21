#include "PluginEditor.h"

namespace
{
    const juce::Identifier uiScaleId ("uiScale");
    constexpr double minScale = 0.6, maxScale = 1.6, defaultScale = 0.9;
}

//==============================================================================
VintageGuitarSuiteAudioProcessorEditor::VintageGuitarSuiteAudioProcessorEditor (
    VintageGuitarSuiteAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), panel (p)
{
    setLookAndFeel (&lookAndFeel);
    panel.setLookAndFeel (&lookAndFeel);
    panel.applySkin (audioProcessor.getSkin());
    addAndMakeVisible (panel);

    constexpr int w = RigPanel::designWidth, h = RigPanel::designHeight;

    // Read the saved window size FIRST (setting limits below triggers resized(),
    // which would otherwise overwrite it)
    const double scale = juce::jlimit (minScale, maxScale,
                                       (double) audioProcessor.apvts.state.getProperty (uiScaleId, defaultScale));

    setResizable (true, true);
    setResizeLimits ((int) (w * minScale), (int) (h * minScale), (int) (w * maxScale), (int) (h * maxScale));
    getConstrainer()->setFixedAspectRatio ((double) w / (double) h);

    setSize ((int) std::round (w * scale), (int) std::round (h * scale));
}

VintageGuitarSuiteAudioProcessorEditor::~VintageGuitarSuiteAudioProcessorEditor()
{
    panel.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

//==============================================================================
void VintageGuitarSuiteAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (JeminiLookAndFeel::panelBlack);
}

void VintageGuitarSuiteAudioProcessorEditor::resized()
{
    const float scale = (float) getWidth() / (float) RigPanel::designWidth;

    panel.setBounds (0, 0, RigPanel::designWidth, RigPanel::designHeight);
    panel.setTransform (juce::AffineTransform::scale (scale));

    audioProcessor.apvts.state.setProperty (uiScaleId, scale, nullptr);
}
