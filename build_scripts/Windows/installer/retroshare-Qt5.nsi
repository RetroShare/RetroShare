; Script generated with the Venis Install Wizard & modified by defnax
; Reworked by Thunder

!include ifexist.nsh

# Needed defines
;!define REVISION ""
;!define RELEASEDIR ""
;!define QTDIR ""
;!define MINGWDIR ""

# Optional defines
;!define OUTDIR ""

# Check needed defines
!ifndef RELEASEDIR
!error "RELEASEDIR is not defined"
!endif
!ifndef QTDIR
!error "QTDIR is not defined"
!endif
!ifndef MINGWDIR
!error "MINGWDIR is not defined"
!endif

# Check optional defines
!ifdef OUTDIR
!define OUTDIR_ "${OUTDIR}\"
!else
!define OUTDIR ""
!define OUTDIR_ ""
!endif

!ifndef INSTALLERADD
!define INSTALLERADD ""
!endif

# Source directory
!define SOURCEDIR "..\..\.."

# Get version from executable
!GetDllVersion "${RELEASEDIR}\retroshare-gui\src\release\retroshare.exe" VERSION_
!define VERSION ${VERSION_1}.${VERSION_2}.${VERSION_3}
;!define REVISION ${VERSION_4}

# Get version of Qt
!GetDllVersion "${QTDIR}\bin\Qt5Core.dll" QTVERSION_
!define QTVERSION ${QTVERSION_1}.${QTVERSION_2}.${QTVERSION_3}

# Check version
!ifndef REVISION
!error "REVISION is not defined"
!endif

# Date
!define /date Date "%Y%m%d"

# Application name and version
!define APPNAME "RetroShare"
!define APPNAMEANDVERSION "${APPNAME} ${VERSION}"
!define PUBLISHER "RetroShare Team"

# Install path
!define INSTDIR_NORMAL "$ProgramFiles\${APPNAME}"
!define INSTDIR_PORTABLE "$Desktop\${APPNAME}"

!define DATADIR_NORMAL "$APPDATA\${APPNAME}"
!define DATADIR_PORTABLE "$INSTDIR\Data"

# Main Install settings
Name "${APPNAMEANDVERSION}"
InstallDirRegKey HKLM "Software\${APPNAME}" ""
OutFile "${OUTDIR_}RetroShare-${VERSION}-${Date}-${REVISION}-Qt-${QTVERSION}${INSTALLERADD}-setup.exe"
BrandingText "${APPNAMEANDVERSION}"
RequestExecutionlevel highest
# Use compression
SetCompressor /SOLID LZMA

# Global variables
Var PortableMode
Var InstDirNormal
Var InstDirPortable
Var DataDir
Var StyleSheetDir

# Modern interface settings
!include Sections.nsh
!include nsDialogs.nsh
!include "MUI.nsh"

# Interface Settings
!define MUI_ABORTWARNING
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${SOURCEDIR}\build_scripts\Windows\installer\HeaderImage.bmp"
;!define MUI_WELCOMEFINISHPAGE_BITMAP "...bmp"

# MUI defines
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\orange-install.ico"
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_LICENSEPAGE_RADIOBUTTONS
!define MUI_COMPONENTSPAGE_SMALLDESC
!define MUI_FINISHPAGE_LINK "Visit the RetroShare forum for the latest news and support"
!define MUI_FINISHPAGE_LINK_LOCATION "http://retroshare.sourceforge.net/forum/"
!define MUI_FINISHPAGE_RUN "$INSTDIR\retroshare.exe"
!define MUI_FINISHPAGE_SHOWREADME $INSTDIR\changelog.txt
!define MUI_FINISHPAGE_SHOWREADME_TEXT changelog.txt
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\orange-uninstall.ico"
!define MUI_UNFINISHPAGE_NOAUTOCLOSE
;!define MUI_LANGDLL_REGISTRY_ROOT HKLM
;!define MUI_LANGDLL_REGISTRY_KEY ${REGKEY}
;!define MUI_LANGDLL_REGISTRY_VALUENAME InstallerLanguage

# Defines the un-/installer logo of RetroShare
!insertmacro MUI_DEFAULT MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange.bmp"
!insertmacro MUI_DEFAULT MUI_UNWELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange-uninstall.bmp"

# Installer pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "$(myLicenseData)"
Page Custom PortableModePageCreate PortableModePageLeave
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE dir_leave
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

# Set languages (first is default language)
!insertmacro MUI_RESERVEFILE_LANGDLL

# Installer languages
!define MUI_LANGDLL_ALLLANGUAGES

# Translations
!macro LANG_LOAD LANGUAGE LANGCODE LANGID LICENCEFILE
  !insertmacro MUI_LANGUAGE "${LANGUAGE}"
;  !verbose off
  !define LANG "${LANGUAGE}"
  !include "lang\${LANGCODE}.nsh"
  LangString LANGUAGEID "${LANG_${LANG}}" "1031"
  LicenseLangString myLicenseData ${LANGCODE} ${LICENCEFILE}
;  !verbose on
  !undef LANG
!macroend

!macro LANG_STRING NAME VALUE
  LangString "${NAME}" "${LANG_${LANG}}" "${VALUE}"
!macroend

!insertmacro LANG_LOAD "English" "en" "1033" "${SOURCEDIR}\retroshare-gui\src\license\license.txt"
!insertmacro LANG_LOAD "French" "fr" "1036" "${SOURCEDIR}\retroshare-gui\src\license\license-FR.txt"
!insertmacro LANG_LOAD "German" "de" "1031" "${SOURCEDIR}\retroshare-gui\src\license\license-GER.txt"
!insertmacro LANG_LOAD "Turkish" "tr" "1055" "${SOURCEDIR}\retroshare-gui\src\license\license-TR.txt"
!insertmacro LANG_LOAD "SimpChinese" "zh_CN" "2052" "${SOURCEDIR}\retroshare-gui\src\license\license.txt"
!insertmacro LANG_LOAD "Polish" "pl" "1045" "${SOURCEDIR}\retroshare-gui\src\license\license.txt"
!insertmacro LANG_LOAD "Spanish" "es" "1034" "${SOURCEDIR}\retroshare-gui\src\license\license.txt"
!insertmacro LANG_LOAD "Russian" "ru" "1049" "${SOURCEDIR}\retroshare-gui\src\license\license.txt"
!insertmacro LANG_LOAD "Catalan" "ca_ES" "1027" "${SOURCEDIR}\retroshare-gui\src\license\license.txt"

LicenseData $(myLicenseData)

# Main binaries
Section $(Section_Main) Section_Main
  ;Set Section required
  SectionIn RO

  ; Set Section properties
  SetOverwrite on

  ; Clears previous error logs
;  Delete "$INSTDIR\*.log"

  ; Main binaries
  SetOutPath "$INSTDIR"
  File /oname=retroshare.exe "${RELEASEDIR}\retroshare-gui\src\release\retroshare.exe"
  File /oname=retroshare-nogui.exe "${RELEASEDIR}\retroshare-nogui\src\release\retroshare-nogui.exe"

  ; Qt binaries
  File "${QTDIR}\bin\Qt5Core.dll"
  File "${QTDIR}\bin\Qt5Gui.dll"
  File "${QTDIR}\bin\Qt5Multimedia.dll"
  File "${QTDIR}\bin\Qt5Network.dll"
  File "${QTDIR}\bin\Qt5PrintSupport.dll"
  File "${QTDIR}\bin\Qt5Svg.dll"
  File "${QTDIR}\bin\Qt5Widgets.dll"
  File "${QTDIR}\bin\Qt5Xml.dll"

  ; Qt audio
  SetOutPath "$INSTDIR\audio"
  File "${QTDIR}\plugins\audio\qtaudio_windows.dll"

  ; Qt platforms
  SetOutPath "$INSTDIR\platforms"
  File "${QTDIR}\plugins\platforms\qwindows.dll"

  ; Qt styles
  SetOutPath "$INSTDIR\styles"
  File /NONFATAL "${QTDIR}\plugins\styles\qwindowsvistastyle.dll"

  ; MinGW binaries
  SetOutPath "$INSTDIR"
  File "${MINGWDIR}\bin\libstdc++-6.dll"
  File "${MINGWDIR}\bin\libgcc_s_dw2-1.dll"
  File "${MINGWDIR}\bin\libwinpthread-1.dll"

  ; External binaries
  File "${EXTERNAL_LIB_DIR}\bin\miniupnpc.dll"
  File "${EXTERNAL_LIB_DIR}\bin\libeay32.dll"
  File "${EXTERNAL_LIB_DIR}\bin\ssleay32.dll"

  ; Other files
  File "${SOURCEDIR}\retroshare-gui\src\changelog.txt"
  File "${SOURCEDIR}\libbitdht\src\bitdht\bdboot.txt"

  ; Image formats
  SetOutPath "$INSTDIR\imageformats"
  File /r "${QTDIR}\plugins\imageformats\qgif.dll"
  File /r "${QTDIR}\plugins\imageformats\qicns.dll"
  File /r "${QTDIR}\plugins\imageformats\qico.dll"
  File /r "${QTDIR}\plugins\imageformats\qjpeg.dll"
  File /r "${QTDIR}\plugins\imageformats\qsvg.dll"
  File /r "${QTDIR}\plugins\imageformats\qtga.dll"
  File /r "${QTDIR}\plugins\imageformats\qtiff.dll"
  File /r "${QTDIR}\plugins\imageformats\qwbmp.dll"
  File /r "${QTDIR}\plugins\imageformats\qwebp.dll"

  ; Sounds
  SetOutPath "$INSTDIR\sounds"
  File /r "${SOURCEDIR}\retroshare-gui\src\sounds\*.*"

  ; Translations
  SetOutPath "$INSTDIR\translations"
  File /r "${SOURCEDIR}\retroshare-gui\src\translations\*.qm"
  File /r "${QTDIR}\translations\qt_*.qm"
  File /r "${QTDIR}\translations\qtbase_*.qm"
  File /r "${QTDIR}\translations\qtscript_*.qm"
  File /r "${QTDIR}\translations\qtquick1_*.qm"
  File /r "${QTDIR}\translations\qtmultimedia_*.qm"
  File /r "${QTDIR}\translations\qtxmlpatterns_*.qm"

  ; WebUI
  SetOutPath "$INSTDIR\webui"
  File /r "${SOURCEDIR}\libresapi\src\webui\*.*"

  ; License
  SetOutPath "$INSTDIR\license"
  File /r "${SOURCEDIR}\retroshare-gui\src\license\*.*"
SectionEnd

# Plugins
${!defineifexist} PLUGIN_FEEDREADER_EXISTS "${RELEASEDIR}\plugins\FeedReader\release\FeedReader.dll"
${!defineifexist} PLUGIN_VOIP_EXISTS "${RELEASEDIR}\plugins\VOIP\release\VOIP.dll"

!ifdef PLUGIN_FEEDREADER_EXISTS
!define /ifndef PLUGIN_EXISTS
!endif
!ifdef PLUGIN_VOIP_EXISTS
!define /ifndef PLUGIN_EXISTS
!endif

!ifdef PLUGIN_EXISTS
  SectionGroup $(Section_Plugins) Section_Plugins
  !ifdef PLUGIN_FEEDREADER_EXISTS
    Section $(Section_Plugin_FeedReader) Section_Plugin_FeedReader
      SetOutPath "$DataDir\extensions6"
      File "${RELEASEDIR}\plugins\FeedReader\release\FeedReader.dll"
    SectionEnd
  !endif

  !ifdef PLUGIN_VOIP_EXISTS
    Section $(Section_Plugin_VOIP) Section_Plugin_VOIP
      SetOutPath "$DataDir\extensions6"
      File "${RELEASEDIR}\plugins\VOIP\release\VOIP.dll"
    SectionEnd
  !endif
  SectionGroupEnd
!endif

# Data (Styles)
Section $(Section_Data) Section_Data
  ; Set Section properties
  SetOverwrite on

  ; Chat style
  SetOutPath "$StyleSheetDir\stylesheets\Bubble"
  File /r "${SOURCEDIR}\retroshare-gui\src\gui\qss\chat\Bubble\*.*"
  SetOutPath "$StyleSheetDir\stylesheets\Bubble_Compact"
  File /r "${SOURCEDIR}\retroshare-gui\src\gui\qss\chat\Bubble_Compact\*.*"

  ; Stylesheets
  SetOutPath "$INSTDIR\qss"
  File /r "${SOURCEDIR}\retroshare-gui\src\qss\*.*"
SectionEnd

;Section $(Section_Link) Section_Link
  ; Delete any existing keys

  ; Write the file association
;  WriteRegStr HKCR .pqi "" retroshare
;  WriteRegStr HKCR retroshare "" "PQI File"
;  WriteRegBin HKCR retroshare EditFlags 00000100
;  WriteRegStr HKCR "retroshare\shell" "" open
;  WriteRegStr HKCR "retroshare\shell\open\command" "" `"$INSTDIR\retroshare.exe" "%1"`
;SectionEnd

# Shortcuts
SectionGroup $(Section_Shortcuts) Section_Shortcuts
Section $(Section_StartMenu) Section_StartMenu
  SetOutPath "$INSTDIR"
  CreateDirectory "$SMPROGRAMS\${APPNAME}"
  CreateShortCut "$SMPROGRAMS\${APPNAME}\$(Link_Uninstall).lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\retroshare.exe" "" "$INSTDIR\retroshare.exe" 0
SectionEnd

Section $(Section_Desktop) Section_Desktop
  CreateShortCut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\retroshare.exe" "" "$INSTDIR\retroshare.exe" 0
SectionEnd

Section $(Section_QuickLaunch) Section_QuickLaunch
  CreateShortCut "$QUICKLAUNCH\${APPNAME}.lnk" "$INSTDIR\retroshare.exe" "" "$INSTDIR\retroshare.exe" 0
SectionEnd
SectionGroupEnd

Section $(Section_AutoStart) Section_AutoStart
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "RetroShare"   "$INSTDIR\retroshare.exe -m"
SectionEnd

;Section $(Section_AutoStart) Section_AutoStart
;  CreateShortCut "$SMSTARTUP\${APPNAME}.lnk" "$INSTDIR\retroshare.exe" "" "$INSTDIR\retroshare.exe -m" 0
;SectionEnd

Section -FinishSection
  ${If} $PortableMode = 0
    WriteRegStr HKLM "Software\${APPNAME}" "" "$INSTDIR"
    WriteRegStr HKLM "Software\${APPNAME}" "Version" "${VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayName" "${APPNAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayVersion" "${VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayIcon" "$INSTDIR\retroshare.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "Publisher" "${PUBLISHER}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoModify" "1"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoRepair" "1"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString" "$INSTDIR\uninstall.exe"
    WriteUninstaller "$INSTDIR\uninstall.exe"
  ${Else}
    ; Create the file the application uses to detect portable mode
    FileOpen $0 "$INSTDIR\portable" w
    FileClose $0
  ${EndIf}
SectionEnd

# Descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${Section_Main} $(Section_Main_Desc)
  !insertmacro MUI_DESCRIPTION_TEXT ${Section_Data} $(Section_Data_Desc)
  !insertmacro MUI_DESCRIPTION_TEXT ${Section_Shortcuts} $(Section_Shortcuts_Desc)
  !insertmacro MUI_DESCRIPTION_TEXT ${Section_StartMenu} $(Section_StartMenu_Desc)
  !insertmacro MUI_DESCRIPTION_TEXT ${Section_Desktop} $(Section_Desktop_Desc)
  !insertmacro MUI_DESCRIPTION_TEXT ${Section_QuickLaunch} $(Section_QuickLaunch_Desc)
  !insertmacro MUI_DESCRIPTION_TEXT ${Section_Plugins} $(Section_Plugins_Desc)
  !insertmacro MUI_DESCRIPTION_TEXT ${Section_Plugin_FeedReader} $(Section_Plugin_FeedReader_Desc)
  !insertmacro MUI_DESCRIPTION_TEXT ${Section_Plugin_VOIP} $(Section_Plugin_VOIP_Desc)
;  !insertmacro MUI_DESCRIPTION_TEXT ${Section_Link} $(Section_Link_Desc)
  !insertmacro MUI_DESCRIPTION_TEXT ${Section_AutoStart} $(Section_AutoStart_Desc)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

# Uninstall
Section "Uninstall"
  ; Remove file association registry keys
;  DeleteRegKey HKCR .pqi
  DeleteRegKey HKCR RetroShare

  ; Remove program/uninstall regsitry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
  DeleteRegKey HKLM SOFTWARE\${APPNAME}

  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "RetroShare"

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\${APPNAME}\*.*"

  ; Remove desktop shortcut
  Delete "$DESKTOP\${APPNAME}.lnk"

  ; Remove Quicklaunch shortcut
  Delete "$QUICKLAUNCH\${APPNAME}.lnk"

  ; Remove Autstart
  Delete "$SMSTARTUP\${APPNAME}.lnk"

  ; Remove directories used
  RMDir "$SMPROGRAMS\${APPNAME}"
  RMDir /r "$INSTDIR"

  ; Don't remove the directory, otherwise
  ; we lose the XPGP keys.
  ; Should make this an option though...
  RMDir /r "${DATADIR_NORMAL}\extensions6"
  RMDir /r "${DATADIR_NORMAL}\stylesheets"
SectionEnd

Function .onInit
  StrCpy $InstDirNormal "${INSTDIR_NORMAL}"
  StrCpy $InstDirPortable "${INSTDIR_PORTABLE}"

  StrCpy $PortableMode 0
  StrCpy $INSTDIR "$InstDirNormal"
  StrCpy $DataDir "${DATADIR_NORMAL}"

  InitPluginsDir
  Push $R1
  File /oname=$PLUGINSDIR\spltmp.bmp "${SOURCEDIR}\retroshare-gui\src\gui\images\logo\logo_splash.png"
  advsplash::show 1200 1000 1000 -1 $PLUGINSDIR\spltmp
  Pop $R1
  Pop $R1
  !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

# Installation mode

Function RequireAdmin
  UserInfo::GetAccountType
  Pop $8
  ${If} $8 != "admin"
    MessageBox MB_ICONSTOP "You need administrator rights to install ${APPNAME}"
    SetErrorLevel 740 ;ERROR_ELEVATION_REQUIRED
    Abort
  ${EndIf}
FunctionEnd

Function SetModeDestinationFromInstdir
  ${If} $PortableMode = 0
    StrCpy $InstDirNormal $INSTDIR
  ${Else}
    StrCpy $InstDirPortable $INSTDIR
  ${EndIf}
FunctionEnd

Function PortableModePageCreate
  Call SetModeDestinationFromInstdir ; If the user clicks BACK on the directory page we will remember their mode specific directory
  !insertmacro MUI_HEADER_TEXT $(Page_InstallMode) $(Page_InstallMode_Desc)
  nsDialogs::Create 1018
  Pop $0
  ${NSD_CreateRadioButton} 5u 25u -10u 8u $(Page_InstallMode_Standard)
  Pop $1
  ${NSD_CreateLabel} 18u 40u -10u 24u $(Page_InstallMode_Standard_Desc)
  Pop $0
  ${NSD_CreateRadioButton} 5u 75u -10u 8u $(Page_InstallMode_Portable)
  Pop $2
  ${NSD_CreateLabel} 18u 90u -10u 24u $(Page_InstallMode_Portable_Desc)
  Pop $0
  ${If} $PortableMode = 0
    SendMessage $1 ${BM_SETCHECK} ${BST_CHECKED} 0
  ${Else}
    SendMessage $2 ${BM_SETCHECK} ${BST_CHECKED} 0
  ${EndIf}
  nsDialogs::Show
FunctionEnd

Function PortableModePageLeave
  ${NSD_GetState} $1 $0
  ${If} $0 <> ${BST_UNCHECKED}
    StrCpy $PortableMode 0
    StrCpy $INSTDIR $InstDirNormal
    Call RequireAdmin
    ; Enable sections
    SectionSetText ${Section_Shortcuts} $(Section_Shortcuts)
    SectionSetText ${Section_StartMenu} $(Section_StartMenu)
    SectionSetText ${Section_Desktop} $(Section_Desktop)
    SectionSetText ${Section_QuickLaunch} $(Section_QuickLaunch)
    SectionSetText ${Section_AutoStart} $(Section_AutoStart)
    !insertmacro SelectSection ${Section_Shortcuts}
    !insertmacro SelectSection ${Section_AutoStart}
    !insertmacro SelectSection ${Section_StartMenu}
    !insertmacro SelectSection ${Section_Desktop}
    !insertmacro SelectSection ${Section_QuickLaunch}
  ${Else}
    StrCpy $PortableMode 1
    StrCpy $INSTDIR $InstDirPortable
    ; Disable sections
    !insertmacro UnselectSection ${Section_Shortcuts}
    !insertmacro UnselectSection ${Section_AutoStart}
    !insertmacro UnselectSection ${Section_StartMenu}
    !insertmacro UnselectSection ${Section_Desktop}
    !insertmacro UnselectSection ${Section_QuickLaunch}
    SectionSetText ${Section_Shortcuts} ""
    SectionSetText ${Section_StartMenu} ""
    SectionSetText ${Section_Desktop} ""
    SectionSetText ${Section_QuickLaunch} ""
    SectionSetText ${Section_AutoStart} ""
  ${EndIf}
FunctionEnd

Function dir_leave
  ${If} $PortableMode = 0
    StrCpy $DataDir "${DATADIR_NORMAL}"
    StrCpy $StyleSheetDir $DataDir
  ${Else}
    StrCpy $DataDir "${DATADIR_PORTABLE}"
    StrCpy $StyleSheetDir $INSTDIR
  ${EndIf}
FunctionEnd
