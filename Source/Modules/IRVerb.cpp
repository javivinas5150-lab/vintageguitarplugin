#include "IRVerb.h"

namespace
{
    constexpr double maxPreDelayMs = 200.0;
    constexpr float minCutHz = 20.0f, maxCutHz = 20000.0f;

    // JUCE normalises every IR to an energy of 0.125 (-18 dB). Undo that so the
    // wet signal at MIX 100 % sits at about the same loudness as the dry signal,
    // whatever the IR's length.
    constexpr float normalisationMakeup = 8.0f;
}

IRVerb::IRVerb() = default;

//==============================================================================
void IRVerb::prepare (double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    const int block = juce::jmax (1, maxBlockSize);

    convolution.prepare ({ sampleRate, (juce::uint32) block, 2 });
    wet.setSize (2, block, false, true, false);

    const auto preSize = (size_t) std::ceil (maxPreDelayMs * 0.001 * sampleRate) + 2;
    preDelayL.assign (preSize, 0.0f);
    preDelayR.assign (preSize, 0.0f);

    smoothedWetGain.reset (sampleRate, 0.05);
    smoothedWetGain.setCurrentAndTargetValue (params.mix * juce::Decibels::decibelsToGain (params.levelDb));

    filtersNeedUpdate = true;
    reset();
}

void IRVerb::reset()
{
    convolution.reset();
    std::fill (preDelayL.begin(), preDelayL.end(), 0.0f);
    std::fill (preDelayR.begin(), preDelayR.end(), 0.0f);
    preDelayPos = 0;

    for (auto& f : lowCut)  f.reset();
    for (auto& f : highCut) f.reset();
}

//==============================================================================
juce::String IRVerb::loadIR (const juce::File& file)
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

    // Always hand over 2 channels: a mono IR is used on both sides
    juce::AudioBuffer<float> buffer (2, numSamples);
    reader->read (&buffer, 0, numSamples, 0, true, reader->numChannels > 1);

    if (reader->numChannels < 2)
        buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);

    irLengthSeconds.store ((double) numSamples / reader->sampleRate);

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

void IRVerb::clearIR()
{
    irLoaded.store (false);
    irLengthSeconds.store (0.0);
    irFile = juce::File();
}

//==============================================================================
void IRVerb::setParameters (const Params& p)
{
    if (p.lowCutHz != params.lowCutHz || p.highCutHz != params.highCutHz)
        filtersNeedUpdate = true;

    params = p;
    smoothedWetGain.setTargetValue (juce::jlimit (0.0f, 1.0f, p.mix) * juce::Decibels::decibelsToGain (p.levelDb));
}

void IRVerb::updateFilters()
{
    using Coeffs = juce::dsp::IIR::ArrayCoefficients<float>;

    lowCutActive  = params.lowCutHz > minCutHz + 0.5f;
    highCutActive = params.highCutHz < maxCutHz - 0.5f && params.highCutHz < sampleRate * 0.45;

    for (size_t ch = 0; ch < 2; ++ch)
    {
        if (lowCutActive)
            *lowCut[ch].coefficients = Coeffs::makeHighPass (sampleRate, params.lowCutHz, 0.707f);
        if (highCutActive)
            *highCut[ch].coefficients = Coeffs::makeLowPass (sampleRate, params.highCutHz, 0.707f);
    }

    filtersNeedUpdate = false;
}

//==============================================================================
void IRVerb::process (float* left, float* right, int numSamples)
{
    // 1) Pick up a newly loaded IR (never waits for the message thread)
    {
        const juce::SpinLock::ScopedTryLockType sl (pendingLock);

        if (sl.isLocked() && hasPendingIR)
        {
            convolution.loadImpulseResponse (std::move (pendingIR), pendingSampleRate,
                                             juce::dsp::Convolution::Stereo::yes,
                                             juce::dsp::Convolution::Trim::no,       // keep the IR's timing
                                             juce::dsp::Convolution::Normalise::yes);
            hasPendingIR = false;
        }
    }

    if (! irLoaded.load())
        return;

    if (numSamples > wet.getNumSamples())
        return; // host broke its promised block size - skip rather than allocate

    if (filtersNeedUpdate)
        updateFilters();

    // 2) Pre-delay into the wet buffer
    const int preSize = (int) preDelayL.size();
    const int preSamples = juce::jlimit (0, preSize - 2, (int) (params.preDelayMs * 0.001 * sampleRate));
    auto* wl = wet.getWritePointer (0);
    auto* wr = wet.getWritePointer (1);

    for (int i = 0; i < numSamples; ++i)
    {
        preDelayL[(size_t) preDelayPos] = left[i];
        preDelayR[(size_t) preDelayPos] = right[i];

        int readPos = preDelayPos - preSamples;
        if (readPos < 0)
            readPos += preSize;

        wl[i] = preDelayL[(size_t) readPos];
        wr[i] = preDelayR[(size_t) readPos];

        if (++preDelayPos >= preSize)
            preDelayPos = 0;
    }

    // 3) Convolution
    juce::dsp::AudioBlock<float> block (wet.getArrayOfWritePointers(), 2, (size_t) numSamples);
    convolution.process (juce::dsp::ProcessContextReplacing<float> (block));

    // 4) Low Cut / High Cut on the wet signal
    for (size_t ch = 0; ch < 2; ++ch)
    {
        auto* w = wet.getWritePointer ((int) ch);

        if (lowCutActive)
            for (int i = 0; i < numSamples; ++i)
                w[i] = lowCut[ch].processSample (w[i]);

        if (highCutActive)
            for (int i = 0; i < numSamples; ++i)
                w[i] = highCut[ch].processSample (w[i]);
    }

    // 5) Mix: dry stays at 100 %, wet added with MIX x LEVEL
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = smoothedWetGain.getNextValue() * normalisationMakeup;
        left[i]  += g * wl[i];
        right[i] += g * wr[i];
    }
}
