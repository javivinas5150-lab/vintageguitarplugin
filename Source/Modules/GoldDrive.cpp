#include "GoldDrive.h"

namespace
{
    constexpr int oversamplingStages = 2; // 2^2 = 4x

    // Germanium-style soft clipper (smooth knee, symmetric pair of diodes)
    inline float softClip (float x)
    {
        return std::tanh (x);
    }
}

//==============================================================================
void GoldDrive::prepare (double sampleRate, int maxBlockSize)
{
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        1, oversamplingStages,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,   // max quality
        true);  // integer latency (so the host can compensate exactly)

    oversampling->initProcessing ((size_t) juce::jmax (1, maxBlockSize));
    osRate = sampleRate * (double) oversampling->getOversamplingFactor();

    filtersNeedUpdate = true;
    updateFilters();
    reset();
}

void GoldDrive::reset()
{
    if (oversampling != nullptr)
        oversampling->reset();

    driveHighPass.reset();
    driveMidBoost.reset();
    driveLowPass.reset();
    trebleShelf.reset();
    bassShelf.reset();
}

int GoldDrive::getLatencySamples() const
{
    return oversampling != nullptr ? (int) std::round (oversampling->getLatencyInSamples()) : 0;
}

//==============================================================================
void GoldDrive::setParameters (float gain0to10, float treble0to10, float output0to10, bool bassBoost)
{
    if (gain0to10 != gainKnob || treble0to10 != trebleKnob || output0to10 != outputKnob || bassBoost != bassSwitch)
    {
        gainKnob   = gain0to10;
        trebleKnob = treble0to10;
        outputKnob = output0to10;
        bassSwitch = bassBoost;
        filtersNeedUpdate = true;
    }
}

void GoldDrive::updateFilters()
{
    using Coeffs = juce::dsp::IIR::ArrayCoefficients<float>;

    // Drive path voicing
    *driveHighPass.coefficients = Coeffs::makeHighPass    (osRate, bassSwitch ? 100.0f : 250.0f, 0.707f);
    *bassShelf.coefficients     = Coeffs::makeLowShelf    (osRate, 120.0, 0.707f,
                                                           juce::Decibels::decibelsToGain (bassSwitch ? 6.0f : 0.0f));
    *driveMidBoost.coefficients = Coeffs::makePeakFilter  (osRate, 1000.0, 0.8f, juce::Decibels::decibelsToGain (6.0f));
    *driveLowPass.coefficients  = Coeffs::makeLowPass     (osRate, 6000.0, 0.707f);

    // TREBLE: 5 = flat, 0 = -10 dB, 10 = +10 dB above ~1.2 kHz
    const float trebleDb = (trebleKnob - 5.0f) * 2.0f;
    *trebleShelf.coefficients = Coeffs::makeHighShelf (osRate, 1200.0, 0.707f,
                                                       juce::Decibels::decibelsToGain (trebleDb));

    // GAIN: dual-gang behaviour
    const float g = juce::jlimit (0.0f, 1.0f, gainKnob / 10.0f);
    driveGain   = juce::Decibels::decibelsToGain (4.0f + 36.0f * g);  // +4 dB .. +40 dB into the diodes
    cleanAmount = 1.0f - 0.85f * g;                                   // clean fades out as gain goes up
    driveAmount = 0.25f + 0.55f * g;

    // OUTPUT: 0 = -20 dB, 5 = unity, 10 = +12 dB
    const float outDb = outputKnob <= 5.0f ? -20.0f + 4.0f * outputKnob
                                           : (outputKnob - 5.0f) * 2.4f;
    outputGain = juce::Decibels::decibelsToGain (outDb);

    filtersNeedUpdate = false;
}

//==============================================================================
void GoldDrive::process (float* data, int numSamples, bool enabled)
{
    if (oversampling == nullptr)
        return;

    if (filtersNeedUpdate)
        updateFilters();

    float* channels[] = { data };
    juce::dsp::AudioBlock<float> block (channels, 1, (size_t) numSamples);

    auto up = oversampling->processSamplesUp (block);

    if (enabled)
    {
        auto* x = up.getChannelPointer (0);
        const auto n = up.getNumSamples();

        for (size_t i = 0; i < n; ++i)
        {
            const float in = x[i];

            // Drive path: bass cut -> mid hump -> gain -> diodes -> top roll-off
            float d = driveHighPass.processSample (in);
            d = driveMidBoost.processSample (d);
            d = softClip (d * driveGain) * 0.5f;
            d = driveLowPass.processSample (d);

            // Mix with the clean path, then TREBLE and OUTPUT
            float y = cleanAmount * in + driveAmount * d;
            y = trebleShelf.processSample (y);
            y = bassShelf.processSample (y);
            x[i] = y * outputGain;
        }
    }

    oversampling->processSamplesDown (block);
}
