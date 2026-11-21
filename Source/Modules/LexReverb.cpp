#include "LexReverb.h"

namespace
{
    // Mutually-prime-ish line lengths in ms (at SIZE = 1.0)
    constexpr std::array<double, 8> lineMs { 31.7, 37.3, 41.9, 47.3, 53.9, 59.3, 67.1, 73.7 };

    // Slightly different, slow modulation rates per line (Hz)
    constexpr std::array<double, 8> lineLfoHz { 0.31, 0.47, 0.53, 0.71, 0.37, 0.59, 0.83, 0.43 };

    // Input diffuser lengths in ms (left / right differ for width)
    constexpr std::array<double, 4> diffMsL { 4.77, 3.59, 12.73, 9.31 };
    constexpr std::array<double, 4> diffMsR { 4.31, 3.97, 11.83, 10.07 };
    constexpr float diffGain = 0.65f;

    constexpr double maxPreDelayMs = 200.0;
    constexpr float earlyGain = 0.25f;

    // The damping filters add extra loss; this keeps the measured RT60 equal to DECAY
    constexpr double decayCompensation = 1.15;
}

//==============================================================================
void LexReverb::prepare (double newSampleRate, int /*maxBlockSize*/)
{
    sampleRate = newSampleRate;
    const double msToSamples = sampleRate * 0.001;

    for (size_t i = 0; i < (size_t) numLines; ++i)
    {
        const double maxLen = lineMs[i] * maxSizeScale * msToSamples + maxModMs * msToSamples + 4.0;
        lines[i].buffer.assign ((size_t) std::ceil (maxLen), 0.0f);
        lines[i].lfoInc = lineLfoHz[i] / sampleRate;
        lines[i].lfoPhase = (double) i / numLines;   // spread the LFO phases
    }

    for (size_t i = 0; i < (size_t) numDiffusers; ++i)
    {
        diffL[i].buffer.assign ((size_t) juce::jmax (1, (int) (diffMsL[i] * msToSamples)), 0.0f);
        diffR[i].buffer.assign ((size_t) juce::jmax (1, (int) (diffMsR[i] * msToSamples)), 0.0f);
    }

    const auto preSize = (size_t) std::ceil (maxPreDelayMs * msToSamples) + 2;
    preDelayL.assign (preSize, 0.0f);
    preDelayR.assign (preSize, 0.0f);

    smoothedMix.reset (sampleRate, 0.05);
    smoothedMix.setCurrentAndTargetValue (params.mix);

    needsUpdate = true;
    updateLines();
    reset();
}

void LexReverb::reset()
{
    for (auto& l : lines)
    {
        std::fill (l.buffer.begin(), l.buffer.end(), 0.0f);
        l.writePos = 0;
        l.damp = 0.0f;
    }

    for (auto* set : { &diffL, &diffR })
        for (auto& ap : *set)
        {
            std::fill (ap.buffer.begin(), ap.buffer.end(), 0.0f);
            ap.pos = 0;
        }

    std::fill (preDelayL.begin(), preDelayL.end(), 0.0f);
    std::fill (preDelayR.begin(), preDelayR.end(), 0.0f);
    preDelayPos = 0;
}

//==============================================================================
void LexReverb::setParameters (const Params& p)
{
    if (p.decaySec != params.decaySec || p.size != params.size || p.preDelayMs != params.preDelayMs
        || p.toneHz != params.toneHz || p.mod != params.mod)
        needsUpdate = true;

    params = p;
    smoothedMix.setTargetValue (juce::jlimit (0.0f, 1.0f, p.mix));
}

void LexReverb::updateLines()
{
    const double msToSamples = sampleRate * 0.001;
    const double sizeScale   = 0.4 + (maxSizeScale - 0.4) * juce::jlimit (0.0, 1.0, (double) params.size);
    const double decay       = juce::jlimit (0.1, 30.0, (double) params.decaySec) * decayCompensation;

    for (size_t i = 0; i < (size_t) numLines; ++i)
    {
        auto& l = lines[i];
        l.baseLength = lineMs[i] * sizeScale * msToSamples;

        // RT60: after 'decay' seconds the tail is 60 dB down
        l.gain = (float) std::pow (10.0, -3.0 * l.baseLength / (decay * sampleRate));
    }

    const double fc = juce::jlimit (500.0, sampleRate * 0.45, (double) params.toneHz);
    dampCoeff = (float) std::exp (-juce::MathConstants<double>::twoPi * fc / sampleRate);

    modDepthSamples = juce::jlimit (0.0, 1.0, (double) params.mod) * maxModMs * msToSamples;
    preDelaySamples = juce::jlimit (0, (int) preDelayL.size() - 2,
                                    (int) (params.preDelayMs * msToSamples));
    needsUpdate = false;
}

//==============================================================================
void LexReverb::process (float* left, float* right, int numSamples)
{
    if (needsUpdate)
        updateLines();

    const int preSize = (int) preDelayL.size();
    std::array<float, numLines> out {};

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = left[i], inR = right[i];

        // 1) Pre-delay
        preDelayL[(size_t) preDelayPos] = inL;
        preDelayR[(size_t) preDelayPos] = inR;
        int readPre = preDelayPos - preDelaySamples;
        if (readPre < 0)
            readPre += preSize;
        float xL = preDelayL[(size_t) readPre];
        float xR = preDelayR[(size_t) readPre];
        if (++preDelayPos >= preSize)
            preDelayPos = 0;

        // 2) Input diffusion (smears the attack into a dense cloud)
        for (size_t d = 0; d < (size_t) numDiffusers; ++d)
        {
            xL = diffL[d].process (xL, diffGain);
            xR = diffR[d].process (xR, diffGain);
        }

        // 3) Read each modulated, damped line
        float sum = 0.0f;
        for (size_t n = 0; n < (size_t) numLines; ++n)
        {
            auto& l = lines[n];
            l.lfoPhase += l.lfoInc;
            if (l.lfoPhase >= 1.0)
                l.lfoPhase -= 1.0;

            const double delay = l.baseLength
                               + modDepthSamples * 0.5 * (1.0 + std::sin (juce::MathConstants<double>::twoPi * l.lfoPhase));

            const int size = (int) l.buffer.size();
            double readPos = (double) l.writePos - delay;
            while (readPos < 0.0)
                readPos += size;

            const int i0 = (int) readPos;
            const int i1 = (i0 + 1 == size) ? 0 : i0 + 1;
            const float frac = (float) (readPos - i0);
            float v = l.buffer[(size_t) i0] + frac * (l.buffer[(size_t) i1] - l.buffer[(size_t) i0]);

            // Damping (high frequencies die faster - like a real room)
            l.damp = v + (l.damp - v) * dampCoeff;
            v = l.damp * l.gain;

            out[n] = v;
            sum += v;
        }

        // 4) Householder feedback matrix: y = x - (2/N) * sum(x)  (lossless mixing)
        const float householder = sum * (2.0f / (float) numLines);

        for (size_t n = 0; n < (size_t) numLines; ++n)
        {
            auto& l = lines[n];
            const float input = (n % 2 == 0) ? xL : xR;   // even lines <- left, odd <- right
            l.buffer[(size_t) l.writePos] = out[n] - householder + input;

            if (++l.writePos >= (int) l.buffer.size())
                l.writePos = 0;
        }

        // 5) Stereo taps: two orthogonal Hadamard rows -> decorrelated, mono-safe L/R.
        //    Plus a little of the diffused input so the reverb starts immediately
        //    (otherwise the first line length acts like hidden pre-delay).
        const float wetL = 0.35f * (out[0] - out[1] + out[2] - out[3] + out[4] - out[5] + out[6] - out[7]) + earlyGain * xL;
        const float wetR = 0.35f * (out[0] + out[1] - out[2] - out[3] + out[4] + out[5] - out[6] - out[7]) + earlyGain * xR;

        const float mix = smoothedMix.getNextValue();
        left[i]  = inL + mix * wetL;
        right[i] = inR + mix * wetR;
    }
}
