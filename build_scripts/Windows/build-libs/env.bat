:: Check MinGW environment
set MinGWPath=
call "%ToolsPath%\find-in-path.bat" MinGWPath gcc.exe
if "%MinGWPath%"=="" %cecho% error "Please run command in the Qt Command Prompt or add the path to MinGW bin folder to PATH variable." & exit /B 1

:: Get gcc versions
call "%ToolsPath%\get-gcc-version.bat" GCCVersion GCCArchitecture
if "%GCCVersion%"=="" %cecho% error "Cannot get gcc version." & exit /B 1
if "%GCCArchitecture%"=="" %cecho% error "Cannot get gcc architecture." & exit /B 1

set BuildLibsPath=%EnvRootPath%\build-libs\gcc-%GCCVersion%\%GCCArchitecture%

exit /B 0
