#pragma once
#include <JuceHeader.h>
#include "SoundTouchAudioSource.h" 

class AudioPlayer
{
public:
    AudioPlayer();
    ~AudioPlayer();

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill);
    void releaseResources();

    void loadFile(const juce::File& audioFile);
    void play();
    void stop();
    void setLooping(bool shouldLoop);
    void setVolume(float volume);

    void setPitch(double semitones);
    void setTempo(double multiplier); // <-- Nuevo
    void setStretch(bool enabled);

    juce::AudioTransportSource& getTransportSource() { return transportSource; }
    juce::AudioFormatManager& getFormatManager() { return formatManager; }

private:
    void updateProcessing();

    juce::AudioFormatManager formatManager;
    juce::AudioTransportSource transportSource;
    SoundTouchAudioSource soundTouchSource{ &transportSource };
    juce::ResamplingAudioSource resamplerSource{ &soundTouchSource, false, 2 };
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;

    bool isLooping = false;
    bool isStretchEnabled = false;
    double currentPitchSemitones = 0.0;
    double currentTempoMultiplier = 1.0; // <-- Nuevo

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPlayer)
};