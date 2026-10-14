# JoHu Crate — revisión técnica de licencias (rama fix2)

**Estado: EN PRUEBAS. No publicar ejecutables Free o Pro hasta completar la validación final.**

## Componentes que intervienen en esta versión de prueba

| Componente | Licencia | Estado |
| --- | --- | --- |
| JUCE 9.0.4 | JUCE 9 Starter (sujeto al plan y términos del usuario) | Uso comercial permitido dentro de los límites de ingresos y demás condiciones. Confirmar la licencia efectivamente activada. |
| Signalsmith Stretch | MIT | Usado para pitch y tempo independientes. Archivo y licencia incluidos en Source/Signalsmith/. |
| Signalsmith Linear | MIT | Dependencia DSP de Stretch. Archivos y licencia incluidos en Source/Signalsmith/. |
| Kiss FFT | BSD-3-Clause | Detección de tonalidad; debe adjuntarse su aviso de copyright y licencia en el producto distribuido. |
| BPM JoHu | Implementación original | Estimador basado en envolvente y autocorrelación en Source/TempoDetector.h. |
| BTrack (GPLv3) | NO incluido en esta rama | Eliminado en una etapa anterior; comprobar binario y paquete final. |
| SoundTouch (LGPLv2.1) | NO incluido en esta rama | Sustituido por Signalsmith y TempoDetector. No enlazar archivos de la carpeta antigua. |

## Enlaces y licencias oficiales

- JUCE 9: https://juce.com/legal/juce-9-licence/
- Signalsmith Stretch: https://github.com/Signalsmith-Audio/signalsmith-stretch
- Signalsmith Linear: https://github.com/Signalsmith-Audio/linear
- MIT de Stretch: Source/Signalsmith/LICENSE-STRETCH.txt
- MIT de Linear: Source/Signalsmith/LICENSE-LINEAR.txt
- Kiss FFT: https://github.com/mborgerding/kissfft/blob/master/COPYING
- BTrack (solo antecedente): https://github.com/adamstark/BTrack

## Antes de distribuir cualquier ejecutable

- [ ] Compilar Debug x64 y Release x64 en Visual Studio 2022 con JUCE 9.0.4.
- [ ] Comparar la calidad y fluidez de Signalsmith con la version estable anterior, sobre todo en cambios de pitch/tempo mientras reproduce.
- [ ] Probar saltos de playback, loop, reset al cambiar de sample, audio corto, WAV, MP3, MIDI y cambios de dispositivo.
- [ ] Validar BPM propio con al menos 20 loops SIN etiqueta; revisar los resultados a mitad/doble tempo.
- [ ] Confirmar con inspección del paquete/binario que no se enlazan ni se distribuyen librerías GPL/LGPL antiguas.
- [ ] Incluir íntegramente licencias MIT de Stretch y Linear y aviso BSD de Kiss FFT en el paquete ZIP/instalador.
- [ ] Confirmar los términos JUCE 9 Starter, especialmente restricciones, avisos y límite de ingresos.
- [ ] Revisar otros recursos con derechos de terceros (iconos, audio de demostración, marcas) si se incorporan.
- [ ] Publicar código/avisos solo con licencias correctamente elegidas para los archivos propios y de terceros.

**Advertencia:** MIT y BSD autorizan comercialización, pero no eliminan todas las obligaciones de atribución. Esta auditoría técnica no sustituye el cumplimiento de los términos completos ni las pruebas del paquete real.
