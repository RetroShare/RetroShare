:: Usage:
:: call prepare-qt.bat version [reinstall|clean]

setlocal enabledelayedexpansion

if "%EnvQtBasePath%"=="" exit /B 1
if not exist "%EnvRootPath%"=="" exit /B 1

set EnvQtVersion=%~1
if "%EnvQtVersion%"=="" (
	%cecho% error "Please specify Qt version"
	goto error
)

for /f "tokens=1,2 delims=." %%A in ("%EnvQtVersion%") do set EnvQtMainVersion=%%A& set EnvQtBaseVersion=%%A.%%B
set EnvQtPath=%EnvQtBasePath%\%EnvQtVersion%

if "%~2"=="clean" (
	%cecho% info "Clean Qt %EnvQtVersion%"
	call "%ToolsPath%\remove-dir.bat" "%EnvQtPath%"
	goto exit
)

set CheckQmakeExe=
if "%EnvQtMainVersion%"=="4" (
	set CheckQmakeExe=%EnvQtPath%\Qt\bin\qmake.exe
) else (
	if "%EnvQtMainVersion%" GEQ "5" (
		call :get_mingw_version EnvQtMinGWVersion "%EnvQtPath%\%EnvQtBaseVersion%"
		if "!EnvQtMinGWVersion!" NEQ "" (
			set CheckQmakeExe=%EnvQtPath%\%EnvQtBaseVersion%\!EnvQtMinGWVersion!\bin\qmake.exe
		)
	)
)

if "%CheckQmakeExe%" NEQ "" (
	if exist "%CheckQmakeExe%" (
		if "%~2"=="reinstall" (
			choice /M "Found existing Qt %EnvQtVersion% version. Do you want to proceed?"
			if !ERRORLEVEL!==2 goto exit
		) else (
			goto exit
		)
	)
)

set QtInstall=qt-opensource-windows-x86-mingw-%EnvQtVersion%.exe
set QtInstallWildcard=qt-opensource-windows-x86-mingw*-%EnvQtVersion%.exe
set QtUrl=http://download.qt.io/official_releases/qt/%EnvQtBaseVersion%/%EnvQtVersion%

%cecho% info "Remove previous Qt %EnvQtVersion% version"
call "%ToolsPath%\remove-dir.bat" "%EnvQtPath%"

%cecho% info "Download Qt installation files"
if not exist "%EnvDownloadPath%\%QtInstall%" (
	call "%ToolsPath%\download-file-wildcard.bat" "%QtUrl%" "%QtInstallWildcard%" "%EnvDownloadPath%" QtInstallDownload
	if "!QtInstallDownload!"=="" %cecho% error "Cannot download Qt %EnvQtVersion%" & goto error
	ren "%EnvDownloadPath%\!QtInstallDownload!" "%QtInstall%"
)
if not exist "%EnvDownloadPath%\%QtInstall%" %cecho% error "Cannot download Qt %EnvQtVersion%" & goto error

mkdir "%EnvQtPath%"

if "%EnvQtMainVersion%"=="4" (
	rem Qt 4
	goto install_qt4
)
if "%EnvQtMainVersion%" GEQ "5" (
	rem Qt >= 5
	goto install_qt5
)

%cecho% error "Unknown Qt version %EnvQtVersion%"

:error
endlocal & set QtDir=& set MinGWDir=
exit /B 1

:exit
set QtDir=
set MinGWDir=

if "%EnvQtMainVersion%"=="4" (
	rem Qt 4
	set QtDir=%EnvQtBasePath%\%EnvQtVersion%\Qt
	set MinGWDir=%EnvQtBasePath%\%EnvQtVersion%\mingw32
) else (
	if "%EnvQtMainVersion%" GEQ "5" (
		call :get_mingw_version EnvQtToolsMinGWVersion "%EnvQtPath%\Tools"

		set QtDir=%EnvQtPath%\%EnvQtBaseVersion%\!EnvQtMinGWVersion!
		set MinGWDir=%EnvQtPath%\Tools\!EnvQtToolsMinGWVersion!
	)
)

endlocal & set QtDir=%QtDir%& set MinGWDir=%MinGWDir%
exit /B 0

:get_mingw_version
setlocal enabledelayedexpansion
set Variable=%~1
set Result=

for /D %%A in (%~2\*) do set Name=%%~nA& if "!Name:~0,5!"=="mingw" set Result=!Name!
endlocal & set %Variable%=%Result%
goto :EOF

:replace
set InFile=%~1
set InFileName=%~nx1
set OutFile=%~1.tmp
set SearchText=%~2
set ReplaceText=%~3

if exist "%OutFile%" del /Q "%OutFile%"

for /f "tokens=1* delims=]" %%A in ('find /n /v ""^<%InFile%') do (
	set string=%%B

	if "!string!"=="" (
		echo.>>%OutFile%
	) else (
		set modified=!string:%SearchText%=%ReplaceText%!
		echo !modified!>> %OutFile%
	)
)
del "%InFile%"
rename "%OutFile%" "%InFileName%"
goto :EOF

:install_qt4
set MinGWInstall=i686-4.8.2-release-posix-dwarf-rt_v3-rev3.7z
set MinGWUrl=http://sourceforge.net/projects/mingw-w64/files/Toolchains targetting Win32/Personal Builds/mingw-builds/4.8.2/threads-posix/dwarf/%MinGWInstall%/download

%cecho% info "Download MinGW installation files"
if not exist "%EnvDownloadPath%\%MinGWInstall%" call "%ToolsPath%\download-file.bat" "%MinGWUrl%" "%EnvDownloadPath%\%MinGWInstall%"
if not exist "%EnvDownloadPath%\%MinGWInstall%" %cecho% error "Cannot download MinGW" & goto error

%cecho% info "Unpack Qt %EnvQtVersion%"
call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
mkdir "%EnvTempPath%"
"%EnvSevenZipExe%" x -o"%EnvTempPath%" "%EnvDownloadPath%\%QtInstall%" $_14_
move "%EnvTempPath%\$_14_" "%EnvQtPath%\Qt"
call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"

%cecho% info "Unpack MinGW"
"%EnvSevenZipExe%" x -o"%EnvQtPath%" "%EnvDownloadPath%\%MinGWInstall%"

echo Prepare Qt %EnvQtVersion%
echo [Paths]>"%EnvQtPath%\Qt\bin\qt.conf"
echo Prefix=..>>"%EnvQtPath%\Qt\bin\qt.conf"

goto exit

:install_qt5
set EnvQtInstallerFrameworkVersion=2.0.3

set QtInstallerFrameworkInstall=QtInstallerFramework-%EnvQtInstallerFrameworkVersion%-win-x86.exe
set QtInstallerFrameworkUrl=http://download.qt.io/official_releases/qt-installer-framework/%EnvQtInstallerFrameworkVersion%/QtInstallerFramework-win-x86.exe

%cecho% info "Download QtInstallerFramework installation files"
if not exist "%EnvDownloadPath%\%QtInstallerFrameworkInstall%" call "%ToolsPath%\download-file.bat" "%QtInstallerFrameworkUrl%" "%EnvDownloadPath%\%QtInstallerFrameworkInstall%"
if not exist "%EnvDownloadPath%\%QtInstallerFrameworkInstall%" %cecho% error "Cannot download Qt Installer Framework %EnvQtInstallerFrameworkVersion%" & goto error

%cecho% info "Unpack Qt Installer Framework"
call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
mkdir "%EnvTempPath%"
"%EnvSevenZipExe%" x -o"%EnvTempPath%" "%EnvDownloadPath%\%QtInstallerFrameworkInstall%" bin\devtool.exe
move "%EnvTempPath%\bin\devtool.exe" "%EnvQtPath%"
call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"

%cecho% info "Unpack Qt %EnvQtVersion%"
call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
mkdir "%EnvTempPath%"
"%EnvQtPath%\devtool.exe" "%EnvDownloadPath%\%QtInstall%" --dump "%EnvTempPath%"

pushd "%EnvTempPath%"
del /S *vcredist*.7z
del /S *qtcreator*.7z
del /S *1installer-changelog.7z
for /R %%F in (*.7z) do "%EnvSevenZipExe%" x -y -o"%EnvQtPath%" "%%F"
popd

call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"

call :get_mingw_version EnvQtMinGWVersion "%EnvQtPath%\%EnvQtBaseVersion%"

%cecho% info "Prepare Qt %EnvQtVersion%"
echo [Paths]>"%EnvQtPath%\%EnvQtBaseVersion%\%EnvQtMinGWVersion%\bin\qt.conf"
echo Documentation=../../Docs/Qt-%EnvQtBaseVersion%>>"%EnvQtPath%\%EnvQtBaseVersion%\%EnvQtMinGWVersion%\bin\qt.conf"
echo Examples=../../Examples/Qt-%EnvQtBaseVersion%>>"%EnvQtPath%\%EnvQtBaseVersion%\%EnvQtMinGWVersion%\bin\qt.conf"
echo Prefix=..>>"%EnvQtPath%\%EnvQtBaseVersion%\%EnvQtMinGWVersion%\bin\qt.conf"

call :replace "%EnvQtPath%\%EnvQtBaseVersion%\%EnvQtMinGWVersion%\mkspecs\qconfig.pri" "Enterprise" "OpenSource"

for /R "%EnvQtPath%\%EnvQtBaseVersion%\%EnvQtMinGWVersion%\lib" %%A in (*.pc) do (
	call :replace "%%A" "c:/Users/qt/work/install" "%EnvQtPath:\=\\%\%EnvQtBaseVersion%\\%EnvQtMinGWVersion%"
	call :replace "%%A" "c:\Users\qt\work\install" "%EnvQtPath:\=/%\%EnvQtBaseVersion%/%EnvQtMinGWVersion%"
)
for /R "%EnvQtPath%\%EnvQtBaseVersion%\%EnvQtMinGWVersion%\lib" %%A in (*.prl) do (
	call :replace "%%A" "c:/Users/qt/work/install" "%EnvQtPath:\=\\%\%EnvQtBaseVersion%\\%EnvQtMinGWVersion%"
	call :replace "%%A" "c:\Users\qt\work\install" "%EnvQtPath:\=/%\%EnvQtBaseVersion%/%EnvQtMinGWVersion%"
)
goto exit
