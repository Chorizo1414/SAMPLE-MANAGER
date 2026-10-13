#include "TransportControls.h"
#include <cmath>

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
    addAndMakeVisible(pitchLabel);
    addAndMakeVisible(tempoLabel);
    addAndMakeVisible(sourceKeyLabel);
    addAndMakeVisible(sourceBpmLabel);
    addAndMakeVisible(targetKeyBox);
    addAndMakeVisible(targetBpmEditor);
    addAndMakeVisible(advancedButton);
    addChildComponent(pitchSlider);
    addChildComponent(tempoSlider);

    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgreen);
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
    randomButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkorange);

    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.setRange(0.0, 1.0);
    volumeSlider.setValue(0.7);
    volumeSlider.setDoubleClickReturnValue(true, 1.0);
    transportSource.setGain(0.7f);
    volumeLabel.setText("Vol:", juce::dontSendNotification);
    volumeLabel.attachToComponent(&volumeSlider, true);
    volumeSlider.onValueChange = [this] { transportSource.setGain((float) volumeSlider.getValue()); };

    pitchLabel.setText("Key:", juce::dontSendNotification);
    tempoLabel.setText("BPM:", juce::dontSendNotification);
    sourceKeyLabel.setJustificationType(juce::Justification::centred);
    sourceBpmLabel.setJustificationType(juce::Justification::centred);
    sourceKeyLabel.setColour(juce::Label::textColourId, juce::Colour(0xffabb2bf));
    sourceBpmLabel.setColour(juce::Label::textColourId, juce::Colour(0xffabb2bf));
    sourceKeyLabel.setFont(juce::Font(12.0f));
    sourceBpmLabel.setFont(juce::Font(12.0f));

    static const char* notes[] = { "C", "C#", "D", "D#", "E", "F",
                                   "F#", "G", "G#", "A", "A#", "B" };
    for (int i = 0; i < 12; ++i)
    {
        targetKeyBox.addItem(juce::String(notes[i]) + " Maj", i + 1);
        targetKeyBox.addItem(juce::String(notes[i]) + "m", i + 13);
    }
    targetKeyBox.setTooltip("Elige una nota de destino con el mismo modo (mayor o menor).");
    targetKeyBox.onChange = [this]
    {
        if (syncingControls || originalPitchClass < 0 || targetKeyBox.getSelectedId() == 0)
            return;
        const int selected = targetKeyBox.getSelectedId() - 1;
        const bool minor = selected >= 12;
        if (minor != originalMinor)
        {
            // Transponer no convierte una escala mayor en menor ni al contrario.
            refreshMusicalControls();
            return;
        }
        int semitones = (selected % 12) - originalPitchClass;
        if (semitones > 6) semitones -= 12;
        if (semitones < -6) semitones += 12;
        applyPitch(static_cast<double>(semitones), true);
    };

    targetBpmEditor.setInputRestrictions(6, "0123456789.");
    targetBpmEditor.setJustification(juce::Justification::centred);
    targetBpmEditor.setSelectAllWhenFocused(true);
    targetBpmEditor.setTooltip("Escribe el BPM deseado y pulsa Enter. Rango: 50%-200% del original.");
    targetBpmEditor.onReturnKey = [this]
    {
        commitTargetBpm();
        targetBpmEditor.giveAwayKeyboardFocus();
    };
    targetBpmEditor.onFocusLost = [this] { commitTargetBpm(); };

    pitchSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    pitchSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 20);
    pitchSlider.setRange(-12.0, 12.0, 1.0);
    pitchSlider.setDoubleClickReturnValue(true, 0.0);
    pitchSlider.onValueChange = [this]
    {
        if (!syncingControls)
            applyPitch(pitchSlider.getValue(), false);
    };

    tempoSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    tempoSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 48, 20);
    tempoSlider.setRange(-50.0, 100.0, 1.0);
    tempoSlider.setTextValueSuffix("%");
    tempoSlider.setDoubleClickReturnValue(true, 0.0);
    tempoSlider.onValueChange = [this]
    {
        if (!syncingControls)
            applyTempo(1.0 + tempoSlider.getValue() / 100.0, false);
    };

    advancedButton.setTooltip("Fine: ajustes manuales de semitonos y porcentaje; util si falta BPM o Key.");
    advancedButton.onClick = [this] { showAdvanced(!advancedMode); };

    playButton.onClick = [this]
    {
        if (onPlayClicked) onPlayClicked();
        else
        {
            if (transportSource.hasStreamFinished()
                || transportSource.getCurrentPosition() >= transportSource.getLengthInSeconds())
                transportSource.setPosition(0.0);
            transportSource.start();
        }
    };
    stopButton.onClick = [this]
    {
        if (onStopClicked) onStopClicked();
        else { transportSource.stop(); transportSource.setPosition(0.0); }
    };
    loopButton.onClick = [this]
    {
        if (onLoopChanged) onLoopChanged(loopButton.getToggleState());
    };
    randomButton.onClick = [this] { if (onRandomClicked) onRandomClicked(); };
    stretchButton.setTooltip("Stretch ON mantiene pitch y tempo independientes; OFF = modo vinilo.");
    stretchButton.onClick = [this]
    {
        if (onStretchChanged) onStretchChanged(stretchButton.getToggleState());
    };
    refreshMusicalControls();
    showAdvanced(false);
}

TransportControls::~TransportControls() {}

bool TransportControls::parseKey(const juce::String& text, int& pitchClass, bool& minor)
{
    juce::String cleaned = text.trim().toLowerCase().removeCharacters(" \t_-");
    if (cleaned.isEmpty() || cleaned == "--") return false;

    static const char* notes = "cdefgab";
    const int idx = juce::String(notes).indexOfChar(cleaned[0]);
    if (idx < 0) return false;
    static const int semitones[] = { 0, 2, 4, 5, 7, 9, 11 };
    int note = semitones[idx];
    int consumed = 1;
    if (cleaned.length() > 1 && cleaned[1] == '#') { ++note; ++consumed; }
    else if (cleaned.length() > 1 && cleaned[1] == 'b') { --note; ++consumed; }

    const auto suffix = cleaned.substring(consumed);
    if (suffix == "min" || suffix == "minor" || suffix == "m") minor = true;
    else if (suffix == "maj" || suffix == "major") minor = false;
    else return false;

    pitchClass = (note + 12) % 12;
    return true;
}

juce::String TransportControls::keyName(int pitchClass, bool minor)
{
    static const char* notes[] = { "C", "C#", "D", "D#", "E", "F",
                                   "F#", "G", "G#", "A", "A#", "B" };
    return juce::String(notes[juce::jlimit(0, 11, pitchClass)])
           + (minor ? "m" : " Maj");
}

void TransportControls::updateMetadata(const juce::String& bpm, const juce::String& key)
{
    const double parsed = bpm.trim().getDoubleValue();
    originalBpm = std::isfinite(parsed) && parsed > 0.0 ? parsed : 0.0;

    int parsedPitch = -1;
    bool parsedMinor = false;
    if (parseKey(key, parsedPitch, parsedMinor))
    {
        originalPitchClass = parsedPitch;
        originalMinor = parsedMinor;
    }
    else
        originalPitchClass = -1;
    refreshMusicalControls();
}

void TransportControls::setSourceMetadata(const juce::String& bpm, const juce::String& key)
{
    pitchOffset = 0.0;
    tempoFactor = 1.0;
    updateMetadata(bpm, key);
    if (onPitchChanged) onPitchChanged(0.0);
    if (onTempoChanged) onTempoChanged(1.0);
}

void TransportControls::updateDetectedMetadata(const juce::String& bpm, const juce::String& key)
{
    // El analizador termina cuando quiere: no eliminar transposiciones elegidas durante la escucha.
    updateMetadata(bpm, key);
}

void TransportControls::ensureIndependentStretch()
{
    // El modo musical nunca altera accidentalmente BPM al elegir Key, ni Key al elegir BPM.
    if (!stretchButton.getToggleState())
    {
        stretchButton.setToggleState(true, juce::dontSendNotification);
        if (onStretchChanged) onStretchChanged(true);
    }
}

void TransportControls::applyPitch(double semitones, bool musicalInput)
{
    pitchOffset = juce::jlimit(-12.0, 12.0, semitones);
    if (musicalInput) ensureIndependentStretch();
    if (onPitchChanged) onPitchChanged(pitchOffset);
    refreshMusicalControls();
}

void TransportControls::applyTempo(double multiplier, bool musicalInput)
{
    tempoFactor = juce::jlimit(0.5, 2.0, multiplier);
    if (musicalInput) ensureIndependentStretch();
    if (onTempoChanged) onTempoChanged(tempoFactor);
    refreshMusicalControls();
}

void TransportControls::commitTargetBpm()
{
    if (syncingControls || originalBpm <= 0.0) return;
    const double entered = targetBpmEditor.getText().trim().getDoubleValue();
    if (std::isfinite(entered) && entered >= originalBpm * 0.5
        && entered <= originalBpm * 2.0)
        applyTempo(entered / originalBpm, true);
    else
        refreshMusicalControls(); // Revertir una entrada no valida, sin bloquear la interfaz.
}

void TransportControls::refreshMusicalControls()
{
    syncingControls = true;
    sourceKeyLabel.setText(originalPitchClass >= 0
        ? keyName(originalPitchClass, originalMinor) + ">" : "-->", juce::dontSendNotification);
    const bool knownKey = originalPitchClass >= 0;
    targetKeyBox.setEnabled(knownKey);
    for (int i = 1; i <= 24; ++i)
        targetKeyBox.setItemEnabled(i, knownKey && ((i > 12) == originalMinor));

    if (knownKey)
    {
        const int targetPitch = (originalPitchClass + juce::roundToInt(pitchOffset) + 120) % 12;
        targetKeyBox.setSelectedId(targetPitch + 1 + (originalMinor ? 12 : 0),
                                   juce::dontSendNotification);
    }
    else
        targetKeyBox.setSelectedId(0, juce::dontSendNotification);

    sourceBpmLabel.setText(originalBpm > 0.0
        ? juce::String(originalBpm, (std::abs(originalBpm - std::round(originalBpm)) < 0.01) ? 0 : 1) + ">"
        : "-->", juce::dontSendNotification);
    targetBpmEditor.setEnabled(originalBpm > 0.0);
    const double targetBpm = originalBpm * tempoFactor;
    targetBpmEditor.setText(originalBpm > 0.0
        ? juce::String(targetBpm, (std::abs(targetBpm - std::round(targetBpm)) < 0.01) ? 0 : 1)
        : "--", false);

    pitchSlider.setValue(pitchOffset, juce::dontSendNotification);
    tempoSlider.setValue((tempoFactor - 1.0) * 100.0, juce::dontSendNotification);
    syncingControls = false;
}

void TransportControls::showAdvanced(bool enabled)
{
    advancedMode = enabled;
    sourceKeyLabel.setVisible(!enabled);
    sourceBpmLabel.setVisible(!enabled);
    targetKeyBox.setVisible(!enabled);
    targetBpmEditor.setVisible(!enabled);
    pitchSlider.setVisible(enabled);
    tempoSlider.setVisible(enabled);
    pitchLabel.setText(enabled ? "Pitch:" : "Key:", juce::dontSendNotification);
    tempoLabel.setText(enabled ? "Speed:" : "BPM:", juce::dontSendNotification);
    advancedButton.setButtonText(enabled ? "Keys" : "Fine");
}

void TransportControls::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff222222));
}

void TransportControls::resized()
{
    auto area = getLocalBounds().reduced(4);
    playButton.setBounds(area.removeFromLeft(60).reduced(2));
    stopButton.setBounds(area.removeFromLeft(60).reduced(2));
    loopButton.setBounds(area.removeFromLeft(60).reduced(2));
    stretchButton.setBounds(area.removeFromLeft(75).reduced(2));
    randomButton.setBounds(area.removeFromLeft(80).reduced(2));

    area.removeFromLeft(34);
    volumeSlider.setBounds(area.removeFromLeft(90).reduced(2));

    auto pitchArea = area.removeFromLeft(178);
    pitchLabel.setBounds(pitchArea.removeFromLeft(35));
    sourceKeyLabel.setBounds(pitchArea.removeFromLeft(46));
    targetKeyBox.setBounds(pitchArea.reduced(1));
    pitchSlider.setBounds(getLocalBounds().withLeft(pitchLabel.getRight())
                          .withRight(targetKeyBox.getRight()).withTop(2).withHeight(getHeight() - 4));

    auto tempoArea = area.removeFromLeft(155);
    tempoLabel.setBounds(tempoArea.removeFromLeft(38));
    sourceBpmLabel.setBounds(tempoArea.removeFromLeft(52));
    targetBpmEditor.setBounds(tempoArea.reduced(1));
    tempoSlider.setBounds(getLocalBounds().withLeft(tempoLabel.getRight())
                          .withRight(targetBpmEditor.getRight()).withTop(2).withHeight(getHeight() - 4));

    advancedButton.setBounds(area.removeFromLeft(52).reduced(2));
}

bool TransportControls::isLoopEnabled() const { return loopButton.getToggleState(); }
bool TransportControls::isStretchEnabled() const { return stretchButton.getToggleState(); }
