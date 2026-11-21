#include "Vibe.h"

void Vibe::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;

    // Incandescent lamp thermal lag ~ 15 ms
    lampCoeff = (float) std::exp (-1.0 / (0.015 * sampleRate));
    reset();
}

void Vibe::reset()
{
    state.fill (0.0f);
    lamp = 0.5f;
}

void Vibe::setParameters (float speedHz, float intensity01, bool vibratoMode, float depth01)
{
    speed     = speedHz;
    intensity = juce::jlimit (0.0f, 1.0f, intensity01);
    vibrato   = vibratoMode;
    depth     = juce::jlimit (0.0f, 1.0f, depth01);
}

void Vibe::process (float* data, int numSamples)
{
    const double phaseInc = speed / sampleRate;
    const double maxFreq  = sampleRate * 0.45;
    const double piOverFs = juce::MathConstants<double>::pi / sampleRate;

    // DEPTH (smoothed per block): sweep range and lamp throb
    smoothedDepth += 0.3f * (depth - smoothedDepth);
    const double rCentre   = std::sqrt (rBright * rDark);
    const double rangeLog  = std::log (rDark / rBright) * (0.6 + 1.2 * (double) smoothedDepth);  // 25:1 at ~33 %
    const double rLow      = rCentre * std::exp (-0.5 * rangeLog);
    const double rHigh     = rCentre * std::exp ( 0.5 * rangeLog);
    const float  throb     = 0.28f * smoothedDepth * intensity;     // up to about -3 dB dips

    for (int i = 0; i < numSamples; ++i)
    {
        // LFO (0..1), slightly skewed like the original's oscillator
        phase += phaseInc;
        if (phase >= 1.0)
            phase -= 1.0;

        const float sine = 0.5f + 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * phase);
        const float lfo  = sine * sine;   // spends more time "dark" -> lopsided sweep

        // Lamp follows the LFO with thermal lag
        lamp = lfo + (lamp - lfo) * lampCoeff;

        // Intensity = how far the light swings around the middle
        const float light = 0.5f + intensity * (lamp - 0.5f) * 2.0f;

        // LDR: resistance changes exponentially with light
        const double r = rHigh * std::pow (rLow / rHigh, (double) juce::jlimit (0.0f, 1.0f, light));

        // 4 first-order all-pass stages, each at its own frequency
        const float dry = data[i];
        float x = dry;

        for (size_t s = 0; s < (size_t) numStages; ++s)
        {
            const double f = juce::jlimit (20.0, maxFreq,
                                           1.0 / (juce::MathConstants<double>::twoPi * r * stageCaps[s]));
            const double t = std::tan (f * piOverFs);
            const float a  = (float) ((t - 1.0) / (t + 1.0));

            const float y = a * x + state[s];
            state[s] = x - a * y;
            x = y;
        }

        // Chorus = dry + wet (notches), Vibrato = wet only.
        // Throb: the lamp bleeds into the level - quieter when it's dark.
        const float level = 1.0f - throb * (1.0f - lamp);
        data[i] = level * (vibrato ? x : 0.5f * (dry + x));
    }
}
