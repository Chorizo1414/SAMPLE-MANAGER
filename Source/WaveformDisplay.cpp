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

// {* NUEVO: Lógica de salto en el tiempo (Seeking) *}
void WaveformDisplay::mouseDown(const juce::MouseEvent& e)
{
    // Solo hacemos el salto si hay un archivo cargado y tiene duración
    if (fileLoaded && transportSource.getLengthInSeconds() > 0.0)
    {
        // 1. Calculamos en qué porcentaje del ancho total hicimos clic
        double clickProportion = (double)e.x / (double)getWidth();

        // 2. Nos aseguramos de que el clic no se salga del 0% al 100% (evita errores)
        clickProportion = juce::jlimit(0.0, 1.0, clickProportion);

        // 3. Multiplicamos el porcentaje por la duración de la canción
        double newPosition = clickProportion * transportSource.getLengthInSeconds();

        // 4. Movemos la barra de reproducción a ese segundo exacto
        transportSource.setPosition(newPosition);
        repaint(); // Actualizamos la línea blanca al instante
    }
}

void WaveformDisplay::mouseDrag(const juce::MouseEvent& e)
{
    // Hacemos exactamente lo mismo por si el usuario mantiene presionado el clic y arrastra
    if (fileLoaded && transportSource.getLengthInSeconds() > 0.0)
    {
        double dragProportion = (double)e.x / (double)getWidth();
        dragProportion = juce::jlimit(0.0, 1.0, dragProportion);
        double newPosition = dragProportion * transportSource.getLengthInSeconds();

        transportSource.setPosition(newPosition);
        repaint();
    }
}