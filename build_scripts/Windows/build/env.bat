:: Process commandline parameter
set ParamRelease=0
set ParamDebug=0
set ParamAutologin=0
set ParamPlugins=0
set ParamTor=0

:parameter_loop
if "%~1" NEQ "" (
	for /f "tokens=1,2 delims==" %%a in ("%~1") do (
		if "%%~a"=="release" (
			set ParamRelease=1
		) else if "%%~a"=="debug" (
			set ParamDebug=1
		) else if "%%~a"=="autologin" (
			set ParamAutologin=1
		) else if "%%~a"=="plugins" (
			set ParamPlugins=1
		) else if "%%~a"=="tor" (
			set ParamTor=1
		) else (
			echo.
			echo Unknown parameter %1
			goto :usage
		)
	)
	shift /1
	goto parameter_loop
)

if "%ParamRelease%"=="1" (
	if "%ParamDebug%"=="1" (
		echo.
		echo Release or Debug?
		goto :usage
	)

	set RsBuildConfig=release
) else if "%ParamDebug%"=="1" (
	set RsBuildConfig=debug
) else goto :usage

if "%ParamTor%"=="1" (
	set RsType=-tor
) else (
	set RsType=
)

set BuildPath=%EnvRootPath%\builds
set DeployPath=%EnvRootPath%\deploy

if not exist "%BuildPath%" mkdir "%BuildPath%"
if not exist "%DeployPath%" mkdir "%DeployPath%"

:: Check Qt environment
set QtPath=
call "%ToolsPath%\find-in-path.bat" QtPath qmake.exe
if "%QtPath%"=="" %cecho% error "Please run command in the Qt Command Prompt." & exit /B 1

:: Check MinGW environment
set MinGWPath=
call "%ToolsPath%\find-in-path.bat" MinGWPath gcc.exe
if "%MinGWPath%"=="" %cecho% error "Please run command in the Qt Command Prompt." & exit /B 1

:: Get Qt version
call "%ToolsPath%\get-qt-version.bat" QtVersion
if "%QtVersion%"=="" %cecho% error "Cannot get Qt version." & exit /B 1

:: Get gcc versions
call "%ToolsPath%\get-gcc-version.bat" GCCVersion GCCArchitecture
if "%GCCVersion%"=="" %cecho% error "Cannot get gcc version." & exit /B 1
if "%GCCArchitecture%"=="" %cecho% error "Cannot get gcc architecture." & exit /B 1

set BuildLibsPath=%EnvRootPath%\build-libs\gcc-%GCCVersion%\%GCCArchitecture%

set RsBuildPath=%BuildPath%\Qt-%QtVersion%-%GCCArchitecture%-%RsBuildConfig%
set RsDeployPath=%DeployPath%\Qt-%QtVersion%%RsType%-%GCCArchitecture%-%RsBuildConfig%
set RsPackPath=%DeployPath%
set RsArchiveAdd=

if not exist "%~dp0env-mod.bat" goto no_mod
call "%~dp0env-mod.bat"
if errorlevel 1 exit /B %ERRORLEVEL%
:no_mod

exit /B 0

:usage
echo.
echo Usage: release^|debug [version autologin plugins]
echo.
echo Mandatory parameter
echo release^|debug      Build release or debug version
echo.
echo Optional parameter (need clean when changed)
echo autologin          Build with autologin
echo plugins            Build plugins
echo.
echo Parameter for pack
echo tor                Pack tor version
echo.
exit /B 2
