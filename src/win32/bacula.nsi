;NSIS Modern User Interface version 1.63
;Start Menu Folder Selection Example Script
;Written by Joost Verburg

!define MUI_PRODUCT "Bacula" ;Define your own software name here
!define MUI_VERSION "1.33.4" ;Define your own software version here

!include "MUI.nsh"
!include "util.nsh"

;--------------------------------
;Configuration

  ;General
  OutFile "bacula-install.exe"

  ;Folder selection page
  InstallDir "$PROGRAMFILES\${MUI_PRODUCT}"
  
  ;Remember install folder
  InstallDirRegKey HKCU "Software\${MUI_PRODUCT}" ""
  
  ;$9 is being used to store the Start Menu Folder.
  ;Do not use this variable in your script (or Push/Pop it)!

  ;To change this variable, use MUI_STARTMENUPAGE_VARIABLE.
  ;Have a look at the Readme for info about other options (default folder,
  ;registry).

  ;Remember the Start Menu Folder
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU" 
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${MUI_PRODUCT}" 
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"

  !define TEMP $R0
  
;--------------------------------
;Modern UI Configuration

  !define MUI_LICENSEPAGE
  !define MUI_COMPONENTSPAGE
  !define MUI_DIRECTORYPAGE
  !define MUI_STARTMENUPAGE
  
  !define MUI_ABORTWARNING
  
  !define MUI_UNINSTALLER
  !define MUI_UNCONFIRMPAGE
  
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"
  
;--------------------------------
;Language Strings

  ;Description
  LangString DESC_SecCopyUI ${LANG_ENGLISH} "Install Bacula client on this system."

;--------------------------------
;Data
  
  LicenseData "License.txt"

;--------------------------------
;Installer Sections

Section "Bacula File Service" SecCopyUI

  ;ADD YOUR OWN STUFF HERE!

  SetOutPath "$INSTDIR"
  File baculafd\Release\bacula-fd.exe
  File pthreads\pthreadVCE.dll
  IfFileExists "$INSTDIR\bacula-fd.conf" sconf 
  File bacula-fd.conf
  goto doDir
  sconf:
  File /oname=bacula-fd.conf.N bacula-fd.conf
  doDir:
  CreateDirectory "$INSTDIR\working"
  
  ;Store install folder
  WriteRegStr HKCU "Software\${MUI_PRODUCT}" "" $INSTDIR
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN
    
    ;Create shortcuts
    CreateDirectory "$SMPROGRAMS\${MUI_STARTMENUPAGE_VARIABLE}"
    CreateShortCut "$SMPROGRAMS\${MUI_STARTMENUPAGE_VARIABLE}\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
    Call IsNT
    Pop $R0
    StrCmp $R0 "false" do98sc

    CreateShortCut "$SMPROGRAMS\${MUI_STARTMENUPAGE_VARIABLE}\Start Service.lnk" "$SYSDIR\net.exe" "start bacula" "$INSTDIR\bacula-fd.exe" 2
    CreateShortCut "$SMPROGRAMS\${MUI_STARTMENUPAGE_VARIABLE}\Stop Service.lnk" "$SYSDIR\net.exe" "stop bacula" "$INSTDIR\bacula-fd.exe" 3
   goto scend
    do98sc:

    CreateShortCut "$SMPROGRAMS\${MUI_STARTMENUPAGE_VARIABLE}\Start Service.lnk" "$INSTDIR\bacula-fd.exe" "-c bacula-fd.conf" "$INSTDIR\bacula-fd.exe" 2
    CreateShortCut "$SMPROGRAMS\${MUI_STARTMENUPAGE_VARIABLE}\Stop Service.lnk" "$INSTDIR\bacula-fd.exe" "/kill" "$INSTDIR\bacula-fd.exe" 3
   scend:
  !insertmacro MUI_STARTMENU_WRITE_END

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  ; Install service
  ExecWait '"$INSTDIR\bacula-fd.exe" /install -c "$INSTDIR\bacula-fd.conf"'

SectionEnd

;Display the Finish header
;Insert this macro after the sections if you are not using a finish page
!insertmacro MUI_SECTIONS_FINISHHEADER

;--------------------------------
;Descriptions

!insertmacro MUI_FUNCTIONS_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${SecCopyUI} $(DESC_SecCopyUI)
!insertmacro MUI_FUNCTIONS_DESCRIPTION_END
 
;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ;ADD YOUR OWN STUFF HERE!
  ExecWait   '"$INSTDIR\bacula-fd.exe" /kill'
  Sleep 1000
  ExecWait   '"$INSTDIR\bacula-fd.exe" /remove'
  Sleep 1000

  Delete "$INSTDIR\bacula-fd.exe"
  Delete "$INSTDIR\bacula-fd.conf.N"
  Delete "$INSTDIR\pthreadVCE.dll"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir  "$INSTDIR\working"  
  ;Remove shortcut
  ReadRegStr ${TEMP} "${MUI_STARTMENUPAGE_REGISTRY_ROOT}" "${MUI_STARTMENUPAGE_REGISTRY_KEY}" "${MUI_STARTMENUPAGE_REGISTRY_VALUENAME}"
  
  StrCmp ${TEMP} "" noshortcuts
  
    Delete "$SMPROGRAMS\${TEMP}\Uninstall.lnk"
    Delete "$SMPROGRAMS\${TEMP}\Start Service.lnk"
    Delete "$SMPROGRAMS\${TEMP}\Stop Service.lnk"
    RMDir "$SMPROGRAMS\${TEMP}" ;Only if empty, so it won't delete other shortcuts
    
  noshortcuts:

  RMDir "$INSTDIR"

  DeleteRegKey /ifempty HKCU "Software\${MUI_PRODUCT}"

  ;Display the Finish header
  !insertmacro MUI_UNFINISHHEADER

SectionEnd
