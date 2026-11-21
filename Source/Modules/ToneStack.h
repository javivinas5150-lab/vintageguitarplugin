#pragma once

#include <JuceHeader.h>

//==============================================================================
// ToneStack - Bass / Middle / Treble EQ placed after the NAM amp.
// Same centre frequencies as the official NAM plugin:
//   Bass   = low shelf  @ 150 Hz
//   Middle = peak       @ 425 Hz
//   Treble = high shelf @ 1800 Hz
// Mono. Real-time safe: coefficients are only recalculated when a knob moves,
// and without allocating memory.
//==============================================================================
class ToneStack
{
public:
    void prepare (double sampleRate);
    void reset();

    // Audio thread. Gains in dB (0 dB = flat).
    void setGains (float bassDb, float middleDb, float trebleDb);

    void process (float* data, int numSamples);

private:
    void updateCoefficients();

    juce::dsp::IIR::Filter<float> bass, middle, treble;

    double sampleRate = 48000.0;
    float bassGainDb = 0.0f, middleGainDb = 0.0f, trebleGainDb = 0.0f;
    bool needsUpdate = true;
};
