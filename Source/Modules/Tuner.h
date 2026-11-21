#pragma once

#include <JuceHeader.h>
#include <vector>
#include <atomic>

//==============================================================================
// Chromatic tuner.
//
//  - The AUDIO thread pushes the clean guitar signal into a lock-free ring
//    buffer (push()).
//  - The GUI thread calls analyse() ~25 times per second: it copies the most
//    recent samples, decimates them and runs the YIN pitch detector.
//    A torn read while the audio thread writes only affects a few samples at
//    the very end of the window, which YIN tolerates.
//==============================================================================
class Tuner
{
public:
    struct Reading
    {
        bool  valid = false;      // a stable pitch was found
        float frequency = 0.0f;   // Hz
        int   midiNote = 0;       // nearest note (69 = A4)
        float cents = 0.0f;       // -50 .. +50 from that note
        float level = 0.0f;       // input RMS (linear)
    };

    Tuner() = default;

    void prepare (double sampleRate);

    // Audio thread
    void push (const float* data, int numSamples) noexcept;

    // GUI thread
    Reading analyse (float referenceA4);

    static juce::String noteName (int midiNote);   // "C", "C#", ...

private:
    static constexpr int ringSize = 1 << 15;       // ~0.7 s at 48 kHz
    std::vector<float> ring = std::vector<float> ((size_t) ringSize, 0.0f);
    std::atomic<int> writePos { 0 };
    std::atomic<double> rate { 48000.0 };

    // analysis scratch (GUI thread only)
    std::vector<float> window, decimated, diff;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Tuner)
};
