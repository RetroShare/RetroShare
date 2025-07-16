@echo off

setlocal

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
	:: Install NSIS
	%EnvMSYS2Cmd% "pacman --noconfirm --needed -S mingw-w64-%RsMSYS2Architecture%-nsis"
)

:: Check deployment
if not exist "%RsDeployPath%\retroshare.exe" echo Project is not deployed. Run pack.bat first! & goto error

:: Get compiled revision
set GetRsVersion=%SourcePath%\build_scripts\Windows-msys2\tools\get-rs-version.bat
if not exist "%GetRsVersion%" (
	%cecho% error "File not found"
	echo %GetRsVersion%
	goto error
)

:: Get compiled version
call "%GetRsVersion%" "%RsDeployPath%\retroshare.exe" RsVersion
if errorlevel 1 %cecho% error "Revision not found."& goto error
if "%RsVersion.Extra%"=="" %cecho% error "Extra number not found".& goto error

:: Build defines for script
set NSIS_PARAM=

set NSIS_PARAM=%NSIS_PARAM% /DDEPLOYDIR="%RsDeployPath%"
set NSIS_PARAM=%NSIS_PARAM% /DOUTDIR="%RsPackPath%"
set NSIS_PARAM=%NSIS_PARAM% /DINSTALLERADD="%RsArchiveAdd%"
set NSIS_PARAM=%NSIS_PARAM% /DARCHITECTURE="%RsArchitecture%"
set NSIS_PARAM=%NSIS_PARAM% /DTOOLCHAIN="%RsToolchain%"
set NSIS_PARAM=%NSIS_PARAM% /DREVISION=%RsVersion.Extra%

set QtMainVersion=%QtVersion:~0,1%

:: Create installer
echo %path%
rem makensis %NSIS_PARAM% "%SourcePath%\build_scripts\Windows-msys2\installer\retroshare-Qt%QtMainVersion%.nsi"
rem pushd "%SourcePath%\build_scripts\Windows-msys2\installer"
rem %EnvMSYS2Cmd% "makensis $0 retroshare-Qt%QtMainVersion%.nsi" "%NSIS_PARAM%"
rem popd
"%RsMinGWPath%\bin\makensis" %NSIS_PARAM% "%SourcePath%\build_scripts\Windows-msys2\installer\retroshare-Qt%QtMainVersion%.nsi"

exit /B %ERRORLEVEL%

:error
endlocal
exit /B 1

:error_env
echo Failed to initialize environment.
endlocal
exit /B 1
