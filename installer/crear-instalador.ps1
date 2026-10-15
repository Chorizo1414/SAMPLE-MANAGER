param(
    [switch]$Portable
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$releaseExe = Join-Path $repo 'Builds\VisualStudio2022\x64\Release\App\SAMPLE MANAGER.exe'
$installerFile = Join-Path $PSScriptRoot 'JoHuCrateFree.nsi'
$outDir = Join-Path $PSScriptRoot 'output'

if (-not (Test-Path -LiteralPath $releaseExe -PathType Leaf)) {
    throw "No existe el EXE Release: $releaseExe. Compila Release / x64 en Visual Studio antes de continuar."
}
if (-not (Test-Path -LiteralPath $installerFile -PathType Leaf)) {
    throw "Falta el archivo NSIS: $installerFile"
}

$temp = Join-Path ([System.IO.Path]::GetTempPath()) ('JoHuCrateFree-' + [Guid]::NewGuid().ToString('N'))
$stage = Join-Path $temp 'JoHu Crate Free'
$licenses = Join-Path $stage 'Licencias'

New-Item -ItemType Directory -Path $licenses -Force | Out-Null
New-Item -ItemType Directory -Path $outDir -Force | Out-Null

try {
    # Lista blanca: nunca incluir Source, Builds, JuceLibraryCode ni muestras de audio.
    Copy-Item -LiteralPath $releaseExe -Destination (Join-Path $stage 'JoHuCrateFree.exe')
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'LEEME.txt') -Destination (Join-Path $stage 'LEEME.txt')
    Copy-Item -LiteralPath (Join-Path $repo 'Source\Signalsmith\LICENSE-STRETCH.txt') -Destination (Join-Path $licenses 'Signalsmith-Stretch-MIT.txt')
    Copy-Item -LiteralPath (Join-Path $repo 'Source\Signalsmith\LICENSE-LINEAR.txt') -Destination (Join-Path $licenses 'Signalsmith-Linear-MIT.txt')
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'KissFFT-BSD-3-Clause.txt') -Destination (Join-Path $licenses 'KissFFT-BSD-3-Clause.txt')

    if ($Portable) {
        $destination = Join-Path $outDir 'JoHu-Crate-Free-Portable-v0.9.0-beta.zip'
        Compress-Archive -LiteralPath $stage -DestinationPath $destination -CompressionLevel Optimal -Force
    }
    else {
        $nsis = Get-Command 'makensis.exe' -ErrorAction SilentlyContinue
        $compiler = if ($null -ne $nsis) { $nsis.Source } else { $null }

        if (-not $compiler) {
            $candidates = @(
                (Join-Path $env:ProgramFiles 'NSIS\makensis.exe'),
                (Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'NSIS\makensis.exe')
            )
            foreach ($candidate in $candidates) {
                if (Test-Path -LiteralPath $candidate -PathType Leaf) {
                    $compiler = $candidate
                    break
                }
            }
        }
        if (-not $compiler) {
            throw 'Falta NSIS 3.x. Instálalo desde https://nsis.sourceforge.io/Download y vuelve a ejecutar este script.'
        }

        & $compiler ("/DSTAGING=" + $stage) $installerFile
        if ($LASTEXITCODE -ne 0) {
            throw "NSIS terminó con código $LASTEXITCODE"
        }
        $destination = Join-Path $outDir 'JoHu-Crate-Free-Setup-v0.9.0-beta.exe'
    }

    if (-not (Test-Path -LiteralPath $destination -PathType Leaf)) {
        throw "El paquete esperado no se encontró: $destination"
    }
    Write-Host "Creado: $destination" -ForegroundColor Green
    Get-FileHash -LiteralPath $destination -Algorithm SHA256 | Format-List Algorithm, Hash, Path
    Write-Host 'No publiques este paquete hasta probarlo en otro Windows y revisar el checklist de licencias.'
}
finally {
    # Solo se elimina el directorio temporal único creado por este script.
    if (Test-Path -LiteralPath $temp) {
        Remove-Item -LiteralPath $temp -Recurse -Force
    }
}
