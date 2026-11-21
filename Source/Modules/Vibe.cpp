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

void Vibe::setParameters (float speedHz, float intensity01, bool vibratoMode)
{
    speed     = speedHz;
    intensity = juce::jlimit (0.0f, 1.0f, intensity01);
    vibrato   = vibratoMode;
}

void Vibe::process (float* data, int numSamples)
{
    const double phaseInc = speed / sampleRate;
    const double maxFreq  = sampleRate * 0.45;
    const double piOverFs = juce::MathConstants<double>::pi / sampleRate;

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
        const double r = rDark * std::pow (rBright / rDark, (double) juce::jlimit (0.0f, 1.0f, light));

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

        // Chorus = dry + wet (notches), Vibrato = wet only
        data[i] = vibrato ? x : 0.5f * (dry + x);
    }
}
