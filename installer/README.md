# Instalador Windows — JoHu Crate Free 0.9 Beta

**Estado: scripts preparados en `fix2`, instalador aún NO compilado/probado en Windows. NO publicar una descarga antes de las pruebas de distribución.**

## ¿Qué descargará el productor?

**Recomendado:** `JoHu-Crate-Free-Setup-v0.9.0-beta.exe`.

- Instala solamente la aplicación standalone, un LEEME y tres avisos de licencia.
- Crea acceso directo en el escritorio y menú Inicio.
- Aparece en "Aplicaciones instaladas" para poder desinstalarlo.
- Se instala únicamente para el usuario actual, en AppData/Local/Programs; no solicita permisos de administrador.
- Nunca borra los favoritos ni la caché de BPM al desinstalarse.

**Opcional:** `JoHu-Crate-Free-Portable-v0.9.0-beta.zip`.

Contiene exactamente una carpeta llamada `JoHu Crate Free` con `JoHuCrateFree.exe`, `LEEME.txt` y `Licencias`. Basta con extraer y abrir `JoHuCrateFree.exe`. No incluye Projucer, código fuente, archivos de Visual Studio ni samples.

## Herramienta elegida

**NSIS 3.x**: https://nsis.sourceforge.io/Download

NSIS indica expresamente que admite el uso comercial gratuito. Se utiliza compresión **zlib**, para evitar otras obligaciones de módulos opcionales: https://nsis.sourceforge.io/License

## Generación en Windows

1. Abre el proyecto `fix2/johu-crate-free` con Projucer 9.0.4 / Visual Studio 2022.
2. Compila en **Release | x64** (no Debug). Debe aparecer `Builds/VisualStudio2022/x64/Release/App/SAMPLE MANAGER.exe`.
3. Instala **NSIS 3.x** desde su web oficial.
4. Cierra la aplicación si está ejecutándose.
5. En PowerShell, situado en la raíz del repositorio, ejecuta:

   ```powershell
   powershell -NoProfile -ExecutionPolicy Bypass -File ".\installer\crear-instalador.ps1"
   ```

   La excepción de ejecución de PowerShell se aplica únicamente a ese proceso; no altera la política global.

6. El resultado quedará en `installer/output/JoHu-Crate-Free-Setup-v0.9.0-beta.exe`. El script imprime el **SHA-256** del archivo.
7. **NO subir el ejecutable todavía**: instalarlo en otro Windows, ejecutar, reproducir, importar WAV/MP3/MIDI, analizar, cambiar pitch/tempo y arrastrar samples a FL Studio/Ableton. Probar también desinstalación y conservación de favoritos.
8. Al distribuirlo públicamente, usar GitHub Releases o la página de descargas; no cargar archivos binarios al código fuente del repositorio.

Para generar el ZIP portable, si finalmente se desea:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\installer\crear-instalador.ps1" -Portable
```

## Dependencias y obligaciones

El paquete **sí** incluye las licencias completas de Signalsmith Stretch (MIT), Signalsmith Linear (MIT) y Kiss FFT (BSD-3-Clause). Los archivos provienen del repositorio, y el empaquetador usa una lista blanca para no incorporar accidentalmente archivos de BTrack/SoundTouch ni proyectos de desarrollo.

**JUCE 9.0.4** sigue sujeto a la licencia/plan específico aplicable a JoHu Studios. Ver `docs/LICENSE_AUDIT.md` y verificar las condiciones finales antes de publicar.

La compilación Release actual usa el **runtime Visual C++ dinámico** (`MultiThreadedDLL`). En PCs sin el runtime de Microsoft puede requerirse instalar el **Visual C++ Redistributable x64**: https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist. No se incluye automáticamente en este prototipo para evitar un ejecutable no verificado o una redistribución indebida.

## Pendientes importantes

- [ ] Ejecutar `crear-instalador.ps1` en Windows con NSIS real; comprobar que el instalador compila correctamente.
- [ ] Probar instalación/desinstalación y un segundo equipo Windows limpio.
- [ ] Confirmar que la carpeta instalada contiene solo ejecutable, LEEME y Licencias.
- [ ] Cambiar el icono predeterminado por un icono original de JoHu Studios.
- [ ] Decidir firma de código para reducir advertencias de SmartScreen (los instaladores sin firmar pueden activarlas).
- [ ] Confirmar requisitos de JUCE Starter y avisos legales de terceros.
- [ ] Publicar primera beta solamente después de esas verificaciones.

**No se ha creado una versión VST3/AU. Esta edición es únicamente standalone.**
