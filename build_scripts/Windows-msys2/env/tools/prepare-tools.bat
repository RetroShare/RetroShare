setlocal

if "%EnvRootPath%"=="" exit /B 1

set CEchoUrl=https://github.com/lordmulder/cecho/releases/download/2015-10-10/cecho.2015-10-10.zip
set CEchoInstall=cecho.2015-10-10.zip
set SevenZipUrl=https://sourceforge.net/projects/sevenzip/files/7-Zip/18.05/7z1805.msi/download
set SevenZipInstall=7z1805.msi
set WgetUrl=https://eternallybored.org/misc/wget/1.19.4/32/wget.exe
set WgetInstall=wget.exe
set SigcheckInstall=Sigcheck.zip
set SigcheckUrl=https://download.sysinternals.com/files/%SigcheckInstall%

if not exist "%EnvToolsPath%\wget.exe" (
	echo Download Wget installation

	if not exist "%EnvDownloadPath%\%WgetInstall%" call "%ToolsPath%\winhttpjs.bat" %WgetUrl% -saveTo "%EnvDownloadPath%\%WgetInstall%"
	if not exist "%EnvDownloadPath%\%WgetInstall%" %cecho% error "Cannot download Wget installation" & goto error

	echo Copy Wget
	copy "%EnvDownloadPath%\wget.exe" "%EnvToolsPath%"
)

if not exist "%EnvToolsPath%\7z.exe" (
	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
	mkdir "%EnvTempPath%"

	echo Download 7z installation

	if not exist "%EnvDownloadPath%\%SevenZipInstall%" call "%ToolsPath%\download-file.bat" "%SevenZipUrl%" "%EnvDownloadPath%\%SevenZipInstall%"
	if not exist "%EnvDownloadPath%\%SevenZipInstall%" echo Cannot download 7z installation& goto error

	echo Unpack 7z
	msiexec /a "%EnvDownloadPath%\%SevenZipInstall%" /qb TARGETDIR="%EnvTempPath%"
	copy "%EnvTempPath%\Files\7-Zip\7z.dll" "%EnvToolsPath%"
	copy "%EnvTempPath%\Files\7-Zip\7z.exe" "%EnvToolsPath%"

	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
)

if not exist "%EnvToolsPath%\cecho.exe" (
	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
	mkdir "%EnvTempPath%"

	echo Download cecho installation

	if not exist "%EnvDownloadPath%\%CEchoInstall%" call "%ToolsPath%\download-file.bat" "%CEchoUrl%" "%EnvDownloadPath%\%CEchoInstall%"
	if not exist "%EnvDownloadPath%\%cCEhoInstall%" echo Cannot download cecho installation& goto error

	echo Unpack cecho
	"%EnvSevenZipExe%" x -o"%EnvTempPath%" "%EnvDownloadPath%\%CEchoInstall%"
	copy "%EnvTempPath%\cecho.exe" "%EnvToolsPath%"

	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
)

if not exist "%EnvToolsPath%\sigcheck.exe" (
	%cecho% info "Download Sigcheck installation"

	if not exist "%EnvDownloadPath%\%SigcheckInstall%" call "%ToolsPath%\download-file.bat" "%SigcheckUrl%" "%EnvDownloadPath%\%SigcheckInstall%"
	if not exist "%EnvDownloadPath%\%SigcheckInstall%" %cecho% error "Cannot download Sigcheck installation" & goto error

	%cecho% info "Unpack Sigcheck"
	"%EnvSevenZipExe%" x -o"%EnvToolsPath%" "%EnvDownloadPath%\%SigcheckInstall%" sigcheck.exe
)

:exit
endlocal
exit /B 0

:error
endlocal
exit /B 1
