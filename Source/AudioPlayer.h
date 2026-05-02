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
    void setTempo(double multiplier);
    void setStretch(bool enabled);

    // {* NUEVO: Control total sobre el MIDI *}
    bool getIsMidiLoaded() const { return isMidiLoaded; }
    double getMidiPosition() const { return currentMidiTime; }
    double getMidiLength() const { return midiSequence.getEndTime(); }
    void setMidiPosition(double newPos) { currentMidiTime = newPos; synth.allNotesOff(0, false); }
    // {* NUEVO: Permitimos que la pantalla lea las notas *}
    const juce::MidiMessageSequence* getMidiSequence() const { return &midiSequence; }

    juce::AudioTransportSource& getTransportSource() { return transportSource; }
    juce::AudioFormatManager& getFormatManager() { return formatManager; }

private:
    void updateProcessing();

    juce::AudioFormatManager formatManager;
    juce::AudioTransportSource transportSource;
    SoundTouchAudioSource soundTouchSource{ &transportSource };
    juce::ResamplingAudioSource resamplerSource{ &soundTouchSource, false, 2 };
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;

    // {* NUEVO: Variables del Mini-Sintetizador *}
    juce::Synthesiser synth;
    juce::MidiMessageSequence midiSequence;
    double currentMidiTime = 0.0;
    double currentSampleRate = 44100.0;
    bool isMidiLoaded = false;
    bool isPlayingMidi = false;

    bool isLooping = false;
    bool isStretchEnabled = false;
    double currentPitchSemitones = 0.0;
    double currentTempoMultiplier = 1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPlayer)
};