#pragma once
#include <JuceHeader.h>
#include "BTrack/kiss_fft.h"
#include <vector>
#include <cmath>

class KeyDetector
{
public:
    KeyDetector(double sampleRate) : sr(sampleRate), chromagram(12, 0.0)
    {
        fftSize = 4096;
        fftCfg = kiss_fft_alloc(fftSize, 0, nullptr, nullptr);
        window.resize(fftSize);
        // Creamos una ventana de Hanning para precisión de frecuencias
        for (int i = 0; i < fftSize; ++i)
            window[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (fftSize - 1)));
    }

    ~KeyDetector()
    {
        kiss_fft_free(fftCfg);
    }

    // Recibe el mismo audio que ya le enviamos a BTrack
    void processAudioFrame(const double* audioData, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i) {
            buffer.push_back(audioData[i]);
            if (buffer.size() >= fftSize) {
                processFFT();
                buffer.clear();
            }
        }
    }

    juce::String getDetectedKey()
    {
        // Perfiles Krumhansl-Schmuckler (Estándar de la Industria para Tonalidades)
        double majProfile[12] = { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 };
        double minProfile[12] = { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };
        juce::String notes[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

        int bestKey = 0;
        double maxCorr = -1.0;
        juce::String detectedKey = "--";

        // Comparamos el audio extraído contra las 24 tonalidades posibles
        for (int i = 0; i < 24; ++i) {
            bool isMinor = (i >= 12);
            int root = i % 12;
            double corr = pearsonCorrelation(chromagram, isMinor ? minProfile : majProfile, root);
            if (corr > maxCorr) {
                maxCorr = corr;
                detectedKey = notes[root] + (isMinor ? " Min" : " Maj");
            }
        }
        return detectedKey;
    }

private:
    double sr;
    int fftSize;
    kiss_fft_cfg fftCfg;
    std::vector<double> buffer;
    std::vector<float> window;
    std::vector<double> chromagram;

    void processFFT()
    {
        std::vector<kiss_fft_cpx> fftIn(fftSize), fftOut(fftSize);
        for (int i = 0; i < fftSize; ++i) {
            fftIn[i].r = (float)buffer[i] * window[i];
            fftIn[i].i = 0.0f;
        }
        kiss_fft(fftCfg, fftIn.data(), fftOut.data());

        // Extraemos la energía musical y la mapeamos a las 12 notas
        for (int i = 1; i < fftSize / 2; ++i) {
            double mag = std::sqrt(fftOut[i].r * fftOut[i].r + fftOut[i].i * fftOut[i].i);
            double freq = i * sr / fftSize;
            if (freq > 20.0) {
                int chroma = (int)std::round(12.0 * std::log2(freq / 440.0) + 9.0) % 12;
                if (chroma < 0) chroma += 12;
                chromagram[chroma] += mag;
            }
        }
    }

    double pearsonCorrelation(const std::vector<double>& chroma, double* profile, int shift)
    {
        double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0, sumY2 = 0;
        for (int i = 0; i < 12; ++i) {
            double x = chroma[i];
            double y = profile[(i - shift + 12) % 12];
            sumX += x; sumY += y; sumXY += x * y; sumX2 += x * x; sumY2 += y * y;
        }
        double num = 12 * sumXY - sumX * sumY;
        double den = std::sqrt((12 * sumX2 - sumX * sumX) * (12 * sumY2 - sumY * sumY));
        return den == 0 ? 0 : num / den;
    }
};