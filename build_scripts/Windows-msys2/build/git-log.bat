@echo off

setlocal

set NoAsk=
if "%~2"=="no-ask" set NoAsk=1

:: Initialize environment
call "%~dp0..\env.bat"
if errorlevel 1 goto error_env
call "%EnvPath%\env.bat"
if errorlevel 1 goto error_env
call "%EnvPath%\env-msys2.bat"
if errorlevel 1 goto error_env

call "%~dp0env.bat" %*
if errorlevel 2 exit /B 2
if errorlevel 1 goto error_env

:: Check git executable
set GitPath=
call "%ToolsPath%\find-in-path.bat" GitPath git.exe
if "%GitPath%"=="" echo Git executable not found in PATH.& exit /B 1

:: Get compiled revision
set GetRsVersion=%SourcePath%\build_scripts\Windows-msys2\tools\get-rs-version.bat
if not exist "%GetRsVersion%" (
	echo File not found
	echo %GetRsVersion%
	exit /B 1
)

call "%GetRsVersion%" RS_REVISION_STRING RsRevision
if "%RsRevision%"=="" echo Revision not found.& exit /B 1

:: Get compiled version
call "%GetRsVersion%" RS_REVISION_STRING RsRevision
if "%RsRevision%"=="" echo Revision not found.& exit /B 1

call "%GetRsVersion%" RS_MAJOR_VERSION RsMajorVersion
if "%RsMajorVersion%"=="" echo Major version not found.& exit /B 1

call "%GetRsVersion%" RS_MINOR_VERSION RsMinorVersion
if "%RsMinorVersion%"=="" echo Minor version not found.& exit /B 1

call "%GetRsVersion%" RS_BUILD_NUMBER RsBuildNumber
if "%RsBuildNumber%"=="" echo Build number not found.& exit /B 1

call "%GetRsVersion%" RS_BUILD_NUMBER_ADD RsBuildNumberAdd

set RsVersion=%RsMajorVersion%.%RsMinorVersion%.%RsBuildNumber%%RsBuildNumberAdd%

:: Check WMIC is available
wmic.exe alias /? >nul 2>&1 || echo WMIC is not available.&& exit /B 1

:: Use WMIC to retrieve date in format YYYYMMDD
set RsDate=
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /format:list') do set RsDate=%%I
set RsDate=%RsDate:~0,4%%RsDate:~4,2%%RsDate:~6,2%

:: Get last revision
set RsLastRefFile=%BuildPath%\Qt-%QtVersion%%RsType%-%RsBuildConfig%-LastRef.txt
set RsLastRef=
if exist "%RsLastRefFile%" set /P RsLastRef=<"%RsLastRefFile%"

if "%NoAsk%"=="1" goto no_ask_for_last_revision
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

if "%NoAsk%"=="1" goto no_confirm
choice /M "Do you want to proceed?"
if %errorlevel%==2 exit /B 1
:no_confirm

if "%RsBuildConfig%" NEQ "release" (
	set RsGitLog=%DeployPath%\RetroShare-%RsVersion%-Windows-Portable-%RsDate%-%RsRevision%-Qt-%QtVersion%%RsType%-msys2%RsArchiveAdd%-%RsBuildConfig%.txt
) else (
	set RsGitLog=%DeployPath%\RetroShare-%RsVersion%-Windows-Portable-%RsDate%-%RsRevision%-Qt-%QtVersion%%RsType%-msys2%RsArchiveAdd%.txt
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
