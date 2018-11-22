call :make_path SourcePath "%~dp0..\.."
call :make_path RootPath "%SourcePath%\.."
call :source_name SourceName "%SourcePath%"
set ToolsPath=%~dp0tools
set EnvPath=%~dp0env

exit /B 0

:make_path
setlocal
set Var=%~1
pushd %2
set CD=%cd%
popd
endlocal & set %Var%=%CD%
goto :EOF

:source_name
set %~1=%~nx2
