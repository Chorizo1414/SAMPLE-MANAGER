#pragma once

// Lightweight BPM estimator for JoHu Crate. Original code by JoHu Studios.
// No BTrack/SoundTouch dependency. Processes a low-rate amplitude envelope in
// a background thread. It does not run inside the audio callback.
#include <algorithm>
#include <cmath>
#include <vector>

class TempoDetector
{
public:
    explicit TempoDetector(double sampleRate)
        : envelopeHop(std::max(1, static_cast<int>(std::round(sampleRate / 100.0)))),
          envelopeRate(sampleRate / envelopeHop)
    {
        envelope.reserve(6500);
    }

    void addSample(double sample)
    {
        energy += std::abs(sample);
        if (++samplesInHop >= envelopeHop)
        {
            envelope.push_back(energy / samplesInHop);
            energy = 0.0;
            samplesInHop = 0;
        }
    }

    double estimateBpm() const
    {
        const int count = static_cast<int>(envelope.size());
        if (count < static_cast<int>(envelopeRate * 3.0)) return 0.0;

        // Positive energy changes indicate drum attacks. Remove slow loudness
        // variation so steady bass or silence do not dominate autocorrelation.
        std::vector<double> onsets(static_cast<size_t>(count), 0.0);
        double smooth = envelope.front();
        double prev = envelope.front();
        double total = 0.0;
        for (int i = 1; i < count; ++i)
        {
            smooth = 0.94 * smooth + 0.06 * envelope[i];
            const double delta = envelope[i] - std::max(prev * 0.65, smooth);
            onsets[i] = std::max(0.0, delta);
            total += onsets[i] * onsets[i];
            prev = envelope[i];
        }
        if (total < 1.0e-8) return 0.0;

        const int minLag = std::max(1, static_cast<int>(envelopeRate * 60.0 / 200.0));
        const int maxLag = std::min(count / 2, static_cast<int>(envelopeRate * 60.0 / 60.0));
        double bestScore = 0.0;
        int bestLag = 0;
        for (int lag = minLag; lag <= maxLag; ++lag)
        {
            double cross = 0.0, e1 = 0.0, e2 = 0.0;
            for (int i = lag; i < count; ++i)
            {
                const double a = onsets[i];
                const double b = onsets[i - lag];
                cross += a*b;
                e1 += a*a;
                e2 += b*b;
            }
            const double score = cross / (std::sqrt(e1 * e2) + 1.0e-12);
            // Only return a BPM if we see repeating onsets with enough support.
            if (score > bestScore)
            {
                bestScore = score;
                bestLag = lag;
            }
        }
        if (bestLag == 0 || bestScore < 0.16) return 0.0;
        return envelopeRate * 60.0 / bestLag;
    }

private:
    int envelopeHop;
    double envelopeRate;
    double energy = 0.0;
    int samplesInHop = 0;
    std::vector<double> envelope;
};
