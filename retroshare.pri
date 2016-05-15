# Gxs is always enabled now.
DEFINES *= RS_ENABLE_GXS
OBJECTS_DIR = obj
#CONFIG -= debug

SECURITY_FLAGS += #-fstack-protector-strong #activated by default on modern gcc and clang. However some compilers can't handle it.
OPTIMIZER_FLAGS += -O2

QMAKE_CXXFLAGS *= -std=c++11

QMAKE_LFLAGS_RELEASE   *= $$SECURITY_FLAGS $$OPTIMIZER_FLAGS
QMAKE_CFLAGS_RELEASE   *= $$SECURITY_FLAGS $$OPTIMIZER_FLAGS
QMAKE_CXXFLAGS_RELEASE *= $$SECURITY_FLAGS $$OPTIMIZER_FLAGS

PRETTY_TRACE += -O1 -fno-omit-frame-pointer -fno-optimize-sibling-calls
DEBUG_FLAGS  += -g $$PRETTY_TRACE
QMAKE_CFLAGS_DEBUG  *= $$DEBUG_FLAGS
QMAKE_CXXFLAGS_DEBUG *= $$DEBUG_FLAGS


profiling {
        QMAKE_CXXFLAGS *= -pg -g $$OPTIMIZER_FLAGS
}

#Activates link time optimization. Might cause build failures if your toolchain doesn't support it.
#CONFIG += lto
*clang*{
    lto{
        OBJECTS_DIR = llvm-bc
        QMAKE_LFLAGS   *= -flto
        QMAKE_CFLAGS   *= -emit-llvm -flto
        QMAKE_CXXFLAGS *= -emit-llvm -flto
        #SECURITY_FLAGS += -fsanitize=cfi
    }
    SECURITY_FLAGS += -D_FORTIFY_SOURCE
    #SECURITY_FLAGS += -fsanitize=safe-stack

    QMAKE_LFLAGS_RELEASE   *= $$SECURITY_FLAGS
    QMAKE_CFLAGS_RELEASE   *= $$SECURITY_FLAGS
    QMAKE_CXXFLAGS_RELEASE *= $$SECURITY_FLAGS

    #DEBUG_FLAGS += -fsanitize=undefined

    #Choose at most one of the 3 following sanitizers
    #DEBUG_FLAGS += -fsanitize=address $$PRETTY_TRACE
    #DEBUG_FLAGS += -fsanitize=memory -fsanitize-memory-track-origins=2
    #DEBUG_FLAGS += -fsanitize=thread

    QMAKE_CFLAGS_DEBUG  *= $$DEBUG_FLAGS
    QMAKE_CXXFLAGS_DEBUG *= $$DEBUG_FLAGS
}

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
