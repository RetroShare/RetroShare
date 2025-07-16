:: Process commandline parameter
set ParamRelease=0
set ParamDebug=0
set ParamAutologin=0
set ParamPlugins=0
set ParamTor=0
set ParamWebui=0
set ClangCompiler=0
set ParamIndexing=0
set ParamFriendserver=0
set ParamNoupdate=0
set CoreCount=%NUMBER_OF_PROCESSORS%
set RS_QMAKE_CONFIG=
set RsToolchain=
set tcc=0

:parameter_loop
if "%~1" NEQ "" (
	for /f "tokens=1,2 delims==" %%a in ("%~1") do (
		if "%%~a"=="32" (
			set RsToolchain=mingw32
			set /A tcc=tcc+1
		) else if "%%~a"=="64" (
			set RsToolchain=mingw64
			set /A tcc=tcc+1
		) else if "%%~a"=="mingw32" (
			set RsToolchain=mingw32
			set /A tcc=tcc+1
		) else if "%%~a"=="mingw64" (
			set RsToolchain=mingw64
			set /A tcc=tcc+1
		) else if "%%~a"=="ucrt64" (
			set RsToolchain=ucrt64
			set /A tcc=tcc+1
		) else if "%%~a"=="clang64" (
			set RsToolchain=clang64
			set /A tcc=tcc+1
		) else if "%%~a"=="clang32" (
			set RsToolchain=clang32
			set /A tcc=tcc+1
		) else if "%%~a"=="clangarm64" (
			set RsToolchain=clangarm64
			set /A tcc=tcc+1
		) else if "%%~a"=="release" (
			set ParamRelease=1
		) else if "%%~a"=="debug" (
			set ParamDebug=1
		) else if "%%~a"=="autologin" (
			set ParamAutologin=1
		) else if "%%~a"=="plugins" (
			set ParamPlugins=1
		) else if "%%~a"=="tor" (
			set ParamTor=1
		) else if "%%~a"=="webui" (
			set ParamWebui=1
		) else if "%%~a"=="singlethread" (
			set CoreCount=1
		) else if "%%~a"=="indexing" (
			set ParamIndexing=1
		) else if "%%~a"=="friendserver" (
			set ParamFriendserver=1
		) else if "%%~a"=="noupdate" (
			set ParamNoupdate=1
		) else if "%%~a"=="CONFIG+" (
			set RS_QMAKE_CONFIG=%RS_QMAKE_CONFIG% %1
		) else (
			echo.
			echo Unknown parameter %1
			goto :usage
		)
	)
	shift /1
	goto parameter_loop
)

if %tcc% NEQ 1 (
	echo Multiple or no toolchain specified
	goto :usage
)

if "%RsToolchain%"=="mingw32" (
	set RsArchitecture=x86
	set RsMSYS2Architecture=i686
	set MSYSTEM=MINGW32
) else if "%RsToolchain%"=="mingw64" (
	set RsArchitecture=x64
	set RsMSYS2Architecture=x86_64
	set MSYSTEM=MINGW64
) else if "%RsToolchain%"=="ucrt64" (
	set RsArchitecture=x64
	set RsMSYS2Architecture=ucrt-x86_64
	set MSYSTEM=UCRT64
) else if "%RsToolchain%"=="clang64" (
	set RsArchitecture=x64
	set RsMSYS2Architecture=clang-x86_64
	set MSYSTEM=CLANG64
	set ClangCompiler=1
) else if "%RsToolchain%"=="clang32" (
	set RsArchitecture=x86
	set RsMSYS2Architecture=clang-i686
	set MSYSTEM=CLANG32
	set ClangCompiler=1
) else if "%RsToolchain%"=="clangarm64" (
	set RsArchitecture=arm64
	set RsMSYS2Architecture=clang-aarch64
	set MSYSTEM=CLANGARM64
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

if "%ParamWebui%"=="1" (
	set RS_QMAKE_CONFIG=%RS_QMAKE_CONFIG% "CONFIG+=rs_jsonapi" "CONFIG+=rs_webui"
)

if "%ParamIndexing%"=="1" (
	set RS_QMAKE_CONFIG=%RS_QMAKE_CONFIG% "CONFIG+=rs_deep_channels_index" "CONFIG+=rs_deep_files_index" "CONFIG+=rs_deep_files_index_ogg" "CONFIG+=rs_deep_files_index_flac" "CONFIG+=rs_deep_files_index_taglib"
)

if "%ParamFriendserver%"=="1" (
	set RS_QMAKE_CONFIG=%RS_QMAKE_CONFIG% "CONFIG+=rs_efs"
)

exit /B 0

:usage
echo.
echo Usage: 32^|64^|other release^|debug [autologin plugins webui singlethread clang indexing friendserver noupdate] ["CONFIG+=..."]
echo.
echo Mandatory parameter
echo 32^|64              32-bit or 64-bit version (same as mingw32 or mingw64)
echo                    Or you can specify any other toolchain supported by msys2:
echo                         mingw32^|mingw64^|clang32^|clang64^|ucrt64^|clangarm64
echo                         More info: https://www.msys2.org/docs/environments
echo release^|debug      Build release or debug version
echo.
echo Optional parameter (need clean when changed)
echo autologin          Build with autologin
echo plugins            Build plugins
echo webui              Enable JsonAPI and pack webui files
echo singlethread       Use only 1 thread for building
echo indexing           Build with deep channel and file indexing support
echo friendserver       Enable friendserver support
echo noupdate           Skip updating the libraries
echo "CONFIG+=..."      Enable some extra features, you can find the almost complete list in retroshare.pri
echo.
echo Parameter for pack
echo tor                Pack tor version
echo.
exit /B 2
