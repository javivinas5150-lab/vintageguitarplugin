#include "CabIR.h"

namespace
{
    constexpr float minCutHz = 20.0f;      // Low Cut at this value = off
    constexpr float maxCutHz = 20000.0f;   // High Cut at this value = off
    constexpr float filterQ  = 0.707f;
}

CabIR::CabIR() = default;

//==============================================================================
void CabIR::prepare (double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;

    convolution.prepare ({ sampleRate, (juce::uint32) juce::jmax (1, maxBlockSize), 1 });

    filtersNeedUpdate = true;
    reset();
}

void CabIR::reset()
{
    convolution.reset();
    lowCut.reset();
    highCut.reset();
}

//==============================================================================
juce::String CabIR::loadIR (const juce::File& file)
{
    if (! file.existsAsFile())
        return "File not found: " + file.getFullPathName();

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));

    if (reader == nullptr)
        return "Could not read this file as audio: " + file.getFileName();

    if (reader->lengthInSamples <= 0 || reader->sampleRate <= 0.0)
        return "The IR file is empty.";

    const auto maxSamples = (juce::int64) (maxIRSeconds * reader->sampleRate);
    const int numSamples  = (int) juce::jmin (reader->lengthInSamples, maxSamples);

    // Cab IRs are mono; for stereo files the left channel is used
    juce::AudioBuffer<float> buffer (1, numSamples);
    reader->read (&buffer, 0, numSamples, 0, true, false);

    {
        const juce::SpinLock::ScopedLockType sl (pendingLock);
        pendingIR = std::move (buffer);
        pendingSampleRate = reader->sampleRate;
        hasPendingIR = true;
    }

    irFile = file;
    irLoaded.store (true);
    return {};
}

void CabIR::clearIR()
{
    irLoaded.store (false);
    irFile = juce::File();
}

//==============================================================================
void CabIR::setFilters (float newLowCutHz, float newHighCutHz)
{
    if (newLowCutHz != lowCutHz || newHighCutHz != highCutHz)
    {
        lowCutHz  = newLowCutHz;
        highCutHz = newHighCutHz;
        filtersNeedUpdate = true;
    }
}

void CabIR::process (float* data, int numSamples)
{
    // 1) Pick up a newly loaded IR (never waits for the message thread)
    {
        const juce::SpinLock::ScopedTryLockType sl (pendingLock);

        if (sl.isLocked() && hasPendingIR)
        {
            // Moving the buffer does not allocate. JUCE trims, normalises and
            // resamples the IR to the project rate on its background thread.
            convolution.loadImpulseResponse (std::move (pendingIR), pendingSampleRate,
                                             juce::dsp::Convolution::Stereo::no,
                                             juce::dsp::Convolution::Trim::yes,
                                             juce::dsp::Convolution::Normalise::yes);
            hasPendingIR = false;
        }
    }

    // 2) Cabinet
    if (irLoaded.load())
    {
        float* channels[] = { data };
        juce::dsp::AudioBlock<float> block (channels, 1, (size_t) numSamples);
        convolution.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

    // 3) Low Cut / High Cut
    if (filtersNeedUpdate)
    {
        using Coeffs = juce::dsp::IIR::ArrayCoefficients<float>;

        const auto nyquistSafe = (float) (sampleRate * 0.45);

        lowCutActive  = lowCutHz > minCutHz + 0.5f;
        highCutActive = highCutHz < maxCutHz - 0.5f && highCutHz < nyquistSafe;

        if (lowCutActive)
            *lowCut.coefficients = Coeffs::makeHighPass (sampleRate, lowCutHz, filterQ);

        if (highCutActive)
            *highCut.coefficients = Coeffs::makeLowPass (sampleRate, highCutHz, filterQ);

        filtersNeedUpdate = false;
    }

    if (lowCutActive)
        for (int i = 0; i < numSamples; ++i)
            data[i] = lowCut.processSample (data[i]);

    if (highCutActive)
        for (int i = 0; i < numSamples; ++i)
            data[i] = highCut.processSample (data[i]);
}
