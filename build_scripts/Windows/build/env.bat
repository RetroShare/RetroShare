set BuildPath=%EnvRootPath%\builds
set DeployPath=%EnvRootPath%\deploy

if not exist "%BuildPath%" mkdir "%BuildPath%"
if not exist "%DeployPath%" mkdir "%DeployPath%"

:: Check Qt environment
set QtPath=
call "%ToolsPath%\find-in-path.bat" QtPath qmake.exe
if "%QtPath%"=="" echo Please run command in the Qt Command Prompt.& exit /B 1

:: Check MinGW environment
set MinGWPath=
call "%ToolsPath%\find-in-path.bat" MinGWPath gcc.exe
if "%MinGWPath%"=="" echo Please run command in the Qt Command Prompt.& exit /B 1

:: Get Qt version
call "%ToolsPath%\get-qt-version.bat" QtVersion
if "%QtVersion%"=="" echo Cannot get Qt version.& exit /B 1

set RsBuildConfig=release
set RsBuildPath=%BuildPath%\Qt-%QtVersion%-%RsBuildConfig%
set RsDeployPath=%DeployPath%\Qt-%QtVersion%-%RsBuildConfig%
set RsPackPath=%DeployPath%
set RsArchiveAdd=

if not exist "%~dp0env-mod.bat" goto no_mod
call "%~dp0env-mod.bat"
if errorlevel 1 exit /B %ERRORLEVEL%
:no_mod

exit /B 0