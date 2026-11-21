#include "StereoDelay.h"

void StereoDelay::prepare (double newSampleRate, int /*maxBlockSize*/)
{
    sampleRate = newSampleRate;

    lineSize = (int) std::ceil ((maxDelayMs + maxModMs + 10.0) * 0.001 * sampleRate) + 4;
    lineL.assign ((size_t) lineSize, 0.0f);
    lineR.assign ((size_t) lineSize, 0.0f);

    smoothedTime.reset (sampleRate, 0.25);   // 250 ms glide when TIME moves
    smoothedMix.reset  (sampleRate, 0.05);
    smoothedTime.setCurrentAndTargetValue ((float) (params.timeMs * 0.001 * sampleRate));
    smoothedMix.setCurrentAndTargetValue (params.mix);

    setParameters (params);
    reset();
}

void StereoDelay::reset()
{
    std::fill (lineL.begin(), lineL.end(), 0.0f);
    std::fill (lineR.begin(), lineR.end(), 0.0f);
    writePos = 0;
    toneStateL = toneStateR = 0.0f;
    lfoPhase = 0.0;
}

void StereoDelay::setParameters (const Params& p)
{
    params = p;
    params.feedback = juce::jlimit (0.0f, 0.95f, p.feedback);

    smoothedTime.setTargetValue ((float) (juce::jlimit (20.0f, (float) maxDelayMs, p.timeMs) * 0.001 * sampleRate));
    smoothedMix.setTargetValue (juce::jlimit (0.0f, 1.0f, p.mix));

    // One-pole low-pass for the repeats
    const double fc = juce::jlimit (500.0, sampleRate * 0.45, (double) p.toneHz);
    toneCoeff = (float) std::exp (-juce::MathConstants<double>::twoPi * fc / sampleRate);
}

float StereoDelay::readInterpolated (const std::vector<float>& line, double delaySamples) const
{
    double readPos = (double) writePos - delaySamples;
    while (readPos < 0.0)
        readPos += lineSize;

    const int i0 = (int) readPos;
    const int i1 = (i0 + 1) % lineSize;
    const float frac = (float) (readPos - i0);

    return line[(size_t) i0] + frac * (line[(size_t) i1] - line[(size_t) i0]);
}

void StereoDelay::process (float* left, float* right, int numSamples)
{
    const double lfoInc   = 0.6 / sampleRate;                         // 0.6 Hz wobble
    const double modDepth = params.mod * maxModMs * 0.001 * sampleRate; // in samples
    const float fb = params.feedback;

    for (int i = 0; i < numSamples; ++i)
    {
        const double baseDelay = smoothedTime.getNextValue();
        const float  mix       = smoothedMix.getNextValue();

        lfoPhase += lfoInc;
        if (lfoPhase >= 1.0)
            lfoPhase -= 1.0;

        const double lfo = std::sin (juce::MathConstants<double>::twoPi * lfoPhase);

        // Left and right wobble in opposite directions -> wide, chorused repeats
        const double dL = juce::jlimit (1.0, (double) lineSize - 2.0, baseDelay + modDepth * (1.0 + lfo) * 0.5);
        const double dR = juce::jlimit (1.0, (double) lineSize - 2.0, baseDelay + modDepth * (1.0 - lfo) * 0.5);

        float wetL = readInterpolated (lineL, dL);
        float wetR = readInterpolated (lineR, dR);

        // TONE: darken the repeats
        toneStateL = wetL + (toneStateL - wetL) * toneCoeff;
        toneStateR = wetR + (toneStateR - wetR) * toneCoeff;
        wetL = toneStateL;
        wetR = toneStateR;

        const float inL = left[i], inR = right[i];

        if (params.pingPong)
        {
            // New sound enters on the left; repeats cross over each time
            lineL[(size_t) writePos] = 0.5f * (inL + inR) + fb * wetR;
            lineR[(size_t) writePos] = fb * wetL;
        }
        else
        {
            lineL[(size_t) writePos] = inL + fb * wetL;
            lineR[(size_t) writePos] = inR + fb * wetR;
        }

        if (++writePos >= lineSize)
            writePos = 0;

        left[i]  = inL + mix * wetL;
        right[i] = inR + mix * wetR;
    }
}
