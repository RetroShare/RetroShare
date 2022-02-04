@echo off

setlocal

set Quite=^>nul

:: Initialize environment
call "%~dp0..\env.bat"
if errorlevel 1 goto error_env
call "%EnvPath%\env.bat"
if errorlevel 1 goto error_env
call "%EnvPath%\env-msys2.bat"
if errorlevel 1 goto error_env

:: Initialize environment
call "%~dp0env.bat" %*
if errorlevel 2 exit /B 2
if errorlevel 1 goto error_env

if not "%ParamNoupdate%"=="1" (
	:: Install ntldd
	%EnvMSYS2Cmd% "pacman --noconfirm --needed -S mingw-w64-%RsMSYS2Architecture%-ntldd-git"
	
	:: Install tor
	if "%ParamTor%"=="1" (
		%EnvMSYS2Cmd% "pacman --noconfirm --needed -S mingw-w64-%RsMSYS2Architecture%-tor"
	)
)

:: Remove deploy path
if exist "%RsDeployPath%" rmdir /S /Q "%RsDeployPath%"

:: Check compilation
if not exist "%RsBuildPath%\Makefile" echo Project is not compiled.& goto error

:: Get compiled revision
set GetRsVersion=%SourcePath%\build_scripts\Windows-msys2\tools\get-rs-version.bat
if not exist "%GetRsVersion%" (
	%cecho% error "File not found"
	echo %GetRsVersion%
	goto error
)

:: Get compiled version
call "%GetRsVersion%" "%RsBuildPath%\retroshare-gui\src\%RsBuildConfig%\retroshare.exe" RsVersion
if errorlevel 1 %cecho% error "Revision not found."& goto error

if "%RsVersion.Major%"=="" %cecho% error "Major version not found."& goto error
if "%RsVersion.Minor%"=="" %cecho% error "Minor version not found."& goto error
if "%RsVersion.Mini%"=="" %cecho% error "Mini number not found".& goto error
if "%RsVersion.Extra%"=="" %cecho% error "Extra number not found".& goto error

set RsVersion=%RsVersion.Major%.%RsVersion.Minor%.%RsVersion.Mini%

:: Check WMIC is available
wmic.exe alias /? >nul 2>&1 || echo WMIC is not available.&& goto error

:: Use WMIC to retrieve date in format YYYYMMDD
set RsDate=
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /format:list') do set RsDate=%%I
set RsDate=%RsDate:~0,4%%RsDate:~4,2%%RsDate:~6,2%

set QtMainVersion=%QtVersion:~0,1%
set QtSharePath=%RsMinGWPath%\share\qt%QtMainVersion%\

rem Qt 4 = QtSvg4.dll
rem Qt 5 = Qt5Svg.dll
set QtMainVersion1=
set QtMainVersion2=
if "%QtMainVersion%"=="4" set QtMainVersion2=4
if "%QtMainVersion%"=="5" set QtMainVersion1=5

if "%RsBuildConfig%" NEQ "release" (
	set Archive=%RsPackPath%\RetroShare-%RsVersion%-Windows-Portable-%RsDate%-%RsVersion.Extra%-%RsArchitecture%-msys2%RsType%%RsArchiveAdd%-%RsBuildConfig%.7z
) else (
	set Archive=%RsPackPath%\RetroShare-%RsVersion%-Windows-Portable-%RsDate%-%RsVersion.Extra%-%RsArchitecture%-msys2%RsType%%RsArchiveAdd%.7z
)

if exist "%Archive%" del /Q "%Archive%"

:: Create deploy path
mkdir "%RsDeployPath%"

title Pack - %SourceName%%RsType%-%RsBuildConfig% [copy files]

set ExtensionsFile=%SourcePath%\libretroshare\src\rsserver\rsinit.cc
set Extensions=
for /f %%e in ('type "%ExtensionsFile%" ^| sed.exe -n "s/^.*\/\(extensions[^/]*\)\/.*$/\1/p" ^| sed.exe -n "1,1p"') do set Extensions=%%e
if "%Extensions%"=="" echo Folder for extensions not found in %ExtensionsFile%& goto error

:: Copy files
mkdir "%RsDeployPath%\Data\%Extensions%"
mkdir "%RsDeployPath%\imageformats"
mkdir "%RsDeployPath%\qss"
mkdir "%RsDeployPath%\stylesheets"
mkdir "%RsDeployPath%\sounds"
mkdir "%RsDeployPath%\translations"
mkdir "%RsDeployPath%\license"

copy nul "%RsDeployPath%\portable" %Quite%

echo copy binaries
copy "%RsBuildPath%\retroshare-gui\src\%RsBuildConfig%\RetroShare*.exe" "%RsDeployPath%" %Quite%
copy "%RsBuildPath%\retroshare-nogui\src\%RsBuildConfig%\retroshare*-nogui.exe" "%RsDeployPath%" %Quite%
copy "%RsBuildPath%\retroshare-service\src\%RsBuildConfig%\retroshare*-service.exe" "%RsDeployPath%" %Quite%
copy "%RsBuildPath%\supportlibs\cmark\build\src\libcmark.dll" "%RsDeployPath%" %Quite%
if exist "%RsBuildPath%\libretroshare\src\lib\retroshare.dll" copy "%RsBuildPath%\libretroshare\src\lib\retroshare.dll" "%RsDeployPath%" %Quite%
if exist "%RsBuildPath%\retroshare-friendserver\src\%RsBuildConfig%\retroshare-friendserver.exe" (
	copy "%RsBuildPath%\retroshare-friendserver\src\%RsBuildConfig%\retroshare-friendserver.exe" "%RsDeployPath%" %Quite%
)

echo copy extensions
for /D %%D in ("%RsBuildPath%\plugins\*") do (
	call :copy_extension "%%D" "%RsDeployPath%\Data\%Extensions%"
)

echo copy Qt DLL's
copy "%RsMinGWPath%\bin\Qt%QtMainVersion1%Svg%QtMainVersion2%.dll" "%RsDeployPath%" %Quite%

if "%QtMainVersion%"=="5" (
	mkdir "%RsDeployPath%\platforms"
	copy "%QtSharePath%\plugins\platforms\qwindows.dll" "%RsDeployPath%\platforms" %Quite%
	mkdir "%RsDeployPath%\audio"
	copy "%QtSharePath%\plugins\audio\qtaudio_windows.dll" "%RsDeployPath%\audio" %Quite%
)

if exist "%QtSharePath%\plugins\styles\qwindowsvistastyle.dll" (
	echo copy styles
	mkdir "%RsDeployPath%\styles" %Quite%
	copy "%QtSharePath%\plugins\styles\qwindowsvistastyle.dll" "%RsDeployPath%\styles" %Quite%
)

copy "%QtSharePath%\plugins\imageformats\*.dll" "%RsDeployPath%\imageformats" %Quite%
del /Q "%RsDeployPath%\imageformats\*d?.dll" %Quite%

if "%ParamTor%"=="1" (
	echo copy tor
	if not exist "%RsDeployPath%\tor" mkdir "%RsDeployPath%\tor"
	copy "%RsMinGWPath%\bin\tor.exe" "%RsDeployPath%\tor" %Quite%
	copy "%RsMinGWPath%\bin\tor-gencert.exe" "%RsDeployPath%\tor" %Quite%

	echo copy tor dependencies
	for /R "%RsDeployPath%\tor" %%D in (*.exe) do (
		call :copy_dependencies "%%D" "%RsDeployPath%\tor"
	)
)

echo copy dependencies
for /R "%RsDeployPath%" %%D in (*.dll, *.exe) do (
	call :copy_dependencies "%%D" "%RsDeployPath%"
)

if exist "%SourcePath%\retroshare-gui\src\qss" (
	echo copy qss
	xcopy /S "%SourcePath%\retroshare-gui\src\qss" "%RsDeployPath%\qss" %Quite%
)

echo copy stylesheets
xcopy /S "%SourcePath%\retroshare-gui\src\gui\qss\chat" "%RsDeployPath%\stylesheets" %Quite%
rmdir /S /Q "%RsDeployPath%\stylesheets\compact" %Quite%
rmdir /S /Q "%RsDeployPath%\stylesheets\standard" %Quite%

echo copy sounds
xcopy /S "%SourcePath%\retroshare-gui\src\sounds" "%RsDeployPath%\sounds" %Quite%

echo copy license
xcopy /S "%SourcePath%\retroshare-gui\src\license" "%RsDeployPath%\license" %Quite%

echo copy translation
copy "%SourcePath%\retroshare-gui\src\translations\qt_tr.qm" "%RsDeployPath%\translations" %Quite%
copy "%QtSharePath%\translations\qt_*.qm" "%RsDeployPath%\translations" %Quite%
if "%QtMainVersion%"=="5" (
	copy "%QtSharePath%\translations\qtbase_*.qm" "%RsDeployPath%\translations" %Quite%
	copy "%QtSharePath%\translations\qtscript_*.qm" "%RsDeployPath%\translations" %Quite%
	copy "%QtSharePath%\translations\qtquick1_*.qm" "%RsDeployPath%\translations" %Quite%
	copy "%QtSharePath%\translations\qtmultimedia_*.qm" "%RsDeployPath%\translations" %Quite%
	copy "%QtSharePath%\translations\qtxmlpatterns_*.qm" "%RsDeployPath%\translations" %Quite%
)

echo copy bdboot.txt
copy "%SourcePath%\libbitdht\src\bitdht\bdboot.txt" "%RsDeployPath%" %Quite%

echo generate changelog.txt
call call "%~dp0\git-log.bat" "%SourcePath%" "%RsDeployPath%\changelog.txt"

echo copy buildinfo.txt
copy "%RsBuildPath%\buildinfo.txt" "%RsDeployPath%" %Quite%

if "%ParamWebui%"=="1" (
	if exist "%RsWebuiPath%\webui" (
		echo copy webui
		mkdir "%RsDeployPath%\webui"
		xcopy /S "%RsWebuiPath%\webui" "%RsDeployPath%\webui" %Quite%
	) else (
		%cecho% error "Webui is enabled, but no webui data found at %RsWebuiPath%\webui"
		goto error
	)
)

rem pack files
title Pack - %SourceName%%RsType%-%RsBuildConfig% [pack files]

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
if exist "%~1\lib\%~n1.dll" (
	copy "%~1\lib\%~n1.dll" %2 %Quite%
)
goto :EOF

:copy_dependencies
for /F "usebackq" %%A in (`%ToolsPath%\depends.bat %1`) do (
	if not exist "%~2\%%A" (
		if exist "%RsMinGWPath%\bin\%%A" (
			copy "%RsMinGWPath%\bin\%%A" %2 %Quite%
		)
	)
)
goto :EOF
