@echo off

setlocal

set Quite=^>nul

:: Initialize environment
call "%~dp0..\env.bat"
if errorlevel 1 goto error_env
call "%EnvPath%\env.bat"
if errorlevel 1 goto error_env

:: Initialize environment
call "%~dp0env.bat" %*
if errorlevel 2 exit /B 2
if errorlevel 1 goto error_env

:: Check external libraries
if not exist "%BuildLibsPath%\libs" %cecho% error "Please build external libraries first." & exit /B 1

:: Check gcc version of external libraries
if not exist "%BuildLibsPath%\libs\gcc-version" %cecho% error "Cannot get gcc version of external libraries." & exit /B 1
set /P LibsGCCVersion=<"%BuildLibsPath%\libs\gcc-version"
if "%LibsGCCVersion%" NEQ "%GCCVersion%" %cecho% error "Please use correct version of external libraries. (gcc %GCCVersion% ^<^> libs %LibsGCCVersion%)." & exit /B 1

:: Remove deploy path
if exist "%RsDeployPath%" rmdir /S /Q "%RsDeployPath%"

:: Check compilation
if not exist "%RsBuildPath%\Makefile" echo Project is not compiled.& goto error

:: Get compiled revision
set GetRsVersion=%SourcePath%\build_scripts\Windows\tools\get-rs-version.bat
if not exist "%GetRsVersion%" (
	%cecho% error "File not found"
	echo %GetRsVersion%
	goto error
)

:: Get compiled version
call "%GetRsVersion%" "%RsBuildPath%\retroshare-gui\src\%RsBuildConfig%\retroshare.exe" RsVersion
if errorlevel 1 %cecho% error "Version not found."& goto error

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

if "%ParamTor%"=="1" (
	:: Check for tor executable
	if not exist "%EnvDownloadPath%\tor\Tor\tor.exe" (
		%cecho% error "Tor binary not found. Please download Tor Expert Bundle from\nhttps://www.torproject.org/download/download.html.en\nand unpack to\n%EnvDownloadPath:\=\\%\\tor"
		goto error
	)
)

set QtMainVersion=%QtVersion:~0,1%

rem Qt 4 = QtSvg4.dll
rem Qt 5 = Qt5Svg.dll
set QtMainVersion1=
set QtMainVersion2=
if "%QtMainVersion%"=="4" set QtMainVersion2=4
if "%QtMainVersion%"=="5" set QtMainVersion1=5

if "%RsBuildConfig%" NEQ "release" (
	set Archive=%RsPackPath%\RetroShare-%RsVersion%-Windows-Portable-%RsDate%-%RsVersion.Extra%-Qt-%QtVersion%%RsType%%RsArchiveAdd%-%RsBuildConfig%.7z
) else (
	set Archive=%RsPackPath%\RetroShare-%RsVersion%-Windows-Portable-%RsDate%-%RsVersion.Extra%-Qt-%QtVersion%%RsType%%RsArchiveAdd%.7z
)

if exist "%Archive%" del /Q "%Archive%"

:: Create deploy path
mkdir "%RsDeployPath%"

title Pack - %SourceName%%RsType%-%RsBuildConfig% [copy files]

set ExtensionsFile=%SourcePath%\libretroshare\src\rsserver\rsinit.cc
set Extensions=
for /f %%e in ('type "%ExtensionsFile%" ^| "%EnvSedExe%" -n "s/^.*\/\(extensions[^/]*\)\/.*$/\1/p" ^| "%EnvSedExe%" -n "1,1p"') do set Extensions=%%e
if "%Extensions%"=="" %cecho% error "Folder for extensions not found in %ExtensionsFile%"& goto error

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
copy "%BuildLibsPath%\libs\bin\*.dll" "%RsDeployPath%" %Quite%

echo copy dependencies
call :copy_dependencies "%RsDeployPath%\retroshare.exe" "%RsDeployPath%"

echo copy Qt DLL's
copy "%QtPath%\Qt%QtMainVersion1%Svg%QtMainVersion2%.dll" "%RsDeployPath%" %Quite%

if "%QtMainVersion%"=="5" (
	mkdir "%RsDeployPath%\platforms"
	copy "%QtPath%\..\plugins\platforms\qwindows.dll" "%RsDeployPath%\platforms" %Quite%
	mkdir "%RsDeployPath%\audio"
	copy "%QtPath%\..\plugins\audio\qtaudio_windows.dll" "%RsDeployPath%\audio" %Quite%
)

if exist "%QtPath%\..\plugins\styles\qwindowsvistastyle.dll" (
	echo Copy styles
	mkdir "%RsDeployPath%\styles" %Quite%
	copy "%QtPath%\..\plugins\styles\qwindowsvistastyle.dll" "%RsDeployPath%\styles" %Quite%
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

if "%ParamTor%"=="1" (
	echo copy tor
	echo n | copy /-y "%EnvDownloadPath%\tor\Tor\*.*" "%RsDeployPath%" %Quite%
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
if exist "%~1\%RsBuildConfig%\%~n1.dll" (
	copy "%~1\%RsBuildConfig%\%~n1.dll" %2 %Quite%
)
goto :EOF

:copy_dependencies
set CopyDependenciesCopiedSomething=0
for /F "usebackq" %%A in (`%ToolsPath%\depends.bat list %1`) do (
	if not exist "%~2\%%A" (
		if exist "%QtPath%\%%A" (
			set CopyDependenciesCopiedSomething=1
			copy "%QtPath%\%%A" %2 %Quite%
		) else (
			if exist "%MinGWPath%\%%A" (
				set CopyDependenciesCopiedSomething=1
				copy "%MinGWPath%\%%A" %2 %Quite%
			)
		)
	)
)
if "%CopyDependenciesCopiedSomething%"=="1" goto copy_dependencies
goto :EOF
