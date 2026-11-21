#include "Recorder.h"

Recorder::Recorder() = default;   // the writer thread only starts when recording

Recorder::~Recorder()
{
    stop();
    writerThread.stopThread (2000);
}

juce::File Recorder::getRecordingsFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("Jemini Guitar Rig")
               .getChildFile ("Recordings");
}

juce::File Recorder::makeNewFileName()
{
    const auto stamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    return getRecordingsFolder().getNonexistentChildFile ("Jemini_" + stamp, ".wav", false);
}

bool Recorder::start (const juce::File& file)
{
    stop();

    const double sr = rate.load();
    if (sr <= 0.0)
        return false;                       // audio not running yet

    if (! writerThread.isThreadRunning())
        writerThread.startThread();

    file.getParentDirectory().createDirectory();
    file.deleteFile();

    std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream> (file);
    if (static_cast<juce::FileOutputStream*> (stream.get())->failedToOpen())
        return false;

    juce::WavAudioFormat wav;
    auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions{}
                                                   .withSampleRate (sr)
                                                   .withNumChannels (2)
                                                   .withBitsPerSample (24));
    if (writer == nullptr)
        return false;

    // ~1.5 s of buffering between the audio thread and the disk
    threaded = std::make_unique<juce::AudioFormatWriter::ThreadedWriter> (writer.release(), writerThread,
                                                                          (int) (sr * 1.5));
    currentFile = file;
    samplesWritten.store (0);

    const juce::ScopedLock sl (writerLock);
    activeWriter.store (threaded.get());
    return true;
}

void Recorder::stop()
{
    {
        const juce::ScopedLock sl (writerLock);
        activeWriter.store (nullptr);
    }

    threaded.reset();   // flushes the remaining audio and closes the file
}

double Recorder::getSeconds() const
{
    const double sr = rate.load();
    return sr > 0.0 ? (double) samplesWritten.load() / sr : 0.0;
}

void Recorder::process (const juce::AudioBuffer<float>& buffer, int numSamples) noexcept
{
    if (activeWriter.load() == nullptr)
        return;

    // Never wait on the audio thread: if stop() holds the lock, skip this block
    const juce::ScopedTryLock sl (writerLock);
    if (! sl.isLocked())
        return;

    if (auto* w = activeWriter.load())
    {
        const float* channels[2] = { buffer.getReadPointer (0),
                                     buffer.getReadPointer (buffer.getNumChannels() > 1 ? 1 : 0) };
        if (w->write (channels, numSamples))
            samplesWritten.fetch_add (numSamples);
    }
}
