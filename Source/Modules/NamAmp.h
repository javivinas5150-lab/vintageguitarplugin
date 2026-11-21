#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

namespace nam { class DSP; }

//==============================================================================
// NamAmp - wraps a Neural Amp Modeler (.nam) model.
//
// Threading:
//  - loadModel() / clearModel() run on the MESSAGE thread (GUI / state restore).
//  - process() runs on the AUDIO thread. It never blocks: if a model swap is
//    happening at that exact moment, the block simply passes through dry.
//==============================================================================
class NamAmp
{
public:
    NamAmp();
    ~NamAmp();

    // Amps are normalised to a common loudness; pedal captures should keep
    // their real level (call before loading a model).
    void setLoudnessNormalisation (bool shouldNormalise) { normalise = shouldNormalise; }

    // Call from prepareToPlay()
    void prepare (double sampleRate, int maxBlockSize);

    // Message thread. Returns an empty string on success, or an error message.
    juce::String loadModel (const juce::File& file);
    void clearModel();

    // Audio thread. Processes one mono channel in place.
    void process (float* data, int numSamples);

    // Info for the GUI
    bool hasModel() const                { return modelLoaded.load(); }
    juce::File getModelFile() const      { return modelFile; }
    juce::String getModelName() const    { return modelFile.getFileNameWithoutExtension(); }

    // True when the model was captured at a different sample rate than the
    // project (e.g. model at 48 kHz, Cubase project at 44.1 kHz).
    bool hasSampleRateMismatch() const;
    double getModelSampleRate() const    { return modelSampleRate.load(); }

private:
    juce::SpinLock lock;
    std::unique_ptr<nam::DSP> model;     // guarded by lock

    std::vector<float> scratch;          // input copy for the model
    float normalisationGain = 1.0f;      // loudness normalisation (guarded by lock)
    bool normalise = true;

    double currentSampleRate = 48000.0;
    int maxBlock = 512;

    std::atomic<bool> modelLoaded { false };
    std::atomic<double> modelSampleRate { 0.0 };
    juce::File modelFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NamAmp)
};
