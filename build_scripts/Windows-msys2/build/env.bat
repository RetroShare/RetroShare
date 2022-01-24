call "%~dp0env-base.bat" %*
if errorlevel 2 exit /B 2
if errorlevel 1 goto error_env

set BuildPath=%EnvRootPath%\builds
set DeployPath=%EnvRootPath%\deploy

if not exist "%BuildPath%" mkdir "%BuildPath%"
if not exist "%DeployPath%" mkdir "%DeployPath%"

:: Get Qt version
call "%ToolsPath%\get-qt-version.bat" QtVersion
if "%QtVersion%"=="" %cecho% error "Cannot get Qt version." & exit /B 1

set RsMinGWPath=%EnvMSYS2BasePath%\%RsToolchain%

set RsBuildPath=%BuildPath%\Qt-%QtVersion%-%RsToolchain%-%RsCompiler%-%RsBuildConfig%
set RsDeployPath=%DeployPath%\Qt-%QtVersion%%RsType%-%RsToolchain%-%RsCompiler%-%RsBuildConfig%
set RsPackPath=%DeployPath%
set RsArchiveAdd=
set RsWebuiPath=%RootPath%\%SourceName%-webui

if not exist "%~dp0env-mod.bat" goto no_mod
call "%~dp0env-mod.bat"
if errorlevel 1 exit /B %ERRORLEVEL%
:no_mod

exit /B 0
