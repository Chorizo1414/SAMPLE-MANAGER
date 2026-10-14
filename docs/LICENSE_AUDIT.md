# JoHu Crate: auditoría preliminar de licencias

ESTADO: EN REVISIÓN. No publicar ejecutables Free ni Pro basándose solo en esta nota.
Esta evaluación técnica no reemplaza una revisión legal del paquete final.

## Componentes
- JUCE 9.0.4: bajo JUCE Starter, el uso comercial está permitido dentro de sus límites y condiciones. Confirmar el tipo de licencia aplicable, el límite de ingresos y la compatibilidad de las dependencias. Fuente: https://juce.com/legal/juce-9-licence/
- BTrack: GPLv3. Sus fuentes se han retirado de este prototipo Free propietario. Verificar que no quede en los binarios ni en los paquetes de descarga. Fuente: https://github.com/adamstark/BTrack
- SoundTouch (incluye BPMDetect): LGPLv2.1 o licencia comercial alternativa. La distribución comercial es posible bajo condiciones LGPL. Como hoy la biblioteca se compila directamente dentro del ejecutable, aún debemos proporcionar textos y fuentes, y cumplir la posibilidad de reemplazo o reenlazado de la biblioteca; evaluar biblioteca dinámica o licencia comercial. Fuente: https://www.surina.net/soundtouch/license.html
- Kiss FFT: BSD-3-Clause. Uso comercial permitido con conservación de copyright, licencia y exención de garantías. Se ha separado en Source/KissFFT. Fuente: https://github.com/mborgerding/kissfft/blob/master/COPYING

## Implementación experimental
- El análisis de BPM usa soundtouch::BPMDetect en lugar de BTrack.
- Los BPM explícitos en el nombre del archivo tienen prioridad; de lo contrario se analiza el sonido.
- Las tonalidades reconocidas en el nombre prevalecen sobre la estimación automática.
- Se mantienen reproducción, favoritos, escaneos recursivos, análisis en segundo plano, pausa y cancelación.

## Antes de distribuir al público
- [ ] Compilar Debug y Release x64 en Visual Studio 2022 con JUCE 9.0.4.
- [ ] Comparar BPM de 20-30 loops etiquetados y 20-30 sin etiquetas contra la versión anterior, incluidas detecciones a mitad/doble tempo.
- [ ] Comprobar tonalidad de nombres y estimación FFT, y revisar resultados inciertos.
- [ ] Probar análisis por lotes, pausa, cancelación, búsqueda, favoritos y persistencia XML.
- [ ] Verificar que BTrack no se compile ni se incluya en la distribución.
- [ ] Preparar licencia LGPLv2.1 completa y avisos para SoundTouch, código fuente correspondiente y mecanismo de reenlazado o licencia comercial.
- [ ] Preparar licencia BSD-3-Clause completa y avisos para Kiss FFT.
- [ ] Confirmar la compatibilidad de la distribución concreta con JUCE Starter mediante asesoramiento jurídico o consulta con los titulares.
- [ ] Verificar las licencias de futuras dependencias antes de incorporarlas.

Advertencia: una licencia que permite usos comerciales no garantiza por sí sola que una forma concreta de empaquetado cumpla todos sus requisitos.
