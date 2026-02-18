call "%~dp0env-base.bat" %*
if errorlevel 2 exit /B 2
if errorlevel 1 exit /B 1

set BuildPath=%EnvRootPath%\builds
set DeployPath=%EnvRootPath%\deploy

if not exist "%BuildPath%" mkdir "%BuildPath%"
if not exist "%DeployPath%" mkdir "%DeployPath%"

set QMakeCmd=
if "%ParamQtVersion%"=="5" set QMakeCmd=qmake-qt5
if "%ParamQtVersion%"=="6" set QMakeCmd=qmake6
if "%QMakeCmd%"=="" %cecho% error "Unknown Qt version %ParamQtVersion%." & exit /B 1

:: Get Qt version
call "%ToolsPath%\get-qt-version.bat" QtVersion %QMakeCmd%
if errorlevel 1 %cecho% error "Cannot get Qt version." & exit /B 1
if "%QtVersion%"=="" %cecho% error "Cannot get Qt version." & exit /B 1

set RsMinGWPath=%EnvMSYS2BasePath%\%RsToolchain%

set RsBuildPath=%BuildPath%\Qt-%QtVersion%-%RsToolchain%-%RsBuildConfig%
set RsDeployPath=%DeployPath%\Qt-%QtVersion%-%RsToolchain%%RsType%-%RsBuildConfig%
set RsPackPath=%DeployPath%
set RsArchiveAdd=
set RsWebuiBuildPath=%RsBuildPath%\retroshare-webui\webui

if not exist "%~dp0env-mod.bat" goto no_mod
call "%~dp0env-mod.bat"
if errorlevel 1 exit /B %ERRORLEVEL%
:no_mod

exit /B 0
