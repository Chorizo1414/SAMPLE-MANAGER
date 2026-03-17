#include "SoundTouchAudioSource.h"

SoundTouchAudioSource::SoundTouchAudioSource(juce::AudioSource* inputSource)
    : source(inputSource) {
}

SoundTouchAudioSource::~SoundTouchAudioSource() {}

void SoundTouchAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    if (source != nullptr)
        source->prepareToPlay(samplesPerBlockExpected, sampleRate);

    soundTouch.setSampleRate((unsigned int)sampleRate);
    soundTouch.setChannels(2);
    soundTouch.setSetting(SETTING_USE_QUICKSEEK, 0);
    soundTouch.setSetting(SETTING_USE_AA_FILTER, 1);
    soundTouch.setPitchSemiTones(0.0);
    soundTouch.setTempo(1.0);

    // {* NUEVO: Preparamos la memoria RAM UNA SOLA VEZ antes de darle play *}
    bufferSize = samplesPerBlockExpected;
    sourceBuffer.setSize(2, bufferSize);
    interleavedBuffer.malloc(bufferSize * 2);

    soundTouch.clear();
}

void SoundTouchAudioSource::releaseResources()
{
    if (source != nullptr)
        source->releaseResources();
}

void SoundTouchAudioSource::setPitchSemiTones(double semitones)
{
    currentSemitones = semitones;
    if (stretchEnabled)
        soundTouch.setPitchSemiTones(semitones);
    else
        soundTouch.setPitchSemiTones(0.0);
}

void SoundTouchAudioSource::setTempoMultiplier(double multiplier)
{
    currentTempoMultiplier = multiplier;
    if (stretchEnabled)
        soundTouch.setTempo(multiplier);
    else
        soundTouch.setTempo(1.0);
}

void SoundTouchAudioSource::setStretchEnabled(bool enabled)
{
    stretchEnabled = enabled;
    setPitchSemiTones(currentSemitones);
    setTempoMultiplier(currentTempoMultiplier);
    soundTouch.clear();
}

void SoundTouchAudioSource::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (source == nullptr || !stretchEnabled || (currentSemitones == 0.0 && currentTempoMultiplier == 1.0))
    {
        if (source != nullptr) source->getNextAudioBlock(bufferToFill);
        else bufferToFill.clearActiveBufferRegion();
        return;
    }

    int numSamplesNeeded = bufferToFill.numSamples;
    int samplesOutput = 0; // Contador de cuántos samples hemos sacado del horno

    float* outL = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
    float* outR = bufferToFill.buffer->getNumChannels() > 1 ? bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample) : outL;

    // {* NUEVO: Bucle que alimenta a SoundTouch hasta que llene la cuota de JUCE *}
    while (samplesOutput < numSamplesNeeded)
    {
        // 1. Si SoundTouch ya cocinó samples, los sacamos
        if (soundTouch.numSamples() > 0)
        {
            int samplesToGet = juce::jmin(numSamplesNeeded - samplesOutput, (int)soundTouch.numSamples());
            int received = soundTouch.receiveSamples(interleavedBuffer.getData(), samplesToGet);

            for (int i = 0; i < received; ++i)
            {
                outL[samplesOutput + i] = interleavedBuffer[i * 2];
                if (bufferToFill.buffer->getNumChannels() > 1)
                    outR[samplesOutput + i] = interleavedBuffer[i * 2 + 1];
            }
            samplesOutput += received;
        }

        // 2. Si todavía faltan samples, sacamos más masa (audio original) y la metemos a SoundTouch
        if (samplesOutput < numSamplesNeeded)
        {
            sourceBuffer.clear();
            juce::AudioSourceChannelInfo info(&sourceBuffer, 0, bufferSize);
            source->getNextAudioBlock(info);

            const float* inL = sourceBuffer.getReadPointer(0);
            const float* inR = sourceBuffer.getNumChannels() > 1 ? sourceBuffer.getReadPointer(1) : inL;

            for (int i = 0; i < bufferSize; ++i)
            {
                interleavedBuffer[i * 2] = inL[i];
                interleavedBuffer[i * 2 + 1] = inR[i];
            }

            soundTouch.putSamples(interleavedBuffer.getData(), bufferSize);
        }
    }
}