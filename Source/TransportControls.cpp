#include "TransportControls.h"

TransportControls::TransportControls(juce::AudioTransportSource& transportToUse)
    : transportSource(transportToUse)
{
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(loopButton);
    addAndMakeVisible(stretchButton);
    addAndMakeVisible(randomButton);
    addAndMakeVisible(volumeSlider);
    addAndMakeVisible(volumeLabel);
    
    // Agregamos el Pitch a la interfaz
    addAndMakeVisible(pitchSlider);
    addAndMakeVisible(pitchLabel);

    addAndMakeVisible(tempoSlider);
    addAndMakeVisible(tempoLabel);

    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgreen);
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
    randomButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkorange);
    
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.setRange(0.0, 1.0); 
    volumeSlider.setValue(0.7);      
    transportSource.setGain(0.7f);

    volumeLabel.setText("Vol:", juce::dontSendNotification);
    volumeLabel.attachToComponent(&volumeSlider, true);

    // Configuración del Pitch Slider
    pitchSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    pitchSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 20);
    pitchSlider.setRange(-12.0, 12.0, 1.0);
    pitchSlider.setValue(0.0);

    pitchLabel.setText("Pitch:", juce::dontSendNotification);
    pitchLabel.attachToComponent(&pitchSlider, true);

	//tempo slider
    // --- Configuración del Tempo en Porcentaje (%) ---
    tempoSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    tempoSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20); // Un poco más ancho para que quepa el %
    tempoSlider.setRange(-50.0, 100.0, 1.0); // Rango desde -50% (mitad de velocidad) hasta +100% (doble velocidad)
    tempoSlider.setValue(0.0); // 0% es la velocidad original
    tempoSlider.setTextValueSuffix("%"); // Magia de JUCE: Le agrega el símbolo de % automáticamente al número

    tempoLabel.setText("Speed:", juce::dontSendNotification);
    tempoLabel.attachToComponent(&tempoSlider, true);

    tempoSlider.onValueChange = [this]
        {
            if (onTempoChanged != nullptr)
            {
                // Convertimos el porcentaje visual de nuevo al multiplicador matemático que requiere el motor
                // Ej: Si el slider marca +1%, la matemática hace: 1.0 + (1.0 / 100.0) = 1.01
                // Ej: Si el slider marca -50%, la matemática hace: 1.0 + (-50.0 / 100.0) = 0.5
                double percentage = tempoSlider.getValue();
                double multiplier = 1.0 + (percentage / 100.0);

                onTempoChanged(multiplier);
            }
        };

    // --- ACCIONES DE LOS BOTONES ---
    playButton.onClick = [this] { transportSource.start(); };
    
    stopButton.onClick = [this] 
    { 
        transportSource.stop(); 
        transportSource.setPosition(0.0); 
    };

    volumeSlider.onValueChange = [this] { transportSource.setGain((float)volumeSlider.getValue()); };

    loopButton.onClick = [this] 
    { 
        if (onLoopChanged != nullptr)
            onLoopChanged(loopButton.getToggleState()); 
    };

    randomButton.onClick = [this]
    {
        if (onRandomClicked != nullptr)
            onRandomClicked();
    };

    pitchSlider.onValueChange = [this]
    {
        if (onPitchChanged != nullptr)
            onPitchChanged(pitchSlider.getValue());
    };
    
    stretchButton.onClick = [this]
    {
        if (onStretchChanged != nullptr)
            onStretchChanged(stretchButton.getToggleState());
    };
}

TransportControls::~TransportControls() {}

void TransportControls::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff222222)); 
}

void TransportControls::resized()
{
    auto area = getLocalBounds().reduced(5);

    // Botones base
    playButton.setBounds(area.removeFromLeft(60).reduced(2));
    stopButton.setBounds(area.removeFromLeft(60).reduced(2));
    loopButton.setBounds(area.removeFromLeft(60).reduced(2));

    // Estos dos necesitan más ancho para que la palabra no se aplaste
    stretchButton.setBounds(area.removeFromLeft(75).reduced(2));
    randomButton.setBounds(area.removeFromLeft(80).reduced(2));

    // Margen GIGANTE de 40 píxeles para que quepa la etiqueta "Vol:" sin chocar
    area.removeFromLeft(40);

    // Dividimos el espacio sobrante entre los 3 sliders
    int sliderWidth = area.getWidth() / 3;
    volumeSlider.setBounds(area.removeFromLeft(sliderWidth).reduced(2));

    area.removeFromLeft(20); // Margen para la etiqueta "Pitch:"
    pitchSlider.setBounds(area.removeFromLeft(sliderWidth).reduced(2));

    area.removeFromLeft(20); // Margen para la etiqueta "Speed:"
    tempoSlider.setBounds(area.reduced(2));
}

bool TransportControls::isLoopEnabled() const
{
    return loopButton.getToggleState();
}

bool TransportControls::isStretchEnabled() const
{
    return stretchButton.getToggleState();
}