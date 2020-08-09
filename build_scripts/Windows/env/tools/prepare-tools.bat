setlocal

if "%EnvRootPath%"=="" exit /B 1

set CEchoUrl=https://github.com/lordmulder/cecho/releases/download/2015-10-10/cecho.2015-10-10.zip
set CEchoInstall=cecho.2015-10-10.zip
set SevenZipUrl=https://sourceforge.net/projects/sevenzip/files/7-Zip/19.00/7z1900.msi/download
set SevenZipInstall=7z1900.msi
set JomUrl=http://download.qt.io/official_releases/jom/jom.zip
set JomInstall=jom.zip
set DependsUrl=http://www.dependencywalker.com/depends22_x86.zip
set DependsInstall=depends22_x86.zip
set UnixToolsUrl=http://unxutils.sourceforge.net/UnxUpdates.zip
set UnixToolsInstall=UnxUpdates.zip
set NSISInstall=nsis-3.05-setup.exe
set NSISUrl=http://prdownloads.sourceforge.net/nsis/%NSISInstall%?download
set NSISInstallPath=%EnvToolsPath%\NSIS
set MinGitInstall=MinGit-2.28.0-32-bit.zip
set MinGitUrl=https://github.com/git-for-windows/git/releases/download/v2.28.0.windows.1/%MinGitInstall%
set MinGitInstallPath=%EnvToolsPath%\MinGit
set CMakeVersion=cmake-3.1.0-win32-x86
set CMakeInstall=%CMakeVersion%.zip
set CMakeUrl=http://www.cmake.org/files/v3.1/%CMakeInstall%
set CMakeInstallPath=%EnvToolsPath%\cmake
set TorProjectUrl=https://www.torproject.org
set TorDownloadIndexUrl=%TorProjectUrl%/download/tor

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

if not exist "%EnvToolsPath%\jom.exe" (
	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
	mkdir "%EnvTempPath%"

	%cecho% info "Download jom installation"

	if not exist "%EnvDownloadPath%\%JomInstall%" call "%ToolsPath%\download-file.bat" %JomUrl% "%EnvDownloadPath%\%JomInstall%"
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

	if not exist "%EnvDownloadPath%\%DependsInstall%" call "%ToolsPath%\download-file.bat" %DependsUrl% "%EnvDownloadPath%\%DependsInstall%"
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

	if not exist "%EnvDownloadPath%\%UnixToolsInstall%" call "%ToolsPath%\download-file.bat" %UnixToolsUrl% "%EnvDownloadPath%\%UnixToolsInstall%"
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

	if not exist "%EnvDownloadPath%\%UnixToolsInstall%" call "%ToolsPath%\download-file.bat" %UnixToolsUrl% "%EnvDownloadPath%\%UnixToolsInstall%"
	if not exist "%EnvDownloadPath%\%UnixToolsInstall%" %cecho% error "Cannot download Unix Tools installation" & goto error

	%cecho% info "Unpack Unix Tools"
	"%EnvSevenZipExe%" x -o"%EnvTempPath%" "%EnvDownloadPath%\%UnixToolsInstall%"
	copy "%EnvTempPath%\sed.exe" "%EnvToolsPath%"

	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
)

if not exist "%EnvDownloadPath%\%NSISInstall%" call "%ToolsPath%\remove-dir.bat" "%NSISInstallPath%"
if not exist "%NSISInstallPath%\nsis.exe" (
	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"

	if exist "%NSISInstallPath%" call "%ToolsPath%\remove-dir.bat" "%NSISInstallPath%"

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

if not exist "%EnvDownloadPath%\%CMakeInstall%" call "%ToolsPath%\remove-dir.bat" "%CMakeInstallPath%"
if not exist "%CMakeInstallPath%\bin\cmake.exe" (
	%cecho% info "Download CMake installation"

	if exist "%CMakeInstallPath%" call "%ToolsPath%\remove-dir.bat" "%CMakeInstallPath%"

	mkdir "%EnvTempPath%"

	if not exist "%EnvDownloadPath%\%CMakeInstall%" call "%ToolsPath%\download-file.bat" "%CMakeUrl%" "%EnvDownloadPath%\%CMakeInstall%"
	if not exist "%EnvDownloadPath%\%CMakeInstall%" %cecho% error "Cannot download CMake installation" & goto error

	%cecho% info "Unpack CMake"
	"%EnvSevenZipExe%" x -o"%EnvTempPath%" "%EnvDownloadPath%\%CMakeInstall%"

	move "%EnvTempPath%\%CMakeVersion%" "%CMakeInstallPath%"

	call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
)

rem Tor
rem Get download link and filename from download page
mkdir "%EnvTempPath%"
call "%ToolsPath%\download-file.bat" "%TorDownloadIndexUrl%" "%EnvTempPath%\index.html"
if not exist "%EnvTempPath%\index.html" %cecho% error "Cannot download Tor installation" & goto error

for /F "tokens=1,2 delims= " %%A in ('%EnvSedExe% -r -n -e"s/.*href=\"^(.*^)^(tor-win32.*\.zip^)\".*/\2 \1\2/p" "%EnvTempPath%\index.html"') do set TorInstall=%%A& set TorDownloadUrl=%TorProjectUrl%%%B
call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
if "%TorInstall%"=="" %cecho% error "Cannot download Tor installation" & goto error
if "%TorDownloadUrl%"=="" %cecho% error "Cannot download Tor installation" & goto error

if not exist "%EnvDownloadPath%\%TorInstall%" call "%ToolsPath%\remove-dir.bat" "%EnvTorPath%"
if not exist "%EnvTorPath%\Tor\tor.exe" (
	%cecho% info "Download Tor installation"

	if not exist "%EnvDownloadPath%\%TorInstall%" call "%ToolsPath%\download-file.bat" "%TorDownloadUrl%" "%EnvDownloadPath%\%TorInstall%"
	if not exist "%EnvDownloadPath%\%TorInstall%" %cecho% error "Cannot download Tor installation" & goto error

	%cecho% info "Unpack Tor"
	"%EnvSevenZipExe%" x -o"%EnvTorPath%" "%EnvDownloadPath%\%TorInstall%"
)

:exit
endlocal
exit /B 0

:error
call "%ToolsPath%\remove-dir.bat" "%EnvTempPath%"
endlocal
exit /B 1
