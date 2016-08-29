@setlocal

@echo off

:: Initialize environment
call "%~dp0_env.bat"

::call :remove_dir "%DownloadPath%"
call :remove_dir "%MSYSPath%"
call :remove_dir "%TempPath%"

call :remove_file "%ToolsPath%\7z.exe"
call :remove_file "%ToolsPath%\7z.dll"
call :remove_file "%ToolsPath%\curl.exe"

call "%~dp0clean.bat"

endlocal
exit /B 0

:remove_dir
if not exist %1 goto :EOF
del /s /f /q %1 >nul
rmdir /s /q %1
goto :EOF

:remove_file
if not exist %1 goto :EOF
del /q %1 >nul
goto :EOF
