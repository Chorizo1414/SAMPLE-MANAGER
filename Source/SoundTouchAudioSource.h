#pragma once
#include <JuceHeader.h>
#include "SoundTouch/SoundTouch.h" 

class SoundTouchAudioSource : public juce::AudioSource
{
public:
    SoundTouchAudioSource(juce::AudioSource* inputSource);
    ~SoundTouchAudioSource() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    void setPitchSemiTones(double semitones);
    void setTempoMultiplier(double multiplier);
    void setStretchEnabled(bool enabled);

private:
    juce::AudioSource* source = nullptr;
    soundtouch::SoundTouch soundTouch;

    bool stretchEnabled = false;
    double currentSemitones = 0.0;
    double currentTempoMultiplier = 1.0;

    // {* NUEVO: Buffers de memoria pre-asignados para optimización extrema *}
    juce::AudioBuffer<float> sourceBuffer;
    juce::HeapBlock<float> interleavedBuffer;
    int bufferSize = 0;
};