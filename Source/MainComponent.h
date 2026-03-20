#pragma once

#include <JuceHeader.h>
#include <regex> // {* nuevo código: Librería para buscar patrones de texto *}
#include "WaveformDisplay.h"
#include "TransportControls.h"
#include "AudioPlayer.h"
#include "SoundTouch/BPMDetect.h"
#include "BTrack/BTrack.h"
#include "KeyDetector.h"
#include "CustomTheme.h"

//==============================================================================
class MainComponent  : public juce::AudioAppComponent, 
                       public juce::FileBrowserListener,
                       public juce::ListBoxModel,
                       public juce::DragAndDropContainer,
                       public juce::TextEditor::Listener,
                       public juce::ComboBox::Listener
{ 
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void selectionChanged() override;                                                                            
    void fileClicked(const juce::File& file, const juce::MouseEvent& e) override {}                              
    void fileDoubleClicked(const juce::File& file) override {}                                                   
    void browserRootChanged(const juce::File& newRoot) override {}                                               
    
    int getNumRows() override;                                                                                   
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override; 

    void listBoxItemClicked(int row, const juce::MouseEvent& e) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    void textEditorTextChanged(juce::TextEditor& editor) override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void applyFilters();
    void analyzeBPM(const juce::File& file);

private:
    juce::TextButton chooseFolderButton{ "Agrega tu carpeta de samples" };
    std::unique_ptr<juce::FileChooser> folderChooser; // Ventana nativa para elegir carpeta
    juce::TextButton settingsButton{ "Configuracion de Audio" };
    juce::String lastSelectedFolder;
    juce::TimeSliceThread directoryThread{ "Directory Thread" };
    juce::WildcardFileFilter fileFilter{ "*.wav;*.mp3;*.aif;*.mid", "*", "Audio Files" };
    juce::DirectoryContentsList directoryList{ &fileFilter, directoryThread };
    juce::FileTreeComponent directoryTree{ directoryList };

    juce::ListBox fileList;
    juce::TextEditor searchBar;
    juce::TextEditor bpmBox;
    juce::ComboBox keyBox;
    juce::ComboBox typeBox;

    juce::Label bpmDisplayLabel;

    // {* NUEVO CÓDIGO: Cabeceras de la tabla *}
    juce::Label headerName{ {}, "Name" };
    juce::Label headerKey{ {}, "Key" };
    juce::Label headerBPM{ {}, "BPM" };

    juce::Array<juce::File> audioFilesInFolder;
    juce::Array<juce::File> filteredAudioFiles;

    // {* NUEVO CÓDIGO: Arreglos paralelos y extractores *}
    juce::StringArray filteredBPMs;
    juce::StringArray filteredKeys; // <-- Para guardar la tonalidad

    juce::StringPairArray bpmDatabase; // Un diccionario que guarda "Ruta del Archivo" = "BPM"
    juce::StringPairArray keyDatabase;
    juce::File databaseFile;           // El archivo físico en tu disco duro

    void loadDatabase();
    void saveDatabase();

    juce::String extractBPMFromName(const juce::String& fileName);
    juce::String extractKeyFromName(const juce::String& fileName); // <-- Nuevo extractor

    AudioPlayer audioPlayer;

    // 2. Los paneles visuales que se conectan al motor
    WaveformDisplay waveformDisplay{ audioPlayer.getFormatManager(), audioPlayer.getTransportSource() };
    TransportControls transportControls{ audioPlayer.getTransportSource() };

    CustomTheme myCustomTheme;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};