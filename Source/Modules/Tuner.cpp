#include "Tuner.h"
#include <cmath>

void Tuner::prepare (double sampleRate)
{
    rate.store (sampleRate);
}

void Tuner::push (const float* data, int numSamples) noexcept
{
    int pos = writePos.load (std::memory_order_relaxed);

    for (int i = 0; i < numSamples; ++i)
    {
        ring[(size_t) pos] = data[i];
        pos = (pos + 1) & (ringSize - 1);
    }

    writePos.store (pos, std::memory_order_release);
}

juce::String Tuner::noteName (int midiNote)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return names[((midiNote % 12) + 12) % 12];
}

Tuner::Reading Tuner::analyse (float referenceA4)
{
    Reading r;
    const double sr = rate.load();

    // Decimate to ~12-24 kHz: plenty for guitar fundamentals (down to ~30 Hz)
    const int factor = sr > 70000.0 ? 4 : (sr > 30000.0 ? 2 : 1);
    const double dsr = sr / factor;

    // Window long enough for 3+ periods of a low B (31 Hz on a 7/8-string or bass)
    const int decimatedLen = juce::jlimit (512, 4096, (int) (dsr * 0.085));
    const int rawLen = decimatedLen * factor;

    window.resize ((size_t) rawLen);
    const int end = writePos.load (std::memory_order_acquire);
    for (int i = 0; i < rawLen; ++i)
        window[(size_t) i] = ring[(size_t) ((end - rawLen + i) & (ringSize - 1))];

    // Simple boxcar low-pass + decimation, and RMS
    decimated.resize ((size_t) decimatedLen);
    double energy = 0.0;
    for (int i = 0; i < decimatedLen; ++i)
    {
        float s = 0.0f;
        for (int k = 0; k < factor; ++k)
            s += window[(size_t) (i * factor + k)];
        s /= (float) factor;
        decimated[(size_t) i] = s;
        energy += (double) s * s;
    }

    r.level = (float) std::sqrt (energy / decimatedLen);
    if (r.level < 0.0025f)            // about -52 dBFS: nothing playing
        return r;

    // --- YIN ---
    const int minLag = juce::jmax (2, (int) (dsr / 1400.0));   // up to ~1.4 kHz
    const int maxLag = juce::jmin (decimatedLen / 2, (int) (dsr / 28.0));
    const int w = decimatedLen - maxLag;

    diff.assign ((size_t) maxLag + 2, 0.0f);
    for (int tau = 1; tau <= maxLag + 1; ++tau)
    {
        double d = 0.0;
        for (int i = 0; i < w; ++i)
        {
            const float delta = decimated[(size_t) i] - decimated[(size_t) (i + tau)];
            d += (double) delta * delta;
        }
        diff[(size_t) tau - 1] = (float) d;
    }

    // Cumulative mean normalised difference (index = tau - 1)
    std::vector<float>& cmnd = diff;
    double running = 0.0;
    for (int tau = 1; tau <= maxLag + 1; ++tau)
    {
        running += cmnd[(size_t) tau - 1];
        cmnd[(size_t) tau - 1] = running > 0.0 ? (float) (cmnd[(size_t) tau - 1] * tau / running) : 1.0f;
    }

    constexpr float threshold = 0.15f;
    int bestTau = -1;
    for (int tau = minLag; tau <= maxLag; ++tau)
    {
        if (cmnd[(size_t) tau - 1] < threshold)
        {
            while (tau + 1 <= maxLag && cmnd[(size_t) tau] < cmnd[(size_t) tau - 1])
                ++tau;
            bestTau = tau;
            break;
        }
    }

    if (bestTau < 0)
        return r;

    // Parabolic interpolation around the minimum
    float tauEstimate = (float) bestTau;
    if (bestTau > 1 && bestTau < maxLag)
    {
        const float s0 = cmnd[(size_t) bestTau - 2], s1 = cmnd[(size_t) bestTau - 1], s2 = cmnd[(size_t) bestTau];
        const float denom = s0 - 2.0f * s1 + s2;
        if (std::abs (denom) > 1.0e-9f)
            tauEstimate += 0.5f * (s0 - s2) / denom;
    }

    const float freq = (float) (dsr / tauEstimate);
    if (freq < 28.0f || freq > 1500.0f)
        return r;

    const float noteFloat = 69.0f + 12.0f * std::log2 (freq / referenceA4);
    r.midiNote  = juce::roundToInt (noteFloat);
    r.cents     = (noteFloat - (float) r.midiNote) * 100.0f;
    r.frequency = freq;
    r.valid     = true;
    return r;
}
