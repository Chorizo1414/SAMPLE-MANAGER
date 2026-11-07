# JoHu Crate Free x64 installer builder (Windows PowerShell 5.1+)
# Runs locally; never publishes or uploads executable files automatically.
$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Split-Path -Parent $here
$releaseExe = Join-Path $root "Builds\VisualStudio2022\x64\Release\App\SAMPLE MANAGER.exe"
$script = Join-Path $here "JoHuCrate-Free.nsi"
$redistDir = Join-Path $here "prereqs"
$redist = Join-Path $redistDir "vc_redist.x64.exe"
$dist = Join-Path $here "dist"
$output = Join-Path $dist "JoHuCrate-Free-0.9.0-beta-Setup-x64.exe"

if (-not (Test-Path -LiteralPath $releaseExe -PathType Leaf)) {
    throw "No encuentro el ejecutable Release x64: $releaseExe. Compila Release / x64 en Visual Studio primero."
}

$makensis = Get-Command makensis.exe -ErrorAction SilentlyContinue
if ($makensis) {
    $compiler = $makensis.Source
} else {
    $candidates = @()
    if (${env:ProgramFiles(x86)}) { $candidates += Join-Path ${env:ProgramFiles(x86)} "NSIS\makensis.exe" }
    if ($env:ProgramFiles) { $candidates += Join-Path $env:ProgramFiles "NSIS\makensis.exe" }
    $compiler = $candidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
}
if (-not $compiler) {
    throw "Instala NSIS desde https://nsis.sourceforge.io/Download y vuelve a ejecutar este script."
}

New-Item -ItemType Directory -Force -Path $redistDir, $dist | Out-Null
if (-not (Test-Path -LiteralPath $redist -PathType Leaf)) {
    Write-Host "Descargando Microsoft Visual C++ Redistributable x64 desde Microsoft..."
    Invoke-WebRequest -Uri "https://aka.ms/vc14/vc_redist.x64.exe" -OutFile $redist -UseBasicParsing
}
$signature = Get-AuthenticodeSignature -FilePath $redist
if ($signature.Status -ne "Valid" -or $signature.SignerCertificate.Subject -notmatch "Microsoft") {
    throw "La firma del redistribuible de Microsoft no es valida. No se generara el instalador."
}

Write-Host "Creando instalador de JoHu Crate Free..."
& $compiler "/V3" $script
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $output -PathType Leaf)) {
    throw "NSIS no genero el instalador. Revisa la salida del compilador."
}
Write-Host "INSTALADOR CREADO: $output"
Get-FileHash -Algorithm SHA256 -LiteralPath $output | Format-List
Write-Warning "No publiques todavia: prueba instalacion/desinstalacion en otro Windows y revisa las condiciones de distribucion."
