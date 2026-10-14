# JoHu Crate Free — alcance inicial y pruebas

## Identidad
- Nombre de pantalla: JoHu Crate Free - by JoHu Studios.
- Se conserva por ahora el nombre técnico del proyecto Projucer y la ruta del XML de favoritos y BPM, para evitar incompatibilidades o pérdida de preferencias. Más adelante se prepararán instaladores separados.

## Funciones Free que deben permanecer
- Explorar carpetas y buscar samples, filtrarlos y marcar favoritos.
- Reproducción de audio y MIDI con waveform, Signalsmith Stretch (MIT) y ajustes musicales de Key/BPM.
- Analizar carpeta raíz, carpeta seleccionada y resultados de búsqueda, en segundo plano y con pausa/cancelación.
- BPM desde nombres de archivos; estimación propia con TempoDetector cuando falta BPM.
- Tonalidad desde nombre o análisis con KeyDetector y Kiss FFT.
- Corrección manual de BPM y restauración explícita de valores desde el nombre.
- Arrastrar el archivo ORIGINAL a un DAW. Es intencional que los cambios de BPM y Key de la vista previa no se apliquen al archivo arrastrado.

## Funciones previstas solo para Pro (NO implementadas en esta rama)
- Exportación/renderizado de audio con pitch y BPM transformados.
- Exportación procesada por lotes.
- Soporte de varias raíces en discos distintos y temas visuales adicionales (futuro).

## Antes de ofrecer descargas públicas
- [ ] Compilar Debug x64 y Release x64 en Visual Studio 2022 con JUCE 9.0.4.
- [ ] Probar arrastre con audio original tras cambiar pitch y BPM; confirmar que el archivo sigue intacto.
- [ ] Verificar reproducción, favoritos, búsqueda y análisis de BPM con nombres sin y con etiquetas.
- [ ] Probar en otra máquina con Windows y otros dispositivos de audio.
- [ ] Validar la nueva integración Signalsmith Stretch + Linear (MIT) y comprobar que no se distribuya SoundTouch ni BTrack. Incluir licencias MIT/BSD y verificar los términos JUCE 9 Starter.
- [ ] Preparar avisos y licencias de terceros, y revisar docs/LICENSE_AUDIT.md.
- [ ] Preparar ZIP/instalador, icono, documentación de usuario y página de descargas.

**No publicar ejecutables Free/Pro como producto comercial todavía:** el cambio de exportación no resuelve por sí solo los requisitos legales ni verifica una compilación funcional.
