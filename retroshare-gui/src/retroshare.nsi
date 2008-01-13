; Script generated with the Venis Install Wizard & modified by defnax

; Define your application name
!define APPNAME "RetroShare"
!define VERSION "0.3.52a"
!define APPNAMEANDVERSION "${APPNAME} ${VERSION}"

; Main Install settings
Name "${APPNAMEANDVERSION}"
InstallDir "$PROGRAMFILES\RetroShare"
InstallDirRegKey HKLM "Software\${APPNAME}" ""
OutFile "RetroShare_${VERSION}_setup.exe"
BrandingText "${APPNAMEANDVERSION}"
; Use compression
SetCompressor LZMA

; Modern interface settings
!include "MUI.nsh"

!define MUI_ABORTWARNING
!define MUI_COMPONENTSPAGE_SMALLDESC
!define MUI_FINISHPAGE_RUN "$INSTDIR\RetroShare.exe"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY

!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Set languages (first is default language)
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_RESERVEFILE_LANGDLL

LangString TEXT_FLocations_TITLE ${LANG_ENGLISH} "Choose Default Save Locations"
LangString TEXT_FLocations_SUBTITLE ${LANG_ENGLISH} "Choose the folders to save your downloads to."

!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

Section "RetroShare Core" Section1

  ; Set Section properties
  SetOverwrite on

  ; Clears previous error logs
  Delete "$INSTDIR\*.log"
	
  ; Set Section Files and Shortcuts
  SetOutPath "$INSTDIR\"
  File /r "release\*"
  

SectionEnd

Section "RetroShare Data" Section1b

  ; Set Section properties
  SetOverwrite on

  ; Set Section Files and Shortcuts
  SetOutPath "$APPDATA\RetroShare\"
  ;File /r "data\*"
  
  ; We're not ready for external skins...
  ; Set Section qss
  ; SetOutPath "$INSTDIR\qss\"
  ; File /r release\qss\*.*   
  
  ; Set Section skin
  ; SetOutPath "$INSTDIR\skin\"
  ; File /r release\skin\*.* 
	
SectionEnd

Section "File Association" section2
  ; Delete any existing keys


  ; Write the file association
  WriteRegStr HKCR .pqi "" retroshare
  WriteRegStr HKCR retroshare "" "PQI File"
  WriteRegBin HKCR retroshare EditFlags 00000100
  WriteRegStr HKCR "retroshare\shell" "" open
  WriteRegStr HKCR "retroshare\shell\open\command" "" `"$INSTDIR\RetroShare.exe" "%1"`

SectionEnd

Section "Start Menu Shortcuts" section3

  SetOutPath "$INSTDIR"
  CreateDirectory "$SMPROGRAMS\${APPNAME}"
  CreateShortCut "$SMPROGRAMS\${APPNAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\RetroShare.exe" "" "$INSTDIR\RetroShare.exe" 0

SectionEnd

Section "Desktop Shortcut" section4

  CreateShortCut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\RetroShare.exe" "" "$INSTDIR\RetroShare.exe" 0
  
SectionEnd

Section "Quicklaunch Shortcut" section5

  CreateShortCut "$QUICKLAUNCH\${APPNAME}.lnk" "$INSTDIR\RetroShare.exe" "" "$INSTDIR\RetroShare.exe" 0
  
SectionEnd
        

Section "Automatic Startup" section6

  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "RetroRun"   "$INSTDIR\${APPNAME}.exe -a"
  
SectionEnd

Section -FinishSection

  WriteRegStr HKLM "Software\${APPNAME}" "" "$INSTDIR"
  WriteRegStr HKLM "Software\${APPNAME}" "Version" "${VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayName" "${APPNAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteUninstaller "$INSTDIR\uninstall.exe"

SectionEnd

; Modern install component descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${Section1}  "RetroShare program files"
!insertmacro MUI_DESCRIPTION_TEXT ${Section1b} "RetroShare data files"
!insertmacro MUI_DESCRIPTION_TEXT ${Section2} "Associate RetroShare with .pqi file extension"
!insertmacro MUI_DESCRIPTION_TEXT ${Section3} "Create RetroShare Start Menu shortcuts"
!insertmacro MUI_DESCRIPTION_TEXT ${Section4} "Create RetroShare Desktop shortcut"	
!insertmacro MUI_DESCRIPTION_TEXT ${Section5} "Auto-Run and Login at Startup"	
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;Uninstall section
Section "Uninstall"
  
  ; Remove file association registry keys
  DeleteRegKey HKCR .pqi
  DeleteRegKey HKCR retroshare
	
  ; Remove program/uninstall regsitry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
  DeleteRegKey HKLM SOFTWARE\${APPNAME}

  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "RetroRun"

  ; Remove files and uninstaller
  Delete $INSTDIR\RetroShare.exe
  Delete $INSTDIR\*.dll
  Delete $INSTDIR\*.dat
  Delete $INSTDIR\*.txt
  Delete $INSTDIR\*.ini
  Delete $INSTDIR\*.log

  Delete $INSTDIR\uninstall.exe

  ; Remove the kadc.ini file.
  ; Don't remove the directory, otherwise
  ; we lose the XPGP keys.
  ; Should make this an option though...
  Delete $APPDATA\RetroShare\kadc.ini

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\${APPNAME}\*.*"

  ; Remove desktop icon
  Delete "$DESKTOP\${APPNAME}.lnk"

  ; Remove directories used
  RMDir "$SMPROGRAMS\${APPNAME}"
  RMDir "$INSTDIR"

SectionEnd

Function .onInit



  ;This was written by Vytautas - http://nsis.sourceforge.net/archive/nsisweb.php?page=453
  System::Call 'kernel32::CreateMutexA(i 0, i 0, t "RetroShare") i .r1 ?e' 
  Pop $R0 
  StrCmp $R0 0 +3 
    MessageBox MB_OK "The ${APPNAME} installer is already running." 
    Abort 

FunctionEnd







; eof
