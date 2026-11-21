#include "NoiseGate.h"

namespace
{
    // One-pole smoothing coefficient for a given time constant
    float coeffForMs (double sampleRate, float ms)
    {
        return (float) std::exp (-1.0 / (juce::jmax (0.01, (double) ms) * 0.001 * sampleRate));
    }
}

void NoiseGate::prepare (double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    gains.assign ((size_t) juce::jmax (1, maxBlockSize), 1.0f);

    envAttack  = coeffForMs (sampleRate, 0.5f);
    envRelease = coeffForMs (sampleRate, 20.0f);
    gateAttack = coeffForMs (sampleRate, openMs);
    holdSamples = (int) (holdMs * 0.001 * sampleRate);

    currentReleaseMs = -1.0f; // force recalculation
    setParameters (thresholdDb, 100.0f);
    reset();
}

void NoiseGate::reset()
{
    envelope = 0.0f;
    gain = 1.0f;
    open = true;
    holdCounter = 0;
    gating.store (false);
}

void NoiseGate::setParameters (float newThresholdDb, float releaseMs)
{
    thresholdDb = newThresholdDb;

    if (releaseMs != currentReleaseMs)
    {
        currentReleaseMs = releaseMs;
        gateRelease = coeffForMs (sampleRate, releaseMs);
    }
}

void NoiseGate::analyse (const float* input, int numSamples)
{
    // Grow if the host sends a bigger block than announced (rare)
    if ((size_t) numSamples > gains.size())
        gains.resize ((size_t) numSamples, 1.0f);

    const float openLevel  = juce::Decibels::decibelsToGain (thresholdDb);
    const float closeLevel = juce::Decibels::decibelsToGain (thresholdDb - hysteresisDb);

    for (int i = 0; i < numSamples; ++i)
    {
        // Peak envelope follower
        const float level = std::abs (input[i]);
        const float c = level > envelope ? envAttack : envRelease;
        envelope = level + (envelope - level) * c;

        // Open / close with hysteresis and hold
        if (envelope > openLevel)
        {
            open = true;
            holdCounter = holdSamples;
        }
        else if (envelope < closeLevel)
        {
            if (holdCounter > 0)
                --holdCounter;
            else
                open = false;
        }

        // Smooth the gain: fast to open, "release" to close
        const float target = open ? 1.0f : 0.0f;
        const float g = target > gain ? gateAttack : gateRelease;
        gain = target + (gain - target) * g;

        gains[(size_t) i] = gain;
    }

    lastNumSamples = numSamples;
    gating.store (gain < 0.5f);
}

void NoiseGate::apply (float* data, int numSamples) const
{
    const int n = juce::jmin (numSamples, lastNumSamples);
    juce::FloatVectorOperations::multiply (data, gains.data(), n);
}
