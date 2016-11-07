@echo off

setlocal

set Quite=^>nul

:: Initialize environment
call "%~dp0..\env.bat"
if errorlevel 1 goto error_env
call "%EnvPath%\env.bat"
if errorlevel 1 goto error_env

:: Get gcc versions
call "%ToolsPath%\get-gcc-version.bat" GCCVersion
if "%GCCVersion%"=="" echo Cannot get gcc version.& exit /B 1

:: Check external libraries
if not exist "%RootPath%\libs" echo Please build external libraries first.& exit /B 1

:: Check gcc version of external libraries
if not exist "%RootPath%\libs\gcc-version" echo Cannot get gcc version of external libraries.& exit /B 1
set /P LibsGCCVersion=<"%RootPath%\libs\gcc-version"
if "%LibsGCCVersion%" NEQ "%GCCVersion%" echo Please use correct version of external libraries. (gcc %GCCVersion% ^<^> libs %LibsGCCVersion%).& exit /B 1

:: Initialize environment
call "%~dp0env.bat"
if errorlevel 1 goto error_env

:: Remove deploy path
if exist "%RsDeployPath%" rmdir /S /Q "%RsDeployPath%"

:: Check compilation
if not exist "%RsBuildPath%\Makefile" echo Project is not compiled.& goto error

:: Get compiled revision
set GetRsVersion=%SourcePath%\build_scripts\Windows\tools\get-rs-version.bat
if not exist "%GetRsVersion%" (
	echo File not found
	echo %GetRsVersion%
	goto error
)

call "%GetRsVersion%" RS_REVISION_STRING RsRevision
if "%RsRevision%"=="" echo Revision not found.& goto error

:: Get compiled version
call "%GetRsVersion%" RS_MAJOR_VERSION RsMajorVersion
if "%RsMajorVersion%"=="" echo Major version not found.& goto error

call "%GetRsVersion%" RS_MINOR_VERSION RsMinorVersion
if "%RsMinorVersion%"=="" echo Minor version not found.& goto error

call "%GetRsVersion%" RS_BUILD_NUMBER RsBuildNumber
if "%RsBuildNumber%"=="" echo Build number not found.& goto error

call "%GetRsVersion%" RS_BUILD_NUMBER_ADD RsBuildNumberAdd

set RsVersion=%RsMajorVersion%.%RsMinorVersion%.%RsBuildNumber%%RsBuildNumberAdd%

:: Check WMIC is available
wmic.exe alias /? >nul 2>&1 || echo WMIC is not available.&& goto error

:: Use WMIC to retrieve date in format YYYYMMDD
set RsDate=
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /format:list') do set RsDate=%%I
set RsDate=%RsDate:~0,4%%RsDate:~4,2%%RsDate:~6,2%

set QtMainVersion=%QtVersion:~0,1%

rem Qt 4 = QtSvg4.dll
rem Qt 5 = Qt5Svg.dll
set QtMainVersion1=
set QtMainVersion2=
if "%QtMainVersion%"=="4" set QtMainVersion2=4
if "%QtMainVersion%"=="5" set QtMainVersion1=5

if "%RsBuildConfig%" NEQ "release" (
	set Archive=%RsPackPath%\RetroShare-%RsVersion%-Windows-Portable-%RsDate%-%RsRevision%-Qt-%QtVersion%%RsArchiveAdd%-%RsBuildConfig%.7z
) else (
	set Archive=%RsPackPath%\RetroShare-%RsVersion%-Windows-Portable-%RsDate%-%RsRevision%-Qt-%QtVersion%%RsArchiveAdd%.7z
)

if exist "%Archive%" del /Q "%Archive%"

:: Create deploy path
mkdir "%RsDeployPath%"

title Pack - %SourceName%-%RsBuildConfig% [copy files]

set ExtensionsFile=%SourcePath%\libretroshare\src\rsserver\rsinit.cc
set Extensions=
for /f %%e in ('type "%ExtensionsFile%" ^| "%EnvSedExe%" -n "s/^.*\/\(extensions[^/]*\)\/.*$/\1/p" ^| "%EnvSedExe%" -n "1,1p"') do set Extensions=%%e
if "%Extensions%"=="" echo Folder for extensions not found in %ExtensionsFile%& goto error

:: Copy files
mkdir "%RsDeployPath%\Data\%Extensions%"
mkdir "%RsDeployPath%\imageformats"
mkdir "%RsDeployPath%\qss"
mkdir "%RsDeployPath%\stylesheets"
mkdir "%RsDeployPath%\sounds"
mkdir "%RsDeployPath%\translations"

copy nul "%RsDeployPath%\portable" %Quite%

echo copy binaries
copy "%RsBuildPath%\retroshare-gui\src\%RsBuildConfig%\RetroShare*.exe" "%RsDeployPath%" %Quite%
copy "%RsBuildPath%\retroshare-nogui\src\%RsBuildConfig%\retroshare*-nogui.exe" "%RsDeployPath%" %Quite%

echo copy extensions
for /D %%D in ("%RsBuildPath%\plugins\*") do (
	call :copy_extension "%%D" "%RsDeployPath%\Data\%Extensions%"
	call :copy_dependencies "%RsDeployPath%\Data\%Extensions%\%%~nxD.dll" "%RsDeployPath%"
)

echo copy external binaries
copy "%RootPath%\libs\bin\*.dll" "%RsDeployPath%" %Quite%

echo copy dependencies
call :copy_dependencies "%RsDeployPath%\RetroShare06.exe" "%RsDeployPath%"

echo copy Qt DLL's
copy "%QtPath%\Qt%QtMainVersion1%Svg%QtMainVersion2%.dll" "%RsDeployPath%" %Quite%

if "%QtMainVersion%"=="5" (
	mkdir "%RsDeployPath%\platforms"
	copy "%QtPath%\..\plugins\platforms\qwindows.dll" "%RsDeployPath%\platforms" %Quite%
	mkdir "%RsDeployPath%\audio"
	copy "%QtPath%\..\plugins\audio\qtaudio_windows.dll" "%RsDeployPath%\audio" %Quite%
)

copy "%QtPath%\..\plugins\imageformats\*.dll" "%RsDeployPath%\imageformats" %Quite%
del /Q "%RsDeployPath%\imageformats\*d?.dll" %Quite%

echo copy qss
xcopy /S "%SourcePath%\retroshare-gui\src\qss" "%RsDeployPath%\qss" %Quite%

echo copy stylesheets
xcopy /S "%SourcePath%\retroshare-gui\src\gui\qss\chat" "%RsDeployPath%\stylesheets" %Quite%
rmdir /S /Q "%RsDeployPath%\stylesheets\compact" %Quite%
rmdir /S /Q "%RsDeployPath%\stylesheets\standard" %Quite%
rmdir /S /Q "%RsDeployPath%\stylesheets\__MACOSX__Bubble" %Quite%

echo copy sounds
xcopy /S "%SourcePath%\retroshare-gui\src\sounds" "%RsDeployPath%\sounds" %Quite%

echo copy translation
copy "%SourcePath%\retroshare-gui\src\translations\qt_tr.qm" "%RsDeployPath%\translations" %Quite%
copy "%QtPath%\..\translations\qt_*.qm" "%RsDeployPath%\translations" %Quite%
if "%QtMainVersion%"=="5" (
	copy "%QtPath%\..\translations\qtbase_*.qm" "%RsDeployPath%\translations" %Quite%
	copy "%QtPath%\..\translations\qtscript_*.qm" "%RsDeployPath%\translations" %Quite%
	copy "%QtPath%\..\translations\qtquick1_*.qm" "%RsDeployPath%\translations" %Quite%
	copy "%QtPath%\..\translations\qtmultimedia_*.qm" "%RsDeployPath%\translations" %Quite%
	copy "%QtPath%\..\translations\qtxmlpatterns_*.qm" "%RsDeployPath%\translations" %Quite%
)

echo copy bdboot.txt
copy "%SourcePath%\libbitdht\src\bitdht\bdboot.txt" "%RsDeployPath%" %Quite%

echo copy changelog.txt
copy "%SourcePath%\retroshare-gui\src\changelog.txt" "%RsDeployPath%" %Quite%

if exist "%SourcePath%\libresapi\src\webui" (
	echo copy webui
	mkdir "%RsDeployPath%\webui"
	xcopy /S "%SourcePath%\libresapi\src\webui" "%RsDeployPath%\webui" %Quite%
)

rem pack files
title Pack - %SourceName%-%RsBuildConfig% [pack files]

"%EnvSevenZipExe%" a -mx=9 -t7z "%Archive%" "%RsDeployPath%\*"

title %COMSPEC%

call :cleanup

endlocal
exit /B 0

:error
call :Cleanup
endlocal
exit /B 1

:cleanup
goto :EOF

:error_env
echo Failed to initialize environment.
endlocal
exit /B 1

:copy_extension
if exist "%~1\%RsBuildConfig%\%~n1.dll" (
	copy "%~1\%RsBuildConfig%\%~n1.dll" %2 %Quite%
)
goto :EOF

:copy_dependencies
for /F "usebackq" %%A in (`%ToolsPath%\depends.bat list %1`) do (
	if exist "%QtPath%\%%A" (
		copy "%QtPath%\%%A" %2 %Quite%
	) else (
		if exist "%MinGWPath%\%%A" (
			copy "%MinGWPath%\%%A" %2 %Quite%
		)
	)
)
goto :EOF
