#pragma once

#include <JuceHeader.h>

//==============================================================================
// JeminiLookAndFeel - vintage hardware look for Jemini Guitar Rig.
//
// Knob styles (set with  slider.getProperties().set ("style", "<name>")):
//   "blackface"   - 60s amp skirted knob: silver skirt numbered 1-10, black cap
//   "rack"        - studio rack knob: black knurled body, tick scale, blue arc
//   "pedal"       - stompbox knob: black body, chrome top ring, white line
//   "pedalLight"  - same, for light (gold) enclosures (dark scale marks)
//
// Toggle styles (button.getProperties().set ("style", "<name>")):
//   "footswitch"  - chrome stomp switch (the pedal's LED shows the state)
//   "modeSwitch"  - horizontal bat-handle switch (left = off, right = on)
//   default       - small bat-handle toggle switch with a label
//
// Text button styles:
//   "rackSelect"  - flat text in the rack's effect list (bright when toggled on)
//   default       - dark beveled button
//==============================================================================
class JeminiLookAndFeel : public juce::LookAndFeel_V4
{
public:
    JeminiLookAndFeel();

    // Palette
    static const juce::Colour cream, ink, panelBlack, silver, blueLed, redLed;

    // Embedded fonts (Resources/Fonts)
    static juce::FontOptions script    (float height);   // Yellowtail - script lettering (currently unused)
    static juce::FontOptions logo      (float height);   // Days One - amp/brand logos (drawn slanted)

    // Draws text in the logo font with a forward slant (60s "speed" logo style)
    static void drawSlantedLogo (juce::Graphics&, const juce::String& text, juce::Rectangle<float> area,
                                 float height, juce::Justification just, float slant = 0.22f);
    static juce::FontOptions condensed (float height);   // Oswald - panel lettering
    static juce::FontOptions display   (float height);   // Share Tech Mono - rack display

    // Default sans-serif text uses the condensed panel font
    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    void drawBubble (juce::Graphics&, juce::BubbleComponent&, const juce::Point<float>& tip,
                     const juce::Rectangle<float>& body) override;
    juce::Font getSliderPopupFont (juce::Slider&) override;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;

    juce::Font getComboBoxFont (juce::ComboBox&) override;

private:
    void drawBlackfaceKnob (juce::Graphics&, juce::Rectangle<float> area, float pos, float start, float end, bool enabled);
    void drawRackKnob      (juce::Graphics&, juce::Rectangle<float> area, float pos, float start, float end, bool enabled);
    void drawPedalKnob     (juce::Graphics&, juce::Rectangle<float> area, float pos, float start, float end, bool lightBody);

    void drawFootswitch    (juce::Graphics&, juce::Rectangle<float> area, bool down);
    void drawMiniToggle    (juce::Graphics&, juce::ToggleButton&, bool highlighted);
    void drawModeSwitch    (juce::Graphics&, juce::ToggleButton&);
};
