Unicode true
!include "MUI2.nsh"
!include "x64.nsh"
!include "WinVer.nsh"
!ifndef RELEASE_DIR
  !error "Pass /DRELEASE_DIR with the staged release directory"
!endif
!ifndef SETUP_FILE
  !error "Pass /DSETUP_FILE with the output installer filename"
!endif
!define VERSION "2.5.4"
!define PRODUCT "Cracked808s"
!ifdef QA_ROOT
  RequestExecutionLevel user
  !define REGROOT HKCU
  !define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Cracked808s-Installer-QA"
  InstallDir "${QA_ROOT}\Application"
!else
  RequestExecutionLevel admin
  !define REGROOT HKLM
  !define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Cracked808s"
  InstallDir "$PROGRAMFILES64\Cracked Vault\Cracked808s"
!endif
Name "Cracked808s ${VERSION}"
OutFile "${SETUP_FILE}"
SetCompressor /SOLID lzma
SetOverwrite on
VIProductVersion "2.5.4.0"
VIAddVersionKey "ProductName" "Cracked808s"
VIAddVersionKey "CompanyName" "Cracked Vault"
VIAddVersionKey "FileDescription" "Cracked808s offline VST3 installer"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "LegalCopyright" "Copyright (C) 2026 Cracked Vault"
Var PluginDir
!include "${RELEASE_DIR}\LicenseUninstall.nsh"
!macro RequireUnlocked FilePath
  ${If} ${FileExists} "${FilePath}"
    System::Call 'kernel32::CreateFileW(w "${FilePath}", i 0xC0000000, i 0, p 0, i 3, i 0, p 0) p.r0'
    ${If} $0 == -1
      MessageBox MB_ICONSTOP "A Cracked808s file is in use or not writable. Close your DAW and standalone app, then try again. No installation files have been changed." /SD IDOK
      SetErrorLevel 1
      Abort
    ${EndIf}
    System::Call 'kernel32::CloseHandle(p r0)'
  ${EndIf}
!macroend
!macro RequireWriteSuccess
  IfErrors 0 +4
    MessageBox MB_ICONSTOP "A required installation file or registration could not be written. Setup is incomplete. Close the DAW and run Setup again. No presets were removed." /SD IDOK
    SetErrorLevel 1
    Abort
!macroend
!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TEXT "Close your DAW before installing or updating.$\r$\n$\r$\nThis installs Cracked808s into the standard Windows VST3 folder automatically. Installation is offline.$\r$\n$\r$\nUpdates are optional: manual checks or an OFF-by-default daily option in Advanced > Settings. Only first-party version metadata is requested; no audio or projects are sent. No automatic installs.$\r$\n$\r$\nYour user presets are untouched. This candidate is unsigned. See the included code signing policy."
!define MUI_FINISHPAGE_TEXT "Cracked808s is installed. Reopen your DAW and rescan plugins.$\r$\n$\r$\nFL Studio: Options > Manage plugins > Find installed plugins.$\r$\n$\r$\nVersion 2.5.4 adds optional update notices, a deeper monochrome UI and aligned Pan. Existing projects and presets remain compatible."
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${RELEASE_DIR}\LICENSE.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"
Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_ICONSTOP "Cracked808s requires 64-bit Windows."
    Abort
  ${EndIf}
  ${IfNot} ${AtLeastWin10}
    MessageBox MB_ICONSTOP "Cracked808s requires Windows 10 or later."
    Abort
  ${EndIf}
  SetRegView 64
!ifdef QA_ROOT
  StrCpy $PluginDir "${QA_ROOT}\VST3\Cracked808s.vst3"
!else
  SetShellVarContext all
  StrCpy $PluginDir "$COMMONFILES64\VST3\Cracked808s.vst3"
!endif
  !insertmacro RequireUnlocked "$PluginDir\Contents\x86_64-win\Cracked808s.vst3"
  !insertmacro RequireUnlocked "$PluginDir\Contents\x86_64-win\Cracked808sUpdate.exe"
  !insertmacro RequireUnlocked "$INSTDIR\Cracked808sUpdate.exe"
  !insertmacro RequireUnlocked "$INSTDIR\Cracked808s.exe"
FunctionEnd
Section "VST3 plugin (required)" PluginSection
  SectionIn RO
  SetOutPath "$PluginDir\Contents\x86_64-win"
  ClearErrors
  File "${RELEASE_DIR}\VST3\Cracked808s.vst3\Contents\x86_64-win\Cracked808s.vst3"
  IfErrors 0 +4
    MessageBox MB_ICONSTOP "The plugin could not be replaced. Close your DAW and run Setup again."
    SetErrorLevel 1
    Abort
  SetOutPath "$PluginDir\Contents\Resources"
  File "${RELEASE_DIR}\VST3\Cracked808s.vst3\Contents\Resources\moduleinfo.json"
  IfErrors 0 +4
    MessageBox MB_ICONSTOP "Plugin metadata could not be installed. Run Setup again."
    SetErrorLevel 1
    Abort
  SetOutPath "$PluginDir\Contents\x86_64-win"
  ClearErrors
  File "${RELEASE_DIR}\VST3\Cracked808s.vst3\Contents\x86_64-win\Cracked808sUpdate.exe"
  !insertmacro RequireWriteSuccess
  SetOutPath "$INSTDIR"
  ClearErrors
  File "${RELEASE_DIR}\LICENSE.txt"
  File "${RELEASE_DIR}\NOTICE.md"
  File "${RELEASE_DIR}\THIRD-PARTY-LICENSES.md"
  File /oname=README.txt "${RELEASE_DIR}\README.md"
  File "${RELEASE_DIR}\CODE_SIGNING.md"
  File "${RELEASE_DIR}\SECURITY.md"
  SetOutPath "$INSTDIR\Licenses"
  File /r "${RELEASE_DIR}\Licenses\*"
  !insertmacro RequireWriteSuccess
  SetOutPath "$INSTDIR"
  ClearErrors
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  !insertmacro RequireWriteSuccess
  ClearErrors
  WriteRegStr ${REGROOT} "${UNINSTALL_KEY}" "DisplayName" "Cracked808s"
  WriteRegStr ${REGROOT} "${UNINSTALL_KEY}" "Publisher" "Cracked Vault"
  WriteRegStr ${REGROOT} "${UNINSTALL_KEY}" "DisplayVersion" "${VERSION}"
  WriteRegStr ${REGROOT} "${UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr ${REGROOT} "${UNINSTALL_KEY}" "PluginDirectory" "$PluginDir"
  WriteRegStr ${REGROOT} "${UNINSTALL_KEY}" "UninstallString" '$\"$INSTDIR\Uninstall.exe$\"'
  WriteRegStr ${REGROOT} "${UNINSTALL_KEY}" "QuietUninstallString" '$\"$INSTDIR\Uninstall.exe$\" /S'
  WriteRegStr ${REGROOT} "${UNINSTALL_KEY}" "DisplayIcon" "$INSTDIR\Uninstall.exe"
  WriteRegDWORD ${REGROOT} "${UNINSTALL_KEY}" "NoModify" 1
  WriteRegDWORD ${REGROOT} "${UNINSTALL_KEY}" "NoRepair" 1
  !insertmacro RequireWriteSuccess
SectionEnd
Section "Standalone application" StandaloneSection
  SetOutPath "$INSTDIR"
  ClearErrors
  File "${RELEASE_DIR}\Standalone\Cracked808s.exe"
  File "${RELEASE_DIR}\Standalone\Cracked808sUpdate.exe"
  !insertmacro RequireWriteSuccess
!ifndef QA_ROOT
  CreateDirectory "$SMPROGRAMS\Cracked Vault"
  CreateShortcut "$SMPROGRAMS\Cracked Vault\Cracked808s.lnk" "$INSTDIR\Cracked808s.exe"
!endif
SectionEnd
Function un.onInit
  SetRegView 64
!ifndef QA_ROOT
  SetShellVarContext all
!endif
  ReadRegStr $PluginDir ${REGROOT} "${UNINSTALL_KEY}" "PluginDirectory"
  ; Do not accept a registry-supplied deletion target. Resolve the fixed path again.
!ifdef QA_ROOT
  StrCpy $PluginDir "${QA_ROOT}\VST3\Cracked808s.vst3"
!else
  StrCpy $PluginDir "$COMMONFILES64\VST3\Cracked808s.vst3"
!endif
  !insertmacro RequireUnlocked "$PluginDir\Contents\x86_64-win\Cracked808s.vst3"
  !insertmacro RequireUnlocked "$PluginDir\Contents\x86_64-win\Cracked808sUpdate.exe"
  !insertmacro RequireUnlocked "$INSTDIR\Cracked808sUpdate.exe"
  !insertmacro RequireUnlocked "$INSTDIR\Cracked808s.exe"
FunctionEnd
Section "Uninstall"
  ; Only remove the exact payload files. Extra files and user presets survive.
  ClearErrors
  Delete "$PluginDir\Contents\x86_64-win\Cracked808s.vst3"
  IfErrors 0 +3
    MessageBox MB_ICONSTOP "Close your DAW, then uninstall again."
    Abort
  Delete "$PluginDir\Contents\Resources\moduleinfo.json"
  Delete "$PluginDir\Contents\x86_64-win\Cracked808sUpdate.exe"
  RMDir "$PluginDir\Contents\x86_64-win"
  RMDir "$PluginDir\Contents\Resources"
  RMDir "$PluginDir\Contents"
  RMDir "$PluginDir"
  Delete "$INSTDIR\Cracked808s.exe"
  Delete "$INSTDIR\Cracked808sUpdate.exe"
  Delete "$INSTDIR\CODE_SIGNING.md"
  Delete "$INSTDIR\SECURITY.md"
  Delete "$INSTDIR\LICENSE.txt"
  Delete "$INSTDIR\NOTICE.md"
  Delete "$INSTDIR\THIRD-PARTY-LICENSES.md"
  Delete "$INSTDIR\README.txt"
  !insertmacro RemoveLicenseFiles
  Delete "$INSTDIR\Uninstall.exe"
!ifndef QA_ROOT
  Delete "$SMPROGRAMS\Cracked Vault\Cracked808s.lnk"
  RMDir "$SMPROGRAMS\Cracked Vault"
!endif
  DeleteRegKey ${REGROOT} "${UNINSTALL_KEY}"
  RMDir "$INSTDIR"
SectionEnd
