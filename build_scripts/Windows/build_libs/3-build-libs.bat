@setlocal

@echo off

:: Initialize environment
call "%~dp0_env.bat"

set MSYSSH=%MSYSPath%\msys\1.0\bin\sh.exe
set MSYSCurPath=/%CurPath:~0,1%/%CurPath:~3%
set MSYSCurPath=%MSYSCurPath:\=/%

if not exist "%MSYSSH%" echo Please install MSYS first.&& exit /B 1

set GCCPath=
call :FIND_IN_PATH g++.exe GCCPath
if "%GCCPath%"=="" echo Please run %~nx0 in the Qt Command Prompt or add the path to MinGW bin folder to PATH variable.&& exit /B 1

"%MSYSSH%" --login -i -c "cd "%MSYSCurPath%" && make -f makefile %*"

exit /B %ERRORLEVEL%

:FIND_IN_PATH
SET PathTemp="%Path:;=";"%"
FOR %%P IN (%PathTemp%) DO (
	IF EXIST "%%~P.\%~1" (
		set %2=%%~P
		goto :EOF
	)
)
