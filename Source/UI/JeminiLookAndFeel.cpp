#include "JeminiLookAndFeel.h"

const juce::Colour JeminiLookAndFeel::cream      (0xfff2ead3);
const juce::Colour JeminiLookAndFeel::ink        (0xff2a2010);
const juce::Colour JeminiLookAndFeel::panelBlack (0xff111113);
const juce::Colour JeminiLookAndFeel::silver     (0xffc9ccd1);
const juce::Colour JeminiLookAndFeel::blueLed    (0xff57d6ff);
const juce::Colour JeminiLookAndFeel::redLed     (0xffff3b2a);

namespace
{
    juce::Point<float> polar (juce::Point<float> centre, float radius, float angle)
    {
        // JUCE angles: 0 = 12 o'clock, clockwise
        return centre.getPointOnCircumference (radius, angle);
    }

    juce::Typeface::Ptr loadFont (const char* data, int size)
    {
        return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
    }

    juce::Typeface::Ptr scriptFace()
    {
        static auto tf = loadFont (BinaryData::YellowtailRegular_ttf, BinaryData::YellowtailRegular_ttfSize);
        return tf;
    }

    juce::Typeface::Ptr condensedFace()
    {
        static auto tf = loadFont (BinaryData::OswaldVariable_ttf, BinaryData::OswaldVariable_ttfSize);
        return tf;
    }

    juce::Typeface::Ptr displayFace()
    {
        static auto tf = loadFont (BinaryData::ShareTechMonoRegular_ttf, BinaryData::ShareTechMonoRegular_ttfSize);
        return tf;
    }
}

juce::FontOptions JeminiLookAndFeel::script (float h)    { return juce::FontOptions (scriptFace()).withHeight (h); }
juce::FontOptions JeminiLookAndFeel::condensed (float h) { return juce::FontOptions (condensedFace()).withHeight (h); }
juce::FontOptions JeminiLookAndFeel::display (float h)   { return juce::FontOptions (displayFace()).withHeight (h); }

juce::Typeface::Ptr JeminiLookAndFeel::getTypefaceForFont (const juce::Font& f)
{
    if (f.getTypefacePtr() == nullptr || f.getTypefaceName() == juce::Font::getDefaultSansSerifFontName())
        if (auto tf = condensedFace())
            return tf;

    return LookAndFeel_V4::getTypefaceForFont (f);
}

JeminiLookAndFeel::JeminiLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId,       cream);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId,  blueLed.withAlpha (0.4f));

    setColour (juce::Label::textColourId, cream);

    setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff2c2c30));
    setColour (juce::TextButton::textColourOffId,  cream);
    setColour (juce::TextButton::textColourOnId,   cream);

    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1c1c20));
    setColour (juce::ComboBox::textColourId,       cream);
    setColour (juce::ComboBox::outlineColourId,    silver.withAlpha (0.5f));
    setColour (juce::ComboBox::arrowColourId,      cream);

    setColour (juce::PopupMenu::backgroundColourId,            juce::Colour (0xff1c1c20));
    setColour (juce::PopupMenu::textColourId,                  cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff3a3a42));
    setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);

    setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff1c1c20));
    setColour (juce::AlertWindow::textColourId,       cream);
}

//==============================================================================
void JeminiLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float pos, float start, float end, juce::Slider& slider)
{
    const auto style = slider.getProperties()["style"].toString();
    const auto size  = (float) juce::jmin (width, height);
    const auto area  = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height)
                           .withSizeKeepingCentre (size, size);

    if (style == "blackface")       drawBlackfaceKnob (g, area, pos, start, end, slider.isEnabled());
    else if (style == "pedal")      drawPedalKnob (g, area, pos, start, end, false);
    else if (style == "pedalLight") drawPedalKnob (g, area, pos, start, end, true);
    else                            drawRackKnob (g, area, pos, start, end, slider.isEnabled());
}

//------------------------------------------------------------------------------
// 60s amp knob: numbered silver skirt + black cap with white pointer
void JeminiLookAndFeel::drawBlackfaceKnob (juce::Graphics& g, juce::Rectangle<float> area,
                                           float pos, float start, float end, bool enabled)
{
    const auto c = area.getCentre();
    const float r = area.getWidth() * 0.5f;
    const float angle = start + pos * (end - start);

    // Numbers 1..10 printed on the panel around the knob
    g.setColour (cream.withAlpha (enabled ? 0.85f : 0.4f));
    g.setFont (condensed (juce::jmax (9.0f, r * 0.3f)));
    for (int n = 1; n <= 10; ++n)
    {
        const float a = start + (float) (n - 1) / 9.0f * (end - start);
        const auto p = polar (c, r * 0.9f, a);
        g.drawText (juce::String (n), juce::Rectangle<float> (r * 0.36f, r * 0.24f).withCentre (p),
                    juce::Justification::centred, false);
    }

    const float skirtR = r * 0.68f;
    const float capR   = r * 0.44f;

    // Drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (juce::Rectangle<float> (skirtR * 2.1f, skirtR * 2.1f).withCentre (c.translated (0, r * 0.05f)));

    // Silver skirt with brushed radial gradient
    juce::ColourGradient skirt (juce::Colour (0xffe9ecef), c.translated (-skirtR * 0.4f, -skirtR * 0.5f),
                                juce::Colour (0xff7d8186), c.translated (skirtR * 0.5f, skirtR * 0.7f), true);
    g.setGradientFill (skirt);
    g.fillEllipse (juce::Rectangle<float> (skirtR * 2.0f, skirtR * 2.0f).withCentre (c));

    // Skirt tick marks (rotate with the knob)
    g.setColour (juce::Colour (0xff2b2b2b));
    for (int i = 0; i < 10; ++i)
    {
        const float a = angle + (float) i / 10.0f * juce::MathConstants<float>::twoPi;
        g.drawLine ({ polar (c, skirtR * 0.82f, a), polar (c, skirtR * 0.96f, a) }, i == 0 ? 2.0f : 0.8f);
    }

    // Black cap
    juce::ColourGradient cap (juce::Colour (0xff4a4a4e), c.translated (-capR * 0.4f, -capR * 0.6f),
                              juce::Colour (0xff050506), c.translated (capR * 0.5f, capR * 0.7f), true);
    g.setGradientFill (cap);
    g.fillEllipse (juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (c));

    // Pointer line
    g.setColour (juce::Colours::white.withAlpha (enabled ? 0.95f : 0.5f));
    g.drawLine ({ polar (c, capR * 0.2f, angle), polar (c, skirtR * 0.97f, angle) }, juce::jmax (1.5f, r * 0.06f));
}

//------------------------------------------------------------------------------
// Studio rack knob: tick scale, blue value arc, knurled black body
void JeminiLookAndFeel::drawRackKnob (juce::Graphics& g, juce::Rectangle<float> area,
                                      float pos, float start, float end, bool enabled)
{
    const auto c = area.getCentre();
    const float r = area.getWidth() * 0.5f;
    const float angle = start + pos * (end - start);

    // Scale ticks
    g.setColour (silver.withAlpha (0.45f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = start + (float) i / 10.0f * (end - start);
        g.drawLine ({ polar (c, r * 0.86f, a), polar (c, r * (i % 5 == 0 ? 0.99f : 0.94f), a) }, 1.0f);
    }

    // Value arc (blue, like the display)
    const float arcR = r * 0.78f;
    juce::Path track, value;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, start, end, true);
    value.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, start, angle, true);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.strokePath (track, juce::PathStrokeType (r * 0.08f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (enabled ? blueLed.withAlpha (0.85f) : silver.withAlpha (0.3f));
    g.strokePath (value, juce::PathStrokeType (r * 0.08f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Knurled body
    const float bodyR = r * 0.62f;
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (juce::Rectangle<float> (bodyR * 2.1f, bodyR * 2.1f).withCentre (c.translated (0, r * 0.05f)));

    juce::ColourGradient body (juce::Colour (0xff3c3d42), c.translated (0, -bodyR),
                               juce::Colour (0xff0d0d0f), c.translated (0, bodyR), false);
    g.setGradientFill (body);
    g.fillEllipse (juce::Rectangle<float> (bodyR * 2.0f, bodyR * 2.0f).withCentre (c));

    g.setColour (juce::Colours::white.withAlpha (0.06f));
    for (int i = 0; i < 36; ++i)
    {
        const float a = (float) i / 36.0f * juce::MathConstants<float>::twoPi;
        g.drawLine ({ polar (c, bodyR * 0.82f, a), polar (c, bodyR * 0.99f, a) }, 1.0f);
    }

    // Aluminium cap
    const float capR = bodyR * 0.62f;
    juce::ColourGradient cap (juce::Colour (0xffb9bcc2), c.translated (-capR * 0.5f, -capR * 0.6f),
                              juce::Colour (0xff55585e), c.translated (capR * 0.5f, capR * 0.7f), true);
    g.setGradientFill (cap);
    g.fillEllipse (juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (c));

    // Pointer
    g.setColour (juce::Colours::white);
    g.drawLine ({ polar (c, bodyR * 0.25f, angle), polar (c, bodyR * 0.95f, angle) }, juce::jmax (1.5f, r * 0.07f));
}

//------------------------------------------------------------------------------
// Stompbox knob
void JeminiLookAndFeel::drawPedalKnob (juce::Graphics& g, juce::Rectangle<float> area,
                                       float pos, float start, float end, bool lightBody)
{
    const auto c = area.getCentre();
    const float r = area.getWidth() * 0.5f;
    const float angle = start + pos * (end - start);

    // Scale dots printed on the enclosure
    g.setColour (lightBody ? ink.withAlpha (0.75f) : cream.withAlpha (0.6f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = start + (float) i / 10.0f * (end - start);
        g.fillEllipse (juce::Rectangle<float> (2.2f, 2.2f).withCentre (polar (c, r * 0.92f, a)));
    }

    const float bodyR = r * 0.74f;
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (juce::Rectangle<float> (bodyR * 2.1f, bodyR * 2.1f).withCentre (c.translated (0, r * 0.07f)));

    juce::ColourGradient body (juce::Colour (0xff3a3a3e), c.translated (-bodyR * 0.4f, -bodyR * 0.6f),
                               juce::Colour (0xff060607), c.translated (bodyR * 0.5f, bodyR * 0.7f), true);
    g.setGradientFill (body);
    g.fillEllipse (juce::Rectangle<float> (bodyR * 2.0f, bodyR * 2.0f).withCentre (c));

    // Chrome top ring
    const float ringR = bodyR * 0.66f;
    juce::ColourGradient ring (juce::Colour (0xfff2f3f5), c.translated (-ringR * 0.5f, -ringR * 0.6f),
                               juce::Colour (0xff6c7076), c.translated (ringR * 0.5f, ringR * 0.6f), true);
    g.setGradientFill (ring);
    g.drawEllipse (juce::Rectangle<float> (ringR * 2.0f, ringR * 2.0f).withCentre (c), juce::jmax (1.5f, r * 0.07f));

    // Pointer
    g.setColour (juce::Colours::white);
    g.drawLine ({ polar (c, bodyR * 0.2f, angle), polar (c, bodyR * 0.96f, angle) }, juce::jmax (1.5f, r * 0.08f));
}

//==============================================================================
void JeminiLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                          bool highlighted, bool down)
{
    const auto style = button.getProperties()["style"].toString();

    if (style == "footswitch")       drawFootswitch (g, button.getLocalBounds().toFloat(), down);
    else if (style == "modeSwitch")  drawModeSwitch (g, button);
    else                             drawMiniToggle (g, button, highlighted);
}

// Horizontal bat-handle switch (labels are drawn by the panel on each side)
void JeminiLookAndFeel::drawModeSwitch (juce::Graphics& g, juce::ToggleButton& button)
{
    auto area = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto slot = area.withSizeKeepingCentre (area.getWidth(), juce::jmin (area.getHeight(), 14.0f));

    // Chrome mounting nut
    const auto c = slot.getCentre();
    juce::ColourGradient nut (juce::Colour (0xffe2e4e8), c.translated (-8, -8), juce::Colour (0xff5d6167), c.translated (8, 8), false);
    g.setGradientFill (nut);
    g.fillEllipse (juce::Rectangle<float> (16.0f, 16.0f).withCentre (c));

    // Lever pointing left (off) or right (on)
    const bool on = button.getToggleState();
    const auto tip = juce::Point<float> (on ? slot.getRight() - 4.0f : slot.getX() + 4.0f, c.y);

    juce::Path lever;
    lever.startNewSubPath (c);
    lever.lineTo (tip);
    g.setColour (juce::Colour (0xffd0d3d8));
    g.strokePath (lever, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (juce::Colours::white);
    g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (tip));
}

void JeminiLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool highlighted, bool down)
{
    if (button.getProperties()["style"].toString() == "rackSelect")
    {
        const bool selected = button.getToggleState();
        g.setColour (selected ? juce::Colours::white : silver.withAlpha (highlighted ? 0.85f : 0.55f));
        g.setFont (condensed (15.0f));
        g.drawText (button.getButtonText(), button.getLocalBounds().withTrimmedLeft (2),
                    juce::Justification::centredLeft, false);
        return;
    }

    LookAndFeel_V4::drawButtonText (g, button, highlighted, down);
}

void JeminiLookAndFeel::drawBubble (juce::Graphics& g, juce::BubbleComponent&, const juce::Point<float>&,
                                    const juce::Rectangle<float>& body)
{
    g.setColour (juce::Colour (0xee101012));
    g.fillRoundedRectangle (body, 4.0f);
    g.setColour (silver.withAlpha (0.6f));
    g.drawRoundedRectangle (body, 4.0f, 1.0f);
}

juce::Font JeminiLookAndFeel::getSliderPopupFont (juce::Slider&)
{
    return condensed (15.0f);
}

// Chrome stomp switch (hex nut + domed button)
void JeminiLookAndFeel::drawFootswitch (juce::Graphics& g, juce::Rectangle<float> area, bool down)
{
    const float size = juce::jmin (area.getWidth(), area.getHeight());
    const auto c = area.getCentre();

    // Hex nut
    juce::Path hex;
    const float hr = size * 0.48f;
    for (int i = 0; i < 6; ++i)
    {
        const auto p = polar (c, hr, (float) i / 6.0f * juce::MathConstants<float>::twoPi + 0.5236f);
        if (i == 0) hex.startNewSubPath (p); else hex.lineTo (p);
    }
    hex.closeSubPath();

    juce::ColourGradient nut (juce::Colour (0xffd9dce0), c.translated (-hr, -hr),
                              juce::Colour (0xff5a5e64), c.translated (hr, hr), false);
    g.setGradientFill (nut);
    g.fillPath (hex);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.strokePath (hex, juce::PathStrokeType (1.0f));

    // Domed button
    const float br = size * (down ? 0.27f : 0.3f);
    juce::ColourGradient dome (juce::Colours::white, c.translated (-br * 0.4f, -br * 0.5f),
                               juce::Colour (down ? 0xff5a5e64 : 0xff8a8e94), c.translated (br * 0.6f, br * 0.7f), true);
    g.setGradientFill (dome);
    g.fillEllipse (juce::Rectangle<float> (br * 2.0f, br * 2.0f).withCentre (c));
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.drawEllipse (juce::Rectangle<float> (br * 2.0f, br * 2.0f).withCentre (c), 1.0f);
}

// Small bat-handle toggle switch + text
void JeminiLookAndFeel::drawMiniToggle (juce::Graphics& g, juce::ToggleButton& button, bool highlighted)
{
    auto area = button.getLocalBounds().toFloat();
    const bool on = button.getToggleState();

    auto sw = area.removeFromLeft (juce::jmin (area.getHeight() * 0.62f, 16.0f)).reduced (0.0f, area.getHeight() * 0.12f);

    // Slot
    g.setColour (juce::Colour (0xff050506));
    g.fillRoundedRectangle (sw, sw.getWidth() * 0.5f);
    g.setColour (silver.withAlpha (0.5f));
    g.drawRoundedRectangle (sw, sw.getWidth() * 0.5f, 1.0f);

    // Lever
    const float lr = sw.getWidth() * 0.42f;
    const auto leverC = juce::Point<float> (sw.getCentreX(), on ? sw.getY() + lr + 1.5f : sw.getBottom() - lr - 1.5f);
    juce::ColourGradient lever (juce::Colours::white, leverC.translated (-lr * 0.4f, -lr * 0.4f),
                                juce::Colour (0xff70747a), leverC.translated (lr * 0.6f, lr * 0.6f), true);
    g.setGradientFill (lever);
    g.fillEllipse (juce::Rectangle<float> (lr * 2.0f, lr * 2.0f).withCentre (leverC));

    // Label
    auto textColour = button.findColour (juce::ToggleButton::textColourId);
    g.setColour (highlighted ? textColour.brighter (0.3f) : textColour);
    g.setFont (condensed (juce::jmin (16.0f, area.getHeight() * 0.8f)));
    g.drawText (button.getButtonText(), area.withTrimmedLeft (5.0f), juce::Justification::centredLeft, true);
}

//==============================================================================
void JeminiLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                              const juce::Colour&, bool highlighted, bool down)
{
    if (button.getProperties()["style"].toString() == "rackSelect")
        return; // flat text only
    auto r = button.getLocalBounds().toFloat().reduced (0.5f);

    juce::ColourGradient grad (juce::Colour (down ? 0xff1a1a1d : (highlighted ? 0xff3e3e44 : 0xff34343a)), r.getTopLeft(),
                               juce::Colour (down ? 0xff2a2a2e : 0xff1c1c20), r.getBottomLeft(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, 4.0f);

    g.setColour (silver.withAlpha (button.isEnabled() ? 0.55f : 0.2f));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

juce::Font JeminiLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return condensed (juce::jmin (17.0f, (float) buttonHeight * 0.68f));
}

juce::Label* JeminiLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (slider);
    l->setFont (condensed (15.0f));
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    return l;
}

void JeminiLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                      int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    juce::Path arrow;
    const float ax = (float) width - 14.0f, ay = (float) height * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

juce::Font JeminiLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return condensed (14.0f);
}
