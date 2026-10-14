# JoHu Crate Free 🎵

**by JoHu Studios** — Busca, escucha y organiza tus samples sin perder el flujo creativo.

> **Estado: beta en preparación.** El código fuente está en desarrollo y todavía **no hay un instalador público autorizado para descarga**. La compilación y el cumplimiento de licencias de terceros siguen en revisión.

## ¿Qué permite hacer?

- Explorar carpetas y subcarpetas con miles de samples.
- Buscar por nombre, filtrar sonidos y guardar favoritos.
- Escuchar WAV, MP3 y MIDI, con forma de onda y controles de reproducción.
- Leer BPM y tonalidad desde nombres de archivo y analizar audio cuando falten datos.
- Cambiar temporalmente tonalidad y BPM para **escuchar** cómo quedaría el sample.
- Analizar la carpeta raíz, una carpeta seleccionada o los resultados de una búsqueda, en segundo plano, con pausa y cancelación.
- Arrastrar el **archivo original** al DAW; los cambios de pitch y BPM de la vista previa **no** se exportan en Free.

## Free frente a Pro

JoHu Crate Free será una aplicación funcional y gratuita, con donaciones voluntarias opcionales.

La edición Pro se desarrollará por separado. Su característica principal prevista es exportar o arrastrar un WAV **con las transformaciones de pitch y BPM aplicadas**. Exportación en lote, gestión de varias raíces y temas visuales alternativos son planes futuros, no funciones disponibles de Free.

## Requisitos de desarrollo

- Windows x64.
- Visual Studio 2022 con soporte C++.
- JUCE 9.0.4 instalado localmente (el proyecto usa rutas locales del entorno del autor).

Para desarrolladores: abrir `SAMPLE MANAGER.jucer` con Projucer, comprobar las rutas de JUCE, guardar y abrir la solución en `Builds/VisualStudio2022/`. Compilar en Debug para pruebas o Release para preparar un paquete de distribución. **La compilación del proyecto por sí sola no significa que su binario esté aprobado para publicación.**

## Estado de distribución y licencias

Los códigos de terceros tienen licencias independientes. Se retiró BTrack del árbol actual de esta edición, se utiliza SoundTouch para BPM y Stretch y Kiss FFT para tonalidad. Hay que terminar de revisar cómo se distribuyen SoundTouch, JUCE y los avisos BSD.

Revisa [auditoría de dependencias](docs/LICENSE_AUDIT.md) y [checklist de lanzamiento](docs/RELEASE_FREE_CHECKLIST.md) antes de entregar un ejecutable a otras personas. Esta sección **no** declara una licencia global para todo el repositorio: se definirá después de revisar los componentes y los derechos aplicables.

## Comentarios y errores

Cuando comience la beta pública, se podrán reportar problemas mediante GitHub Issues. Los resultados automáticos de BPM y tonalidad son estimaciones y pueden requerir corrección manual.

**JoHu Crate — by JoHu Studios.** Hecho para productores.
