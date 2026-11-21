#include "ToneStack.h"

namespace
{
    constexpr float bassFreq   = 150.0f;
    constexpr float middleFreq = 425.0f;
    constexpr float trebleFreq = 1800.0f;

    constexpr float shelfQ  = 0.707f;
    constexpr float middleQ = 0.7f;
}

void ToneStack::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    needsUpdate = true;
    updateCoefficients();
    reset();
}

void ToneStack::reset()
{
    bass.reset();
    middle.reset();
    treble.reset();
}

void ToneStack::setGains (float bassDb, float middleDb, float trebleDb)
{
    if (bassDb != bassGainDb || middleDb != middleGainDb || trebleDb != trebleGainDb)
    {
        bassGainDb   = bassDb;
        middleGainDb = middleDb;
        trebleGainDb = trebleDb;
        needsUpdate  = true;
    }
}

void ToneStack::updateCoefficients()
{
    using Coeffs = juce::dsp::IIR::ArrayCoefficients<float>;
    const auto toGain = [] (float db) { return juce::Decibels::decibelsToGain (db); };

    // ArrayCoefficients returns a std::array -> no memory allocation on the audio thread
    *bass.coefficients   = Coeffs::makeLowShelf  (sampleRate, bassFreq,   shelfQ,  toGain (bassGainDb));
    *middle.coefficients = Coeffs::makePeakFilter (sampleRate, middleFreq, middleQ, toGain (middleGainDb));
    *treble.coefficients = Coeffs::makeHighShelf (sampleRate, trebleFreq, shelfQ,  toGain (trebleGainDb));

    needsUpdate = false;
}

void ToneStack::process (float* data, int numSamples)
{
    if (needsUpdate)
        updateCoefficients();

    for (int i = 0; i < numSamples; ++i)
    {
        auto s = data[i];
        s = bass.processSample (s);
        s = middle.processSample (s);
        s = treble.processSample (s);
        data[i] = s;
    }
}
