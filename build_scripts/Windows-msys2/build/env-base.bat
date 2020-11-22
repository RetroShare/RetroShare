:: Process commandline parameter
set Param32=0
set Param64=0
set ParamRelease=0
set ParamDebug=0
set ParamAutologin=0
set ParamPlugins=0
set ParamTor=0
set ParamWebui=0
set ParamClang=0
set ParamIndexing=0
set ParamNoupdate=0
set CoreCount=%NUMBER_OF_PROCESSORS%
set RS_QMAKE_CONFIG=

:parameter_loop
if "%~1" NEQ "" (
	for /f "tokens=1,2 delims==" %%a in ("%~1") do (
		if "%%~a"=="32" (
			set Param32=1
		) else if "%%~a"=="64" (
			set Param64=1
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
		) else if "%%~a"=="clang" (
			set ParamClang=1
		) else if "%%~a"=="indexing" (
			set ParamIndexing=1
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

if "%Param32%"=="1" (
	if "%Param64%"=="1" (
		echo.
		echo 32-bit or 64-bit?
		goto :usage
	)

	set RsBit=32
	set RsArchitecture=x86
	set RsMSYS2Architecture=i686
)

if "%Param64%"=="1" (
	set RsBit=64
	set RsArchitecture=x64
	set RsMSYS2Architecture=x86_64
)

if "%ParamClang%"=="1" (
	set RsCompiler=Clang
) else (
	set RsCompiler=GCC
)

if "%RsBit%"=="" goto :usage

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

exit /B 0

:usage
echo.
echo Usage: 32^|64 release^|debug [autologin plugins webui singlethread clang indexing noupdate] ["CONFIG+=..."]
echo.
echo Mandatory parameter
echo 32^|64              32-bit or 64-bit Version
echo release^|debug      Build release or debug version
echo.
echo Optional parameter (need clean when changed)
echo autologin          Build with autologin
echo plugins            Build plugins
echo webui              Enable JsonAPI and pack webui files
echo singlethread       Use only 1 thread for building
echo clang              Use clang compiler instead of GCC
echo indexing           Build with deep channel and file indexing support
echo noupdate           Skip updating the libraries
echo "CONFIG+=..."      Enable some extra features, you can find the almost complete list in retroshare.pri
echo.
echo Parameter for pack
echo tor                Pack tor version
echo.
exit /B 2
