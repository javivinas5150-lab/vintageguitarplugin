#pragma once

#include <JuceHeader.h>
#include <vector>

//==============================================================================
// StereoDelay - studio/rack style digital delay.
//  - TIME      20..2000 ms (changes glide smoothly, like a tape delay)
//  - FEEDBACK  0..95 %
//  - TONE      high cut inside the feedback loop: each repeat gets darker
//  - MOD       gentle pitch wobble on the repeats (chorused, lush)
//  - MIX       dry/wet
//  - PING-PONG repeats bounce left <-> right
// Real-time safe: all memory is allocated in prepare().
//==============================================================================
class StereoDelay
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    struct Params
    {
        float timeMs = 380.0f, feedback = 0.35f, toneHz = 6000.0f,
              mod = 0.2f, mix = 0.25f;
        bool pingPong = false;
    };

    // Audio thread
    void setParameters (const Params& p);
    void process (float* left, float* right, int numSamples);

private:
    float readInterpolated (const std::vector<float>& line, double delaySamples) const;

    std::vector<float> lineL, lineR;
    int writePos = 0;
    int lineSize = 0;

    double sampleRate = 48000.0;
    Params params;

    juce::SmoothedValue<float> smoothedTime;   // in samples
    juce::SmoothedValue<float> smoothedMix;
    float toneCoeff = 0.0f, toneStateL = 0.0f, toneStateR = 0.0f;
    double lfoPhase = 0.0;

    static constexpr double maxDelayMs = 2000.0;
    static constexpr double maxModMs   = 4.0;
};
