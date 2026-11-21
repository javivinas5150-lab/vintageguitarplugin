#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

//==============================================================================
// IRVerb - convolution reverb / delay from an impulse response file.
//  - Mono or stereo IRs, up to 10 seconds (rooms, halls, plates, springs,
//    captured tape echoes...)
//  - Zero latency, non-uniform partitioned convolution (efficient for long IRs)
//  - The IR's own timing is kept (no trimming), so captured pre-delays and
//    echo spacing stay exact.
//  - MIX (wet amount, dry stays 100%), PRE-DELAY, LOW CUT, HIGH CUT, LEVEL
//
// Threading: like CabIR - the file is read on the message thread and handed to
// the audio thread without locks that could block it.
//==============================================================================
class IRVerb
{
public:
    IRVerb();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Message thread. Returns "" on success, or an error message.
    juce::String loadIR (const juce::File& file);
    void clearIR();

    struct Params
    {
        float mix = 0.3f, preDelayMs = 0.0f, lowCutHz = 20.0f, highCutHz = 20000.0f, levelDb = 0.0f;
    };

    // Audio thread
    void setParameters (const Params& p);
    void process (float* left, float* right, int numSamples);

    // GUI info
    bool hasIR() const                  { return irLoaded.load(); }
    juce::String getIRName() const      { return irFile.getFileNameWithoutExtension(); }
    double getIRLengthSeconds() const   { return irLengthSeconds.load(); }

    static juce::String getSupportedWildcard() { return "*.wav;*.aif;*.aiff;*.flac"; }
    static constexpr double maxIRSeconds = 10.0;

private:
    void updateFilters();

    // Head of 512 samples: zero latency, much less CPU for long IRs
    juce::dsp::Convolution convolution { juce::dsp::Convolution::NonUniform { 512 } };

    juce::AudioBuffer<float> wet;
    std::vector<float> preDelayL, preDelayR;
    int preDelayPos = 0;

    std::array<juce::dsp::IIR::Filter<float>, 2> lowCut, highCut;
    bool lowCutActive = false, highCutActive = false;
    bool filtersNeedUpdate = true;

    // Hand-over from message thread to audio thread
    juce::SpinLock pendingLock;
    juce::AudioBuffer<float> pendingIR;
    double pendingSampleRate = 0.0;
    bool hasPendingIR = false;

    std::atomic<bool> irLoaded { false };
    std::atomic<double> irLengthSeconds { 0.0 };
    juce::File irFile;

    double sampleRate = 48000.0;
    Params params;
    juce::SmoothedValue<float> smoothedWetGain;
};
