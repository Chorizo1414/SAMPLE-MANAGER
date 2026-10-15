; Instalador JoHu Crate Free (Windows x64)
; NSIS 3.x: https://nsis.sourceforge.io/Docs/
; Compilar con installer/crear-instalador.ps1, no directamente con makensis.
!ifndef STAGING
  !error "Falta STAGING: ejecutar crear-instalador.ps1 desde PowerShell."
!endif

!include "MUI2.nsh"

Unicode true
Name "JoHu Crate Free"
OutFile "${__FILEDIR__}\output\JoHu-Crate-Free-Setup-v0.9.0-beta.exe"
InstallDir "$LOCALAPPDATA\Programs\JoHu Crate Free"
InstallDirRegKey HKCU "Software\JoHuStudios\JoHuCrateFree" "InstallDir"
RequestExecutionLevel user
SetShellVarContext current
SetCompressor /SOLID zlib
ShowInstDetails show
ShowUninstDetails show

VIProductVersion "0.9.0.0"
VIAddVersionKey "ProductName" "JoHu Crate Free"
VIAddVersionKey "CompanyName" "JoHu Studios"
VIAddVersionKey "FileDescription" "Instalador JoHu Crate Free"
VIAddVersionKey "FileVersion" "0.9.0.0"
VIAddVersionKey "ProductVersion" "0.9.0.0"

!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\JoHuCrateFree.exe"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "Spanish"

Section "Instalar JoHu Crate Free" SecMain
  SectionIn RO
  SetOutPath "$INSTDIR"
  File "${STAGING}\JoHuCrateFree.exe"
  File "${STAGING}\LEEME.txt"
  SetOutPath "$INSTDIR\Licencias"
  File "${STAGING}\Licencias\*.txt"
  SetOutPath "$INSTDIR"

  WriteUninstaller "$INSTDIR\Desinstalar.exe"
  CreateDirectory "$SMPROGRAMS\JoHu Crate Free"
  CreateShortCut "$SMPROGRAMS\JoHu Crate Free\JoHu Crate Free.lnk" "$INSTDIR\JoHuCrateFree.exe"
  CreateShortCut "$SMPROGRAMS\JoHu Crate Free\Desinstalar JoHu Crate Free.lnk" "$INSTDIR\Desinstalar.exe"
  CreateShortCut "$DESKTOP\JoHu Crate Free.lnk" "$INSTDIR\JoHuCrateFree.exe"

  WriteRegStr HKCU "Software\JoHuStudios\JoHuCrateFree" "InstallDir" "$INSTDIR"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JoHuCrateFree" "DisplayName" "JoHu Crate Free"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JoHuCrateFree" "DisplayVersion" "0.9.0 Beta"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JoHuCrateFree" "Publisher" "JoHu Studios"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JoHuCrateFree" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JoHuCrateFree" "DisplayIcon" "$INSTDIR\JoHuCrateFree.exe"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JoHuCrateFree" "UninstallString" "$\"$INSTDIR\Desinstalar.exe$\""
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JoHuCrateFree" "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JoHuCrateFree" "NoRepair" 1
SectionEnd

Section "Uninstall"
  Delete "$DESKTOP\JoHu Crate Free.lnk"
  Delete "$SMPROGRAMS\JoHu Crate Free\JoHu Crate Free.lnk"
  Delete "$SMPROGRAMS\JoHu Crate Free\Desinstalar JoHu Crate Free.lnk"
  RMDir "$SMPROGRAMS\JoHu Crate Free"

  Delete "$INSTDIR\JoHuCrateFree.exe"
  Delete "$INSTDIR\LEEME.txt"
  Delete "$INSTDIR\Licencias\*.txt"
  RMDir "$INSTDIR\Licencias"
  Delete "$INSTDIR\Desinstalar.exe"
  RMDir "$INSTDIR"

  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\JoHuCrateFree"
  DeleteRegKey HKCU "Software\JoHuStudios\JoHuCrateFree"
  ; NO borrar los caches, favoritos ni configuraciones en AppData.
SectionEnd
