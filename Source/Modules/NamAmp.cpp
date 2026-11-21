#include "NamAmp.h"

#include "NAM/dsp.h"
#include "NAM/get_dsp.h"

#include <filesystem>

namespace
{
    // NAM models are normalised to this level so different captures
    // have a similar volume (same target as the official NAM plugin).
    constexpr double targetLoudnessDb = -18.0;

    std::filesystem::path toFsPath (const juce::File& file)
    {
       #if JUCE_WINDOWS
        // Wide string keeps accents (e.g. "Viñas") working on Windows
        return std::filesystem::path (file.getFullPathName().toWideCharPointer());
       #else
        return std::filesystem::path (file.getFullPathName().toStdString());
       #endif
    }
}

//==============================================================================
NamAmp::NamAmp()  = default;
NamAmp::~NamAmp() = default;

//==============================================================================
void NamAmp::prepare (double sampleRate, int maxBlockSize)
{
    currentSampleRate = sampleRate;
    maxBlock = juce::jmax (1, maxBlockSize);
    scratch.assign ((size_t) maxBlock, 0.0f);

    const juce::SpinLock::ScopedLockType sl (lock);

    if (model != nullptr)
        model->Reset (currentSampleRate, maxBlock);
}

//==============================================================================
juce::String NamAmp::loadModel (const juce::File& file)
{
    if (! file.existsAsFile())
        return "File not found: " + file.getFullPathName();

    std::unique_ptr<nam::DSP> newModel;

    try
    {
        newModel = nam::get_dsp (toFsPath (file));
    }
    catch (const std::exception& e)
    {
        return "Could not load model: " + juce::String (e.what());
    }

    if (newModel == nullptr)
        return "Could not load model.";

    if (newModel->NumInputChannels() != 1 || newModel->NumOutputChannels() != 1)
        return "Only mono NAM models are supported.";

    // Prepare the new model BEFORE it reaches the audio thread
    newModel->Reset (currentSampleRate, maxBlock);

    float newGain = 1.0f;
    if (newModel->HasLoudness())
        newGain = juce::Decibels::decibelsToGain ((float) (targetLoudnessDb - newModel->GetLoudness()));

    modelSampleRate.store (newModel->GetExpectedSampleRate());

    {
        // Swap quickly; the old model is destroyed below, outside the lock
        const juce::SpinLock::ScopedLockType sl (lock);
        std::swap (model, newModel);
        normalisationGain = newGain;
    }

    modelFile = file;
    modelLoaded.store (true);
    return {};
}

void NamAmp::clearModel()
{
    std::unique_ptr<nam::DSP> old;

    {
        const juce::SpinLock::ScopedLockType sl (lock);
        std::swap (model, old);
    }

    modelFile = juce::File();
    modelLoaded.store (false);
    modelSampleRate.store (0.0);
}

bool NamAmp::hasSampleRateMismatch() const
{
    const double modelRate = modelSampleRate.load();

    // A value <= 0 means the model didn't say; assume it's fine
    return hasModel() && modelRate > 0.0 && std::abs (modelRate - currentSampleRate) > 1.0;
}

//==============================================================================
void NamAmp::process (float* data, int numSamples)
{
    const juce::SpinLock::ScopedTryLockType sl (lock);

    if (! sl.isLocked() || model == nullptr)
        return; // pass through

    // Process in chunks no larger than the size the model was prepared for
    int pos = 0;
    while (pos < numSamples)
    {
        const int n = juce::jmin (maxBlock, numSamples - pos);

        std::copy (data + pos, data + pos + n, scratch.data());

        float* in  = scratch.data();
        float* out = data + pos;
        model->process (&in, &out, n);

        juce::FloatVectorOperations::multiply (out, normalisationGain, n);
        pos += n;
    }
}
