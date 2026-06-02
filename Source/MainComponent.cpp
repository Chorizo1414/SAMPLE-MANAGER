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

    // {* NUEVO: Configuramos el botón de Audio *}
    addAndMakeVisible(settingsButton);
    settingsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2c313a));
    settingsButton.onClick = [this] {
        // Invocamos el panel nativo de JUCE para tarjetas de sonido
        auto* audioSettings = new juce::AudioDeviceSelectorComponent(deviceManager, 0, 0, 0, 2, false, false, true, false);
        audioSettings->setSize(500, 400);

        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(audioSettings);
        options.dialogTitle = "Configuracion de Audio";
        options.dialogBackgroundColour = juce::Colour(0xff181a1f);
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = false;
        options.launchAsync();
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
    favFilterButton.onClick = [this] { applyFilters(); };

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

            juce::File selectedAudioFile = filteredAudioFiles[randomIndex];
            waveformDisplay.setAudioFile(selectedAudioFile);
            
            // Ya no hay código espagueti, le decimos al motor que cargue y toque
            audioPlayer.loadFile(selectedAudioFile);
            audioPlayer.play();
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
    // 1. Botón de agregar carpeta arriba (30px de alto)
    chooseFolderButton.setBounds(0, 0, 300, 30);

    // 2. El árbol de carpetas en medio
    directoryTree.setBounds(0, 30, 300, getHeight() - 30 - 45);
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
        // 0. DIBUJAMOS LA ESTRELLA DE FAVORITO (Con más margen izquierdo)
        bool isFav = filteredFavorites[rowNumber];
        g.setColour(isFav ? juce::Colour(0xffe5c07b) : juce::Colour(0xff5c6370));
        g.setFont(16.0f);
        juce::String solidStar = juce::String::fromUTF8("\xe2\x98\x85");
        juce::String hollowStar = juce::String::fromUTF8("\xe2\x98\x86");

        // La movemos al pixel X = 15 para que no se esconda
        g.drawText(isFav ? solidStar : hollowStar, 15, 0, 30, height, juce::Justification::centred, true);

        // 1. DIBUJAMOS EL NOMBRE (Empujado a la derecha para no chocar)
        g.setColour(juce::Colour(0xffabb2bf));
        g.setFont(14.0f);

        // Lo movemos al pixel X = 50 y ajustamos su límite derecho
        g.drawText(filteredAudioFiles[rowNumber].getFileName(), 50, 0, width - 280, height, juce::Justification::centredLeft, true);

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
    if (e.x < 30 && !e.mods.isPopupMenu())
    {
        bool currentlyFav = (favoritesDatabase[filePath] == "1");
        juce::String newState = currentlyFav ? "0" : "1";

        favoritesDatabase.set(filePath, newState);
        filteredFavorites.set(row, !currentlyFav);
        saveDatabase();

        // Si el botón "Favs" está presionado y le quitas la estrella a uno, desaparece de la vista al instante
        if (favFilterButton.getToggleState()) applyFilters();
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

                    if (newType != "LOOP")
                    {
                        filteredBPMs.set(row, "--");
                        bpmDatabase.set(filePath, "--");
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
                                    categoryDatabase.set(filePath, "LOOP");

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
    filteredCategories.clear();
    filteredFavorites.clear(); // Limpiamos la memoria de estrellas

    bool showOnlyFavorites = favFilterButton.getToggleState();

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

    int typeFilter = typeBox.getSelectedId();
    int catFilter = categoryBox.getSelectedId(); // 1:Todos, 2:Loops, 3:OneShots, 4:Samples

    for (auto& file : audioFilesInFolder)
    {
        juce::String fileName = file.getFileName().toLowerCase();
        bool isMidi = file.hasFileExtension(".mid");
        juce::String filePath = file.getFullPathName();

        // Extraemos la categoría
        juce::String currentCat = categoryDatabase.containsKey(filePath) ? categoryDatabase[filePath] : "--";
        if (currentCat == "--" && !isMidi)
        {
            juce::String parentFolder = file.getParentDirectory().getFileName().toLowerCase();

            // {* CONSULTA AL MOTOR DE APRENDIZAJE AUTÓNOMO *}
            for (auto& keyword : learnedPatternsDatabase.getAllKeys())
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

        // {* NUEVO: EL ESCUDO DEL FILTRO DE ESTRELLA *}
        bool isFav = (favoritesDatabase[filePath] == "1");
        if (showOnlyFavorites && !isFav) continue; // Si el botón está presionado y no es favorito, se salta este sample

        bool matchesType = true;
        if (typeFilter == 2 && isMidi) matchesType = false;
        if (typeFilter == 3 && !isMidi) matchesType = false;

        // Evaluamos el filtro de Categoría
        bool matchesCat = true;
        if (catFilter == 2 && currentCat != "LOOP") matchesCat = false;
        if (catFilter == 3 && currentCat != "ONE SHOT") matchesCat = false;
        if (catFilter == 4 && currentCat != "SAMPLE") matchesCat = false;

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
                matchesKey = false;
        }

        if (matchesText && matchesBpm && matchesKey && matchesType && matchesCat)
        {
            filteredAudioFiles.add(file);
            filteredCategories.add(currentCat);
            filteredFavorites.add(isFav);       // Sincronizamos la estrella visual

            if (bpmDatabase.containsKey(filePath)) {
                filteredBPMs.add(bpmDatabase[filePath]);
                filteredKeys.add(keyDatabase[filePath]);
            }
            else {
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
            filteredCategories.set(index, "MIDI");
            fileList.repaintRow(index);
        }
        return;
    }

    if (bpmDatabase.containsKey(filePath))
    {
        juce::String savedBPM = bpmDatabase[filePath];
        juce::String savedKey = keyDatabase[filePath];
        if (savedKey.isEmpty()) savedKey = "--";

        bpmDisplayLabel.setText("BPM: " + savedBPM + " | Key: " + savedKey, juce::dontSendNotification);

        int index = filteredAudioFiles.indexOf(file);
        if (index >= 0) {
            filteredBPMs.set(index, savedBPM);
            filteredKeys.set(index, savedKey);
            fileList.repaintRow(index);
        }
        return;
    }

    bpmDisplayLabel.setText("Analizando audio...", juce::dontSendNotification);

    auto* reader = audioPlayer.getFormatManager().createReaderFor(file);
    if (reader != nullptr)
    {
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
            samplesRead += samplesToGet;
        }

        double detectedBPM = bpmDetector.getCurrentTempoEstimate();
        juce::String detectedKey = keyDetector.getDetectedKey();
        delete reader;

        juce::String fileNameLower = file.getFileName().toLowerCase();
        juce::String audioType;
        juce::String parentFolder = file.getParentDirectory().getFileName().toLowerCase();

        bool learnedTypeFound = false;
        for (auto& keyword : learnedPatternsDatabase.getAllKeys())
        {
            if (parentFolder.contains(keyword.toLowerCase()))
            {
                audioType = learnedPatternsDatabase[keyword];
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

        juce::String nameBPM = extractBPMFromName(file.getFileName());
        if (nameBPM != "--" && audioType == "LOOP")
        {
            detectedBPM = nameBPM.getDoubleValue();
        }

        int index = filteredAudioFiles.indexOf(file);

        if (audioType == "LOOP")
        {
            int finalBPM = std::round(detectedBPM);
            juce::String bpmString = juce::String(finalBPM);

            bpmDisplayLabel.setText(audioType + " | BPM: " + bpmString + " | Key: " + detectedKey, juce::dontSendNotification);

            if (index >= 0) {
                filteredBPMs.set(index, bpmString);
                filteredKeys.set(index, detectedKey);
                filteredCategories.set(index, audioType);
                fileList.repaintRow(index);
            }
            bpmDatabase.set(filePath, bpmString);
            keyDatabase.set(filePath, detectedKey);
            categoryDatabase.set(filePath, audioType);
        }
        else
        {
            bpmDisplayLabel.setText(audioType + " | Key: " + detectedKey, juce::dontSendNotification);

            if (index >= 0) {
                filteredBPMs.set(index, "--");
                filteredKeys.set(index, detectedKey);
                filteredCategories.set(index, audioType);
                fileList.repaintRow(index);
            }
            bpmDatabase.set(filePath, "--");
            keyDatabase.set(filePath, detectedKey);
            categoryDatabase.set(filePath, audioType);
        }
        saveDatabase();
    }
    else
    {
        bpmDisplayLabel.setText("Archivo corrupto", juce::dontSendNotification);
    }
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

                bpmDatabase.set(path, bpm);
                keyDatabase.set(path, key);
                categoryDatabase.set(path, cat);
                favoritesDatabase.set(path, fav); // Guarda en RAM
            }
        }
    }
}

    void MainComponent::saveDatabase()
    {
        juce::XmlElement xml("BPM_CACHE");

        xml.setAttribute("lastFolder", lastSelectedFolder);

        // Guardamos las reglas que el programa ha aprendido autónomamente
        for (auto keyword : learnedPatternsDatabase.getAllKeys())
        {
            auto* child = xml.createNewChildElement("LEARNED_RULE");
            child->setAttribute("keyword", keyword);
            child->setAttribute("type", learnedPatternsDatabase[keyword]);
        }

        // Guardamos el caché de los archivos individuales
        for (auto path : bpmDatabase.getAllKeys())
        {
            auto* child = xml.createNewChildElement("FILE");
            child->setAttribute("path", path);
            child->setAttribute("bpm", bpmDatabase[path]);
            child->setAttribute("key", keyDatabase[path]);
            child->setAttribute("category", categoryDatabase[path]);
        }

        xml.writeTo(databaseFile);
    }

// {* NUEVO: El actualizador de la pantalla MIDI *}
void MainComponent::timerCallback()
{
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