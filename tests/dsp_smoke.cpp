// Optional self-contained DSP smoke test for GCC/Clang/MSVC (no JUCE dependency).
// g++ -std=c++17 -O2 -I Source tests/dsp_smoke.cpp -o dsp_smoke && ./dsp_smoke
#include "../Source/TempoDetector.h"
#include "../Source/Signalsmith/signalsmith-stretch.h"
#include <cmath>
#include <iostream>
#include <array>

static bool testBpm(double bpm)
{
    constexpr double sr = 44100.0;
    TempoDetector detector(sr);
    const int total = static_cast<int>(sr*10.0);
    const int beatSamples = static_cast<int>(sr*60.0/bpm);
    for (int i=0; i<total; ++i)
    {
        const int phase = i % beatSamples;
        const double impulse = (phase < 150) ? (1.0-phase/150.0) : 0.0;
        detector.addSample(impulse);
    }
    const double detected = detector.estimateBpm();
    std::cout << "Expected BPM " << bpm << ", detected " << detected << "\n";
    return std::abs(detected - bpm) <= 3.0;
}

static bool testPitchTempo()
{
    signalsmith::stretch::SignalsmithStretch<float> stretch;
    stretch.presetCheaper(2, 44100.0f);
    stretch.setTransposeSemitones(2.0f);

    std::array<float, 160> left{}, right{};
    std::array<float, 128> outL{}, outR{};
    float* input[2] = {left.data(), right.data()};
    float* output[2] = {outL.data(), outR.data()};
    for (int block=0; block<100; ++block)
    {
        for (int i=0;i<160;++i)
        {
            left[i] = static_cast<float>(std::sin(2.0*3.141592653589793*440.0*(i+block*160)/44100.0));
            right[i]=left[i];
        }
        stretch.process(input, 160, output, 128);
        for (float value:outL)
            if (!std::isfinite(value)) return false;
    }
    return true;
}

int main()
{
    const bool ok = testBpm(80) && testBpm(100) && testBpm(125) && testPitchTempo();
    std::cout << (ok ? "DSP smoke test OK\n" : "DSP smoke test FAILED\n");
    return ok ? 0 : 1;
}
