#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    addAndMakeVisible(directoryTree);
    addAndMakeVisible(fileList);

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
    bpmDisplayLabel.setColour(juce::Label::textColourId, juce::Colours::lawngreen); // Color verde pro
    bpmDisplayLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    bpmDisplayLabel.setText("BPM: --", juce::dontSendNotification);

    // {* NUEVO CÓDIGO: Configuramos los colores de los títulos *}
    addAndMakeVisible(headerName);
    headerName.setColour(juce::Label::textColourId, juce::Colours::grey);

    addAndMakeVisible(headerKey);
    headerKey.setColour(juce::Label::textColourId, juce::Colours::grey);
    headerKey.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(headerBPM);
    headerBPM.setColour(juce::Label::textColourId, juce::Colours::grey);
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
    setSize(1100, 750);
}

MainComponent::~MainComponent()
{
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

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    directoryTree.setBounds(0, 0, 300, getHeight());

    auto topBar = juce::Rectangle<int>(300, 0, getWidth() - 300, 30);
    searchBar.setBounds(topBar.removeFromLeft(300).reduced(2));
    bpmBox.setBounds(topBar.removeFromLeft(70).reduced(2));
    keyBox.setBounds(topBar.reduced(2));

    // {* NUEVO CÓDIGO: Acomodamos los títulos justo debajo del buscador *}
    int listY = 30;
    headerName.setBounds(310, listY, getWidth() - 300 - 140, 25);
    headerKey.setBounds(getWidth() - 130, listY, 60, 25);
    headerBPM.setBounds(getWidth() - 60, listY, 50, 25);

    // Bajamos la lista (listY + 25) para que no tape los títulos
    fileList.setBounds(300, listY + 25, getWidth() - 300, getHeight() - (listY + 25) - 130 - 45);

    bpmDisplayLabel.setBounds(getWidth() - 150, getHeight() - 175 - 25, 140, 25);
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
            juce::Array<juce::File> filesFound = folderToSearch.findChildFiles(juce::File::findFiles, false, "*.wav;*.mp3;*.aif");
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
    if (rowIsSelected)
        g.fillAll(juce::Colours::darkcyan);
    else
        g.fillAll(juce::Colours::transparentBlack);

    if (rowNumber < filteredAudioFiles.size())
    {
        // 1. DIBUJAMOS EL NOMBRE
        g.setColour(juce::Colours::white);
        g.setFont(14.0f);
        // Acortamos el nombre para que no invada las columnas derechas
        g.drawText(filteredAudioFiles[rowNumber].getFileName(), 10, 0, width - 150, height, juce::Justification::centredLeft, true);

        // 2. DIBUJAMOS LA KEY
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText(filteredKeys[rowNumber], width - 130, 0, 60, height, juce::Justification::centred, true);

        // 3. DIBUJAMOS EL BPM
        g.setColour(juce::Colours::cyan);
        g.drawText(filteredBPMs[rowNumber], width - 60, 0, 50, height, juce::Justification::centred, true);
    }
}

void MainComponent::listBoxItemClicked(int row, const juce::MouseEvent& e) 
{
    // Dejamos vacío si prefieres que solo "selectedRowsChanged" haga el trabajo
} 

void MainComponent::selectedRowsChanged(int lastRowSelected)
{
    if (lastRowSelected >= 0 && lastRowSelected < filteredAudioFiles.size())
    {
        juce::File selectedAudioFile = filteredAudioFiles[lastRowSelected];
        waveformDisplay.setAudioFile(selectedAudioFile);

        analyzeBPM(selectedAudioFile); // <-- NUEVO: Dispara el escáner

        audioPlayer.loadFile(selectedAudioFile);
        audioPlayer.play();
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

    for (auto& file : audioFilesInFolder)
    {
        juce::String fileName = file.getFileName().toLowerCase();

        bool matchesText = searchText.isEmpty() || fileName.contains(searchText);
        bool matchesBpm = bpmText.isEmpty() || fileName.contains(bpmText);
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

        if (matchesText && matchesBpm && matchesKey)
        {
            filteredAudioFiles.add(file);

            // {* NUEVO CÓDIGO: El programa revisa la caché al instante *}
            juce::String filePath = file.getFullPathName();
            if (bpmDatabase.containsKey(filePath))
            {
                // Si ya lo escaneaste ayer, pon el número de inmediato
                filteredBPMs.add(bpmDatabase[filePath]);
            }
            else
            {
                // Si es un archivo virgen, busca en el nombre o pon "--"
                filteredBPMs.add(extractBPMFromName(fileName));
            }

            filteredKeys.add(extractKeyFromName(fileName));
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

    if (bpmDatabase.containsKey(filePath))
    {
        juce::String savedBPM = bpmDatabase[filePath];
        bpmDisplayLabel.setText("BPM: " + savedBPM, juce::dontSendNotification);

        int index = filteredAudioFiles.indexOf(file);
        if (index >= 0) {
            filteredBPMs.set(index, savedBPM);
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
            samplesRead += samplesToGet;
        }

        // Le pedimos el cálculo final a BTrack
        double detectedBPM = bpmDetector.getCurrentTempoEstimate();
        delete reader;

        if (detectedBPM > 0.0)
        {
            int finalBPM = std::round(detectedBPM);
            juce::String bpmString = juce::String(finalBPM);

            bpmDisplayLabel.setText("BPM: " + bpmString, juce::dontSendNotification);

            int index = filteredAudioFiles.indexOf(file);
            if (index >= 0)
            {
                filteredBPMs.set(index, bpmString);
                fileList.repaintRow(index);
            }

            bpmDatabase.set(filePath, bpmString);
            saveDatabase();
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
            bpmDatabase.set(path, bpm);
        }
    }
}

void MainComponent::saveDatabase()
{
    juce::XmlElement xml("BPM_CACHE");

    // Guardamos cada archivo y su BPM en el XML
    for (auto key : bpmDatabase.getAllKeys())
    {
        auto* child = xml.createNewChildElement("FILE");
        child->setAttribute("path", key);
        child->setAttribute("bpm", bpmDatabase[key]);
    }

    xml.writeTo(databaseFile);
}