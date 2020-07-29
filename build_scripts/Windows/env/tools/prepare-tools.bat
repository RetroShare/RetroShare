setlocal

if "%EnvRootPath%"=="" exit /B 1

set CEchoUrl=https://github.com/lordmulder/cecho/releases/download/2015-10-10/cecho.2015-10-10.zip
set CEchoInstall=cecho.2015-10-10.zip
set SevenZipUrl=https://sourceforge.net/projects/sevenzip/files/7-Zip/19.00/7z1900.msi/download
set SevenZipInstall=7z1900.msi
::set CurlUrl=https://bintray.com/artifact/download/vszakats/generic/curl-7.50.1-win32-mingw.7z
::set CurlInstall=curl-7.50.1-win32-mingw.7z
set WgetUrl=https://eternallybored.org/misc/wget/1.19.4/32/wget.exe
set WgetInstall=wget.exe
set JomUrl=http://download.qt.io/official_releases/jom/jom.zip
set JomInstall=jom.zip
set DependsUrl=http://www.dependencywalker.com/depends22_x86.zip
set DependsInstall=depends22_x86.zip
set UnixToolsUrl=http://unxutils.sourceforge.net/UnxUpdates.zip
set UnixToolsInstall=UnxUpdates.zip
set NSISUrl=http://prdownloads.sourceforge.net/nsis/nsis-3.0-setup.exe?download
set NSISInstall=nsis-3.0-setup.exe
set NSISInstallPath=%EnvToolsPath%\NSIS
set MinGitInstall=MinGit-2.28.0-32-bit.zip
set MinGitUrl=https://github.com/git-for-windows/git/releases/download/v2.28.0.windows.1/%MinGitInstall%
set MinGitInstallPath=%EnvToolsPath%\MinGit
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

::if not exist "%EnvToolsPath%\curl.exe" (
::	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
::	mkdir "%EnvTempPath%"
::
::	echo Download Curl installation
::
::	if not exist "%EnvDownloadPath%\%CurlInstall%" call "%ToolsPath%\winhttpjs.bat" %CurlUrl% -saveTo "%EnvDownloadPath%\%CurlInstall%"
::	if not exist "%EnvDownloadPath%\%CurlInstall%" echo Cannot download Curl installation& goto error
::
::	echo Unpack Curl
::	"%EnvSevenZipExe%" x -o"%EnvTempPath%" "%EnvDownloadPath%\%CurlInstall%"
::	copy "%EnvTempPath%\curl-7.50.1-win32-mingw\bin\curl.exe" "%EnvToolsPath%"
::
::	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
::)

if not exist "%EnvToolsPath%\jom.exe" (
	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
	mkdir "%EnvTempPath%"

	%cecho% info "Download jom installation"

	if not exist "%EnvDownloadPath%\%JomInstall%" call "%ToolsPath%\winhttpjs.bat" %JomUrl% -saveTo "%EnvDownloadPath%\%JomInstall%"
	if not exist "%EnvDownloadPath%\%JomInstall%" %cecho% error "Cannot download jom installation" & goto error

	%cecho% info "Unpack jom"
	"%EnvSevenZipExe%" x -o"%EnvTempPath%" "%EnvDownloadPath%\%JomInstall%"
	copy "%EnvTempPath%\jom.exe" "%EnvToolsPath%"

	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
)

if not exist "%EnvToolsPath%\depends.exe" (
	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
	mkdir "%EnvTempPath%"

	%cecho% info "Download Dependency Walker installation"

	if not exist "%EnvDownloadPath%\%DependsInstall%" call "%ToolsPath%\winhttpjs.bat" %DependsUrl% -saveTo "%EnvDownloadPath%\%DependsInstall%"
	if not exist "%EnvDownloadPath%\%DependsInstall%" %cecho% error "Cannot download Dependendy Walker installation" & goto error

	%cecho% info "Unpack Dependency Walker"
	"%EnvSevenZipExe%" x -o"%EnvTempPath%" "%EnvDownloadPath%\%DependsInstall%"
	copy "%EnvTempPath%\*" "%EnvToolsPath%"

	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
)

if not exist "%EnvToolsPath%\cut.exe" (
	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
	mkdir "%EnvTempPath%"

	%cecho% info "Download Unix Tools installation"

	if not exist "%EnvDownloadPath%\%UnixToolsInstall%" call "%ToolsPath%\winhttpjs.bat" %UnixToolsUrl% -saveTo "%EnvDownloadPath%\%UnixToolsInstall%"
	if not exist "%EnvDownloadPath%\%UnixToolsInstall%" %cecho% error "Cannot download Unix Tools installation" & goto error

	%cecho% info "Unpack Unix Tools"
	"%EnvSevenZipExe%" x -o"%EnvTempPath%" "%EnvDownloadPath%\%UnixToolsInstall%"
	copy "%EnvTempPath%\cut.exe" "%EnvToolsPath%"

	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
)

if not exist "%EnvToolsPath%\sed.exe" (
	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
	mkdir "%EnvTempPath%"

	%cecho% info "Download Unix Tools installation"

	if not exist "%EnvDownloadPath%\%UnixToolsInstall%" call "%ToolsPath%\winhttpjs.bat" %UnixToolsUrl% -saveTo "%EnvDownloadPath%\%UnixToolsInstall%"
	if not exist "%EnvDownloadPath%\%UnixToolsInstall%" %cecho% error "Cannot download Unix Tools installation" & goto error

	%cecho% info "Unpack Unix Tools"
	"%EnvSevenZipExe%" x -o"%EnvTempPath%" "%EnvDownloadPath%\%UnixToolsInstall%"
	copy "%EnvTempPath%\sed.exe" "%EnvToolsPath%"

	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
)

if not exist "%NSISInstallPath%\nsis.exe" (
	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
	mkdir "%EnvTempPath%"

	%cecho% info "Download NSIS installation"

	if not exist "%EnvDownloadPath%\%NSISInstall%" call "%ToolsPath%\download-file.bat" "%NSISUrl%" "%EnvDownloadPath%\%NSISInstall%"
	if not exist "%EnvDownloadPath%\%NSISInstall%" %cecho% error "Cannot download NSIS installation" & goto error

	%cecho% info "Unpack NSIS"
	"%EnvSevenZipExe%" x -o"%EnvTempPath%" "%EnvDownloadPath%\%NSISInstall%"
	if not exist "%NSISInstallPath%" mkdir "%NSISInstallPath%"
	xcopy /s "%EnvTempPath%" "%NSISInstallPath%"

	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
)

if not exist "%MinGitInstallPath%\cmd\git.exe" (
	%cecho% info "Download MinGit installation"

	if not exist "%EnvDownloadPath%\%MinGitInstall%" call "%ToolsPath%\download-file.bat" "%MinGitUrl%" "%EnvDownloadPath%\%MinGitInstall%"
	if not exist "%EnvDownloadPath%\%MinGitInstall%" %cecho% error "Cannot download MinGit installation" & goto error

	%cecho% info "Unpack MinGit"
	"%EnvSevenZipExe%" x -o"%MinGitInstallPath%" "%EnvDownloadPath%\%MinGitInstall%"
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
