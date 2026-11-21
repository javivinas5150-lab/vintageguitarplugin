#pragma once

#include <JuceHeader.h>

//==============================================================================
// GoldDrive - Klon Centaur style overdrive.
//
// What makes a Klon a Klon (and what this models):
//  - The signal is split into a CLEAN path and a DRIVE path, then mixed.
//  - The GAIN knob is a dual pot: turning it up raises the drive AND lowers
//    the clean path. At low gain it's mostly a transparent boost; at high gain
//    the clipped path takes over.
//  - The drive path has a bass cut and a mid hump before soft (germanium
//    diode) clipping, then a gentle top roll-off.
//  - TREBLE is an active shelf after the mix; OUTPUT has lots of clean boost.
//
// Runs at 4x oversampling to keep the clipping free of aliasing.
// Latency is a small whole number of samples (reported to the host).
// Mono, real-time safe.
//==============================================================================
class GoldDrive
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    int getLatencySamples() const;

    // Audio thread. Knobs are 0..10 like the real pedal.
    void setParameters (float gain0to10, float treble0to10, float output0to10);

    // Always call (even when off) so latency stays constant; 'enabled' false = clean pass.
    void process (float* data, int numSamples, bool enabled);

private:
    void updateFilters();

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    double osRate = 192000.0;

    juce::dsp::IIR::Filter<float> driveHighPass, driveMidBoost, driveLowPass, trebleShelf;

    float gainKnob = 3.0f, trebleKnob = 5.0f, outputKnob = 5.0f;
    bool filtersNeedUpdate = true;

    // Derived from the knobs
    float driveGain = 1.0f, cleanAmount = 1.0f, driveAmount = 0.2f, outputGain = 1.0f;
};
