#pragma once
#include <JuceHeader.h>

class WaveformDisplay : public juce::Component, public juce::ChangeListener, public juce::Timer 
{
public:
    WaveformDisplay(juce::AudioFormatManager& formatManagerToUse, juce::AudioTransportSource& transportToUse); 
    ~WaveformDisplay() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void setAudioFile(const juce::File& file);

    void timerCallback() override; 
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    juce::AudioThumbnailCache thumbnailCache;
    juce::AudioThumbnail thumbnail;
    bool fileLoaded = false;

    juce::AudioTransportSource& transportSource; 

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};