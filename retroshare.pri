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

# To enable RetroShare-QML-app append the following assignation to
# qmake command line "CONFIG+=retroshare_qml_app"
CONFIG *= no_retroshare_qml_app
retroshare_qml_app:CONFIG -= no_retroshare_qml_app

# To enable libresapi via local socket (unix domain socket or windows named
# pipes) append the following assignation to qmake command line
#"CONFIG+=libresapilocalserver"
CONFIG *= no_libresapilocalserver
libresapilocalserver:CONFIG -= no_libresapilocalserver

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

# To disable Deprecated Warning append the following
# assignation to qmake command line "CONFIG+=rs_nodeprecatedwarning"
CONFIG *= no_rs_nodeprecatedwarning
rs_nodeprecatedwarning:CONFIG -= no_rs_nodeprecatedwarning


unix {
	isEmpty(PREFIX)   { PREFIX   = "/usr" }
	isEmpty(BIN_DIR)  { BIN_DIR  = "$${PREFIX}/bin" }
	isEmpty(INC_DIR)  { INC_DIR  = "$${PREFIX}/include/retroshare06" }
	isEmpty(LIB_DIR)  { LIB_DIR  = "$${PREFIX}/lib" }
	isEmpty(DATA_DIR) { DATA_DIR = "$${PREFIX}/share/RetroShare06" }
	isEmpty(PLUGIN_DIR) { PLUGIN_DIR  = "$${LIB_DIR}/retroshare/extensions6" }

    rs_autologin {
        !macx {
            DEFINES *= HAS_GNOME_KEYRING
            PKGCONFIG *= gnome-keyring-1
        }
    }
}

android-g++ {
    CONFIG *= no_libresapihttpserver no_sqlcipher upnp_libupnp
    CONFIG -= libresapihttpserver sqlcipher upnp_miniupnpc
    QT *= androidextras
    DEFINES *= "fopen64=fopen"
    DEFINES *= "fseeko64=fseeko"
    DEFINES *= "ftello64=ftello"
    INCLUDEPATH += $$NDK_TOOLCHAIN_PATH/sysroot/usr/include
    LIBS *= -L$$NDK_TOOLCHAIN_PATH/sysroot/usr/lib/
    LIBS *= -lbz2 -lupnp -lixml -lthreadutil -lsqlite3
    ANDROID_EXTRA_LIBS *= $$NDK_TOOLCHAIN_PATH/sysroot/usr/lib/libsqlite3.so
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

macx {
	message(***retroshare.pri:MacOSX)
	BIN_DIR += "/usr/bin"
	INC_DIR += "/usr/include"
	INC_DIR += "/usr/local/include"
	INC_DIR += "/opt/local/include"
	LIB_DIR += "/usr/local/lib"
	LIB_DIR += "/opt/local/lib"
	!QMAKE_MACOSX_DEPLOYMENT_TARGET {
		message(***retroshare.pri: No Target. Set it to MacOS 10.11 )
		QMAKE_MACOSX_DEPLOYMENT_TARGET=10.11
	}
	!QMAKE_MAC_SDK {
		message(***retroshare.pri: No SDK. Set it to MacOS 10.11 )
		QMAKE_MAC_SDK = macosx10.11
	}
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
libresapihttpserver:DEFINES *= ENABLE_WEBUI
sqlcipher:DEFINES -= NO_SQLCIPHER
no_sqlcipher:DEFINES *= NO_SQLCIPHER
rs_autologin {
    DEFINES *= RS_AUTOLOGIN
    warning("You have enabled RetroShare auto-login, this is discouraged. The usage of auto-login on some linux distributions may allow someone having access to your session to steal the SSL keys of your node location and therefore compromise your security")
}

rs_nodeprecatedwarning {
    QMAKE_CXXFLAGS += -Wno-deprecated
    QMAKE_CXXFLAGS += -Wno-deprecated-declarations
    DEFINES *= RS_NO_WARN_DEPRECATED
    warning("QMAKE: You have disable deprecated warnings.")
}

rs_gxs_mail:DEFINES *= RS_GXS_MAIL
