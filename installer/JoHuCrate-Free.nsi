; JoHu Crate Free — Windows x64 setup (NSIS)
; Use installer/build-installer.ps1. No VST/DAW plugin is installed.
; Do not publish before legal/release checklist is complete.
Unicode true
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

!define APP_NAME "JoHu Crate Free"
!define APP_VERSION "0.9.0-beta"
!define APP_PUBLISHER "JoHu Studios"
!define INSTALL_REGKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\JoHuCrateFree"
!define EXE_INPUT "${__FILEDIR__}\..\Builds\VisualStudio2022\x64\Release\App\SAMPLE MANAGER.exe"
!define REDIST_INPUT "${__FILEDIR__}\prereqs\vc_redist.x64.exe"

Name "${APP_NAME}"
OutFile "${__FILEDIR__}\dist\JoHuCrate-Free-0.9.0-beta-Setup-x64.exe"
InstallDir "$PROGRAMFILES64\JoHu Studios\JoHu Crate Free"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
CRCCheck on
BrandingText "JoHu Studios"
ShowInstDetails show
ShowUninstDetails show
VIProductVersion "0.9.0.0"
VIAddVersionKey "ProductName" "${APP_NAME}"
VIAddVersionKey "CompanyName" "${APP_PUBLISHER}"
VIAddVersionKey "FileDescription" "Instalador de JoHu Crate Free"
VIAddVersionKey "FileVersion" "${APP_VERSION}"

!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\JoHu Crate Free.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Abrir JoHu Crate Free"
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "English"

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_ICONSTOP "JoHu Crate Free requiere Windows de 64 bits."
    Abort
  ${EndIf}
FunctionEnd

Section "-Microsoft Visual C++ Runtime"
  SetRegView 64
  ClearErrors
  ReadRegDWORD $R0 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Installed"
  StrCmp $R0 "1" vc_runtime_ready

  InitPluginsDir
  SetOutPath "$PLUGINSDIR"
  File "/oname=vc_redist.x64.exe" "${REDIST_INPUT}"
  DetailPrint "Instalando Microsoft Visual C++ Redistributable (x64)..."
  ClearErrors
  ExecWait '"$PLUGINSDIR\vc_redist.x64.exe" /install /passive /norestart' $R1
  IfErrors vc_runtime_failed
  StrCmp $R1 "0" vc_runtime_ready
  StrCmp $R1 "3010" vc_runtime_ready
  Goto vc_runtime_failed
vc_runtime_failed:
  MessageBox MB_ICONSTOP "No se pudo instalar Visual C++ Redistributable (codigo $R1). Instalacion cancelada."
  Abort
vc_runtime_ready:
SectionEnd

Section "JoHu Crate Free" SEC_MAIN
  SectionIn RO
  SetRegView 64
  SetShellVarContext all
  SetOutPath "$INSTDIR"
  ; Build output is still named SAMPLE MANAGER.exe, but install as JoHu Crate Free.
  File "/oname=JoHu Crate Free.exe" "${EXE_INPUT}"

  SetOutPath "$INSTDIR\Licencias"
  File "/oname=Signalsmith-Stretch-MIT.txt" "${__FILEDIR__}\..\Source\Signalsmith\LICENSE-STRETCH.txt"
  File "/oname=Signalsmith-Linear-MIT.txt" "${__FILEDIR__}\..\Source\Signalsmith\LICENSE-LINEAR.txt"
  File "/oname=KissFFT-BSD.txt" "${__FILEDIR__}\KissFFT-BSD.txt"

  SetOutPath "$INSTDIR"
  File "/oname=AVISOS-DE-TERCEROS.txt" "${__FILEDIR__}\THIRD_PARTY_NOTICES.txt"
  File "/oname=LEEME.txt" "${__FILEDIR__}\USER_GUIDE.txt"
  WriteUninstaller "$INSTDIR\Desinstalar JoHu Crate Free.exe"

  CreateDirectory "$SMPROGRAMS\JoHu Studios"
  CreateShortcut "$SMPROGRAMS\JoHu Studios\JoHu Crate Free.lnk" "$INSTDIR\JoHu Crate Free.exe"
  CreateShortcut "$SMPROGRAMS\JoHu Studios\Desinstalar JoHu Crate Free.lnk" "$INSTDIR\Desinstalar JoHu Crate Free.exe"
  CreateShortcut "$DESKTOP\JoHu Crate Free.lnk" "$INSTDIR\JoHu Crate Free.exe"

  WriteRegStr HKLM "${INSTALL_REGKEY}" "DisplayName" "${APP_NAME}"
  WriteRegStr HKLM "${INSTALL_REGKEY}" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr HKLM "${INSTALL_REGKEY}" "Publisher" "${APP_PUBLISHER}"
  WriteRegStr HKLM "${INSTALL_REGKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${INSTALL_REGKEY}" "DisplayIcon" "$INSTDIR\JoHu Crate Free.exe"
  WriteRegStr HKLM "${INSTALL_REGKEY}" "UninstallString" '"$INSTDIR\Desinstalar JoHu Crate Free.exe"'
  WriteRegDWORD HKLM "${INSTALL_REGKEY}" "NoModify" 1
  WriteRegDWORD HKLM "${INSTALL_REGKEY}" "NoRepair" 1
SectionEnd

Section "Uninstall"
  SetRegView 64
  SetShellVarContext all
  Delete "$DESKTOP\JoHu Crate Free.lnk"
  Delete "$SMPROGRAMS\JoHu Studios\JoHu Crate Free.lnk"
  Delete "$SMPROGRAMS\JoHu Studios\Desinstalar JoHu Crate Free.lnk"
  RMDir "$SMPROGRAMS\JoHu Studios"

  Delete "$INSTDIR\JoHu Crate Free.exe"
  Delete "$INSTDIR\Licencias\Signalsmith-Stretch-MIT.txt"
  Delete "$INSTDIR\Licencias\Signalsmith-Linear-MIT.txt"
  Delete "$INSTDIR\Licencias\KissFFT-BSD.txt"
  RMDir "$INSTDIR\Licencias"
  Delete "$INSTDIR\AVISOS-DE-TERCEROS.txt"
  Delete "$INSTDIR\LEEME.txt"
  Delete "$INSTDIR\Desinstalar JoHu Crate Free.exe"
  RMDir "$INSTDIR"
  DeleteRegKey HKLM "${INSTALL_REGKEY}"

  ; NEVER remove AppData: keep user favorites, metadata and sample libraries.
SectionEnd
