call "%~dp0env-base.bat" %*
if errorlevel 2 exit /B 2
if errorlevel 1 goto error_env

set MSYSTEM=MINGW%RsBit%

set BuildPath=%EnvRootPath%\builds
set DeployPath=%EnvRootPath%\deploy

if not exist "%BuildPath%" mkdir "%BuildPath%"
if not exist "%DeployPath%" mkdir "%DeployPath%"

:: Get Qt version
call "%ToolsPath%\get-qt-version.bat" QtVersion
if "%QtVersion%"=="" %cecho% error "Cannot get Qt version." & exit /B 1

set RsMinGWPath=%EnvMSYS2BasePath%\mingw%RsBit%

set RsBuildPath=%BuildPath%\Qt-%QtVersion%-%RsArchitecture%-%RsBuildConfig%
set RsDeployPath=%DeployPath%\Qt-%QtVersion%%RsType%-%RsArchitecture%-%RsBuildConfig%
set RsPackPath=%DeployPath%
set RsArchiveAdd=

if not exist "%~dp0env-mod.bat" goto no_mod
call "%~dp0env-mod.bat"
if errorlevel 1 exit /B %ERRORLEVEL%
:no_mod

exit /B 0
