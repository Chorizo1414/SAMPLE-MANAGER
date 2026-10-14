#include "StretchAudioSource.h"
#include <cmath>

void StretchAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    if (source != nullptr)
        source->prepareToPlay(samplesPerBlockExpected, sampleRate);

    // All DSP configuration and audio buffers are allocated outside callbacks.
    processor.presetCheaper(2, static_cast<float>(sampleRate));
    inputBuffer.setSize(2, outputChunk * 2 + 4, false, false, true);
    outputBuffer.setSize(2, outputChunk, false, false, true);
    appliedPitch = 0.0;
    readFraction = 0.0;
    previousEnabled = false;
    resetPending.store(true);
}

void StretchAudioSource::releaseResources()
{
    if (source != nullptr) source->releaseResources();
}

void StretchAudioSource::setPitchSemiTones(double semitones)
{
    requestedPitch.store(juce::jlimit(-12.0, 12.0, semitones));
}

void StretchAudioSource::setTempoMultiplier(double multiplier)
{
    requestedTempo.store(juce::jlimit(0.5, 2.0, multiplier));
}

void StretchAudioSource::setStretchEnabled(bool enabled)
{
    requestedEnabled.store(enabled);
    resetPending.store(true);
}

void StretchAudioSource::resetForNewFile()
{
    resetPending.store(true);
}

void StretchAudioSource::getNextAudioBlock(const juce::AudioSourceChannelInfo& info)
{
    if (source == nullptr)
    {
        info.clearActiveBufferRegion();
        return;
    }

    const bool enabled = requestedEnabled.load();
    const auto semitones = requestedPitch.load();
    const auto tempo = requestedTempo.load();

    // At unity or in vinyl mode, leave JUCE's original, low-latency path intact.
    if (!enabled || (std::abs(semitones) < 0.0001 && std::abs(tempo - 1.0) < 0.0001))
    {
        if (previousEnabled)
            resetPending.store(true);
        previousEnabled = false;
        source->getNextAudioBlock(info);
        return;
    }

    if (!previousEnabled || resetPending.exchange(false))
    {
        processor.reset();
        readFraction = 0.0;
        appliedPitch = 1000.0; // force parameter refresh
    }
    previousEnabled = true;

    if (std::abs(appliedPitch - semitones) > 0.0001)
    {
        processor.setTransposeSemitones(static_cast<float>(semitones));
        appliedPitch = semitones;
    }

    int written = 0;
    while (written < info.numSamples)
    {
        const int outputCount = juce::jmin(outputChunk, info.numSamples - written);
        const double requiredExact = readFraction + outputCount * tempo;
        const int inputCount = juce::jlimit(0, outputChunk * 2 + 4,
                                           static_cast<int>(requiredExact));
        readFraction = requiredExact - inputCount;

        inputBuffer.clear();
        if (inputCount > 0)
        {
            juce::AudioSourceChannelInfo srcInfo(&inputBuffer, 0, inputCount);
            source->getNextAudioBlock(srcInfo);
        }

        float* in[2] = { inputBuffer.getWritePointer(0), inputBuffer.getWritePointer(1) };
        float* out[2] = { outputBuffer.getWritePointer(0), outputBuffer.getWritePointer(1) };
        processor.process(in, inputCount, out, outputCount);

        for (int channel = 0; channel < info.buffer->getNumChannels(); ++channel)
        {
            info.buffer->copyFrom(channel, info.startSample + written,
                                  outputBuffer, juce::jmin(channel, 1), 0, outputCount);
        }
        written += outputCount;
    }
}
