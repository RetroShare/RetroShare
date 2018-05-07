if "%~1"=="standard" (
	set RsRetroTor=
	set RsType=
) else (
	if "%~1"=="retrotor" (
		set RsRetroTor=1
		set RsType=-tor
	) else (
		echo.
		echo Usage: standard^|retrotor
		echo.
		exit /B 2
	)
)

set BuildPath=%EnvRootPath%\builds
set DeployPath=%EnvRootPath%\deploy

if not exist "%BuildPath%" mkdir "%BuildPath%"
if not exist "%DeployPath%" mkdir "%DeployPath%"

:: Check Qt environment
set QtPath=
call "%ToolsPath%\find-in-path.bat" QtPath qmake.exe
if "%QtPath%"=="" %cecho% error "Please run command in the Qt Command Prompt." & exit /B 1

:: Check MinGW environment
set MinGWPath=
call "%ToolsPath%\find-in-path.bat" MinGWPath gcc.exe
if "%MinGWPath%"=="" %cecho% error "Please run command in the Qt Command Prompt." & exit /B 1

:: Get Qt version
call "%ToolsPath%\get-qt-version.bat" QtVersion
if "%QtVersion%"=="" %cecho% error "Cannot get Qt version." & exit /B 1

:: Get gcc versions
call "%ToolsPath%\get-gcc-version.bat" GCCVersion
if "%GCCVersion%"=="" %cecho% error "Cannot get gcc version." & exit /B 1

set BuildLibsPath=%EnvRootPath%\build-libs\gcc-%GCCVersion%

set RsBuildConfig=release
set RsBuildPath=%BuildPath%\Qt-%QtVersion%%RsType%-%RsBuildConfig%
set RsDeployPath=%DeployPath%\Qt-%QtVersion%%RsType%-%RsBuildConfig%
set RsPackPath=%DeployPath%
set RsArchiveAdd=

if not exist "%~dp0env-mod.bat" goto no_mod
call "%~dp0env-mod.bat"
if errorlevel 1 exit /B %ERRORLEVEL%
:no_mod

exit /B 0