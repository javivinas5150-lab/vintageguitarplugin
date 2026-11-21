#include "TunerOverlay.h"
#include "JeminiLookAndFeel.h"
#include "Realism.h"

using LnF = JeminiLookAndFeel;

namespace
{
    constexpr int unitW = 660, unitH = 340;
    constexpr float sweep = 0.80f;                 // needle swing each side (radians)
    const juce::Colour ivoryHot   (0xfffff3d6);    // back-lit centre of the dial
    const juce::Colour ivoryEdge  (0xffd9c294);
    const juce::Colour dialInk    (0xff2b2118);
    const juce::Colour inTuneGreen (0xff59f28a);
    const juce::Colour lampRed    (0xffff4030);

    void setParam (VintageGuitarSuiteAudioProcessor& p, const char* id, float plainValue)
    {
        if (auto* prm = p.apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (plainValue));
    }
}

//==============================================================================
TunerOverlay::TunerOverlay (VintageGuitarSuiteAudioProcessor& p)
    : processor (p), unit (p)
{
    setInterceptsMouseClicks (true, true);
    addAndMakeVisible (unit);
}

void TunerOverlay::resized()
{
    unit.setBounds (juce::Rectangle<int> (unitW, unitH).withCentre (getLocalBounds().getCentre().translated (0, 10)));

    // Dimmed backdrop + the unit's soft shadow, rendered once
    backdrop = juce::Image (juce::Image::ARGB, juce::jmax (1, getWidth()), juce::jmax (1, getHeight()), true);
    juce::Graphics g (backdrop);
    g.fillAll (juce::Colours::black.withAlpha (0.55f));
    const auto c = unit.getBounds().getCentre().toFloat();
    juce::ColourGradient vignette (juce::Colours::transparentBlack, c.x, c.y,
                                   juce::Colours::black.withAlpha (0.45f), 0.0f, 0.0f, true);
    g.setGradientFill (vignette);
    g.fillAll();
    realism::dropShadow (g, unit.getBounds().toFloat(), 6.0f, 30, { 0, 16 }, 0.9f);
}

void TunerOverlay::paint (juce::Graphics& g)
{
    g.drawImageAt (backdrop, 0, 0);
}

void TunerOverlay::mouseDown (const juce::MouseEvent& e)
{
    if (! unit.getBounds().contains (e.getPosition()))
        setParam (processor, "tunerOn", 0.0f);
}

//==============================================================================
TunerOverlay::Unit::Unit (VintageGuitarSuiteAudioProcessor& p) : processor (p)
{
    setOpaque (true);

    for (auto* b : { &refDown, &refUp, &closeButton })
        addAndMakeVisible (*b);

    refDown.onClick = [this] { setParam (processor, "tunerRef", processor.apvts.getRawParameterValue ("tunerRef")->load() - 1.0f); repaint(); };
    refUp.onClick   = [this] { setParam (processor, "tunerRef", processor.apvts.getRawParameterValue ("tunerRef")->load() + 1.0f); repaint(); };
    closeButton.onClick = [this] { setParam (processor, "tunerOn", 0.0f); };
    closeButton.setTooltip ("Close the tuner (unmutes the output)");
}

void TunerOverlay::Unit::resized()
{
    meterArea = { 62, 52, 364, 226 };
    noteArea  = { 450, 58, 150, 118 };
    lampsArea = { 450, 188, 150, 30 };
    refArea   = { 450, 230, 150, 30 };

    refDown.setBounds (refArea.getX(), refArea.getY() + 2, 26, 26);
    refUp.setBounds (refArea.getRight() - 26, refArea.getY() + 2, 26, 26);
    closeButton.setBounds (noteArea.getRight() - 90, 290, 90, 30);

    staticLayer = {};
}

void TunerOverlay::Unit::visibilityChanged()
{
    if (isShowing()) startTimerHz (60);
    else             stopTimer();
}

float TunerOverlay::Unit::needleAngle (float cents) const
{
    return juce::jlimit (-1.0f, 1.0f, cents / 50.0f) * sweep;
}

void TunerOverlay::Unit::timerCallback()
{
    // Pitch analysis at ~30 Hz, needle physics at 60 Hz
    if ((++frame & 1) == 0)
    {
        const auto r = processor.readTuner();
        const auto now = juce::Time::getMillisecondCounterHiRes();

        if (r.valid)
        {
            shown = r;
            targetCents = r.cents;
            lastValidMs = now;
            hasSignal = true;
        }
        else if (now - lastValidMs > 450.0)
        {
            hasSignal = false;
            targetCents = -50.0f;            // needle drops back to rest
        }
    }

    // Damped spring: slightly under-damped like a real moving-coil meter
    constexpr float dt = 1.0f / 60.0f, stiffness = 170.0f, damping = 20.0f;
    needleVel += ((targetCents - needlePos) * stiffness - needleVel * damping) * dt;
    needlePos = juce::jlimit (-56.0f, 56.0f, needlePos + needleVel * dt);

    repaint();
}

//==============================================================================
void TunerOverlay::Unit::drawMeterFace (juce::Graphics& g, juce::Rectangle<float> win) const
{
    // Back-lit ivory dial
    juce::ColourGradient face (ivoryHot, win.getCentreX(), win.getBottom() - win.getHeight() * 0.25f,
                               ivoryEdge, win.getX(), win.getY(), true);
    g.setGradientFill (face);
    g.fillRect (win);

    const auto pivot = juce::Point<float> (win.getCentreX(), win.getBottom() + 46.0f);
    const float R = win.getHeight() + 6.0f;

    auto arc = [&] (float fromCents, float toCents, float radius, float width, juce::Colour c)
    {
        juce::Path p;
        p.addCentredArc (pivot.x, pivot.y, radius, radius, 0.0f, needleAngle (fromCents), needleAngle (toCents), true);
        g.setColour (c);
        g.strokePath (p, juce::PathStrokeType (width));
    };

    // Coloured zones: in-tune band and outer warning bands
    arc (-3.0f, 3.0f, R - 30.0f, 14.0f, juce::Colour (0xff3aa860).withAlpha (0.75f));
    arc (-50.0f, -25.0f, R - 30.0f, 5.0f, juce::Colour (0xffc03020).withAlpha (0.55f));
    arc (25.0f, 50.0f, R - 30.0f, 5.0f, juce::Colour (0xffc03020).withAlpha (0.55f));

    // Mirror strip (anti-parallax, like precision meters)
    {
        juce::Path mirror;
        mirror.addCentredArc (pivot.x, pivot.y, R - 62.0f, R - 62.0f, 0.0f, -sweep, sweep, true);
        juce::ColourGradient silverGrad (juce::Colour (0xffeef1f4), win.getX(), win.getY() + 40.0f,
                                         juce::Colour (0xff8d939b), win.getRight(), win.getY() + 90.0f, false);
        silverGrad.addColour (0.5, juce::Colour (0xffb9bfc6));
        g.setGradientFill (silverGrad);
        g.strokePath (mirror, juce::PathStrokeType (7.0f));
        g.setColour (dialInk.withAlpha (0.5f));
        juce::Path edge1, edge2;
        edge1.addCentredArc (pivot.x, pivot.y, R - 58.5f, R - 58.5f, 0.0f, -sweep, sweep, true);
        edge2.addCentredArc (pivot.x, pivot.y, R - 65.5f, R - 65.5f, 0.0f, -sweep, sweep, true);
        g.strokePath (edge1, juce::PathStrokeType (0.7f));
        g.strokePath (edge2, juce::PathStrokeType (0.7f));
    }

    // Main scale line + ticks
    arc (-50.0f, 50.0f, R - 38.0f, 1.4f, dialInk);
    for (int c = -50; c <= 50; c += 2)
    {
        const bool major = c % 10 == 0, mid = c % 10 == 5 || c % 10 == -5;
        const float a = needleAngle ((float) c);
        const float r0 = R - 38.0f, r1 = r0 + (major ? 16.0f : (mid ? 11.0f : 7.0f));
        g.setColour (dialInk);
        g.drawLine (juce::Line<float> (pivot.getPointOnCircumference (r0, a), pivot.getPointOnCircumference (r1, a)),
                    major ? 2.0f : 1.0f);

        if (major)
        {
            const auto lp = pivot.getPointOnCircumference (r1 + 12.0f, a);
            g.setFont (LnF::condensed (c == 0 ? 19.0f : 15.0f));
            g.drawText (c == 0 ? juce::String ("0") : (c > 0 ? "+" : "") + juce::String (c),
                        juce::Rectangle<float> (40.0f, 18.0f).withCentre (lp), juce::Justification::centred, false);
        }
    }

    // Legends
    g.setColour (dialInk);
    g.setFont (LnF::logo (26.0f));
    g.drawText (juce::String ("b"), juce::Rectangle<float> (win.getX() + 16.0f, win.getY() + 12.0f, 30.0f, 30.0f), juce::Justification::centred, false);
    g.drawText (juce::String ("#"), juce::Rectangle<float> (win.getRight() - 46.0f, win.getY() + 12.0f, 30.0f, 30.0f), juce::Justification::centred, false);
    g.setFont (LnF::condensed (14.0f));
    g.drawText ("CENTS", juce::Rectangle<float> (win.getCentreX() - 40.0f, win.getBottom() - 92.0f, 80.0f, 16.0f), juce::Justification::centred, false);
    g.setColour (dialInk.withAlpha (0.75f));
    LnF::drawSlantedLogo (g, "JEMINI", juce::Rectangle<float> (win.getCentreX() - 60.0f, win.getBottom() - 74.0f, 120.0f, 22.0f), 17.0f, juce::Justification::centred);

    // Needle housing across the bottom (hides the pivot)
    const auto housing = win.withTop (win.getBottom() - 38.0f);
    juce::ColourGradient hg (juce::Colour (0xff2a2a2e), housing.getX(), housing.getY(),
                             juce::Colour (0xff0b0b0c), housing.getX(), housing.getBottom(), false);
    g.setGradientFill (hg);
    g.fillRect (housing);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawHorizontalLine ((int) housing.getY(), housing.getX(), housing.getRight());
    g.setColour (juce::Colour (0xffb9b4a6));
    g.setFont (LnF::condensed (13.0f));
    g.drawText ("CHROMATIC  TUNER   JT-1", housing, juce::Justification::centred, false);
}

void TunerOverlay::Unit::buildStaticLayer()
{
    constexpr float k = 2.0f;   // render at 2x so it stays crisp when the window is enlarged
    staticLayer = juce::Image (juce::Image::RGB, (int) (unitW * k), (int) (unitH * k), true);
    juce::Graphics g (staticLayer);
    g.addTransform (juce::AffineTransform::scale (k));

    const auto all = juce::Rectangle<float> ((float) unitW, (float) unitH);
    g.fillAll (juce::Colour (0xff070707));

    // Walnut cheeks
    for (auto cheek : { all.withWidth (34.0f), all.withLeft (all.getRight() - 34.0f) })
    {
        juce::ColourGradient wood (juce::Colour (0xff6a4024), cheek.getX(), cheek.getY(),
                                   juce::Colour (0xff2a160a), cheek.getRight(), cheek.getBottom(), false);
        g.setGradientFill (wood);
        g.fillRoundedRectangle (cheek, 4.0f);
        juce::Random rnd ((juce::int64) cheek.getX() + 7);
        for (int i = 0; i < 26; ++i)
        {
            const float x = cheek.getX() + rnd.nextFloat() * cheek.getWidth();
            juce::Path grain;
            grain.startNewSubPath (x, cheek.getY());
            for (float y = 0.0f; y <= cheek.getBottom(); y += 40.0f)
                grain.lineTo (x + std::sin (y * 0.02f + (float) i) * 2.5f, y);
            g.setColour ((rnd.nextBool() ? juce::Colours::black : juce::Colour (0xffe0a060)).withAlpha (0.06f + rnd.nextFloat() * 0.08f));
            g.strokePath (grain, juce::PathStrokeType (0.7f + rnd.nextFloat()));
        }
        realism::bevel (g, cheek, 4.0f, 2.0f, 0.25f, 0.6f);
    }

    // Brushed black face
    const auto face = all.reduced (34.0f, 0.0f);
    const auto brushed = realism::makeBrushed (512, 256, juce::Colour (0xff17181b), 5.0f, 61);
    g.setFillType (juce::FillType (brushed, juce::AffineTransform::translation (face.getX(), 0.0f)));
    g.fillRect (face);
    juce::ColourGradient light (juce::Colours::white.withAlpha (0.07f), face.getX(), face.getY(),
                                juce::Colours::black.withAlpha (0.35f), face.getX(), face.getBottom(), false);
    g.setGradientFill (light);
    g.fillRect (face);
    realism::bevel (g, face, 0.0f, 2.0f, 0.3f, 0.7f);

    // Header
    g.setColour (LnF::cream);
    LnF::drawSlantedLogo (g, "JEMINI", { 60.0f, 14.0f, 140.0f, 28.0f }, 22.0f, juce::Justification::centredLeft);
    g.setColour (LnF::silver);
    g.setFont (LnF::condensed (15.0f));
    g.drawText ("CHROMATIC TUNER", juce::Rectangle<float> (185.0f, 16.0f, 240.0f, 24.0f), juce::Justification::centredLeft, false);

    // Corner screws
    for (auto p : { juce::Point<float> (48, 16), juce::Point<float> ((float) unitW - 48, 16),
                    juce::Point<float> (48, (float) unitH - 16), juce::Point<float> ((float) unitW - 48, (float) unitH - 16) })
    {
        realism::softShadowEllipse (g, p, 5.0f, 2.5f, { 0.6f, 1.4f }, 0.4f);
        realism::spunMetal (g, p, 5.0f, juce::Colour (0xffb0b4ba), 50.0f);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawLine (p.x - 3.0f, p.y, p.x + 3.0f, p.y, 1.2f);
        g.drawLine (p.x, p.y - 3.0f, p.x, p.y + 3.0f, 1.2f);
    }

    // Meter: chrome bezel + recessed window + dial
    const auto win = meterArea.toFloat();
    const auto bezel = win.expanded (9.0f);
    realism::dropShadow (g, bezel, 12.0f, 8, { 0, 4 }, 0.8f);
    juce::ColourGradient chromeGrad = realism::chrome (bezel);
    g.setGradientFill (chromeGrad);
    g.fillRoundedRectangle (bezel, 12.0f);
    realism::bevel (g, bezel, 12.0f, 2.0f, 0.6f, 0.6f);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (win.expanded (2.0f), 6.0f);
    {
        juce::Graphics::ScopedSaveState s (g);
        juce::Path clip;
        clip.addRoundedRectangle (win, 5.0f);
        g.reduceClipRegion (clip);
        drawMeterFace (g, win);
    }

    // Note window: smoked glass
    const auto note = noteArea.toFloat();
    g.setColour (juce::Colour (0xff050505));
    g.fillRoundedRectangle (note.expanded (4.0f), 8.0f);
    realism::bevel (g, note.expanded (4.0f), 8.0f, 2.0f, 0.0f, 0.0f);
    g.setColour (juce::Colours::white.withAlpha (0.16f));
    g.drawRoundedRectangle (note.expanded (4.0f).translated (0.0f, 1.0f), 8.0f, 0.8f);
    juce::ColourGradient smoke (juce::Colour (0xff141210), note.getX(), note.getY(),
                                juce::Colour (0xff050404), note.getX(), note.getBottom(), false);
    g.setGradientFill (smoke);
    g.fillRoundedRectangle (note, 5.0f);

    // Lamp bezels
    const auto lamps = lampsArea.toFloat();
    for (int i = 0; i < 3; ++i)
    {
        const auto c = juce::Point<float> (lamps.getX() + lamps.getWidth() * (0.18f + 0.32f * (float) i), lamps.getCentreY());
        realism::softShadowEllipse (g, c, 12.0f, 4.0f, { 0.8f, 2.0f }, 0.5f);
        realism::spunMetal (g, c, 13.0f, juce::Colour (0xffaeb2b8), 60.0f);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawEllipse (juce::Rectangle<float> (26.0f, 26.0f).withCentre (c), 0.8f);
    }
    g.setColour (LnF::silver);
    g.setFont (LnF::condensed (12.0f));
    g.drawText ("FLAT", juce::Rectangle<float> (lamps.getX() - 8.0f, lamps.getBottom() + 2.0f, 70.0f, 14.0f).withCentre ({ lamps.getX() + lamps.getWidth() * 0.18f, lamps.getBottom() + 9.0f }), juce::Justification::centred, false);
    g.drawText ("IN TUNE", juce::Rectangle<float> (70.0f, 14.0f).withCentre ({ lamps.getX() + lamps.getWidth() * 0.50f, lamps.getBottom() + 9.0f }), juce::Justification::centred, false);
    g.drawText ("SHARP", juce::Rectangle<float> (70.0f, 14.0f).withCentre ({ lamps.getX() + lamps.getWidth() * 0.82f, lamps.getBottom() + 9.0f }), juce::Justification::centred, false);

    // Reference window
    const auto ref = refArea.toFloat().reduced (30.0f, 2.0f);
    g.setColour (juce::Colour (0xff050505));
    g.fillRoundedRectangle (ref, 4.0f);
    realism::innerShadow (g, ref, 4.0f, 4.0f, 0.9f);

    // Mute legend
    g.setColour (LnF::silver.withAlpha (0.85f));
    g.setFont (LnF::condensed (14.0f));
    g.drawText ("OUTPUT MUTED WHILE TUNING", juce::Rectangle<float> (meterArea.getX() + 28.0f, 292.0f, 300.0f, 26.0f), juce::Justification::centredLeft, false);
}

void TunerOverlay::Unit::paint (juce::Graphics& g)
{
    if (! staticLayer.isValid())
        buildStaticLayer();

    g.drawImageTransformed (staticLayer, juce::AffineTransform::scale (0.5f));

    const bool inTune = hasSignal && std::abs (shown.cents) <= 2.0f;
    const auto win = meterArea.toFloat();

    // --- Needle (with shadow on the dial), clipped to the window ---
    {
        juce::Graphics::ScopedSaveState s (g);
        juce::Path clip;
        clip.addRoundedRectangle (win, 5.0f);
        g.reduceClipRegion (clip);
        g.excludeClipRegion (win.withTop (win.getBottom() - 38.0f).toNearestInt());   // housing covers the root

        const auto pivot = juce::Point<float> (win.getCentreX(), win.getBottom() + 46.0f);
        const float a = needleAngle (needlePos);
        const float R = win.getHeight() + 6.0f;
        const auto tip = pivot.getPointOnCircumference (R - 20.0f, a);

        g.setColour (juce::Colours::black.withAlpha (0.18f));
        g.drawLine (juce::Line<float> (pivot.translated (5.0f, 6.0f), tip.translated (5.0f, 6.0f)), 3.2f);
        g.setColour (juce::Colour (0xff141210));
        g.drawLine (juce::Line<float> (pivot, tip), 2.2f);
        g.setColour (juce::Colour (0xffd8241a));
        g.drawLine (juce::Line<float> (pivot.getPointOnCircumference (R - 64.0f, a), tip), 2.4f);

        // Warm lamp glow when in tune
        if (inTune)
        {
            juce::ColourGradient glow (inTuneGreen.withAlpha (0.18f), win.getCentreX(), win.getBottom() - 40.0f,
                                       inTuneGreen.withAlpha (0.0f), win.getCentreX(), win.getY(), true);
            g.setGradientFill (glow);
            g.fillRect (win);
        }
    }

    // Glass over the meter
    realism::sheen (g, win, 5.0f, 0.22f);
    realism::innerShadow (g, win, 5.0f, 10.0f, 0.8f);

    // --- Note window ---
    const auto note = noteArea.toFloat();
    const auto colour = ! hasSignal ? accent.withAlpha (0.10f) : (inTune ? inTuneGreen : accent);
    const auto name = hasSignal ? Tuner::noteName (shown.midiNote) : juce::String ("-");
    const auto noteBox = note.withTrimmedBottom (34.0f).withTrimmedRight (30.0f);

    g.setFont (LnF::logo (70.0f));
    if (hasSignal)
    {
        for (auto [dx, dy] : { std::pair { -2.0f, 0.0f }, std::pair { 2.0f, 0.0f }, std::pair { 0.0f, -2.0f }, std::pair { 0.0f, 2.0f } })
        {
            g.setColour (colour.withAlpha (0.18f));
            g.drawText (name.substring (0, 1), noteBox.translated (dx, dy), juce::Justification::centred, false);
        }
    }
    g.setColour (colour);
    g.drawText (name.substring (0, 1), noteBox, juce::Justification::centred, false);

    if (hasSignal)
    {
        g.setFont (LnF::logo (30.0f));
        if (name.length() > 1)
            g.drawText (juce::String ("#"), juce::Rectangle<float> (note.getRight() - 46.0f, note.getY() + 10.0f, 34.0f, 34.0f), juce::Justification::centred, false);
        g.setFont (LnF::display (22.0f));
        g.drawText (juce::String (shown.midiNote / 12 - 1), juce::Rectangle<float> (note.getRight() - 42.0f, note.getY() + 50.0f, 30.0f, 30.0f), juce::Justification::centred, false);

        g.setColour (colour.withAlpha (0.85f));
        g.setFont (LnF::display (18.0f));
        g.drawText (juce::String (shown.frequency, 2) + " Hz", note.withTop (note.getBottom() - 32.0f), juce::Justification::centred, false);
    }
    realism::sheen (g, note, 5.0f, 0.10f);

    // --- Lamps ---
    const auto lamps = lampsArea.toFloat();
    const std::array<bool, 3> lit { hasSignal && shown.cents < -2.0f, inTune, hasSignal && shown.cents > 2.0f };
    const std::array<juce::Colour, 3> lampColours { lampRed, inTuneGreen, lampRed };
    for (size_t i = 0; i < 3; ++i)
    {
        const auto c = juce::Point<float> (lamps.getX() + lamps.getWidth() * (0.18f + 0.32f * (float) i), lamps.getCentreY());
        const auto col = lampColours[i];
        if (lit[i])
        {
            juce::ColourGradient spill (col.withAlpha (0.55f), c, col.withAlpha (0.0f), c.translated (30.0f, 0.0f), true);
            g.setGradientFill (spill);
            g.fillEllipse (juce::Rectangle<float> (60.0f, 60.0f).withCentre (c));
        }
        juce::ColourGradient lens (lit[i] ? col.brighter (0.8f) : col.darker (2.6f), c.translated (-2.0f, -2.0f),
                                   lit[i] ? col.darker (0.2f) : col.darker (4.0f), c.translated (9.0f, 9.0f), true);
        g.setGradientFill (lens);
        g.fillEllipse (juce::Rectangle<float> (18.0f, 18.0f).withCentre (c));
        realism::specular (g, c, 9.0f, 0.6f);
    }

    // --- Reference ---
    const auto ref = refArea.toFloat().reduced (30.0f, 2.0f);
    g.setColour (accent);
    g.setFont (LnF::display (17.0f));
    g.drawText ("A4 " + juce::String (juce::roundToInt (processor.apvts.getRawParameterValue ("tunerRef")->load())) + " Hz",
                ref, juce::Justification::centred, false);
}
