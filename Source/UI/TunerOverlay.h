#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"

//==============================================================================
// TunerOverlay - dims the rig and shows a vintage analog tuner unit:
// back-lit ivory meter with a damped needle, glowing note window, flat /
// in-tune / sharp lamps and the A4 reference. Visible while "tunerOn" is set.
//==============================================================================
class TunerOverlay : public juce::Component
{
public:
    explicit TunerOverlay (VintageGuitarSuiteAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;   // click outside the unit = close
    void visibilityChanged() override;                    // starts / stops the meter

    void setAccent (juce::Colour displayColour) { unit.accent = displayColour; unit.invalidateCache(); }

private:
    //==========================================================================
    class Unit : public juce::Component, public juce::Timer
    {
    public:
        explicit Unit (VintageGuitarSuiteAudioProcessor&);
        ~Unit() override { stopTimer(); }

        void paint (juce::Graphics&) override;
        void resized() override;
        void setRunning (bool shouldRun);
        void invalidateCache() { staticLayer = {}; repaint(); }

        juce::Colour accent { 0xffff3a2a };

    private:
        void timerCallback() override;
        void buildStaticLayer();
        void drawMeterFace (juce::Graphics&, juce::Rectangle<float>) const;
        float needleAngle (float cents) const;

        VintageGuitarSuiteAudioProcessor& processor;

        juce::Rectangle<int> meterArea, noteArea, lampsArea, refArea;
        juce::TextButton refDown { "-" }, refUp { "+" }, closeButton { "CLOSE" };
        juce::Image staticLayer;
        float staticScale = 2.0f;

        // needle ballistics (in cents, -50..+50; rests at -50)
        float needlePos = -50.0f, needleVel = 0.0f, targetCents = -50.0f;
        Tuner::Reading shown;
        double lastValidMs = 0.0;
        int frame = 0;
        bool hasSignal = false;
    };

    VintageGuitarSuiteAudioProcessor& processor;
    Unit unit;
    juce::Image backdrop;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TunerOverlay)
};
