# Checklist para publicar JoHu Crate Free v0.9 Beta

**Estado actual: código funcional validado por el autor en Debug; distribución pública pendiente.**
**NO crear una Release descargable hasta completar los bloqueos.** Mantener `master` y Pro sin cambios.

## 1. Bloqueos de publicación

- [ ] **Licencias:** confirmar por escrito la modalidad de JUCE 9.0.4 aplicable y los requisitos de distribución, incluido el límite de ingresos de Starter. Fuente: https://juce.com/legal/juce-9-licence/
- [ ] **SoundTouch LGPLv2.1:** determinar con precisión la forma de cumplimiento para el enlace actual (código compilado dentro del ejecutable). Preparar atribuciones, texto legal completo, código fuente correspondiente y los medios de modificación/reenlazado exigibles, **o** obtener una licencia comercial específica. Ver https://www.surina.net/soundtouch/license.html y https://soundtouch.surina.net/faq.html. No asumir que enlazar estáticamente es automáticamente libre de obligaciones.
- [ ] **Kiss FFT (BSD):** verificar licencia de la copia integrada, incluir avisos de copyright, texto de licencia y disclaimers aplicables en ZIP/instalador.
- [ ] **BTrack:** verificar que ningún archivo GPLv3 suyo aparezca en el binario o en el paquete distribuido; el historial del repositorio debe tratarse de manera separada.
- [ ] **Licencia del software de JoHu Studios:** escoger la licencia para el código original sin atribuir a JoHu Studios la propiedad de componentes de terceros.

## 2. Pruebas técnicas

- [ ] Desde `feature/johu-crate-free`, compilar y ejecutar **Release x64** con Visual Studio 2022 y JUCE 9.0.4. Verificar que no haya errores al compilar desde una copia limpia.
- [ ] Abrir la aplicación sin Visual Studio en un Windows x64 diferente; verificar que no falten DLL, runtime de Visual C++ ni drivers. Incluir el redistribuible oficial si corresponde.
- [ ] Probar dispositivos de audio: audio predeterminado, interfaz conectada, desconexión del dispositivo y ASIO ocupado. Mostrar error amigable si no se puede iniciar audio.
- [ ] Repetir prueba de arrastre con original y con pitch/BPM modificados en la vista previa. Ambos deben entregar **el original**, sin crear `Processed_*.wav`.
- [ ] Probar búsqueda, cambio de carpetas, favoritos después de reiniciar, clasificación explícita ONE SHOT/LOOP/SAMPLE y correcciones manuales.
- [ ] Verificar análisis de raíz, carpeta y búsqueda en segundo plano, con pausa, cancelación y resultados guardados.
- [ ] Validar sonidos WAV, MP3 y MIDI, muestras corruptas y librerías grandes.
- [ ] Comparar BPM y tonalidad de un lote de samples frente a datos conocidos; evitar suponer que una estimación es exacta.
- [ ] Verificar que versiones antiguas no pierden favoritos ni BPM al actualizar: hacer copia de `SampleManager_BPM_Cache.xml` antes de migrar.

## 3. Empaquetado y presentación

- [ ] Decidir identidad visual final: JoHu Crate Free — by JoHu Studios. Cambiar el nombre de binario y de proyecto **solo cuando haya plan de migración de caché** y compilación probada.
- [ ] Crear icono original (no reutilizar logos de JUCE) y capturas de pantalla.
- [ ] Crear ZIP portable o instalador para Windows x64 desde la carpeta de salida Release, con ejecutable, licencias, avisos y manual.
- [ ] Probar ZIP/instalador en un Windows limpio; comprobar desinstalación y persistencia segura de datos de usuario.
- [ ] Añadir número de versión visible y notas de versión.
- [ ] Preparar página pública de descarga, instrucciones, informe de errores, donaciones opcionales y política de privacidad si se recopilan datos.

## 4. Lanzamiento

- [ ] Beta cerrada con 5-10 productores que prueben sistemas diferentes y FL Studio/Ableton/otros DAWs.
- [ ] Solucionar fallos de alta prioridad, especialmente cierres y bloqueos de audio.
- [ ] Solo tras resolver licencias y pruebas: crear GitHub Release o un repositorio público de descargas con binario, suma SHA-256 y notas claras.
- [ ] Publicar en redes sociales un video demostrando búsquedas, favoritos y arrastre **original** en Free.
- [ ] Mantener Pro en su rama aparte y comenzar su rediseño después de congelar una Free estable.

## Criterio de listo

**Ready for Public Beta** = licencias resueltas + compilación Release reproducible + prueba en otra computadora + descarga documentada + arrastre original verificado.

Este documento es un registro de ingeniería, no asesoría jurídica.
