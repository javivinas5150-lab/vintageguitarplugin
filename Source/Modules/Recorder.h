#pragma once

#include <JuceHeader.h>
#include <atomic>

//==============================================================================
// Recorder - writes the processed output to a 24-bit stereo WAV file.
//
// The audio thread only hands blocks to a ThreadedWriter (a FIFO emptied by a
// background thread), so disk access never blocks the audio. start() / stop()
// are called from the message thread.
//==============================================================================
class Recorder
{
public:
    Recorder();
    ~Recorder();

    void prepare (double sampleRate) { rate.store (sampleRate); }

    // Message thread
    bool start (const juce::File& file);
    void stop();
    bool isRecording() const          { return activeWriter.load() != nullptr; }
    double getSeconds() const;
    juce::File getFile() const        { return currentFile; }

    // Audio thread
    void process (const juce::AudioBuffer<float>& buffer, int numSamples) noexcept;

    // Default place for recordings: Documents/Jemini Guitar Rig/Recordings
    static juce::File getRecordingsFolder();
    static juce::File makeNewFileName();

private:
    juce::TimeSliceThread writerThread { "Jemini Recorder" };
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threaded;
    juce::CriticalSection writerLock;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };
    std::atomic<juce::int64> samplesWritten { 0 };
    std::atomic<double> rate { 0.0 };
    juce::File currentFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Recorder)
};
