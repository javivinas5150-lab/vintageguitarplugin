#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

//==============================================================================
// LexReverb - lush, modulated hall/plate reverb in the spirit of Lexicon PCM.
//
// Structure:
//   PRE-DELAY -> 4 input diffusers (all-pass) per side
//             -> 8-line Feedback Delay Network (Householder matrix)
//                · each line slowly modulated (the Lexicon "shimmer/chorus" tail)
//                · each line damped by a low-pass (TONE)
//                · feedback gains set so the tail lasts DECAY seconds (RT60)
//             -> stereo taps -> MIX
// Real-time safe: all memory is allocated in prepare().
//==============================================================================
class LexReverb
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    struct Params
    {
        float decaySec = 2.5f, size = 0.6f, preDelayMs = 20.0f,
              toneHz = 6000.0f, mod = 0.4f, mix = 0.2f;
    };

    // Audio thread
    void setParameters (const Params& p);
    void process (float* left, float* right, int numSamples);

private:
    static constexpr int numLines = 8;
    static constexpr int numDiffusers = 4;

    struct Line
    {
        std::vector<float> buffer;
        int writePos = 0;
        double baseLength = 0.0;   // samples (depends on SIZE)
        float gain = 0.0f;         // feedback gain for RT60
        float damp = 0.0f;         // low-pass state
        double lfoPhase = 0.0, lfoInc = 0.0;
    };

    struct AllPass
    {
        std::vector<float> buffer;
        int pos = 0;
        float process (float x, float g)
        {
            const float delayed = buffer[(size_t) pos];
            const float y = -g * x + delayed;
            buffer[(size_t) pos] = x + g * y;
            if (++pos >= (int) buffer.size())
                pos = 0;
            return y;
        }
    };

    void updateLines();

    std::array<Line, numLines> lines;
    std::array<AllPass, numDiffusers> diffL, diffR;

    std::vector<float> preDelayL, preDelayR;
    int preDelayPos = 0;

    double sampleRate = 48000.0;
    Params params;
    bool needsUpdate = true;

    float dampCoeff = 0.0f;
    double modDepthSamples = 0.0;
    int preDelaySamples = 0;
    juce::SmoothedValue<float> smoothedMix;

    static constexpr double maxSizeScale = 1.6;
    static constexpr double maxModMs     = 1.2;
};
