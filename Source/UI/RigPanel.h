#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"

//==============================================================================
// RigPanel - the whole Jemini Guitar Rig front panel, drawn at a fixed design
// size (the editor scales it to the window).
//
//   +--------------------------- title bar ---------------------------+
//   | VIBE pedal      |  AMP HEAD (black panel + grille, NAM loader)   |
//   | GOLD DRIVE      |  JGR-41 RACK (display, Delay/Reverb/IR Verb)   |
//   +-----------------------------------------------------------------+
//   | CABINET / IR grille  | CAB TONE | GATE | INPUT | OUTPUT + meter   |
//   +-----------------------------------------------------------------+
//==============================================================================
class RigPanel : public juce::Component,
                 private juce::Timer
{
public:
    explicit RigPanel (VintageGuitarSuiteAudioProcessor&);
    ~RigPanel() override = default;

    static constexpr int designWidth = 1280, designHeight = 720;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    //==========================================================================
    // Knob helper: a slider + its name label + its parameter attachment
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    enum class ValueBox { none, below };

    void setupKnob (Knob&, const juce::String& paramId, const juce::String& name, const juce::String& style,
                    ValueBox, int decimals, const juce::String& unit, juce::Colour labelColour);
    void setupToggle (juce::ToggleButton&, std::unique_ptr<ButtonAttachment>&, const juce::String& paramId,
                      const juce::String& style, juce::Colour textColour);
    void setupTextButton (juce::TextButton&);

    void chooseNamFile();
    void chooseIRFile();
    void chooseIRVerbFile();
    void refreshAmpInfo();
    void refreshCabInfo();
    void refreshIRVerbInfo();
    void setRackPage (int page);
    juce::String getDisplayText() const;
    void timerCallback() override;

    // Drawing helpers
    void drawPedal (juce::Graphics&, juce::Rectangle<float>, juce::Colour top, juce::Colour bottom) const;
    void drawScrew (juce::Graphics&, juce::Point<float>, float radius) const;
    void drawLed (juce::Graphics&, juce::Rectangle<int>, bool on, juce::Colour colour) const;
    void drawAmpHead (juce::Graphics&) const;
    void drawRack (juce::Graphics&) const;
    void drawBottomStrip (juce::Graphics&) const;
    void drawSubPanel (juce::Graphics&, juce::Rectangle<int>, const juce::String& title) const;
    void buildTextures();

    VintageGuitarSuiteAudioProcessor& audioProcessor;

    //==========================================================================
    // Layout regions (set in resized)
    juce::Rectangle<int> titleBar, vibeArea, odArea, ampArea, ampPanelArea, ampGrilleArea, modelTagArea,
                         rackArea, rackDisplayArea, rackListArea, rackControlsArea,
                         bottomArea, cabGrilleArea, cabFileArea, cabToneArea, gateArea, inputArea, outputArea,
                         meterArea, vibeLedArea, odLedArea, gateLedArea, jewelArea;
    std::array<juce::Rectangle<int>, 3> rackLedAreas;

    // Textures (generated once)
    juce::Image tolex, ampGrille, cabGrille;

    //==========================================================================
    // Pedals
    Knob vibeSpeed, vibeIntensity;
    juce::ToggleButton vibeOn, vibeMode;
    std::unique_ptr<ButtonAttachment> vibeOnAttachment, vibeModeAttachment;

    Knob odGain, odTreble, odOutput;
    juce::ToggleButton odOn;
    std::unique_ptr<ButtonAttachment> odOnAttachment;

    // Amp
    std::array<Knob, 5> ampKnobs;   // GAIN BASS MIDDLE TREBLE VOLUME
    juce::TextButton loadNamButton { "LOAD NAM..." }, clearNamButton { "X" };
    juce::ToggleButton ampBypass { "BYPASS" };
    std::unique_ptr<ButtonAttachment> ampBypassAttachment;
    juce::Label modelNameLabel, warningLabel;

    // Rack
    int rackPage = 0;
    std::array<juce::TextButton, 3> rackSelect;
    juce::ToggleButton dlyOn { "ON" }, dlyPingPong { "PING-PONG" }, revOn { "ON" }, irvOn { "ON" };
    std::unique_ptr<ButtonAttachment> dlyOnAttachment, dlyPingPongAttachment, revOnAttachment, irvOnAttachment;
    std::array<Knob, 5> dlyKnobs;
    std::array<Knob, 6> revKnobs;
    std::array<Knob, 5> irvKnobs;
    juce::TextButton loadIRVButton { "LOAD IR..." }, clearIRVButton { "X" };
    juce::String lastDisplayText;

    // Bottom strip
    juce::TextButton loadCabButton { "LOAD IR..." }, clearCabButton { "X" };
    juce::ToggleButton cabBypass { "BYPASS" };
    std::unique_ptr<ButtonAttachment> cabBypassAttachment;
    juce::Label cabNameLabel;
    Knob lowCut, highCut;

    juce::ToggleButton gateOn { "ON" };
    std::unique_ptr<ButtonAttachment> gateOnAttachment;
    Knob gateThreshold, gateRelease;
    bool gateLedLit = false;

    Knob input, output;
    float meterL = 0.0f, meterR = 0.0f;

    // Files
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::File lastNamFolder, lastCabFolder, lastIRVFolder;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RigPanel)
};
