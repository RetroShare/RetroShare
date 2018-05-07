:: Usage:
:: call build-libs.bat [make tasks]

@echo off

setlocal

:: Parameter
set MakeParam="DOWNLOAD_PATH=../download"

set MakeTask=
:param_loop
if "%~1" NEQ "" (
		set MakeTask=%MakeTask% %1
		shift /1
		goto param_loop
)

:: Initialize environment
call "%~dp0..\env.bat"
if errorlevel 1 goto error_env
call "%EnvPath%\env-msys.bat"
if errorlevel 1 goto error_env

:: Check MSYS environment
if not exist "%EnvMSYSSH%" %cecho% error "Please install MSYS first." & exit /B 1

:: Initialize environment
call "%~dp0env.bat"
if errorlevel 1 goto error_env

:: Add tools path to PATH environment
set PATH=%EnvToolsPath%;%PATH%

call "%ToolsPath%\msys-path.bat" "%~dp0" MSYSCurPath
call "%ToolsPath%\msys-path.bat" "%BuildLibsPath%" MSYSBuildLibsPath

if not exist "%BuildLibsPath%" mkdir "%BuildLibsPath%"

%EnvMSYSCmd% "cd "%MSYSBuildLibsPath%" && make -f %MSYSCurPath%/makefile %MakeParam% %MakeTask%"

exit /B %ERRORLEVEL%

:error_env
echo Failed to initialize environment.
endlocal
exit /B 1
