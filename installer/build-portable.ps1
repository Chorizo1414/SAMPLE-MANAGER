# Optional portable ZIP: one clearly named folder, not raw project sources.
# Requires Visual C++ Redistributable x64 to be present on the destination PC.
$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Split-Path -Parent $here
$exe = Join-Path $root "Builds\VisualStudio2022\x64\Release\App\SAMPLE MANAGER.exe"
$dist = Join-Path $here "dist"
$stageRoot = Join-Path $dist "_portable_staging"
$folder = Join-Path $stageRoot "JoHu Crate Free"
$licenses = Join-Path $folder "Licencias"
$zip = Join-Path $dist "JoHuCrate-Free-0.9.0-beta-Portable-win64.zip"
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) {
    throw "Compila Release x64 antes de crear el ZIP portable: $exe"
}
New-Item -ItemType Directory -Force -Path $dist | Out-Null
if (Test-Path -LiteralPath $stageRoot) { Remove-Item -LiteralPath $stageRoot -Recurse -Force }
New-Item -ItemType Directory -Force -Path $licenses | Out-Null
Copy-Item -LiteralPath $exe -Destination (Join-Path $folder "JoHu Crate Free.exe")
Copy-Item -LiteralPath (Join-Path $here "USER_GUIDE.txt") -Destination (Join-Path $folder "LEEME.txt")
Copy-Item -LiteralPath (Join-Path $here "THIRD_PARTY_NOTICES.txt") -Destination (Join-Path $folder "AVISOS-DE-TERCEROS.txt")
Copy-Item -LiteralPath (Join-Path $here "KissFFT-BSD.txt") -Destination (Join-Path $licenses "KissFFT-BSD.txt")
Copy-Item -LiteralPath (Join-Path $root "Source\Signalsmith\LICENSE-STRETCH.txt") -Destination (Join-Path $licenses "Signalsmith-Stretch-MIT.txt")
Copy-Item -LiteralPath (Join-Path $root "Source\Signalsmith\LICENSE-LINEAR.txt") -Destination (Join-Path $licenses "Signalsmith-Linear-MIT.txt")
if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
Compress-Archive -Path $folder -DestinationPath $zip -CompressionLevel Optimal
Remove-Item -LiteralPath $stageRoot -Recurse -Force
Write-Host "ZIP PORTABLE (opcional): $zip"
Get-FileHash -Algorithm SHA256 -LiteralPath $zip | Format-List
Write-Warning "Portable requiere Microsoft Visual C++ Runtime x64 instalado. Recomendar el instalador a usuarios normales."
