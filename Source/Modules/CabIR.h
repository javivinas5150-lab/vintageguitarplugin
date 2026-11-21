#pragma once

#include <JuceHeader.h>

//==============================================================================
// CabIR - cabinet impulse response loader + Low Cut / High Cut filters.
//
// Threading:
//  - loadIR() runs on the MESSAGE thread: it reads the .wav into memory and
//    hands the buffer over to the audio thread (no file access on audio thread).
//  - process() runs on the AUDIO thread. It picks up a new IR without waiting
//    (JUCE's Convolution prepares it in the background, then crossfades).
//  - Zero latency (JUCE Convolution default).
//==============================================================================
class CabIR
{
public:
    CabIR();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Message thread. Returns "" on success, or an error message.
    juce::String loadIR (const juce::File& file);
    void clearIR();

    // Audio thread
    void setFilters (float lowCutHz, float highCutHz);
    void process (float* data, int numSamples);   // mono, in place

    // GUI info
    bool hasIR() const                  { return irLoaded.load(); }
    juce::String getIRName() const      { return irFile.getFileNameWithoutExtension(); }
    juce::File getIRFile() const        { return irFile; }

    static juce::String getSupportedWildcard() { return "*.wav;*.aif;*.aiff;*.flac"; }

private:
    juce::dsp::Convolution convolution;
    juce::dsp::IIR::Filter<float> lowCut, highCut;

    // Hand-over from message thread to audio thread
    juce::SpinLock pendingLock;
    juce::AudioBuffer<float> pendingIR;
    double pendingSampleRate = 0.0;
    bool hasPendingIR = false;

    std::atomic<bool> irLoaded { false };
    juce::File irFile;

    double sampleRate = 48000.0;
    float lowCutHz = 20.0f, highCutHz = 20000.0f;
    bool lowCutActive = false, highCutActive = false;
    bool filtersNeedUpdate = true;

    static constexpr double maxIRSeconds = 2.0; // longer files are cut (cab IRs are ~20-500 ms)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CabIR)
};
