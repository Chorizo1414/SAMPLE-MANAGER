#include "AudioPlayer.h"

// ==============================================================================
// {* NUEVO: LA FÍSICA DEL PIANO ELÉCTRICO *}
// 1. El sonido (Acepta todas las notas)
struct SineWaveSound : public juce::SynthesiserSound
{
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

// 2. La voz (Genera la onda senoidal suave con caída tipo piano)
class SineWaveVoice : public juce::SynthesiserVoice
{
public:
    SineWaveVoice() {
        // Attack 0.01s, Decay 0.5s, Sustain 0.2, Release 1.0s
        adsr.setParameters({ 0.01f, 0.5f, 0.2f, 1.0f });
    }

    bool canPlaySound(juce::SynthesiserSound* sound) override {
        return dynamic_cast<SineWaveSound*>(sound) != nullptr;
    }

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override {
        level = velocity * 0.25f; // Volumen suave
        frequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        currentAngle = 0.0;
        angleDelta = (frequency / getSampleRate()) * 2.0 * juce::MathConstants<double>::pi;
        adsr.noteOn();
    }

    void stopNote(float velocity, bool allowTailOff) override {
        adsr.noteOff();
        if (!allowTailOff) { clearCurrentNote(); angleDelta = 0.0; }
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override {
        if (!isVoiceActive()) return;

        while (--numSamples >= 0) {
            float currentSample = (float)(std::sin(currentAngle) * level * adsr.getNextSample());
            
            for (int i = outputBuffer.getNumChannels(); --i >= 0;)
                outputBuffer.addSample(i, startSample, currentSample);

            currentAngle += angleDelta;
            ++startSample;

            if (!adsr.isActive()) { clearCurrentNote(); angleDelta = 0.0; break; }
        }
    }

private:
    double currentAngle = 0.0, angleDelta = 0.0, level = 0.0, frequency = 0.0;
    juce::ADSR adsr;
};
// ==============================================================================

AudioPlayer::AudioPlayer()
{
    formatManager.registerBasicFormats();

    // Le agregamos 8 voces de polifonía a nuestro sinte (puede tocar 8 teclas a la vez)
    for (int i = 0; i < 8; ++i)
        synth.addVoice(new SineWaveVoice());
    
    synth.addSound(new SineWaveSound());
}

AudioPlayer::~AudioPlayer()
{
    transportSource.setSource(nullptr);
}

void AudioPlayer::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    synth.setCurrentPlaybackSampleRate(sampleRate);
    resamplerSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void AudioPlayer::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // {* NUEVO: SI ES MIDI, RENDERIZAMOS EL SINTETIZADOR *}
    if (isMidiLoaded)
    {
        auto* buffer = bufferToFill.buffer;
        buffer->clear(bufferToFill.startSample, bufferToFill.numSamples);

        if (isPlayingMidi)
        {
            juce::MidiBuffer midiBuffer;

            // 1. SPEED FIX: Aceleramos o ralentizamos el tiempo
            double speed = currentTempoMultiplier;
            double timePerSample = 1.0 / currentSampleRate;
            double timeAdvance = bufferToFill.numSamples * timePerSample * speed;
            double endTime = currentMidiTime + timeAdvance;

            for (int i = 0; i < midiSequence.getNumEvents(); ++i)
            {
                auto* event = midiSequence.getEventPointer(i);
                double eventTime = event->message.getTimeStamp();

                if (eventTime >= currentMidiTime && eventTime < endTime)
                {
                    auto msg = event->message;

                    // 2. PITCH FIX: Transponemos las notas sumando semitonos
                    if (msg.isNoteOn() || msg.isNoteOff())
                    {
                        int newNote = msg.getNoteNumber() + (int)currentPitchSemitones;
                        newNote = juce::jlimit(0, 127, newNote); // Evitamos que la nota explote
                        msg.setNoteNumber(newNote);
                    }

                    int samplePos = bufferToFill.startSample + (int)((eventTime - currentMidiTime) * currentSampleRate / speed);
                    midiBuffer.addEvent(msg, samplePos);
                }
            }

            synth.renderNextBlock(*buffer, midiBuffer, bufferToFill.startSample, bufferToFill.numSamples);
            buffer->applyGain(bufferToFill.startSample, bufferToFill.numSamples, transportSource.getGain());

            currentMidiTime = endTime;

            if (currentMidiTime > midiSequence.getEndTime())
            {
                currentMidiTime = 0.0;
                if (!isLooping) isPlayingMidi = false;
            }
        }
        return;
    }

    // SI NO ES MIDI, TOCA EL AUDIO NORMAL
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
    // {* NUEVO: LECTOR DE PARTITURAS MIDI *}
    if (audioFile.hasFileExtension(".mid"))
    {
        juce::FileInputStream fileStream(audioFile);
        juce::MidiFile midiFile;
        if (midiFile.readFrom(fileStream))
        {
            midiFile.convertTimestampTicksToSeconds();
            midiSequence.clear();
            for (int i = 0; i < midiFile.getNumTracks(); ++i)
                midiSequence.addSequence(*midiFile.getTrack(i), 0.0);

            midiSequence.updateMatchedPairs(); // Une el NoteOn con su NoteOff
            
            isMidiLoaded = true;
            isPlayingMidi = false;
            currentMidiTime = 0.0;
            synth.allNotesOff(0, false);
            return; // Salimos para no intentar cargarlo como .WAV
        }
    }

    // SI NO ES MIDI, LO CARGAMOS NORMALMENTE
    isMidiLoaded = false;
    isPlayingMidi = false;

    auto* reader = formatManager.createReaderFor(audioFile);
    if (reader != nullptr)
    {
        std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
        newSource->setLooping(isLooping);

        transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
        readerSource.reset(newSource.release());

        updateProcessing();
    }
}

void AudioPlayer::play()
{
    if (isMidiLoaded)
    {
        if (currentMidiTime >= midiSequence.getEndTime()) currentMidiTime = 0.0;
        isPlayingMidi = true;
    }
    else
    {
        if (transportSource.hasStreamFinished() || transportSource.getCurrentPosition() >= transportSource.getLengthInSeconds())
            transportSource.setPosition(0.0);
        
        transportSource.start();
    }
}

void AudioPlayer::stop()
{
    if (isMidiLoaded)
    {
        isPlayingMidi = false;
        currentMidiTime = 0.0;
        synth.allNotesOff(0, true);
    }
    else
    {
        transportSource.stop();
        transportSource.setPosition(0.0);
    }
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

void AudioPlayer::updateProcessing()
{
    if (isStretchEnabled)
    {
        soundTouchSource.setPitchSemiTones(currentPitchSemitones);
        soundTouchSource.setTempoMultiplier(currentTempoMultiplier);
        resamplerSource.setResamplingRatio(1.0);
    }
    else
    {
        soundTouchSource.setPitchSemiTones(0.0);
        soundTouchSource.setTempoMultiplier(1.0);
        double pitchRatio = std::pow(2.0, currentPitchSemitones / 12.0);
        resamplerSource.setResamplingRatio(pitchRatio * currentTempoMultiplier);
    }
}