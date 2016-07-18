# To {dis,en}able libresapi via local socket (unix domain socket or windows named pipes)
# {,un}comment the following line
#CONFIG *= libresapilocalserver

# Gxs is always enabled now.
DEFINES *= RS_ENABLE_GXS

unix {
	isEmpty(PREFIX)   { PREFIX   = "/usr" }
	isEmpty(BIN_DIR)  { BIN_DIR  = "$${PREFIX}/bin" }
	isEmpty(INC_DIR)  { INC_DIR  = "$${PREFIX}/include/retroshare06" }
	isEmpty(LIB_DIR)  { LIB_DIR  = "$${PREFIX}/lib" }
	isEmpty(DATA_DIR) { DATA_DIR = "$${PREFIX}/share/RetroShare06" }
	isEmpty(PLUGIN_DIR) { PLUGIN_DIR  = "$${LIB_DIR}/retroshare/extensions6" }
}

win32 {
	message(***retroshare.pri:Win32)
	exists($$PWD/../libs) {
		message(Get pre-compiled libraries.)
		isEmpty(PREFIX)   { PREFIX   = "$$PWD/../libs" }
		isEmpty(BIN_DIR)  { BIN_DIR  = "$${PREFIX}/bin" }
		isEmpty(INC_DIR)  { INC_DIR  = "$${PREFIX}/include" }
		isEmpty(LIB_DIR)  { LIB_DIR  = "$${PREFIX}/lib" }
	}
	exists(C:/msys32/mingw32/include) {
		message(msys2 32b is installed.)
		PREFIX_MSYS2   = "C:/msys32/mingw32"
		BIN_DIR  += "$${PREFIX_MSYS2}/bin"
		INC_DIR  += "$${PREFIX_MSYS2}/include"
		LIB_DIR  += "$${PREFIX_MSYS2}/lib"
	}
	exists(C:/msys64/mingw32/include) {
		message(msys2 64b is installed.)
		PREFIX_MSYS2   = "C:/msys64/mingw32"
		BIN_DIR  += "$${PREFIX_MSYS2}/bin"
		INC_DIR  += "$${PREFIX_MSYS2}/include"
		LIB_DIR  += "$${PREFIX_MSYS2}/lib"
	}
}

macx {
	message(***retroshare.pri:MacOSX)
	BIN_DIR += "/usr/bin"
	INC_DIR += "/usr/include"
	INC_DIR += "/usr/local/include"
	INC_DIR += "/opt/local/include"
	LIB_DIR += "/usr/local/lib"
	LIB_DIR += "/opt/local/lib"
	!QMAKE_MACOSX_DEPLOYMENT_TARGET {
		message(***retroshare.pri: No Target, set it to MacOS 10.10 )
		QMAKE_MACOSX_DEPLOYMENT_TARGET=10.10
	}
	!QMAKE_MAC_SDK {
		message(***retroshare.pri: No SDK, set it to MacOS 10.10 )
		QMAKE_MAC_SDK = macosx10.10
	}
	CONFIG += c+11
}

unfinished {
	CONFIG += gxscircles
	CONFIG += gxsthewire
	CONFIG += gxsphotoshare
	CONFIG += wikipoos
}

wikipoos:DEFINES *= RS_USE_WIKI

libresapilocalserver:DEFINES *= LIBRESAPI_LOCAL_SERVER
