#pragma once
#include <JuceHeader.h>
#include <functional>

class TransportControls : public juce::Component
{
public:
    explicit TransportControls(juce::AudioTransportSource& transportToUse);
    ~TransportControls() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    bool isLoopEnabled() const;
    bool isStretchEnabled() const;

    // Los valores musicales se presentan a partir del análisis del sample.
    // setSourceMetadata reinicia transposicion y velocidad al cambiar de archivo.
    void setSourceMetadata(const juce::String& bpm, const juce::String& key);
    // Cuando finaliza el analisis en segundo plano, conserva cualquier ajuste hecho.
    void updateDetectedMetadata(const juce::String& bpm, const juce::String& key);

    std::function<void(bool)> onLoopChanged;
    std::function<void()> onPlayClicked;
    std::function<void()> onStopClicked;
    std::function<void()> onRandomClicked;
    std::function<void(double)> onPitchChanged;
    std::function<void(bool)> onStretchChanged;
    std::function<void(double)> onTempoChanged;

private:
    static bool parseKey(const juce::String& text, int& pitchClass, bool& minor);
    static juce::String keyName(int pitchClass, bool minor);
    void updateMetadata(const juce::String& bpm, const juce::String& key);
    void refreshMusicalControls();
    void showAdvanced(bool enabled);
    void ensureIndependentStretch();
    void applyPitch(double semitones, bool musicalInput);
    void applyTempo(double multiplier, bool musicalInput);
    void commitTargetBpm();

    juce::AudioTransportSource& transportSource;

    juce::TextButton playButton{ "Play" };
    juce::TextButton stopButton{ "Stop" };
    juce::ToggleButton loopButton{ "Loop" };
    juce::ToggleButton stretchButton{ "Stretch" };
    juce::TextButton randomButton{ "Random" };

    juce::Slider volumeSlider;
    juce::Label volumeLabel;

    juce::Slider pitchSlider;  // Ajuste fino en semitonos
    juce::Slider tempoSlider;  // Ajuste fino de velocidad en %
    juce::Label pitchLabel, tempoLabel;
    juce::Label sourceKeyLabel, sourceBpmLabel;
    juce::ComboBox targetKeyBox;
    juce::TextEditor targetBpmEditor;
    juce::TextButton advancedButton{ "Fine" };

    bool advancedMode = false;
    bool syncingControls = false;
    double originalBpm = 0.0, pitchOffset = 0.0, tempoFactor = 1.0;
    int originalPitchClass = -1;
    bool originalMinor = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportControls)
};
