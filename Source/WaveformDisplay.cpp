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

void WaveformDisplay::setMidiMode(bool isMidi)
{
    isMidiMode = isMidi;
    repaint();
}

void WaveformDisplay::updateMidiPosition(double position, double totalLength)
{
    currentMidiPos = position;
    totalMidiLength = totalLength > 0.0 ? totalLength : 1.0;
    repaint();
}

void WaveformDisplay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));

    if (isMidiMode)
    {
        // 1. Dibujamos una Rejilla de Piano Roll elegante
        g.setColour(juce::Colour(0xff2c313a));
        for (int i = 0; i < getWidth(); i += 25)
            g.drawLine(i, 0, i, getHeight(), 1.0f);

        // 2. Título de Fondo
        g.setColour(juce::Colour(0xff56b6c2));
        g.setFont(18.0f);
        g.drawText("🎹 SECUENCIA MIDI", getLocalBounds(), juce::Justification::centred, true);

        // 3. Barra de Progreso
        float proportion = (float)(currentMidiPos / totalMidiLength);
        float drawPosition = proportion * getWidth();
        g.setColour(juce::Colours::white);
        g.drawLine(drawPosition, 0.0f, drawPosition, (float)getHeight(), 2.0f);
    }
    else if (fileLoaded)
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
    if (isMidiMode && onMidiScrub != nullptr) {
        double clickProportion = juce::jlimit(0.0, 1.0, (double)e.x / (double)getWidth());
        onMidiScrub(clickProportion * totalMidiLength);
        return;
    }

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
    if (isMidiMode && onMidiScrub != nullptr) {
        double clickProportion = juce::jlimit(0.0, 1.0, (double)e.x / (double)getWidth());
        onMidiScrub(clickProportion * totalMidiLength);
        return;
    }

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

