#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include "Signalsmith/signalsmith-stretch.h"

// Independent pitch/tempo playback. Signalsmith Stretch + Signalsmith Linear
// are MIT licensed; see Source/Signalsmith/LICENSE-*.txt.
// The vinyl mode stays in AudioPlayer's JUCE resampler.
class StretchAudioSource : public juce::AudioSource
{
public:
    explicit StretchAudioSource(juce::AudioSource* input) : source(input) {}

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    void setPitchSemiTones(double semitones);
    void setTempoMultiplier(double factor);
    void setStretchEnabled(bool enabled);
    void resetForNewFile();

private:
    juce::AudioSource* source = nullptr;
    signalsmith::stretch::SignalsmithStretch<float> processor;
    std::atomic<double> requestedPitch{0.0};
    std::atomic<double> requestedTempo{1.0};
    std::atomic<bool> requestedEnabled{false};
    std::atomic<bool> resetPending{true};

    bool previousEnabled = false;
    double appliedPitch = 0.0, readFraction = 0.0;
    static constexpr int outputChunk = 128;
    juce::AudioBuffer<float> inputBuffer;
    juce::AudioBuffer<float> outputBuffer;
};
