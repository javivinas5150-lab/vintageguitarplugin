#pragma once

#include <JuceHeader.h>
#include <vector>

//==============================================================================
// NoiseGate - "4-cable" style gate:
//   1) analyse()  looks at the CLEAN guitar signal (before the amp) and
//                 works out a gain curve
//   2) apply()    applies that gain curve AFTER the amp, so the amp's own
//                 hiss is silenced too, while long saturated notes are
//                 judged by the guitar, not by the compressed amp signal.
//
// Uses hysteresis (opens at threshold, closes a few dB below) and a short
// hold time so notes don't chatter when they decay around the threshold.
//==============================================================================
class NoiseGate
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Audio thread
    void setParameters (float thresholdDb, float releaseMs);
    void analyse (const float* input, int numSamples);  // before the amp
    void apply (float* data, int numSamples) const;     // after the amp (call per channel)

    // For the GUI LED: true while the gate is closed / closing
    bool isGating() const { return gating.load(); }

private:
    std::vector<float> gains;
    int lastNumSamples = 0;

    double sampleRate = 48000.0;

    float thresholdDb = -60.0f;
    float envelope = 0.0f;
    float gain = 1.0f;
    bool open = true;
    int holdCounter = 0;

    // Coefficients
    float envAttack = 0.0f, envRelease = 0.0f;   // level detector
    float gateAttack = 0.0f, gateRelease = 0.0f; // gain smoothing
    int holdSamples = 0;
    float currentReleaseMs = -1.0f;

    std::atomic<bool> gating { false };

    static constexpr float hysteresisDb = 4.0f;
    static constexpr float holdMs       = 20.0f;
    static constexpr float openMs       = 0.5f;   // how fast it opens
};
