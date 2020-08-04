@echo off

setlocal

:: Initialize environment
call "%~dp0..\env.bat"
if errorlevel 1 goto error_env
call "%EnvPath%\env.bat"
if errorlevel 1 goto error_env

call "%~dp0env.bat" %*
if errorlevel 2 exit /B 2
if errorlevel 1 goto error_env

:: Check git executable
set GitPath=
call "%ToolsPath%\find-in-path.bat" GitPath git.exe
if "%GitPath%"=="" echo Git executable not found in PATH.& exit /B 1

:: Get compiled revision
set GetRsVersion=%SourcePath%\build_scripts\Windows\tools\get-rs-version.bat
if not exist "%GetRsVersion%" (
	echo File not found
	echo %GetRsVersion%
	exit /B 1
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
wmic.exe alias /? >nul 2>&1 || echo WMIC is not available.&& exit /B 1

:: Use WMIC to retrieve date in format YYYYMMDD
set RsDate=
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /format:list') do set RsDate=%%I
set RsDate=%RsDate:~0,4%%RsDate:~4,2%%RsDate:~6,2%

:: Get last revision
set RsLastRefFile=%BuildPath%\Qt-%QtVersion%-%GCCArchitecture%%RsType%-%RsBuildConfig%-LastRef.txt
set RsLastRef=
if exist "%RsLastRefFile%" set /P RsLastRef=<"%RsLastRefFile%"

if "%NonInteractive%"=="1" goto no_ask_for_last_revision
if not "%RsLastRef%"=="" echo Last Revision was %RsLastRef%
set /P RsLastRefInput=Last Revision: 
if "%RsLastRefInput%" NEQ "" set RsLastRef=%RsLastRefInput%
:no_ask_for_last_revision

:: Get current revision
pushd "%SourcePath%"
call "%ToolsPath%\get-git-ref.bat" RsRef
popd

if errorlevel 1 exit /B 1
if "%RsRef%"=="" echo Cannot get git revision.& exit /B 1

echo.
echo Creating log from %RsLastRef%
echo                to %RsRef%

if "%NonInteractive%"=="1" goto no_confirm
choice /M "Do you want to proceed?"
if %errorlevel%==2 exit /B 1
:no_confirm

if "%RsBuildConfig%" NEQ "release" (
	set RsGitLog=%DeployPath%\RetroShare-%RsVersion%-Windows-Portable-%RsDate%-%RsVersion.Extra%-Qt-%QtVersion%-%GCCArchitecture%%RsType%%RsArchiveAdd%-%RsBuildConfig%.txt
) else (
	set RsGitLog=%DeployPath%\RetroShare-%RsVersion%-Windows-Portable-%RsDate%-%RsVersion.Extra%-Qt-%QtVersion%-%GCCArchitecture%%RsType%%RsArchiveAdd%.txt
)

title %SourceName%-%RsBuildConfig% [git log]

pushd "%SourcePath%"
if "%RsLastRef%"=="" (
	git log %RsRef% >"%RsGitLog%"
) else (
	if "%RsLastRef%"=="%RsRef%" (
		git log %RsRef% --max-count=1 >"%RsGitLog%"
	) else (
		git log %RsLastRef%..%RsRef% >"%RsGitLog%"
	)
)
popd

title %COMSPEC%

echo %RsRef%>"%RsLastRefFile%"

exit /B %ERRORLEVEL%

:error_env
echo Failed to initialize environment.
endlocal
exit /B 1
