#include "MainComponent.h"
#include <functional>
#include <memory>
#include <utility>
#include <unordered_set>
#include <vector>

namespace
{
class BackgroundTask final : public juce::ThreadPoolJob
{
public:
    BackgroundTask(const juce::String& name, std::function<void(BackgroundTask&)> action)
        : juce::ThreadPoolJob(name), work(std::move(action)) {}

    JobStatus runJob() override
    {
        work(*this);
        return jobHasFinished;
    }

private:
    std::function<void(BackgroundTask&)> work;
};

struct AudioAnalysisResult
{
    bool valid = false;
    juce::String bpm, key, category;
};

struct BatchCompletedResult
{
    juce::File file;
    AudioAnalysisResult analysis;
};

AudioAnalysisResult analyzeAudioInBackground(BackgroundTask& job, const juce::File& file,
                                             const juce::StringPairArray& learnedPatterns,
                                             const juce::String& nameBPM,
                                             const std::shared_ptr<SampleBatchControl>& batch = nullptr)
{
    AudioAnalysisResult result;
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
        return result;
        double durationInSeconds = (double)reader->lengthInSamples / (double)reader->sampleRate;

        BTrack bpmDetector;
        KeyDetector keyDetector(reader->sampleRate);

        int maxSeconds = 60;
        juce::int64 numSamplesToRead = juce::jmin(reader->lengthInSamples, (juce::int64)(reader->sampleRate * maxSeconds));

        int hopSize = 512;
        juce::AudioBuffer<float> buffer(reader->numChannels, hopSize);
        juce::HeapBlock<double> monoFrame(hopSize);
        juce::int64 samplesRead = 0;

        int peakCount = 0;
        int cooldownSamples = 0;
        const int cooldownThreshold = (int)(reader->sampleRate * 0.15);
        double runningAverage = 0.0;

        while (samplesRead < numSamplesToRead)
        {
            if (job.shouldExit() || (batch && batch->cancelRequested.load())) return result;

            // El análisis masivo cede el procesador durante reproducciones y pausas.
            if (batch && (samplesRead / hopSize) % 16 == 0)
            {
                while ((batch->paused.load() || batch->playbackActive.load())
                       && !batch->cancelRequested.load() && !job.shouldExit())
                    juce::Thread::sleep(40);

                if (job.shouldExit() || batch->cancelRequested.load()) return result;
            }

            int samplesToGet = (int)juce::jmin((juce::int64)hopSize, numSamplesToRead - samplesRead);
            buffer.clear();
            reader->read(&buffer, 0, samplesToGet, samplesRead, true, true);

            for (int i = 0; i < hopSize; ++i)
            {
                if (i < samplesToGet)
                {
                    float sum = 0.0f;
                    for (int ch = 0; ch < reader->numChannels; ++ch)
                        sum += buffer.getReadPointer(ch)[i];

                    monoFrame[i] = (double)(sum / reader->numChannels);

                    double absSample = std::abs(monoFrame[i]);
                    runningAverage = 0.999 * runningAverage + 0.001 * absSample;

                    if (cooldownSamples > 0) cooldownSamples--;

                    if (absSample > runningAverage * 3.0 && absSample > 0.05 && cooldownSamples == 0)
                    {
                        peakCount++;
                        cooldownSamples = cooldownThreshold;
                    }
                }
                else { monoFrame[i] = 0.0; }
            }

            bpmDetector.processAudioFrame(monoFrame.getData());
            keyDetector.processAudioFrame(monoFrame.getData(), samplesToGet);
            // Cedemos CPU a la reproducción y a la UI durante análisis largos.
            if ((samplesRead / hopSize) % 16 == 0) juce::Thread::sleep(3);
            samplesRead += samplesToGet;
        }

        double detectedBPM = bpmDetector.getCurrentTempoEstimate();
        juce::String detectedKey = keyDetector.getDetectedKey();

        juce::String fileNameLower = file.getFileName().toLowerCase();
        juce::String audioType;
        juce::String parentFolder = file.getParentDirectory().getFileName().toLowerCase();

        bool learnedTypeFound = false;
        for (auto& keyword : learnedPatterns.getAllKeys())
        {
            if (parentFolder.contains(keyword.toLowerCase()))
            {
                audioType = learnedPatterns[keyword];
                learnedTypeFound = true;
                if (audioType == "ONE SHOT" || audioType == "SAMPLE") detectedBPM = 0.0;
                break;
            }
        }

        bool isExplicitLoop = false;
        bool isExplicitOneShot = false;

        if (!learnedTypeFound)
        {
            isExplicitLoop = fileNameLower.contains("loop") || fileNameLower.contains("bpm") ||
                parentFolder.contains("loop") || parentFolder.contains("melody");

            isExplicitOneShot = fileNameLower.contains("oneshot") || fileNameLower.contains("one shot") ||
                fileNameLower.contains("fx") || fileNameLower.contains("impact") ||
                fileNameLower.contains("crash") || fileNameLower.contains("riser") ||
                fileNameLower.contains("sweep") || fileNameLower.contains("cymbal") ||
                fileNameLower.contains("kick") || fileNameLower.contains("snare") ||
                fileNameLower.contains("clap") || fileNameLower.contains("hat") ||
                fileNameLower.contains("808") || fileNameLower.contains("shaker") ||
                fileNameLower.contains("perc") || fileNameLower.contains("tom") ||
                fileNameLower.contains("vocal") || fileNameLower.contains("chant") ||
                fileNameLower.contains("fill") ||
                parentFolder.contains("oneshot") || parentFolder.contains("one shot") ||
                parentFolder.contains("drum") || parentFolder.contains("fx") ||
                parentFolder.contains("kick") || parentFolder.contains("snare") ||
                parentFolder.contains("hat") || parentFolder.contains("perc") ||
                parentFolder.contains("cymbal");
        }

        if (learnedTypeFound)
        {
            // audioType ya se definió por la base de datos de aprendizaje
        }
        else if (isExplicitOneShot)
        {
            audioType = "ONE SHOT";
            detectedBPM = 0.0;
        }
        else if (isExplicitLoop)
        {
            audioType = "LOOP";
        }
        else
        {
            if (durationInSeconds <= 3.5)
            {
                audioType = "ONE SHOT";
                detectedBPM = 0.0;
            }
            else if (detectedBPM > 0.0 && peakCount >= 4)
            {
                audioType = "LOOP";
            }
            else if (peakCount < 4)
            {
                audioType = "ONE SHOT";
                detectedBPM = 0.0;
            }
            else
            {
                audioType = "SAMPLE";
            }
        }

        // El trabajador de lote calcula el BPM del nombre sin acceder a MainComponent.
        juce::String inferredBPM = nameBPM;
        if (inferredBPM.isEmpty())
        {
            static const std::regex bpmPattern("([0-9]{2,3})\\s*bp");
            std::smatch match;
            const std::string lowercaseName = file.getFileName().toLowerCase().toStdString();
            inferredBPM = std::regex_search(lowercaseName, match, bpmPattern)
                              ? juce::String(match[1].str()) : "--";
        }
        if (inferredBPM != "--" && audioType == "LOOP")
            detectedBPM = inferredBPM.getDoubleValue();


    result.valid = true;
    result.bpm = audioType == "LOOP" ? juce::String((int)std::round(detectedBPM)) : "--";
    result.key = detectedKey;
    result.category = audioType;
    return result;
}
} // namespace


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

    // {* NUEVO: Configuramos el botón de Audio *}
    addAndMakeVisible(settingsButton);
    settingsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2c313a));
    settingsButton.setButtonText("Configuración y análisis");
    addAndMakeVisible(batchStatusLabel);
    batchStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff56b6c2));
    batchStatusLabel.setJustificationType(juce::Justification::centredLeft);
    batchStatusLabel.setFont(juce::Font(12.0f));
    batchStatusLabel.setText("Análisis: disponible desde Configuración", juce::dontSendNotification);

    settingsButton.onClick = [this]
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Configuración de audio...");
        menu.addSeparator();
        menu.addItem(2, "Analizar carpeta raíz y todas sus subcarpetas", !batchRunning);
        menu.addItem(3, batchControl && batchControl->paused.load()
                            ? "Reanudar análisis" : "Pausar análisis", batchRunning);
        menu.addItem(4, "Cancelar análisis", batchRunning);

        juce::Component::SafePointer<MainComponent> safeThis(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settingsButton),
            [safeThis](int choice)
        {
            if (safeThis == nullptr) return;

            if (choice == 1)
            {
                auto* audioSettings = new juce::AudioDeviceSelectorComponent(
                    safeThis->deviceManager, 0, 0, 0, 2, false, false, true, false);
                audioSettings->setSize(500, 400);

                juce::DialogWindow::LaunchOptions options;
                options.content.setOwned(audioSettings);
                options.dialogTitle = "Configuración de Audio";
                options.dialogBackgroundColour = juce::Colour(0xff181a1f);
                options.escapeKeyTriggersCloseButton = true;
                options.useNativeTitleBar = true;
                options.resizable = false;
                options.launchAsync();
            }
            else if (choice == 2) safeThis->startBatchAnalysis();
            else if (choice == 3) safeThis->pauseOrResumeBatchAnalysis();
            else if (choice == 4) safeThis->cancelBatchAnalysis();
        });
    };

    // {* NUEVO: Botón para elegir la carpeta de samples *}
    addAndMakeVisible(chooseFolderButton);
    chooseFolderButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff56b6c2)); // Cyan para que resalte
    chooseFolderButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff121418)); // Letra oscura
    chooseFolderButton.onClick = [this]
        {
            // Abrimos el explorador de Windows para que el usuario elija su carpeta
            folderChooser = std::make_unique<juce::FileChooser>("Selecciona tu carpeta principal de samples", juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*");

            auto folderChooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;

            folderChooser->launchAsync(folderChooserFlags, [this](const juce::FileChooser& fc)
                {
                    juce::File chosenFolder = fc.getResult();
                    if (chosenFolder.isDirectory())
                    {
                        // Cambiar la raíz detiene el lote anterior sin descartar análisis completados.
                        if (batchRunning && chosenFolder.getFullPathName() != lastSelectedFolder)
                            cancelBatchAnalysis();
                        // Guardamos la nueva raíz
                        lastSelectedFolder = chosenFolder.getFullPathName();
                        saveDatabase();

                        // Le decimos al árbol que se actualice y muestre SOLO esa carpeta
                        directoryList.setDirectory(chosenFolder, true, true);
                    }
                });
        };

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

    // {* NUEVO: Filtro de Categoría y Título *}
    addAndMakeVisible(categoryBox);
    categoryBox.addItem("TODOS", 1);
    categoryBox.addItem("SOLO LOOPS", 2);
    categoryBox.addItem("SOLO ONE SHOTS", 3);
    categoryBox.addItem("SOLO SAMPLES", 4);
    categoryBox.setSelectedId(1, juce::dontSendNotification);
    categoryBox.addListener(this);

    addAndMakeVisible(headerCategory);
    headerCategory.setColour(juce::Label::textColourId, juce::Colour(0xff6b7280));
    headerCategory.setJustificationType(juce::Justification::centred);

    // {* NUEVO: Botón interactivo de Favoritos *}
    addAndMakeVisible(favFilterButton);
    favFilterButton.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffe5c07b)); // Color Dorado Premium
    favFilterButton.onClick = [this] { updateFavoriteView(); };

    addAndMakeVisible(waveformDisplay);
    addAndMakeVisible(transportControls);

    // Conectamos los controles visuales con nuestro nuevo motor de audio
    // Conectamos los controles visuales con nuestro nuevo motor de audio
    transportControls.onLoopChanged = [this](bool shouldLoop) { audioPlayer.setLooping(shouldLoop); };
    transportControls.onPitchChanged = [this](double semitones) { audioPlayer.setPitch(semitones); currentPitchSemitones = semitones; };
    transportControls.onTempoChanged = [this](double multiplier) { audioPlayer.setTempo(multiplier); currentTempoMultiplier = multiplier; };
    transportControls.onStretchChanged = [this](bool enabled) { audioPlayer.setStretch(enabled); isStretchEnabled = enabled; };

    // {* NUEVO: Cables para los botones principales *}
    transportControls.onPlayClicked = [this]() { audioPlayer.play(); };
    transportControls.onStopClicked = [this]() { audioPlayer.stop(); };

    // {* NUEVO: Cable para saltar con el clic en el panel MIDI *}
    waveformDisplay.onMidiScrub = [this](double newTime) { audioPlayer.setMidiPosition(newTime); };
    // Le enseñamos al panel visual a actualizarse a 30 FPS (Frames por segundo)
    startTimerHz(30);

    transportControls.onRandomClicked = [this]()
    {
        if (filteredAudioFiles.size() > 0)
        {
            int randomIndex = juce::Random::getSystemRandom().nextInt(filteredAudioFiles.size());
            fileList.selectRow(randomIndex, false, true);
            previewAudio(filteredAudioFiles[randomIndex]);
        }
    };

    directoryThread.startThread(juce::Thread::Priority::normal);

    juce::File startFolder(lastSelectedFolder.isEmpty() ? "C:\\" : lastSelectedFolder);
    directoryList.setDirectory(startFolder, true, true);

    fileList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff2b2b2b));

    setAudioChannels(0, 2);
    setSize(1180, 650);
}

MainComponent::~MainComponent()
{
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);

    if (batchControl) batchControl->cancelRequested.store(true);
    batchPool.removeAllJobs(true, 10000);
    scanPool.removeAllJobs(true, 10000);
    analysisPool.removeAllJobs(true, 10000);
    // Al cerrar no se pierden cambios que seguían pendientes en memoria.
    if (databaseSavePending) flushDatabaseSave();
    databaseWritePool.removeAllJobs(false, 30000);
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
    // 1. Botón de agregar carpeta arriba (30px de alto
    chooseFolderButton.setBounds(0, 0, 300, 30);

    // 2. El árbol de carpetas en medio
    directoryTree.setBounds(0, 30, 300, getHeight() - 30 - 72);
    batchStatusLabel.setBounds(10, getHeight() - 65, 280, 24);
    settingsButton.setBounds(10, getHeight() - 35, 280, 26);

    int topMargin = 15;
    int badgeWidth = 240;

    // Matemáticas corregidas para el nuevo menú:
    favFilterButton.setBounds(315, topMargin, 60, 26); // ¡Fuera del panel izquierdo!
    searchBar.setBounds(380, topMargin, 120, 26);
    categoryBox.setBounds(505, topMargin, 125, 26);
    typeBox.setBounds(635, topMargin, 95, 26);
    bpmBox.setBounds(735, topMargin, 60, 26);
    keyBox.setBounds(800, topMargin, 120, 26);

    bpmDisplayLabel.setBounds(getWidth() - badgeWidth - 15, topMargin, badgeWidth, 26);

    // Bajamos los títulos para que no se peguen a los buscadores
    int listY = 55;
    headerName.setBounds(315, listY, getWidth() - 300 - 240, 25);
    headerCategory.setBounds(getWidth() - 220, listY, 80, 25); // <-- Nuevo título
    headerKey.setBounds(getWidth() - 130, listY, 60, 25);
    headerBPM.setBounds(getWidth() - 60, listY, 50, 25);

    // Ajustamos la lista a su nueva posición
    fileList.setBounds(300, listY + 25, getWidth() - 300, getHeight() - (listY + 25) - 130 - 45);

    waveformDisplay.setBounds(300, getHeight() - 175, getWidth() - 300, 130);
    transportControls.setBounds(300, getHeight() - 45, getWidth() - 300, 45);
}

void MainComponent::selectionChanged()
{
    const juce::File selectedFolder = directoryTree.getSelectedFile();
    ++scanGeneration;
    scanPool.removeAllJobs(true, 0);
    audioFilesInFolder.clear();
    applyFilters(); // Limpiamos la vista anterior sin borrar los filtros.

    if (!selectedFolder.isDirectory()) return;

    const int generation = scanGeneration;
    juce::Component::SafePointer<MainComponent> safeThis(this);
    scanPool.addJob(new BackgroundTask("Scan sample folders", [selectedFolder, generation, safeThis](BackgroundTask& job)
    {
        juce::Array<juce::File> found;
        juce::Array<juce::File> pending;
        pending.add(selectedFolder);

        while (!pending.isEmpty() && !job.shouldExit())
        {
            const juce::File folder = pending.removeAndReturn(pending.size() - 1);
            juce::DirectoryIterator iterator(folder, false, "*", juce::File::findFilesAndDirectories);

            while (iterator.next())
            {
                if (job.shouldExit()) return;
                const juce::File child = iterator.getFile();
                if (child.isDirectory())
                    pending.add(child);
                else if (child.hasFileExtension(".wav;.mp3;.aif;.mid"))
                    found.add(child);
            }
        }

        if (job.shouldExit()) return;
        juce::MessageManager::callAsync([safeThis, generation, found = std::move(found)]() mutable
        {
            if (safeThis == nullptr || safeThis->scanGeneration != generation) return;
            safeThis->audioFilesInFolder = std::move(found);
            safeThis->applyFilters();
        });
    }), true);
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
        // 0. DIBUJAMOS LA ESTRELLA DE FAVORITO (Con más margen izquierdo)
        bool isFav = filteredFavorites[rowNumber];
        g.setColour(isFav ? juce::Colour(0xffe5c07b) : juce::Colour(0xff5c6370));
        g.setFont(22.0f);
        juce::String solidStar = juce::String::fromUTF8("\xe2\x98\x85");
        juce::String hollowStar = juce::String::fromUTF8("\xe2\x98\x86");

        // La movemos al pixel X = 15 para que no se esconda
        g.drawText(isFav ? solidStar : hollowStar, 10, 0, 38, height, juce::Justification::centred, true);

        // 1. DIBUJAMOS EL NOMBRE (Empujado a la derecha para no chocar)
        g.setColour(juce::Colour(0xffabb2bf));
        g.setFont(14.0f);

        // Lo movemos al pixel X = 50 y ajustamos su límite derecho
        g.drawText(filteredAudioFiles[rowNumber].getFileName(), 54, 0, width - 284, height, juce::Justification::centredLeft, true);

        // 2. DIBUJAMOS LA CATEGORÍA (Loop / One Shot)
        g.setColour(juce::Colour(0xffc678dd)); // Moradito premium sutil
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText(filteredCategories[rowNumber], width - 220, 0, 80, height, juce::Justification::centred, true);

        // 3. DIBUJAMOS LA KEY
        g.setColour(juce::Colour(0xff5c6370));
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText(filteredKeys[rowNumber], width - 130, 0, 60, height, juce::Justification::centred, true);

        // 4. DIBUJAMOS EL BPM
        g.setColour(juce::Colour(0xff56b6c2));
        g.drawText(filteredBPMs[rowNumber], width - 60, 0, 50, height, juce::Justification::centred, true);
    }
}

void MainComponent::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    if (row < 0 || row >= filteredAudioFiles.size()) return;

    juce::File selectedAudioFile = filteredAudioFiles[row];
    juce::String filePath = selectedAudioFile.getFullPathName();

    // {* NUEVO: DETECTAR CLIC IZQUIERDO EXACTO EN LA ESTRELLA *}
    if (e.x < 48 && !e.mods.isPopupMenu())
    {
        bool currentlyFav = favoritesLookup[filePath];
        juce::String newState = currentlyFav ? "0" : "1";

        favoritesDatabase.set(filePath, newState);
        favoritesLookup.set(filePath, !currentlyFav);
        filteredFavorites.set(row, !currentlyFav);
        saveDatabase(); // Guardado diferido; el clic jamás espera al disco.

        // Si el botón "Favs" está presionado y le quitas la estrella a uno, desaparece de la vista al instante
        if (favFilterButton.getToggleState()) updateFavoriteView();
        else fileList.repaintRow(row);

        return; // Salimos de la función para que el audio no se reproduzca solo por marcar la estrella
    }

    // {* SI EL USUARIO HACE CLIC DERECHO (OVERRIDE MANUAL) *}
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Forzar como LOOP");
        menu.addItem(2, "Forzar como ONE SHOT");
        menu.addItem(3, "Forzar como SAMPLE");
        menu.addSeparator();
        menu.addItem(4, "Corregir BPM manualmente...");

        menu.showMenuAsync(juce::PopupMenu::Options(), [this, row, selectedAudioFile, filePath](int result)
            {
                if (result == 0) return; // Si cerró el menú sin elegir nada

                juce::String parentFolder = selectedAudioFile.getParentDirectory().getFileName().toLowerCase();

                if (result >= 1 && result <= 3)
                {
                    juce::String newType = (result == 1) ? "LOOP" : (result == 2) ? "ONE SHOT" : "SAMPLE";

                    // 1. Corregimos los datos visuales en tiempo real
                    filteredCategories.set(row, newType);
                    categoryDatabase.set(filePath, newType);
                    categoryLookup.set(filePath, newType);

                    if (newType != "LOOP")
                    {
                        filteredBPMs.set(row, "--");
                        bpmDatabase.set(filePath, "--");
                        bpmLookup.set(filePath, "--");
                    }

                    // 2. ¡SISTEMA DE APRENDIZAJE AUTÓNOMO!
                    // El programa memoriza que esta palabra en la carpeta padre significa esta categoría
                    if (parentFolder.isNotEmpty() && parentFolder.length() > 2)
                    {
                        learnedPatternsDatabase.set(parentFolder, newType);
                    }

                    fileList.repaintRow(row);
                    saveDatabase();
                    applyFilters();
                }
                else if (result == 4) // Corregir BPM por cuadro de texto
                {
                    // 1. Construimos la ventana de alerta manualmente
                    auto* alert = new juce::AlertWindow("Corregir BPM", "Introduce el BPM correcto para este sample:", juce::AlertWindow::NoIcon);

                    // 2. Le agregamos el cuadro de texto y los botones
                    alert->addTextEditor("bpmInput", filteredBPMs[row], "BPM:");
                    alert->addButton("Guardar", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
                    alert->addButton("Cancelar", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));

                    // {* EL ARREGLO: Pasamos 'false' para que JUCE no nos borre la memoria antes de tiempo *}
                    alert->enterModalState(false, juce::ModalCallbackFunction::create([this, filePath, alert](int buttonResult)
                        {
                            if (buttonResult == 1) // Si el usuario hizo clic en "Guardar" o presionó Enter
                            {
                                // Ahora es 100% seguro leer el texto porque la ventana sigue viva en la memoria
                                juce::String input = alert->getTextEditorContents("bpmInput");

                                if (input.isNotEmpty() && input.getIntValue() > 0)
                                {
                                    juce::String bpmString = juce::String(input.getIntValue());

                                    // Actualizamos la base de datos maestra
                                    bpmDatabase.set(filePath, bpmString);
                                    bpmLookup.set(filePath, bpmString);
                                    categoryDatabase.set(filePath, "LOOP");
                                    categoryLookup.set(filePath, "LOOP");

                                    // Refrescamos TODA la UI y guardamos en el XML para que el cambio se vea al instante
                                    saveDatabase();
                                    applyFilters();
                                }
                            }

                            // {* MAGIA: Destruimos la ventana manualmente al terminar el trabajo *}
                            delete alert;
                        }));
                }
            });
    }
    else // {* CLIC IZQUIERDO NORMAL: REPRODUCIR *}
    {
        if (fileList.getSelectedRow() == row)
        {
            previewAudio(selectedAudioFile);
        }
    }
}

void MainComponent::previewAudio(const juce::File& file)
{
    // Cancelar análisis anterior de forma no bloqueante. La reproducción tiene prioridad.
    pendingAnalysisFile = juce::File();
    analysisPool.removeAllJobs(true, 0);

    previewPauseUntil = juce::Time::getMillisecondCounter() + 800;
    if (batchControl) batchControl->playbackActive.store(true);
    waveformDisplay.setAudioFile(file);
    audioPlayer.loadFile(file);
    audioPlayer.play();

    if (file.hasFileExtension(".mid") || bpmLookup.contains(file.getFullPathName()))
    {
        analyzeBPM(file);
        return;
    }

    bpmDisplayLabel.setText("Analizando audio...", juce::dontSendNotification);
    pendingAnalysisFile = file;
    pendingAnalysisRequestedAt = juce::Time::getMillisecondCounter();
}

void MainComponent::selectedRowsChanged(int lastRowSelected)
{
    // ListBox dispara esta notificación ANTES del clic: una estrella no debe iniciar audio/análisis.
    if (juce::ModifierKeys::getCurrentModifiersRealtime().isAnyMouseButtonDown())
        return;

    if (lastRowSelected >= 0 && lastRowSelected < filteredAudioFiles.size())
        previewAudio(filteredAudioFiles[lastRowSelected]);
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
            juce::File selectedFile = filteredAudioFiles[row];
            juce::StringArray filesToDrag;

            // Regla: Si es MIDI o no hay alteraciones, exportamos el original rápido
            bool hasProcessing = (currentPitchSemitones != 0.0 || currentTempoMultiplier != 1.0);
            if (selectedFile.hasFileExtension(".mid") || !hasProcessing)
            {
                filesToDrag.add(selectedFile.getFullPathName());
                juce::DragAndDropContainer::performExternalDragDropOfFiles(filesToDrag, false, this);
                return;
            }

            // --- MOTOR DE EXPORTACIÓN AVANZADO (Con Sistema Dual: Stretch + Resampler) ---
            juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
            juce::String uniqueName = "Processed_" + juce::String(juce::Time::currentTimeMillis()) + "_" + selectedFile.getFileNameWithoutExtension() + ".wav";
            juce::File tempFile = tempDir.getChildFile(uniqueName);

            auto* reader = audioPlayer.getFormatManager().createReaderFor(selectedFile);
            if (reader != nullptr)
            {
                auto readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);

                // 1. RECREAMOS LA CADENA DE AUDIO EXACTA DE TU REPRODUCTOR
                SoundTouchAudioSource offlineSoundTouch(readerSource.get());
                juce::ResamplingAudioSource offlineResampler(&offlineSoundTouch, false, 2);

                // {* EL ARREGLO: PRIMERO INICIAMOS LOS MOTORES (Para que no borren las instrucciones) *}
                readerSource->prepareToPlay(512, reader->sampleRate);
                offlineSoundTouch.prepareToPlay(512, reader->sampleRate);
                offlineResampler.prepareToPlay(512, reader->sampleRate);

                double effectiveMultiplier = 1.0;

                // 2. LUEGO APLICAMOS LA LÓGICA Y LOS PARÁMETROS
                if (isStretchEnabled)
                {
                    // Modo Warp / Stretch (Conserva el tiempo original)
                    offlineSoundTouch.setStretchEnabled(true);
                    offlineSoundTouch.setPitchSemiTones(currentPitchSemitones);
                    offlineSoundTouch.setTempoMultiplier(currentTempoMultiplier);
                    offlineResampler.setResamplingRatio(1.0);
                    effectiveMultiplier = currentTempoMultiplier;
                }
                else
                {
                    // Modo Vinilo clásico (Alterar pitch cambia la velocidad)
                    offlineSoundTouch.setStretchEnabled(false);
                    offlineSoundTouch.setPitchSemiTones(0.0);
                    offlineSoundTouch.setTempoMultiplier(1.0);

                    double pitchRatio = std::pow(2.0, currentPitchSemitones / 12.0);
                    double finalRatio = pitchRatio * currentTempoMultiplier;
                    offlineResampler.setResamplingRatio(finalRatio);
                    effectiveMultiplier = finalRatio;
                }

                auto outStream = new juce::FileOutputStream(tempFile);
                if (outStream->openedOk())
                {
                    juce::WavAudioFormat wavFormat;
                    std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(
                        outStream,
                        reader->sampleRate,
                        reader->numChannels,
                        16,
                        {},
                        0));

                    if (writer != nullptr)
                    {
                        // Calculamos cuánto va a durar el audio nuevo basado en las matemáticas
                        double estimatedDuration = (reader->lengthInSamples / reader->sampleRate) / effectiveMultiplier;
                        juce::int64 estimatedSamples = (juce::int64)(estimatedDuration * reader->sampleRate);

                        juce::AudioBuffer<float> renderBuffer(reader->numChannels, 512);
                        juce::AudioSourceChannelInfo info(&renderBuffer, 0, 512);

                        juce::int64 samplesWritten = 0;
                        int consecutiveSilenceBlocks = 0;

                        while (samplesWritten < estimatedSamples + (reader->sampleRate * 2.0))
                        {
                            renderBuffer.clear();

                            // LE PEDIMOS EL AUDIO AL ÚLTIMO ESLABÓN DE LA CADENA (El Resampler)
                            offlineResampler.getNextAudioBlock(info);

                            float magnitude = renderBuffer.getMagnitude(0, 512);
                            if (magnitude < 0.0001f && samplesWritten >= estimatedSamples)
                            {
                                consecutiveSilenceBlocks++;
                                if (consecutiveSilenceBlocks > 10) break; // Terminar si hay silencio absoluto
                            }
                            else
                            {
                                consecutiveSilenceBlocks = 0;
                            }

                            writer->writeFromAudioSampleBuffer(renderBuffer, 0, 512);
                            samplesWritten += 512;
                        }

                        // Destruimos el escritor primero para soltar el candado de Windows
                        writer.reset();
                        offlineResampler.releaseResources();
                        offlineSoundTouch.releaseResources();
                        readerSource->releaseResources();

                        filesToDrag.add(tempFile.getFullPathName());
                    }
                    else
                    {
                        delete outStream;
                        filesToDrag.add(selectedFile.getFullPathName());
                    }
                }
                else
                {
                    delete outStream;
                    filesToDrag.add(selectedFile.getFullPathName());
                }
            }
            else
            {
                filesToDrag.add(selectedFile.getFullPathName());
            }

            juce::DragAndDropContainer::performExternalDragDropOfFiles(filesToDrag, false, this);
        }
    }
}

void MainComponent::textEditorTextChanged(juce::TextEditor& editor)
{
    // Evita recalcular miles de resultados por cada pulsación rápida.
    juce::ignoreUnused(editor);
    filterInputPending = true;
    lastFilterInputTime = juce::Time::getMillisecondCounter();
}

void MainComponent::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    juce::ignoreUnused(comboBoxThatHasChanged);
    filterInputPending = false;
    applyFilters();
}

void MainComponent::applyFilters()
{
    matchingAudioFiles.clear();
    matchingBPMs.clear();
    matchingKeys.clear();
    matchingCategories.clear();
    const juce::String searchText = searchBar.getText().toLowerCase();
    const juce::String bpmText = bpmBox.getText().toLowerCase();
    const bool isAllKeys = keyBox.getSelectedId() == 1;
    const juce::String selectedKey = isAllKeys ? "" : keyBox.getText().toLowerCase();
    juce::String note, scale;
    if (!isAllKeys)
    {
        note = selectedKey.upToFirstOccurrenceOf(" ", false, false).trim();
        scale = selectedKey.fromFirstOccurrenceOf(" ", false, false).trim();
    }

    // Se calculan una sola vez por búsqueda, no por cada archivo.
    const juce::String key1 = note + " " + scale;
    const juce::String key2 = note + scale;
    const juce::String key3 = note + "_" + scale;
    const juce::String key4 = note + "-" + scale;
    const auto learnedKeywords = learnedPatternsDatabase.getAllKeys();
    const int typeFilter = typeBox.getSelectedId();
    const int catFilter = categoryBox.getSelectedId();

    for (const auto& file : audioFilesInFolder)
    {
        const juce::String fileName = file.getFileName().toLowerCase();

        // Filtros baratos primero: descartamos archivos no coincidentes antes de clasificarlos.
        if (searchText.isNotEmpty() && !fileName.contains(searchText)) continue;
        if (bpmText.isNotEmpty() && !fileName.contains(bpmText)) continue;
        if (!isAllKeys && !fileName.contains(key1) && !fileName.contains(key2)
            && !fileName.contains(key3) && !fileName.contains(key4)) continue;

        const bool isMidi = file.hasFileExtension(".mid");
        if (typeFilter == 2 && isMidi) continue;
        if (typeFilter == 3 && !isMidi) continue;

        const juce::String filePath = file.getFullPathName();
        // El filtro Favs se aplica después sobre una vista ya calculada.

        // Extraemos la categoría
        juce::String currentCat = categoryLookup.contains(filePath) ? categoryLookup[filePath] : "--";
        if (currentCat == "--" && !isMidi)
        {
            juce::String parentFolder = file.getParentDirectory().getFileName().toLowerCase();

            // {* CONSULTA AL MOTOR DE APRENDIZAJE AUTÓNOMO *}
            for (const auto& keyword : learnedKeywords)
            {
                if (parentFolder.contains(keyword.toLowerCase()))
                {
                    currentCat = learnedPatternsDatabase[keyword];
                    break;
                }
            }

            if (currentCat == "--")
            {
                bool isLoopHint = fileName.contains("loop") || fileName.contains("bpm") ||
                    parentFolder.contains("loop") || parentFolder.contains("melody");

                bool isOneShotHint = fileName.contains("oneshot") || fileName.contains("kick") ||
                    fileName.contains("snare") || fileName.contains("hat") ||
                    fileName.contains("fx") || parentFolder.contains("oneshot") ||
                    parentFolder.contains("one shot") || parentFolder.contains("drum") ||
                    parentFolder.contains("cymbal") || parentFolder.contains("perc");

                if (isLoopHint) currentCat = "LOOP";
                else if (isOneShotHint) currentCat = "ONE SHOT";
            }
        }

        if (isMidi) currentCat = "MIDI";

        if (catFilter == 2 && currentCat != "LOOP") continue;
        if (catFilter == 3 && currentCat != "ONE SHOT") continue;
        if (catFilter == 4 && currentCat != "SAMPLE") continue;

        matchingAudioFiles.add(file);
        matchingCategories.add(currentCat);

        if (bpmLookup.contains(filePath))
        {
            matchingBPMs.add(bpmLookup[filePath]);
            matchingKeys.add(keyLookup[filePath]);
        }
        else
        {
            matchingBPMs.add(extractBPMFromName(fileName));
            matchingKeys.add(extractKeyFromName(fileName));
        }
    }

    updateFavoriteView();
}

void MainComponent::updateFavoriteView()
{
    const bool onlyFavorites = favFilterButton.getToggleState();
    filteredAudioFiles.clear();
    filteredBPMs.clear();
    filteredKeys.clear();
    filteredCategories.clear();
    filteredFavorites.clear();

    for (int i = 0; i < matchingAudioFiles.size(); ++i)
    {
        const bool isFavorite = favoritesLookup[matchingAudioFiles[i].getFullPathName()];
        if (onlyFavorites && !isFavorite) continue;

        filteredAudioFiles.add(matchingAudioFiles[i]);
        filteredBPMs.add(matchingBPMs[i]);
        filteredKeys.add(matchingKeys[i]);
        filteredCategories.add(matchingCategories[i]);
        filteredFavorites.add(isFavorite);
    }
    fileList.updateContent();
}

// {* nuevo código *}
juce::String MainComponent::extractBPMFromName(const juce::String& fileName)
{
    std::string nameStr = fileName.toLowerCase().toStdString();

    // Ahora busca números seguidos de "bpm", o solo "bp", o " bpm" (es a prueba de balas)
    static const std::regex bpmRegex("([0-9]{2,3})\\s*bp");
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
    static const std::regex keyRegex("([a-g][#b]?)\\s*(min|maj|minor|major|m\\b)");
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
    const juce::String path = file.getFullPathName();

    if (file.hasFileExtension(".mid"))
    {
        bpmDisplayLabel.setText("Archivo MIDI (Sin vista previa)", juce::dontSendNotification);
        const int row = filteredAudioFiles.indexOf(file);
        if (row >= 0)
        {
            filteredBPMs.set(row, "MIDI");
            filteredKeys.set(row, "--");
            filteredCategories.set(row, "MIDI");
            fileList.repaintRow(row);
        }
        return;
    }

    if (bpmLookup.contains(path))
    {
        const juce::String savedBPM = bpmLookup[path];
        juce::String savedKey = keyLookup[path];
        if (savedKey.isEmpty()) savedKey = "--";
        bpmDisplayLabel.setText("BPM: " + savedBPM + " | Key: " + savedKey, juce::dontSendNotification);
        const int row = filteredAudioFiles.indexOf(file);
        if (row >= 0)
        {
            filteredBPMs.set(row, savedBPM);
            filteredKeys.set(row, savedKey);
            fileList.repaintRow(row);
        }
        return;
    }

    // El análisis largo se ejecuta fuera del hilo de interfaz; la reproducción ya ha empezado.
    bpmDisplayLabel.setText("Analizando audio...", juce::dontSendNotification);
    analysisPool.removeAllJobs(true, 0);
    juce::Component::SafePointer<MainComponent> safeThis(this);
    auto rules = learnedPatternsDatabase;
    // La extracción del nombre usa el método de MainComponent antes de iniciar la tarea.
    const juce::String nameBPM = extractBPMFromName(file.getFileName());
    analysisPool.addJob(new BackgroundTask("Analyse sample", [file, path, rules, safeThis, nameBPM](BackgroundTask& job)
    {
        const auto result = analyzeAudioInBackground(job, file, rules, nameBPM);
        if (job.shouldExit()) return;
        juce::MessageManager::callAsync([safeThis, file, path, result]()
        {
            if (safeThis == nullptr) return;

            // Respeta correcciones manuales hechas mientras se analizaba.
            if (safeThis->bpmLookup.contains(path)) return;
            if (!result.valid)
            {
                const int selected = safeThis->fileList.getSelectedRow();
                if (selected >= 0 && selected < safeThis->filteredAudioFiles.size()
                    && safeThis->filteredAudioFiles[selected] == file)
                    safeThis->bpmDisplayLabel.setText("Archivo corrupto", juce::dontSendNotification);
                return;
            }

            safeThis->bpmDatabase.set(path, result.bpm);
            safeThis->keyDatabase.set(path, result.key);
            safeThis->categoryDatabase.set(path, result.category);
            safeThis->bpmLookup.set(path, result.bpm);
            safeThis->keyLookup.set(path, result.key);
            safeThis->categoryLookup.set(path, result.category);
            const int baseRow = safeThis->matchingAudioFiles.indexOf(file);
            if (baseRow >= 0)
            {
                safeThis->matchingBPMs.set(baseRow, result.bpm);
                safeThis->matchingKeys.set(baseRow, result.key);
                safeThis->matchingCategories.set(baseRow, result.category);
            }
            const int row = safeThis->filteredAudioFiles.indexOf(file);
            if (row >= 0)
            {
                safeThis->filteredBPMs.set(row, result.bpm);
                safeThis->filteredKeys.set(row, result.key);
                safeThis->filteredCategories.set(row, result.category);
                safeThis->fileList.repaintRow(row);

                if (safeThis->fileList.getSelectedRow() == row)
                {
                    const juce::String label = result.category == "LOOP"
                        ? result.category + " | BPM: " + result.bpm + " | Key: " + result.key
                        : result.category + " | Key: " + result.key;
                    safeThis->bpmDisplayLabel.setText(label, juce::dontSendNotification);
                }
            }
            safeThis->saveDatabase();
        });
    }), true);
}

// El usuario puede seguir navegando, buscando y reproduciendo durante todo el lote.
void MainComponent::startBatchAnalysis()
{
    if (batchRunning) return;

    if (lastSelectedFolder.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            "Análisis de librerías", "Primero selecciona una carpeta raíz válida.");
        return;
    }
    const juce::File root(lastSelectedFolder);
    if (!root.isDirectory())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            "Análisis de librerías", "La carpeta raíz ya no existe.");
        return;
    }

    const int generation = ++batchGeneration;
    auto control = std::make_shared<SampleBatchControl>();
    batchControl = control;
    batchRunning = true;
    lastBatchPersistAt = juce::Time::getMillisecondCounter();
    batchStatusLabel.setText("Analizando: buscando archivos...", juce::dontSendNotification);

    // Copias de datos: el trabajador nunca leerá bases ni controles de la interfaz.
    const auto rules = learnedPatternsDatabase;
    const auto analyzedPaths = bpmDatabase.getAllKeys();
    juce::Component::SafePointer<MainComponent> safeThis(this);

    batchPool.addJob(new BackgroundTask("Index and analyze entire sample library",
        [root, rules, analyzedPaths, control, generation, safeThis](BackgroundTask& job)
    {
        const auto stopped = [&job, &control]()
        {
            return job.shouldExit() || control->cancelRequested.load();
        };

        // La pausa manual y la reproducción se comprueban también mientras se buscan archivos.
        const auto waitForTurn = [&stopped, &control]()
        {
            while ((control->paused.load() || control->playbackActive.load()) && !stopped())
                juce::Thread::sleep(40);
            return !stopped();
        };

        const auto report = [safeThis, generation](const juce::String& message)
        {
            juce::MessageManager::callAsync([safeThis, generation, message]()
            {
                if (safeThis != nullptr && safeThis->batchGeneration == generation && safeThis->batchRunning)
                    safeThis->batchStatusLabel.setText(message, juce::dontSendNotification);
            });
        };

        // Indexamos solo WAV/MP3/AIF/AIFF. MIDI no requiere BPM ni detección de key.
        std::vector<juce::File> files;
        std::vector<juce::File> folders { root };
        std::unordered_set<std::string> visited;
        while (!folders.empty() && !stopped())
        {
            if (!waitForTurn()) break;
            auto folder = folders.back();
            folders.pop_back();
            if (!visited.insert(folder.getFullPathName().toLowerCase().toStdString()).second)
                continue;

            juce::DirectoryIterator iterator(folder, false, "*", juce::File::findFilesAndDirectories);
            while (iterator.next() && !stopped())
            {
                const auto child = iterator.getFile();
                if (child.isDirectory())
                {
                    // No seguir enlaces/junctions que puedan formar ciclos o salir de la librería.
                    if (!child.isSymbolicLink()) folders.push_back(child);
                }
                else if (child.hasFileExtension(".wav;.mp3;.aif;.aiff"))
                    files.push_back(child);
            }
        }

        const int total = static_cast<int>(files.size());
        int done = 0, skipped = 0, analyzed = 0, failed = 0;
        std::vector<BatchCompletedResult> completed;
        completed.reserve(8);
        auto lastReport = juce::Time::getMillisecondCounter();
        auto lastFlush = lastReport;

        // Se envían pequeños grupos completos al hilo UI; solo él modifica el XML/cache.
        const auto postCompleted = [&completed, safeThis]()
        {
            if (completed.empty()) return;
            auto ready = std::move(completed);
            completed.clear();
            juce::MessageManager::callAsync([safeThis, ready = std::move(ready)]()
            {
                if (safeThis == nullptr) return;
                bool changed = false;
                for (const auto& item : ready)
                {
                    const auto path = item.file.getFullPathName();
                    // Puede haberse analizado en la vista previa o editado manualmente.
                    if (safeThis->bpmLookup.contains(path)) continue;
                    safeThis->bpmDatabase.set(path, item.analysis.bpm);
                    safeThis->keyDatabase.set(path, item.analysis.key);
                    safeThis->categoryDatabase.set(path, item.analysis.category);
                    safeThis->bpmLookup.set(path, item.analysis.bpm);
                    safeThis->keyLookup.set(path, item.analysis.key);
                    safeThis->categoryLookup.set(path, item.analysis.category);
                    changed = true;
                }
                if (changed) safeThis->saveDatabase();
            });
        };

        if (!stopped())
        {
            report("Análisis: 0 / " + juce::String(total) + " archivos");
            std::unordered_set<std::string> known;
            known.reserve(static_cast<size_t>(analyzedPaths.size()) + 64);
            for (const auto& path : analyzedPaths)
                known.insert(path.toLowerCase().toStdString());

            for (const auto& file : files)
            {
                if (stopped() || !waitForTurn()) break;
                const auto path = file.getFullPathName();
                if (known.find(path.toLowerCase().toStdString()) != known.end())
                    ++skipped;
                else
                {
                    // La misma función que usa la vista previa; evita resultados incompatibles.
                    const auto result = analyzeAudioInBackground(job, file, rules, {}, control);
                    if (stopped()) break;
                    if (result.valid)
                    {
                        completed.push_back({ file, result });
                        ++analyzed;
                    }
                    else ++failed;
                }
                ++done;
                const auto now = juce::Time::getMillisecondCounter();
                if (completed.size() >= 8 || now - lastFlush >= 2500)
                {
                    postCompleted();
                    lastFlush = now;
                }

                if (now - lastReport >= 850)
                {
                    report("Análisis: " + juce::String(done) + "/" + juce::String(total)
                           + " (" + juce::String(total > 0 ? (done * 100 / total) : 100)
                           + "%) | Nuevos: " + juce::String(analyzed));
                    lastReport = now;
                }
            }
        }

        // Incluso tras Cancelar: los resultados ya terminados se entregan y guardan.
        postCompleted();
        const bool wasCancelled = stopped();
        juce::MessageManager::callAsync([safeThis, generation, wasCancelled, done, total, analyzed, skipped, failed]()
        {
            if (safeThis == nullptr || safeThis->batchGeneration != generation) return;
            safeThis->batchRunning = false;
            safeThis->batchControl.reset();
            const auto summary = (wasCancelled ? "Cancelado: " : "Finalizado: ")
                + juce::String(done) + "/" + juce::String(total)
                + " | Nuevos " + juce::String(analyzed)
                + ", omitidos " + juce::String(skipped)
                + ", errores " + juce::String(failed);
            safeThis->batchStatusLabel.setText(summary, juce::dontSendNotification);
            // No esperamos al timer para persistir el último grupo de resultados.
            safeThis->flushDatabaseSave();
            // Refrescamos la lista visible una sola vez; nunca después de cada archivo.
            safeThis->applyFilters();
        });
    }), true);
}

void MainComponent::pauseOrResumeBatchAnalysis()
{
    if (!batchRunning || !batchControl || batchControl->cancelRequested.load()) return;
    const bool paused = !batchControl->paused.load();
    batchControl->paused.store(paused);
    batchStatusLabel.setText(paused ? "Análisis pausado (Config. para reanudar)"
                                    : "Análisis reanudado...", juce::dontSendNotification);
}

void MainComponent::cancelBatchAnalysis()
{
    if (!batchRunning || !batchControl) return;
    batchControl->cancelRequested.store(true);
    batchStatusLabel.setText("Cancelando... se guardarán los archivos terminados.",
                             juce::dontSendNotification);
}

void MainComponent::loadDatabase()
{
    std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(databaseFile);
    if (xml != nullptr)
    {
        lastSelectedFolder = xml->getStringAttribute("lastFolder", "C:\\");
        for (auto* child : xml->getChildIterator())
        {
            if (child->getTagName() == "LEARNED_RULE")
            {
                learnedPatternsDatabase.set(child->getStringAttribute("keyword"), child->getStringAttribute("type"));
            }
            else if (child->getTagName() == "FILE")
            {
                juce::String path = child->getStringAttribute("path");
                juce::String bpm = child->getStringAttribute("bpm");
                juce::String key = child->getStringAttribute("key", "--");
                juce::String cat = child->getStringAttribute("category", "--");
                juce::String fav = child->getStringAttribute("fav", "0"); // Carga si es favorito

                // No inventamos metadatos para archivos guardados únicamente como favoritos.
                if (child->hasAttribute("bpm")) { bpmDatabase.set(path, bpm); bpmLookup.set(path, bpm); }
                if (child->hasAttribute("key")) { keyDatabase.set(path, key); keyLookup.set(path, key); }
                if (child->hasAttribute("category")) { categoryDatabase.set(path, cat); categoryLookup.set(path, cat); }
                favoritesDatabase.set(path, fav);
                favoritesLookup.set(path, fav == "1");
            }
        }
    }
}

void MainComponent::saveDatabase()
{
    databaseSavePending = true;
    databaseSaveRequestedAt = juce::Time::getMillisecondCounter();
}

void MainComponent::flushDatabaseSave()
{
    if (!databaseSavePending) return;
    databaseSavePending = false;

    // Copia coherente en el hilo de UI; la construcción del XML y la escritura serán en disco aparte.
    const auto folder = lastSelectedFolder;
    const auto target = databaseFile;
    const auto rules = learnedPatternsDatabase;
    const auto bpms = bpmDatabase;
    const auto keys = keyDatabase;
    const auto categories = categoryDatabase;
    const auto favorites = favoritesDatabase;

    databaseWritePool.addJob(new BackgroundTask("Save sample cache",
        [folder, target, rules, bpms, keys, categories, favorites](BackgroundTask&)
    {
        juce::XmlElement xml("BPM_CACHE");
        xml.setAttribute("lastFolder", folder);

        for (int i = 0; i < rules.size(); ++i)
        {
            auto* rule = xml.createNewChildElement("LEARNED_RULE");
            rule->setAttribute("keyword", rules.getAllKeys()[i]);
            rule->setAttribute("type", rules.getAllValues()[i]);
        }

        // Un índice por ruta evita búsquedas cuadráticas en bibliotecas grandes.
        juce::HashMap<juce::String, juce::XmlElement*> entries;
        auto writeAttribute = [&xml, &entries](const juce::StringPairArray& values, const char* attribute)
        {
            for (int i = 0; i < values.size(); ++i)
            {
                const auto& path = values.getAllKeys()[i];
                auto* node = entries[path.toLowerCase()];
                if (node == nullptr)
                {
                    node = xml.createNewChildElement("FILE");
                    node->setAttribute("path", path);
                    entries.set(path.toLowerCase(), node);
                }
                node->setAttribute(attribute, values.getAllValues()[i]);
            }
        };

        writeAttribute(bpms, "bpm");
        writeAttribute(keys, "key");
        writeAttribute(categories, "category");
        writeAttribute(favorites, "fav");

        // JUCE File::replaceWithText crea su propio archivo temporal y evita
        // depender de la construcción/copia de juce::TemporaryFile aquí.
        target.replaceWithText(xml.toString());
    }), true);
}

// {* NUEVO: El actualizador de la pantalla MIDI *}
void MainComponent::timerCallback()
{
    const auto now = juce::Time::getMillisecondCounter();

    // Un guardado periódico cada 10 s protege lo completado sin escribir XML
    // después de cada resultado; fuera del lote mantenemos 650 ms.
    if (databaseSavePending &&
        ((batchRunning && now - lastBatchPersistAt >= 10000)
         || (!batchRunning && now - databaseSaveRequestedAt >= 650)))
    {
        lastBatchPersistAt = now;
        flushDatabaseSave();
    }

    if (batchControl)
    {
        const bool grace = static_cast<juce::int32>(previewPauseUntil - now) > 0;
        batchControl->playbackActive.store(grace || audioPlayer.isPreviewPlaying());
    }

    if (pendingAnalysisFile != juce::File() && now - pendingAnalysisRequestedAt >= 450)
    {
        const auto file = pendingAnalysisFile;
        pendingAnalysisFile = juce::File();
        if (file.existsAsFile()) analyzeBPM(file);
    }

    if (filterInputPending && (juce::Time::getMillisecondCounter() - lastFilterInputTime) >= 120)
    {
        filterInputPending = false;
        applyFilters();
    }

    if (audioPlayer.getIsMidiLoaded())
    {
        waveformDisplay.setMidiMode(true);
        waveformDisplay.setMidiSequence(audioPlayer.getMidiSequence()); // <--- NUEVO
        waveformDisplay.updateMidiPosition(audioPlayer.getMidiPosition(), audioPlayer.getMidiLength());
    }
    else
    {
        waveformDisplay.setMidiMode(false);
        waveformDisplay.setMidiSequence(nullptr); // <--- NUEVO
    }
}