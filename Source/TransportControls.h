#pragma once
#include <JuceHeader.h>
#include <functional> 

class TransportControls : public juce::Component
{
public:
    TransportControls(juce::AudioTransportSource& transportToUse);
    ~TransportControls() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    bool isLoopEnabled() const;
    bool isStretchEnabled() const;

    std::function<void(bool)> onLoopChanged;
    std::function<void()> onPlayClicked;
    std::function<void()> onStopClicked;
    std::function<void()> onRandomClicked;
    std::function<void(double)> onPitchChanged; // Cable del pitch
    std::function<void(bool)> onStretchChanged;
    std::function<void(double)> onTempoChanged;

private:
    juce::AudioTransportSource& transportSource;

    juce::TextButton playButton{ "Play" };
    juce::TextButton stopButton{ "Stop" };
    juce::ToggleButton loopButton{ "Loop" };
    juce::ToggleButton stretchButton{ "Stretch" };
    juce::TextButton randomButton{ "Random" };
    
    juce::Slider volumeSlider;
    juce::Label volumeLabel;

    juce::Slider pitchSlider;
    juce::Label pitchLabel;

    juce::Slider tempoSlider;
    juce::Label tempoLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportControls)
};