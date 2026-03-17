#include "WaveformDisplay.h"

WaveformDisplay::WaveformDisplay(juce::AudioFormatManager& formatManagerToUse, juce::AudioTransportSource& transportToUse) 
    : thumbnailCache(5),
    thumbnail(512, formatManagerToUse, thumbnailCache),
    transportSource(transportToUse) 
{
    thumbnail.addChangeListener(this);
    startTimerHz(30); 
}

WaveformDisplay::~WaveformDisplay() {}

void WaveformDisplay::setAudioFile(const juce::File& file)
{
    thumbnail.setSource(new juce::FileInputSource(file));
    fileLoaded = true;
    repaint();
}

void WaveformDisplay::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &thumbnail)
        repaint();
}

void WaveformDisplay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));

    if (fileLoaded)
    {
        g.setColour(juce::Colours::cyan);
        thumbnail.drawChannels(g, getLocalBounds().reduced(2), 0.0, thumbnail.getTotalLength(), 1.0f);

        double length = transportSource.getLengthInSeconds();
        if (length > 0.0)
        {
            double position = transportSource.getCurrentPosition();
            float proportion = (float)(position / length);
            float drawPosition = proportion * getWidth();

            g.setColour(juce::Colours::white);
            g.drawLine(drawPosition, 0.0f, drawPosition, (float)getHeight(), 2.0f);
        }
    }
    else
    {
        g.setColour(juce::Colours::grey);
        g.setFont(14.0f);
        g.drawText("Selecciona un audio...", getLocalBounds(), juce::Justification::centred, true);
    }
}

void WaveformDisplay::resized() {}

void WaveformDisplay::timerCallback() 
{ 
    if (transportSource.isPlaying()) 
        repaint(); 
}