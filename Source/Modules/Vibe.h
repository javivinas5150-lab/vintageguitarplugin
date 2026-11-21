#pragma once

#include <JuceHeader.h>
#include <array>

//==============================================================================
// Vibe - Uni-Vibe style phase shifter.
//
// How the original works (and what this models):
//  - 4 phase-shift stages, each with a DIFFERENT capacitor (15 nF, 220 nF,
//    470 pF, 4.7 nF). The stages are spread over the spectrum, which is why
//    a Uni-Vibe sounds "throbby" and not like a regular phaser.
//  - All stages are controlled by light-dependent resistors (LDRs) lit by a
//    lamp. The lamp follows the LFO slowly and the LDR response is
//    exponential - that gives the vibe its lopsided, liquid sweep.
//  - CHORUS mode mixes the dry signal with the shifted one (notches).
//    VIBRATO mode outputs only the shifted signal (pitch wobble).
// Mono, real-time safe.
//==============================================================================
class Vibe
{
public:
    void prepare (double sampleRate);
    void reset();

    // Audio thread
    void setParameters (float speedHz, float intensity01, bool vibratoMode);
    void process (float* data, int numSamples);

private:
    static constexpr int numStages = 4;

    // Uni-Vibe stage capacitors (farads)
    static constexpr std::array<double, numStages> stageCaps { 15e-9, 220e-9, 470e-12, 4.7e-9 };

    // LDR resistance range: bright lamp -> low R, dark -> high R (ohms)
    static constexpr double rBright = 2.0e3;
    static constexpr double rDark   = 50.0e3;

    double sampleRate = 48000.0;
    double phase = 0.0;          // LFO phase 0..1
    float lamp = 0.5f;           // lamp brightness (slow follower of the LFO)
    float lampCoeff = 0.0f;

    std::array<float, numStages> state {};

    float speed = 3.0f, intensity = 0.7f;
    bool vibrato = false;
};
