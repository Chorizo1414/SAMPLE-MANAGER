#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    juce::LookAndFeel::setDefaultLookAndFeel(&myCustomTheme);

    addAndMakeVisible(directoryTree);
    addAndMakeVisible(fileList);

    // {* FASE 1: PALETA DE COLORES PREMIUM (Estilo Splice/Arcade) *}
    // Colores globales de la ventana y cajas de texto
    getLookAndFeel().setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff181a1f));
    getLookAndFeel().setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff121418));
    getLookAndFeel().setColour(juce::TextEditor::textColourId, juce::Colour(0xffabb2bf));
    getLookAndFeel().setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff2c313a));
    getLookAndFeel().setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff4faccc)); // Cyan al hacer clic

    // Colores del árbol de carpetas lateral (más claro para dar profundidad)
    directoryTree.setColour(juce::TreeView::backgroundColourId, juce::Colour(0xff1c1f26));
    directoryTree.setColour(juce::TreeView::linesColourId, juce::Colours::transparentBlack);

    directoryTree.addListener(this);
    databaseFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("SampleManager_BPM_Cache.xml");
    loadDatabase();
    fileList.setModel(this);
    fileList.addMouseListener(this, true);

    addAndMakeVisible(searchBar);
    searchBar.setTextToShowWhenEmpty("Buscar nombre...", juce::Colours::grey);
    searchBar.addListener(this);

    addAndMakeVisible(bpmBox);
    bpmBox.setTextToShowWhenEmpty("BPM...", juce::Colours::grey);
    bpmBox.addListener(this);

    addAndMakeVisible(bpmDisplayLabel);
    bpmDisplayLabel.setColour(juce::Label::textColourId, juce::Colour(0xff56b6c2));
    bpmDisplayLabel.setFont(juce::Font(14.0f, juce::Font::bold)); // Un poco más pequeña y sutil
    bpmDisplayLabel.setJustificationType(juce::Justification::centred); // Centrada en su nueva "píldora"
    bpmDisplayLabel.setText("Selecciona un audio...", juce::dontSendNotification);

    // {* NUEVO CÓDIGO: Configuramos los colores de los títulos *}
    addAndMakeVisible(headerName);
    headerName.setColour(juce::Label::textColourId, juce::Colour(0xff6b7280)); // Gris premium

    addAndMakeVisible(headerKey);
    headerKey.setColour(juce::Label::textColourId, juce::Colour(0xff6b7280));
    headerKey.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(headerBPM);
    headerBPM.setColour(juce::Label::textColourId, juce::Colour(0xff6b7280));
    headerBPM.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(keyBox);
    keyBox.addItem("TODAS LAS KEYS", 1); 

    juce::StringArray notas = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    int idCout = 2; 
    for (auto nota : notas)
    {
        keyBox.addItem(nota + " Maj", idCout++);
        keyBox.addItem(nota + " Min", idCout++);
    }

    keyBox.setSelectedId(1, juce::dontSendNotification);
    keyBox.addListener(this);

    addAndMakeVisible(typeBox);
    typeBox.addItem("TODOS", 1);
    typeBox.addItem("SOLO AUDIO", 2);
    typeBox.addItem("SOLO MIDI", 3);
    typeBox.setSelectedId(1, juce::dontSendNotification);
    typeBox.addListener(this);

    addAndMakeVisible(waveformDisplay);
    addAndMakeVisible(transportControls);

    // Conectamos los controles visuales con nuestro nuevo motor de audio
    transportControls.onLoopChanged = [this](bool shouldLoop) { audioPlayer.setLooping(shouldLoop); };
    transportControls.onPitchChanged = [this](double semitones) { audioPlayer.setPitch(semitones); };
    transportControls.onTempoChanged = [this](double multiplier) { audioPlayer.setTempo(multiplier); };
    transportControls.onStretchChanged = [this](bool enabled) { audioPlayer.setStretch(enabled); };

    transportControls.onRandomClicked = [this]()
    {
        if (filteredAudioFiles.size() > 0)
        {
            int randomIndex = juce::Random::getSystemRandom().nextInt(filteredAudioFiles.size());
            fileList.selectRow(randomIndex, false, true);

            juce::File selectedAudioFile = filteredAudioFiles[randomIndex];
            waveformDisplay.setAudioFile(selectedAudioFile);
            
            // Ya no hay código espagueti, le decimos al motor que cargue y toque
            audioPlayer.loadFile(selectedAudioFile);
            audioPlayer.play();
        }
    };

    directoryThread.startThread(juce::Thread::Priority::normal);
    directoryList.setDirectory(juce::File("D:\\"), true, true);

    fileList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff2b2b2b));

    setAudioChannels(0, 2);
    setSize(1180, 650);
}

MainComponent::~MainComponent()
{
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);

    shutdownAudio();
    directoryThread.stopThread(1000);
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    audioPlayer.prepareToPlay(samplesPerBlockExpected, sampleRate); 
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    audioPlayer.getNextAudioBlock(bufferToFill);                    
}

void MainComponent::releaseResources()
{
    audioPlayer.releaseResources();                                 
}

void MainComponent::paint(juce::Graphics& g)
{
    // 1. Fondo principal de la lista de samples (Oscuro profundo minimalista)
    g.fillAll(juce::Colour(0xff121419));

    // 2. Fondo del Header superior (Mismo color que las carpetas para unificarlos)
    g.setColour(juce::Colour(0xff1c1f26));
    g.fillRect(300, 0, getWidth() - 300, 80); // 80 píxeles de altura

    // 3. Dibujamos una "Píldora" moderna para el BPM y Key arriba a la derecha
    int badgeWidth = 240;
    int badgeHeight = 26;
    int badgeX = getWidth() - badgeWidth - 15;
    int badgeY = 15;

    g.setColour(juce::Colour(0xff121419)); // Fondo interior de la píldora
    g.fillRoundedRectangle(badgeX, badgeY, badgeWidth, badgeHeight, badgeHeight / 2.0f);
    g.setColour(juce::Colour(0xff2c313a)); // Borde sutil
    g.drawRoundedRectangle(badgeX, badgeY, badgeWidth, badgeHeight, badgeHeight / 2.0f, 1.0f);
}

void MainComponent::resized()
{
    directoryTree.setBounds(0, 0, 300, getHeight());

    int topMargin = 15;
    int badgeWidth = 240;

    // Matemáticas corregidas para que nada se encime:
    searchBar.setBounds(315, topMargin, 180, 26); // Más corto para dar espacio
    typeBox.setBounds(505, topMargin, 120, 26);   // Encaja perfecto después del buscador
    bpmBox.setBounds(635, topMargin, 70, 26);     // Empujado a la derecha
    keyBox.setBounds(715, topMargin, 140, 26);    // Empujado a la derecha

    bpmDisplayLabel.setBounds(getWidth() - badgeWidth - 15, topMargin, badgeWidth, 26);

    // Bajamos los títulos para que no se peguen a los buscadores
    int listY = 55;
    headerName.setBounds(315, listY, getWidth() - 300 - 150, 25);
    headerKey.setBounds(getWidth() - 130, listY, 60, 25);
    headerBPM.setBounds(getWidth() - 60, listY, 50, 25);

    // Ajustamos la lista a su nueva posición
    fileList.setBounds(300, listY + 25, getWidth() - 300, getHeight() - (listY + 25) - 130 - 45);

    waveformDisplay.setBounds(300, getHeight() - 175, getWidth() - 300, 130);
    transportControls.setBounds(300, getHeight() - 45, getWidth() - 300, 45);
}

void MainComponent::selectionChanged()
{
    audioFilesInFolder.clear();
    filteredAudioFiles.clear();
    juce::File selectedFolder = directoryTree.getSelectedFile();

    if (selectedFolder.isDirectory())
    {
        std::function<void(const juce::File&)> searchSubfolders = [&](const juce::File& folderToSearch)
        {
            // Le agregamos ;*.mid al final de la lista
            juce::Array<juce::File> filesFound = folderToSearch.findChildFiles(juce::File::findFiles, false, "*.wav;*.mp3;*.aif;*.mid");
            audioFilesInFolder.addArray(filesFound);

            juce::Array<juce::File> subfoldersFound = folderToSearch.findChildFiles(juce::File::findDirectories, false);
            for (auto& subfolder : subfoldersFound)
            {
                searchSubfolders(subfolder);
            }
        };

        searchSubfolders(selectedFolder);
        filteredAudioFiles = audioFilesInFolder;
        applyFilters();
    }

    searchBar.setText("", juce::dontSendNotification);
    fileList.updateContent();
}

int MainComponent::getNumRows() 
{ 
    return filteredAudioFiles.size();
} 

void MainComponent::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    // {* ZEBRA STRIPING: Líneas intercaladas y Selección Premium *}
    // {* ZEBRA STRIPING MINIMALISTA *}
    if (rowIsSelected)
    {
        g.fillAll(juce::Colour(0xff2c313a)); // Sombreado elegante al seleccionar
    }
    else if (rowNumber % 2 == 0)
    {
        g.fillAll(juce::Colour(0xff121419)); // Fila Par: Fondo oscuro profundo
    }
    else
    {
        g.fillAll(juce::Colour(0xff16191f)); // Fila Impar: Ligeramente más claro
    }

    if (rowNumber < filteredAudioFiles.size())
    {
        // 1. DIBUJAMOS EL NOMBRE (Añadimos margen de 15px para que respire)
        g.setColour(juce::Colour(0xffabb2bf));
        g.setFont(14.0f);
        g.drawText(filteredAudioFiles[rowNumber].getFileName(), 15, 0, width - 160, height, juce::Justification::centredLeft, true);

        // 2. DIBUJAMOS LA KEY
        g.setColour(juce::Colour(0xff5c6370));
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText(filteredKeys[rowNumber], width - 130, 0, 60, height, juce::Justification::centred, true);

        // 3. DIBUJAMOS EL BPM
        g.setColour(juce::Colour(0xff56b6c2));
        g.drawText(filteredBPMs[rowNumber], width - 60, 0, 50, height, juce::Justification::centred, true);
    }
}

void MainComponent::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    if (fileList.getSelectedRow() == row)
    {
        juce::File selectedAudioFile = filteredAudioFiles[row];
        if (!selectedAudioFile.hasFileExtension(".mid")) // <--- PROTECCIÓN MIDI
        {
            audioPlayer.loadFile(selectedAudioFile);
            audioPlayer.play();
        }
    }
}

void MainComponent::selectedRowsChanged(int lastRowSelected)
{
    if (lastRowSelected >= 0 && lastRowSelected < filteredAudioFiles.size())
    {
        juce::File selectedAudioFile = filteredAudioFiles[lastRowSelected];
        waveformDisplay.setAudioFile(selectedAudioFile);
        analyzeBPM(selectedAudioFile);

        // SOLO se reproduce si NO es MIDI
        if (!selectedAudioFile.hasFileExtension(".mid"))
        {
            audioPlayer.loadFile(selectedAudioFile);
            audioPlayer.play();
        }
    }
}

void MainComponent::mouseDrag(const juce::MouseEvent& e) 
{ 
    if (e.getDistanceFromDragStart() < 3) 
        return; 

    if (fileList.isParentOf(e.originalComponent) || e.originalComponent == &fileList) 
    { 
        int row = fileList.getSelectedRow(); 
        if (row >= 0 && row < filteredAudioFiles.size())
        { 
            juce::StringArray filesToDrag; 
            filesToDrag.add(filteredAudioFiles[row].getFullPathName());
            juce::DragAndDropContainer::performExternalDragDropOfFiles(filesToDrag, false, this); 
        } 
    } 
}

void MainComponent::textEditorTextChanged(juce::TextEditor& editor)
{
    applyFilters();
}

void MainComponent::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    applyFilters();
}

void MainComponent::applyFilters()
{
    filteredAudioFiles.clear();
    filteredBPMs.clear();
    filteredKeys.clear();

    juce::String searchText = searchBar.getText().toLowerCase();
    juce::String bpmText = bpmBox.getText().toLowerCase();

    bool isAllKeys = (keyBox.getSelectedId() == 1);
    juce::String selectedKey = isAllKeys ? "" : keyBox.getText().toLowerCase();

    juce::String note = "";
    juce::String scale = "";
    if (!isAllKeys)
    {
        note = selectedKey.upToFirstOccurrenceOf(" ", false, false).trim();
        scale = selectedKey.fromFirstOccurrenceOf(" ", false, false).trim();
    }

    int typeFilter = typeBox.getSelectedId(); // Sacamos esto del 'for' para que sea más rápido

    for (auto& file : audioFilesInFolder)
    {
        juce::String fileName = file.getFileName().toLowerCase();
        bool isMidi = file.hasFileExtension(".mid");

        // 1. EVALUAMOS TIPO (Audio o MIDI)
        bool matchesType = true;
        if (typeFilter == 2 && isMidi) matchesType = false;  // Quería solo Audio, pero es MIDI
        if (typeFilter == 3 && !isMidi) matchesType = false; // Quería solo MIDI, pero es Audio

        // 2. EVALUAMOS TEXTO Y BPM
        bool matchesText = searchText.isEmpty() || fileName.contains(searchText);
        bool matchesBpm = bpmText.isEmpty() || fileName.contains(bpmText);

        // 3. EVALUAMOS LA KEY
        bool matchesKey = true;
        if (!isAllKeys)
        {
            juce::String var1 = note + " " + scale;
            juce::String var2 = note + scale;
            juce::String var3 = note + "_" + scale;
            juce::String var4 = note + "-" + scale;

            if (!fileName.contains(var1) && !fileName.contains(var2) && !fileName.contains(var3) && !fileName.contains(var4))
            {
                matchesKey = false;
            }
        }

        // 4. VEREDICTO FINAL: Si cumple todo, lo agregamos a la lista visual
        if (matchesText && matchesBpm && matchesKey && matchesType)
        {
            filteredAudioFiles.add(file);

            juce::String filePath = file.getFullPathName();
            if (bpmDatabase.containsKey(filePath))
            {
                filteredBPMs.add(bpmDatabase[filePath]);

                juce::String savedKey = keyDatabase[filePath];
                if (savedKey.isEmpty() || savedKey == "--")
                    savedKey = extractKeyFromName(fileName);

                filteredKeys.add(savedKey);
            }
            else
            {
                filteredBPMs.add(extractBPMFromName(fileName));
                filteredKeys.add(extractKeyFromName(fileName));
            }
        }
    }

    fileList.updateContent();
}

// {* nuevo código *}
juce::String MainComponent::extractBPMFromName(const juce::String& fileName)
{
    std::string nameStr = fileName.toLowerCase().toStdString();

    // Ahora busca números seguidos de "bpm", o solo "bp", o " bpm" (es a prueba de balas)
    std::regex bpmRegex("([0-9]{2,3})\\s*bp");
    std::smatch match;

    if (std::regex_search(nameStr, match, bpmRegex)) {
        return juce::String(match[1].str());
    }
    return "--";
}

juce::String MainComponent::extractKeyFromName(const juce::String& fileName)
{
    std::string nameStr = fileName.toLowerCase().toStdString();

    // Busca las letras A-G, opcional un '#' o 'b', seguidos de min/maj/minor/major
    std::regex keyRegex("([a-g][#b]?)\\s*(min|maj|minor|major|m\\b)");
    std::smatch match;

    if (std::regex_search(nameStr, match, keyRegex)) {
        juce::String note = juce::String(match[1].str()).toUpperCase(); // Ej: "g#" -> "G#"
        juce::String scale = juce::String(match[2].str());

        // Limpiamos la escala para que se vea súper profesional
        if (scale.startsWith("min") || scale == "m") scale = "Min";
        if (scale.startsWith("maj")) scale = "Maj";

        return note + " " + scale; // Resultado final: "G# Min"
    }
    return "--";
}

void MainComponent::analyzeBPM(const juce::File& file)
{
    juce::String filePath = file.getFullPathName();

    if (file.hasFileExtension(".mid"))
    {
        bpmDisplayLabel.setText("Archivo MIDI (Sin vista previa)", juce::dontSendNotification);
        int index = filteredAudioFiles.indexOf(file);
        if (index >= 0) {
            filteredBPMs.set(index, "MIDI");
            filteredKeys.set(index, "--");
            fileList.repaintRow(index);
        }
        return;
    }

    // --- EL BLOQUE CORREGIDO ---
    if (bpmDatabase.containsKey(filePath))
    {
        juce::String savedBPM = bpmDatabase[filePath];
        juce::String savedKey = keyDatabase[filePath];
        if (savedKey.isEmpty()) savedKey = "--";

        // Ahora sí imprime ambos datos al cargar de la memoria
        bpmDisplayLabel.setText("BPM: " + savedBPM + " | Key: " + savedKey, juce::dontSendNotification);

        int index = filteredAudioFiles.indexOf(file);
        if (index >= 0) {
            filteredBPMs.set(index, savedBPM);
            filteredKeys.set(index, savedKey); // Faltaba actualizar la tabla visual aquí
            fileList.repaintRow(index);
        }
        return;
    }

    bpmDisplayLabel.setText("BPM: Calculando...", juce::dontSendNotification);

    auto* reader = audioPlayer.getFormatManager().createReaderFor(file);
    if (reader != nullptr)
    {
        // Inicializamos la nueva inteligencia artificial de BTrack
        BTrack bpmDetector;
        KeyDetector keyDetector(reader->sampleRate);

        int maxSeconds = 60;
        juce::int64 numSamplesToRead = juce::jmin(reader->lengthInSamples, (juce::int64)(reader->sampleRate * maxSeconds));

        // BTrack procesa el audio en "cuadros" matemáticos de 512 samples
        int hopSize = 512;
        juce::AudioBuffer<float> buffer(reader->numChannels, hopSize);
        juce::HeapBlock<double> monoFrame(hopSize);

        juce::int64 samplesRead = 0;

        while (samplesRead < numSamplesToRead)
        {
            int samplesToGet = (int)juce::jmin((juce::int64)hopSize, numSamplesToRead - samplesRead);
            buffer.clear();
            reader->read(&buffer, 0, samplesToGet, samplesRead, true, true);

            // BTrack requiere que el audio sea Mono y en formato de alta precisión ('double')
            for (int i = 0; i < hopSize; ++i)
            {
                if (i < samplesToGet)
                {
                    float sum = 0.0f;
                    // Mezclamos Izquierda y Derecha en un solo canal
                    for (int ch = 0; ch < reader->numChannels; ++ch)
                        sum += buffer.getReadPointer(ch)[i];

                    monoFrame[i] = (double)(sum / reader->numChannels);
                }
                else
                {
                    monoFrame[i] = 0.0; // Rellenamos con silencio si el sample es más corto
                }
            }

            // Alimentamos a la bestia cuadro por cuadro
            bpmDetector.processAudioFrame(monoFrame.getData());
            keyDetector.processAudioFrame(monoFrame.getData(), samplesToGet);
            samplesRead += samplesToGet;
        }

        // Le pedimos el cálculo final a BTrack
        double detectedBPM = bpmDetector.getCurrentTempoEstimate();
        juce::String detectedKey = keyDetector.getDetectedKey();
        delete reader;

        if (detectedBPM > 0.0)
        {
            int finalBPM = std::round(detectedBPM);
            juce::String bpmString = juce::String(finalBPM);

            bpmDisplayLabel.setText("BPM: " + bpmString + " | Key: " + detectedKey, juce::dontSendNotification);

            int index = filteredAudioFiles.indexOf(file);
            if (index >= 0)
            {
                filteredBPMs.set(index, bpmString);
                filteredKeys.set(index, detectedKey); // <--- NUEVO: Actualiza la tabla visual
                fileList.repaintRow(index);
            }

            bpmDatabase.set(filePath, bpmString);
            keyDatabase.set(filePath, detectedKey); // <--- NUEVO: Guarda en RAM
            saveDatabase();                         // <--- NUEVO: Guarda en XML
        }
        else
        {
            bpmDisplayLabel.setText("BPM: --", juce::dontSendNotification);
        }
    }
    else
    {
        bpmDisplayLabel.setText("BPM: Error", juce::dontSendNotification);
    }
}

void MainComponent::loadDatabase()
{
    std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(databaseFile);
    if (xml != nullptr)
    {
        for (auto* child : xml->getChildIterator())
        {
            juce::String path = child->getStringAttribute("path");
            juce::String bpm = child->getStringAttribute("bpm");
            juce::String key = child->getStringAttribute("key", "--"); // <-- Lee la Key

            bpmDatabase.set(path, bpm);
            keyDatabase.set(path, key); // <-- La guarda en memoria RAM
        }
    }
}

void MainComponent::saveDatabase()
{
    juce::XmlElement xml("BPM_CACHE");

    for (auto path : bpmDatabase.getAllKeys())
    {
        auto* child = xml.createNewChildElement("FILE");
        child->setAttribute("path", path);
        child->setAttribute("bpm", bpmDatabase[path]);
        child->setAttribute("key", keyDatabase[path]); // <-- Escribe la Key en el disco duro
    }

    xml.writeTo(databaseFile);
}