#include "JeminiLookAndFeel.h"
#include "Realism.h"

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

    juce::Typeface::Ptr logoFace()
    {
        static auto tf = loadFont (BinaryData::DaysOneRegular_ttf, BinaryData::DaysOneRegular_ttfSize);
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
juce::FontOptions JeminiLookAndFeel::logo (float h)      { return juce::FontOptions (logoFace()).withHeight (h); }

void JeminiLookAndFeel::drawSlantedLogo (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                                         float height, juce::Justification just, float slant)
{
    // Shear around the vertical centre so the text leans right without drifting sideways
    const float cy = area.getCentreY();
    juce::Graphics::ScopedSaveState state (g);
    g.addTransform (juce::AffineTransform::shear (-slant, 0.0f).translated (slant * cy, 0.0f));
    g.setFont (juce::Font (logo (height)).withExtraKerningFactor (0.04f));
    g.drawText (text, area, just, false);
}

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

    if (ampKnobChicken)
    {
        // Printed triangle under the knob: apex hidden under the knob, base below it
        juce::Path tri;
        tri.startNewSubPath (c.x, c.y + r * 0.10f);
        tri.lineTo (c.x + r * 0.80f, c.y + r * 0.95f);
        tri.lineTo (c.x - r * 0.80f, c.y + r * 0.95f);
        tri.closeSubPath();
        g.setColour (ampKnobNumbers.withAlpha (enabled ? 0.9f : 0.4f));
        g.strokePath (tri, juce::PathStrokeType (juce::jmax (1.0f, r * 0.035f), juce::PathStrokeType::mitered));
    }
    else
    {
    // Numbers 1..10 printed on the panel around the knob
    g.setColour (ampKnobNumbers.withAlpha (enabled ? 0.88f : 0.4f));
    g.setFont (condensed (juce::jmax (9.0f, r * 0.3f)));
    for (int n = 1; n <= 10; ++n)
    {
        const float a = start + (float) (n - 1) / 9.0f * (end - start);
        const auto p = polar (c, r * 0.9f, a);
        g.drawText (juce::String (n), juce::Rectangle<float> (r * 0.36f, r * 0.24f).withCentre (p),
                    juce::Justification::centred, false);
    }
    }

    if (ampKnobChicken)
    {
        // Two-piece white knob: wide round skirt, with a raised chicken-head pointer moulded on top
        const auto lightGrad = [] (juce::Point<float> pc, float rad, float t, bool face)
        {
            const auto b = juce::Rectangle<float> (rad * 2.0f, rad * 2.0f).withCentre (pc);
            if (face)
            {
                juce::ColourGradient gr (juce::Colour (0xffffffff), b.getTopLeft(), juce::Colour (0xffd2cec3), b.getBottomRight(), false);
                gr.addColour (0.5, juce::Colour (0xfff4f2ec));
                return gr;
            }
            const auto dark = juce::Colour (0xff8a867c), mid = juce::Colour (0xffe6e3da), lite = juce::Colour (0xfff7f5ef);
            juce::ColourGradient gr (lite.interpolatedWith (dark, 0.55f * (1.0f - t)), b.getX(), pc.y,
                                     dark.interpolatedWith (juce::Colour (0xff55524b), 1.0f - t), b.getRight(), pc.y, false);
            gr.addColour (0.3, mid.interpolatedWith (dark, 0.5f * (1.0f - t)));
            return gr;
        };

        // Extrude a path upward from 'from' to 'to', tapering from scale k0 to k1, then fill its top face
        const auto extrude = [&] (const juce::Path& shape, float rad, juce::Point<float> from, juce::Point<float> to, float k0, float k1)
        {
            const int steps = juce::jmax (5, (int) std::ceil (to.getDistanceFrom (from)));
            for (int i = 0; i <= steps; ++i)
            {
                const float t = (float) i / (float) steps;
                const auto pc = from + (to - from) * t;
                juce::Path q (shape);
                q.applyTransform (juce::AffineTransform::scale (k0 + (k1 - k0) * t).translated (pc));
                g.setGradientFill (lightGrad (pc, rad, t, false));
                g.fillPath (q);
            }
            juce::Path top (shape);
            top.applyTransform (juce::AffineTransform::scale (k1).translated (to));
            g.setGradientFill (lightGrad (to, rad, 1.0f, true));
            g.fillPath (top);
            return top;
        };

        // --- skirt -----------------------------------------------------------
        const float skirtR = r * 0.52f, skirtH = r * 0.13f;
        juce::Path disc;  disc.addEllipse (-skirtR, -skirtR, skirtR * 2.0f, skirtR * 2.0f);
        const auto s0 = c.translated (0.0f, skirtH * 0.5f), s1 = c.translated (0.0f, -skirtH * 0.5f);

        juce::Path shadowDisc (disc); shadowDisc.applyTransform (juce::AffineTransform::scale (1.08f).translated (s0));
        juce::DropShadow (juce::Colours::black.withAlpha (0.85f), (int) (r * 0.2f),
                          { (int) std::round (r * 0.07f), (int) std::round (r * 0.13f) }).drawForPath (g, shadowDisc);

        const auto skirtTop = extrude (disc, skirtR, s0, s1, 1.08f, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        g.strokePath (skirtTop, juce::PathStrokeType (0.8f));
        // soft rounded rim highlight
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.drawEllipse (juce::Rectangle<float> (skirtR * 1.86f, skirtR * 1.86f).withCentre (s1.translated (-0.4f, -0.6f)), 0.8f);

        // --- raised pointer ----------------------------------------------------
        const float u = r;
        juce::Path fin;
        fin.startNewSubPath (-0.075f * u, -0.66f * u);
        fin.cubicTo (-0.09f * u, -0.46f * u, -0.17f * u, -0.30f * u, -0.25f * u, -0.17f * u);
        fin.addArc (-0.30f * u, -0.26f * u, 0.60f * u, 0.60f * u, -0.99f, -juce::MathConstants<float>::twoPi + 0.99f, false);
        fin.cubicTo (0.17f * u, -0.30f * u, 0.09f * u, -0.46f * u, 0.075f * u, -0.66f * u);
        fin.cubicTo (0.06f * u, -0.73f * u, -0.06f * u, -0.73f * u, -0.075f * u, -0.66f * u);
        fin.closeSubPath();
        fin.applyTransform (juce::AffineTransform::rotation (angle));

        const float finH = r * 0.16f;
        const auto f0 = s1, f1 = s1.translated (0.0f, -finH);
        juce::Path finShadow (fin); finShadow.applyTransform (juce::AffineTransform::translation (f0.translated (r * 0.03f, r * 0.05f)));
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        g.fillPath (finShadow);
        const auto finTop = extrude (fin, r * 0.5f, f0, f1, 1.06f, 1.0f);

        // rounded top edge of the pointer
        g.saveState();
        g.reduceClipRegion (finTop);
        juce::Path rimLo (finTop); rimLo.applyTransform (juce::AffineTransform::translation (r * 0.025f, r * 0.035f));
        g.setColour (juce::Colours::black.withAlpha (0.12f));
        g.strokePath (rimLo, juce::PathStrokeType (r * 0.06f));
        juce::Path rimHi (finTop); rimHi.applyTransform (juce::AffineTransform::translation (-r * 0.02f, -r * 0.025f));
        g.setColour (juce::Colours::white.withAlpha (0.95f));
        g.strokePath (rimHi, juce::PathStrokeType (r * 0.04f));
        g.restoreState();
        g.setColour (juce::Colour (0xff77726a).withAlpha (0.45f));
        g.strokePath (finTop, juce::PathStrokeType (0.7f));

        // black indicator line along the pointer
        g.setColour (juce::Colour (0xff121212).withAlpha (enabled ? 0.95f : 0.5f));
        g.drawLine ({ polar (f1, r * 0.20f, angle), polar (f1, r * 0.62f, angle) }, juce::jmax (1.3f, r * 0.04f));
        return;
    }

    if (ampKnobAluminium)
    {
        // Black knurled base, then a large machined aluminium cap
        const float baseR = r * 0.68f, alCapR = r * 0.54f;
        realism::softShadowEllipse (g, c, baseR, baseR * 0.4f, { r * 0.07f, r * 0.18f }, 0.55f);

        juce::ColourGradient base (juce::Colour (0xff3a3a3e), c.translated (-baseR * 0.5f, -baseR * 0.6f),
                                   juce::Colour (0xff050506), c.translated (baseR * 0.6f, baseR * 0.7f), true);
        g.setGradientFill (base);
        g.fillEllipse (juce::Rectangle<float> (baseR * 2.0f, baseR * 2.0f).withCentre (c));
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        for (int i = 0; i < 48; ++i)
        {
            const float a = angle + (float) i / 48.0f * juce::MathConstants<float>::twoPi;
            g.drawLine ({ polar (c, alCapR * 1.02f, a), polar (c, baseR * 0.98f, a) }, 0.8f);
        }
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.drawEllipse (juce::Rectangle<float> (baseR * 1.96f, baseR * 1.96f).withCentre (c.translated (0.0f, -0.5f)), 0.8f);

        realism::softShadowEllipse (g, c, alCapR, alCapR * 0.18f, { alCapR * 0.04f, alCapR * 0.12f }, 0.5f);
        realism::spunMetal (g, c, alCapR, juce::Colour (0xffc9cdd2), 75.0f);
        juce::ColourGradient dome (juce::Colours::white.withAlpha (0.35f), c.translated (-alCapR * 0.4f, -alCapR * 0.5f),
                                   juce::Colours::black.withAlpha (0.25f), c.translated (alCapR * 0.6f, alCapR * 0.8f), true);
        g.setGradientFill (dome);
        g.fillEllipse (juce::Rectangle<float> (alCapR * 2.0f, alCapR * 2.0f).withCentre (c));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (juce::Rectangle<float> (alCapR * 2.0f, alCapR * 2.0f).withCentre (c), 0.9f);
        realism::specular (g, c, alCapR, 0.55f);

        // engraved pointer line across the cap
        g.setColour (juce::Colour (0xff141416).withAlpha (enabled ? 0.95f : 0.5f));
        g.drawLine ({ polar (c, alCapR * 0.15f, angle), polar (c, alCapR * 0.96f, angle) }, juce::jmax (1.8f, r * 0.065f));
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawLine ({ polar (c.translated (0.6f, 0.6f), alCapR * 0.15f, angle), polar (c.translated (0.6f, 0.6f), alCapR * 0.96f, angle) }, 0.6f);
        return;
    }

    const float skirtR = r * 0.68f;
    const float capR   = r * 0.44f;

    // Soft shadow cast down-right on the panel
    realism::softShadowEllipse (g, c, skirtR, skirtR * 0.35f, { r * 0.06f, r * 0.16f }, 0.45f);

    // Spun-aluminium skirt with a polished rim
    realism::spunMetal (g, c, skirtR, juce::Colour (0xffb8bcc2), 70.0f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (juce::Rectangle<float> (skirtR * 2.0f, skirtR * 2.0f).withCentre (c), 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.drawEllipse (juce::Rectangle<float> (skirtR * 1.9f, skirtR * 1.9f).withCentre (c.translated (0.0f, -0.4f)), 0.7f);

    // Skirt tick marks (rotate with the knob)
    g.setColour (juce::Colour (0xff2b2b2b));
    for (int i = 0; i < 10; ++i)
    {
        const float a = angle + (float) i / 10.0f * juce::MathConstants<float>::twoPi;
        g.drawLine ({ polar (c, skirtR * 0.82f, a), polar (c, skirtR * 0.96f, a) }, i == 0 ? 2.0f : 0.8f);
    }

    // Black bakelite cap: shadow on the skirt, knurled edge, glossy top
    realism::softShadowEllipse (g, c, capR, capR * 0.25f, { capR * 0.05f, capR * 0.15f }, 0.4f);
    juce::ColourGradient cap (ampKnobCap.interpolatedWith (juce::Colours::white, 0.25f), c.translated (-capR * 0.4f, -capR * 0.6f),
                              ampKnobCap.interpolatedWith (juce::Colours::black, 0.6f), c.translated (capR * 0.5f, capR * 0.7f), true);
    g.setGradientFill (cap);
    g.fillEllipse (juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (c));
    g.setColour (juce::Colours::white.withAlpha (0.09f));
    for (int i = 0; i < 40; ++i)
    {
        const float a = (float) i / 40.0f * juce::MathConstants<float>::twoPi;
        g.drawLine ({ polar (c, capR * 0.86f, a), polar (c, capR * 0.99f, a) }, 0.7f);
    }
    realism::specular (g, c, capR, 0.35f);

    // Pointer line (dark on light caps)
    const bool lightCap = ampKnobCap.getPerceivedBrightness() > 0.5f;
    g.setColour ((lightCap ? juce::Colour (0xff1a1410) : juce::Colours::white).withAlpha (enabled ? 0.95f : 0.5f));
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
    g.setColour (enabled ? rackAccent.withAlpha (0.9f) : silver.withAlpha (0.3f));
    g.strokePath (value, juce::PathStrokeType (r * 0.08f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Knurled body
    const float bodyR = r * 0.62f;
    realism::softShadowEllipse (g, c, bodyR, bodyR * 0.3f, { r * 0.05f, r * 0.14f }, 0.5f);

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

    // Rim light on the knurled body
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawEllipse (juce::Rectangle<float> (bodyR * 1.96f, bodyR * 1.96f).withCentre (c.translated (0.0f, -0.5f)), 0.8f);

    // Spun aluminium cap
    const float capR = bodyR * 0.62f;
    realism::spunMetal (g, c, capR, juce::Colour (0xff9fa3aa), 60.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (c), 0.8f);

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
    realism::softShadowEllipse (g, c, bodyR, bodyR * 0.35f, { r * 0.06f, r * 0.18f }, 0.55f);

    juce::ColourGradient body (juce::Colour (0xff3a3a3e), c.translated (-bodyR * 0.4f, -bodyR * 0.6f),
                               juce::Colour (0xff060607), c.translated (bodyR * 0.5f, bodyR * 0.7f), true);
    g.setGradientFill (body);
    g.fillEllipse (juce::Rectangle<float> (bodyR * 2.0f, bodyR * 2.0f).withCentre (c));

    // Grip ridges around the skirt + rim light
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    for (int i = 0; i < 30; ++i)
    {
        const float a = (float) i / 30.0f * juce::MathConstants<float>::twoPi;
        g.drawLine ({ polar (c, bodyR * 0.75f, a), polar (c, bodyR * 0.98f, a) }, 1.0f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.drawEllipse (juce::Rectangle<float> (bodyR * 1.96f, bodyR * 1.96f).withCentre (c.translated (0.0f, -0.5f)), 0.8f);
    realism::specular (g, c, bodyR * 0.66f, 0.3f);

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
    if (button.getProperties()["style"].toString() == "recButton")
        return;   // the red dome says it all

    if (button.getProperties()["style"].toString() == "presetButton")
    {
        g.setColour (cream.withAlpha (highlighted ? 1.0f : 0.85f));
        g.setFont (condensed (button.getButtonText().length() > 1 ? 14.0f : 17.0f));
        g.drawText (button.getButtonText(), button.getLocalBounds().translated (0, down ? 1 : 0), juce::Justification::centred, false);
        return;
    }

    if (button.getProperties()["style"].toString() == "inputSelect")
    {
        g.setColour (button.getToggleState() ? cream : silver.withAlpha (highlighted ? 0.8f : 0.5f));
        g.setFont (condensed (14.0f));
        g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
        return;
    }

    if (button.getProperties()["style"].toString() == "tunerButton")
    {
        g.setColour (button.getToggleState() ? juce::Colour (0xffeafff0) : cream.withAlpha (highlighted ? 1.0f : 0.85f));
        g.setFont (condensed (16.0f));
        g.drawText (button.getButtonText(), button.getLocalBounds().withTrimmedLeft (20), juce::Justification::centred, false);
        return;
    }

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

    realism::softShadowEllipse (g, c, hr * 0.9f, hr * 0.35f, { 1.0f, 3.0f }, 0.5f);
    juce::ColourGradient nut (juce::Colour (0xfff0f2f4), c.translated (-hr, -hr),
                              juce::Colour (0xff4a4e54), c.translated (hr, hr), false);
    nut.addColour (0.5, juce::Colour (0xff9da1a8));
    g.setGradientFill (nut);
    g.fillPath (hex);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.strokePath (hex, juce::PathStrokeType (0.8f), juce::AffineTransform::scale (0.9f, 0.9f, c.x, c.y));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.strokePath (hex, juce::PathStrokeType (1.0f));

    // Domed button
    const float br = size * (down ? 0.27f : 0.3f);
    juce::ColourGradient dome (juce::Colours::white, c.translated (-br * 0.4f, -br * 0.5f),
                               juce::Colour (down ? 0xff5a5e64 : 0xff8a8e94), c.translated (br * 0.6f, br * 0.7f), true);
    realism::softShadowEllipse (g, c, br, br * 0.3f, { 0.5f, 1.5f }, 0.45f);
    realism::spunMetal (g, c, br, juce::Colour (down ? 0xff8a8e94 : 0xffb2b6bc), 65.0f);
    g.setGradientFill (dome);
    g.setOpacity (0.45f);
    g.fillEllipse (juce::Rectangle<float> (br * 2.0f, br * 2.0f).withCentre (c));
    g.setOpacity (1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (juce::Rectangle<float> (br * 2.0f, br * 2.0f).withCentre (c), 1.0f);
    realism::specular (g, c, br, 0.8f);
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
    realism::softShadowEllipse (g, leverC, lr, lr * 0.4f, { 0.4f, 1.4f }, 0.5f);
    g.setGradientFill (lever);
    g.fillEllipse (juce::Rectangle<float> (lr * 2.0f, lr * 2.0f).withCentre (leverC));
    realism::specular (g, leverC, lr, 0.6f);

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
    const auto style = button.getProperties()["style"].toString();
    if (style == "rackSelect")
        return; // flat text only

    if (style == "inputSelect")
    {
        auto r = button.getLocalBounds().toFloat().reduced (1.0f);
        const bool on = button.getToggleState();
        g.setColour (juce::Colour (0xff050506));
        g.fillRoundedRectangle (r, 4.0f);
        if (on)
        {
            juce::ColourGradient lit (juce::Colour (0xff3a3326), r.getX(), r.getY(), juce::Colour (0xff1a160f), r.getX(), r.getBottom(), false);
            g.setGradientFill (lit);
            g.fillRoundedRectangle (r.reduced (1.5f), 3.0f);
        }
        g.setColour (on ? cream.withAlpha (0.6f) : silver.withAlpha (highlighted ? 0.5f : 0.25f));
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
        return;
    }

    if (style == "recButton")
    {
        const auto r = button.getLocalBounds().toFloat().reduced (1.0f);
        const auto c = r.getCentre();
        const float rad = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
        const bool rec = button.getToggleState();
        const bool blinkOn = ! rec || (juce::Time::getMillisecondCounter() / 500) % 2 == 0;
        const auto red = juce::Colour (0xffff3020);

        realism::softShadowEllipse (g, c, rad - 1.0f, rad * 0.35f, { 0.8f, 2.0f }, 0.55f);
        realism::spunMetal (g, c, rad, juce::Colour (0xffb8bcc2), 60.0f);                    // chrome bezel
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (juce::Rectangle<float> (rad * 2.0f, rad * 2.0f).withCentre (c), 0.8f);

        const float domeR = rad * 0.68f;
        if (rec && blinkOn)
        {
            juce::ColourGradient spill (red.withAlpha (0.6f), c, red.withAlpha (0.0f), c.translated (rad * 1.8f, 0.0f), true);
            g.setGradientFill (spill);
            g.fillEllipse (juce::Rectangle<float> (rad * 3.6f, rad * 3.6f).withCentre (c));
        }
        juce::ColourGradient dome (rec && blinkOn ? red.brighter (0.8f) : (highlighted ? red.darker (0.8f) : red.darker (1.4f)), c.translated (-domeR * 0.3f, -domeR * 0.3f),
                                   rec && blinkOn ? red.darker (0.2f) : red.darker (3.0f), c.translated (domeR, domeR), true);
        g.setGradientFill (dome);
        g.fillEllipse (juce::Rectangle<float> (domeR * 2.0f, domeR * 2.0f).withCentre (c.translated (0.0f, down ? 0.8f : 0.0f)));
        realism::specular (g, c, domeR, 0.55f);
        return;
    }

    if (style == "presetButton")
    {
        auto r = button.getLocalBounds().toFloat().reduced (1.0f);
        g.setGradientFill (realism::chrome (r));
        g.fillRoundedRectangle (r, 5.0f);
        const auto inner = r.reduced (1.5f);
        juce::ColourGradient body (juce::Colour (down ? 0xff111113 : (highlighted ? 0xff303036 : 0xff242428)), inner.getX(), inner.getY(),
                                   juce::Colour (0xff0a0a0b), inner.getX(), inner.getBottom(), false);
        g.setGradientFill (body);
        g.fillRoundedRectangle (inner, 4.0f);
        realism::bevel (g, inner, 4.0f, 1.0f, 0.22f, 0.5f);
        return;
    }

    if (style == "tunerButton")
    {
        auto r = button.getLocalBounds().toFloat().reduced (1.0f);
        const bool on = button.getToggleState();
        const float rad = r.getHeight() * 0.5f;

        g.setGradientFill (realism::chrome (r));
        g.fillRoundedRectangle (r, rad);
        const auto inner = r.reduced (2.0f);
        juce::ColourGradient body (juce::Colour (down ? 0xff111113 : (highlighted ? 0xff2e2e33 : 0xff232327)), inner.getX(), inner.getY(),
                                   juce::Colour (0xff0a0a0b), inner.getX(), inner.getBottom(), false);
        g.setGradientFill (body);
        g.fillRoundedRectangle (inner, rad - 2.0f);
        realism::bevel (g, inner, rad - 2.0f, 1.2f, 0.25f, 0.5f);

        // Status lamp
        const auto lc = juce::Point<float> (inner.getX() + rad, inner.getCentreY());
        const auto lamp = juce::Colour (0xff59f28a);
        if (on)
        {
            juce::ColourGradient spill (lamp.withAlpha (0.6f), lc, lamp.withAlpha (0.0f), lc.translated (14.0f, 0.0f), true);
            g.setGradientFill (spill);
            g.fillEllipse (juce::Rectangle<float> (28.0f, 28.0f).withCentre (lc));
        }
        juce::ColourGradient lens (on ? lamp.brighter (0.7f) : lamp.darker (3.0f), lc.translated (-1.5f, -1.5f),
                                   on ? lamp.darker (0.3f) : lamp.darker (4.5f), lc.translated (5.0f, 5.0f), true);
        g.setGradientFill (lens);
        g.fillEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre (lc));
        realism::specular (g, lc, 5.0f, 0.6f);
        return;
    }
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
