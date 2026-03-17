#include "AudioPlayer.h"

AudioPlayer::AudioPlayer()
{
    formatManager.registerBasicFormats();
}

AudioPlayer::~AudioPlayer()
{
    transportSource.setSource(nullptr);
}

void AudioPlayer::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    resamplerSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void AudioPlayer::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (readerSource.get() == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }
    resamplerSource.getNextAudioBlock(bufferToFill);
}

void AudioPlayer::releaseResources()
{
    resamplerSource.releaseResources();
}

void AudioPlayer::loadFile(const juce::File& audioFile)
{
    auto* reader = formatManager.createReaderFor(audioFile);
    if (reader != nullptr)
    {
        std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
        newSource->setLooping(isLooping);

        transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
        readerSource.reset(newSource.release());

        updateProcessing(); // Recalcula la velocidad y el pitch al cargar un audio nuevo
    }
}

void AudioPlayer::play()
{
    transportSource.setPosition(0.0);
    transportSource.start();
}

void AudioPlayer::stop()
{
    transportSource.stop();
    transportSource.setPosition(0.0);
}

void AudioPlayer::setLooping(bool shouldLoop)
{
    isLooping = shouldLoop;
    if (readerSource != nullptr)
        readerSource->setLooping(isLooping);
}

void AudioPlayer::setVolume(float volume)
{
    transportSource.setGain(volume);
}

void AudioPlayer::setPitch(double semitones)
{
    currentPitchSemitones = semitones;
    updateProcessing();
}

void AudioPlayer::setTempo(double multiplier)
{
    currentTempoMultiplier = multiplier;
    updateProcessing();
}

void AudioPlayer::setStretch(bool enabled)
{
    isStretchEnabled = enabled;
    soundTouchSource.setStretchEnabled(enabled);
    updateProcessing();
}

// Esta función maestra es súper elegante. Mantiene la física del audio perfecta.
void AudioPlayer::updateProcessing()
{
    if (isStretchEnabled)
    {
        // Con Stretch: SoundTouch hace la magia independiente
        soundTouchSource.setPitchSemiTones(currentPitchSemitones);
        soundTouchSource.setTempoMultiplier(currentTempoMultiplier);
        resamplerSource.setResamplingRatio(1.0);
    }
    else
    {
        // Sin Stretch (Vinilo): Tono y velocidad están amarrados en el Resampler
        soundTouchSource.setPitchSemiTones(0.0);
        soundTouchSource.setTempoMultiplier(1.0);

        double pitchRatio = std::pow(2.0, currentPitchSemitones / 12.0);
        resamplerSource.setResamplingRatio(pitchRatio * currentTempoMultiplier);
    }
}