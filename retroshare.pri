# To disable RetroShare-gui append the following
# assignation to qmake command line "CONFIG+=no_retroshare_gui"
CONFIG *= retroshare_gui
no_retroshare_gui:CONFIG -= retroshare_gui

# To disable RetroShare-nogui append the following
# assignation to qmake command line "CONFIG+=no_retroshare_nogui"
CONFIG *= retroshare_nogui
no_retroshare_nogui:CONFIG -= retroshare_nogui

# To disable RetroShare plugins append the following
# assignation to qmake command line "CONFIG+=no_retroshare_plugins"
CONFIG *= retroshare_plugins
no_retroshare_plugins:CONFIG -= retroshare_plugins

# To enable RetroShare-android-service append the following assignation to
# qmake command line "CONFIG+=retroshare_android_service"
CONFIG *= no_retroshare_android_service
retroshare_android_service:CONFIG -= no_retroshare_android_service

# To enable RetroShare-android-notify-service append the following
# assignation to qmake command line
# "CONFIG+=retroshare_android_notify_service"
CONFIG *= no_retroshare_android_notify_service
retroshare_android_notify_service:CONFIG -= no_retroshare_android_notify_service

# To enable RetroShare-QML-app append the following assignation to
# qmake command line "CONFIG+=retroshare_qml_app"
CONFIG *= no_retroshare_qml_app
retroshare_qml_app:CONFIG -= no_retroshare_qml_app

# To enable libresapi via local socket (unix domain socket or windows named
# pipes) append the following assignation to qmake command line
#"CONFIG+=libresapilocalserver"
CONFIG *= no_libresapilocalserver
libresapilocalserver:CONFIG -= no_libresapilocalserver

# To enable Qt dependencies in libresapi append the following
# assignation to qmake command line "CONFIG+=qt_dependencies"
CONFIG *= no_qt_dependencies
qt_dependencies:CONFIG -= no_qt_dependencies

# To disable libresapi via HTTP (based on libmicrohttpd) append the following
# assignation to qmake command line "CONFIG+=no_libresapihttpserver"
CONFIG *= libresapihttpserver
no_libresapihttpserver:CONFIG -= libresapihttpserver

# To disable SQLCipher support append the following assignation to qmake
# command line "CONFIG+=no_sqlcipher"
CONFIG *= sqlcipher
no_sqlcipher:CONFIG -= sqlcipher

# To enable autologin (this is higly discouraged as it may compromise your node
# security in multiple ways) append the following assignation to qmake command
# line "CONFIG+=rs_autologin"
CONFIG *= no_rs_autologin
rs_autologin:CONFIG -= no_rs_autologin

# To disable GXS (General eXchange System) append the following
# assignation to qmake command line "CONFIG+=no_rs_gxs"
CONFIG *= rs_gxs
no_rs_gxs:CONFIG -= rs_gxs

# To enable RS Deprecated Warnings append the following assignation to qmake
# command line "CONFIG+=rs_deprecatedwarning"
CONFIG *= no_rs_deprecatedwarning
rs_deprecatedwarning:CONFIG -= no_rs_deprecatedwarning

# To enable CPP #warning append the following assignation to qmake command
# line "CONFIG+=rs_cppwarning"
CONFIG *= no_rs_cppwarning
rs_cppwarning:CONFIG -= no_rs_cppwarning

# To disable GXS mail append the following assignation to qmake command line
# "CONFIG+=no_rs_gxs_trans"
CONFIG *= rs_gxs_trans
#no_rs_gxs_trans:CONFIG -= rs_gxs_trans ## Disabing not supported ATM

# To enable GXS based async chat append the following assignation to qmake
# command line "CONFIG+=rs_async_chat"
CONFIG *= no_rs_async_chat
rs_async_chat:CONFIG -= no_rs_async_chat

# To select your MacOsX version append the following assignation to qmake
# command line "CONFIG+=rs_macos10.11" where 10.11(default for Travis_CI) depends your version
CONFIG *= rs_macos10.11
rs_macos10.8:CONFIG -= rs_macos10.11
rs_macos10.9:CONFIG -= rs_macos10.11
rs_macos10.10:CONFIG -= rs_macos10.11
rs_macos10.12:CONFIG -= rs_macos10.11


linux-* {
	isEmpty(PREFIX)   { PREFIX   = "/usr" }
	isEmpty(BIN_DIR)  { BIN_DIR  = "$${PREFIX}/bin" }
	isEmpty(INC_DIR)  { INC_DIR  = "$${PREFIX}/include/retroshare" }
	isEmpty(LIB_DIR)  { LIB_DIR  = "$${PREFIX}/lib" }
	isEmpty(DATA_DIR) { DATA_DIR = "$${PREFIX}/share/retroshare" }
	isEmpty(PLUGIN_DIR) { PLUGIN_DIR = "$${LIB_DIR}/retroshare/extensions6" }

    rs_autologin {
        !macx {
            DEFINES *= HAS_GNOME_KEYRING
            PKGCONFIG *= gnome-keyring-1
        }
    }
}

android-g++ {
    isEmpty(NATIVE_LIBS_TOOLCHAIN_PATH) {
        NATIVE_LIBS_TOOLCHAIN_PATH = $$(NATIVE_LIBS_TOOLCHAIN_PATH)
    }
    retroshare_qml_app {
        CONFIG -= no_retroshare_android_notify_service
        CONFIG *= retroshare_android_notify_service
    }
    CONFIG *= no_libresapihttpserver no_sqlcipher upnp_libupnp
    CONFIG -= libresapihttpserver sqlcipher upnp_miniupnpc
    QT *= androidextras
    DEFINES *= "fopen64=fopen"
    DEFINES *= "fseeko64=fseeko"
    DEFINES *= "ftello64=ftello"
    INCLUDEPATH += $$NATIVE_LIBS_TOOLCHAIN_PATH/sysroot/usr/include
    LIBS *= -L$$NDK_TOOLCHAIN_PATH/sysroot/usr/lib/
    LIBS *= -lbz2 -lupnp -lixml -lthreadutil -lsqlite3
    ANDROID_EXTRA_LIBS *= $$NATIVE_LIBS_TOOLCHAIN_PATH/sysroot/usr/lib/libsqlite3.so
#    message(LIBS: $$LIBS)
#    message(ANDROID_EXTRA_LIBS: $$ANDROID_EXTRA_LIBS)
#    message(ANDROID_PLATFORM: $$ANDROID_PLATFORM)
#    message(ANDROID_PLATFORM_ROOT_PATH: $$ANDROID_PLATFORM_ROOT_PATH)
#    message(NDK_TOOLCHAIN_PATH: $$NDK_TOOLCHAIN_PATH)
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

	# Check for msys2
	PREFIX_MSYS2 = $$(MINGW_PREFIX)
	isEmpty(PREFIX_MSYS2) {
		exists(C:/msys32/mingw32/include) {
			message(MINGW_PREFIX is empty. Set it in your environment variables.)
			message(Found it here:C:\msys32\mingw32)
			PREFIX_MSYS2 = "C:\msys32\mingw32"
		}
		exists(C:/msys64/mingw32/include) {
			message(MINGW_PREFIX is empty. Set it in your environment variables.)
			message(Found it here:C:\msys64\mingw32)
			PREFIX_MSYS2 = "C:\msys64\mingw32"
		}
	}
	!isEmpty(PREFIX_MSYS2) {
		message(msys2 is installed.)
		BIN_DIR  += "$${PREFIX_MSYS2}/bin"
		INC_DIR  += "$${PREFIX_MSYS2}/include"
		LIB_DIR  += "$${PREFIX_MSYS2}/lib"
	}
}

rs_macos10.8 {
	message(***retroshare.pri: Set Target and SDK to MacOS 10.8 )
	QMAKE_MACOSX_DEPLOYMENT_TARGET=10.8
	QMAKE_MAC_SDK = macosx10.8
}

rs_macos10.9 {
	message(***retroshare.pri: Set Target and SDK to MacOS 10.9 )
	QMAKE_MACOSX_DEPLOYMENT_TARGET=10.9
	QMAKE_MAC_SDK = macosx10.9
}

rs_macos10.10 {
	message(***retroshare.pri: Set Target and SDK to MacOS 10.10 )
	QMAKE_MACOSX_DEPLOYMENT_TARGET=10.10
	QMAKE_MAC_SDK = macosx10.10
}

rs_macos10.11 {
	message(***retroshare.pri: Set Target and SDK to MacOS 10.11 )
	QMAKE_MACOSX_DEPLOYMENT_TARGET=10.11
	QMAKE_MAC_SDK = macosx10.11
}

rs_macos10.12 {
	message(***retroshare.pri: Set Target and SDK to MacOS 10.12 )
	QMAKE_MACOSX_DEPLOYMENT_TARGET=10.12
	QMAKE_MAC_SDK = macosx10.12
}

macx {
	message(***retroshare.pri:MacOSX)
	BIN_DIR += "/usr/bin"
	INC_DIR += "/usr/include"
	INC_DIR += "/usr/local/include"
	INC_DIR += "/opt/local/include"
	LIB_DIR += "/usr/local/lib"
	LIB_DIR += "/opt/local/lib"
	CONFIG += c++11
}

unfinished {
	CONFIG += gxscircles
	CONFIG += gxsthewire
	CONFIG += gxsphotoshare
	CONFIG += wikipoos
}

wikipoos:DEFINES *= RS_USE_WIKI
rs_gxs:DEFINES *= RS_ENABLE_GXS
libresapilocalserver:DEFINES *= LIBRESAPI_LOCAL_SERVER
qt_dependencies:DEFINES *= LIBRESAPI_QT
libresapihttpserver:DEFINES *= ENABLE_WEBUI
sqlcipher:DEFINES -= NO_SQLCIPHER
no_sqlcipher:DEFINES *= NO_SQLCIPHER
rs_autologin {
    DEFINES *= RS_AUTOLOGIN
    warning("You have enabled RetroShare auto-login, this is discouraged. The usage of auto-login on some linux distributions may allow someone having access to your session to steal the SSL keys of your node location and therefore compromise your security")
}

no_rs_deprecatedwarning {
    QMAKE_CXXFLAGS += -Wno-deprecated
    QMAKE_CXXFLAGS += -Wno-deprecated-declarations
    DEFINES *= RS_NO_WARN_DEPRECATED
    warning("QMAKE: You have disabled deprecated warnings.")
}

no_rs_cppwarning {
    QMAKE_CXXFLAGS += -Wno-cpp
    DEFINES *= RS_NO_WARN_CPP
    warning("QMAKE: You have disabled C preprocessor warnings.")
}

rs_gxs_trans {
    DEFINES *= RS_GXS_TRANS
    greaterThan(QT_MAJOR_VERSION, 4) {
        CONFIG += c++11
    } else {
        QMAKE_CXXFLAGS += -std=c++0x
    }
}

rs_async_chat {
    DEFINES *= RS_ASYNC_CHAT
}

rs_chatserver {
    DEFINES *= RS_CHATSERVER
}
