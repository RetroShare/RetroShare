# To disable RetroShare-gui append the following
# assignation to qmake command line "CONFIG+=no_retroshare_gui"
CONFIG *= retroshare_gui
no_retroshare_gui:CONFIG -= retroshare_gui

# To build the RetroTor executable, just uncomment the following option.
# RetroTor is a version of RS that automatically configures Tor for its own usage
# using only hidden nodes. It will not start if Tor is not working.

# CONFIG *= retrotor

# To disable RetroShare-nogui append the following
# assignation to qmake command line "CONFIG+=no_retroshare_nogui"
CONFIG *= retroshare_nogui
no_retroshare_nogui:CONFIG -= retroshare_nogui

# To enable RetroShare plugins append the following
# assignation to qmake command line "CONFIG+=retroshare_plugins"
CONFIG *= no_retroshare_plugins
retroshare_plugins:CONFIG -= no_retroshare_plugins

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

# To have only hidden node generation append the following assignation
# to qmake command line "CONFIG+=rs_onlyhiddennode"
CONFIG *= no_rs_onlyhiddennode
rs_onlyhiddennode:CONFIG -= no_rs_onlyhiddennode

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
# command line "CONFIG+=rs_macos10.11" where 10.11 depends your version
macx:CONFIG *= rs_macos10.11
rs_macos10.8:CONFIG -= rs_macos10.11
rs_macos10.9:CONFIG -= rs_macos10.11
rs_macos10.10:CONFIG -= rs_macos10.11
rs_macos10.12:CONFIG -= rs_macos10.11

## This function is useful to look for the location of a file in a list of paths
## like the which command on linux
defineReplace(findFileInPath) {
    fileName=$$1
    pathList=$$2

    for(mDir, $$pathList) {
        attempt = "$$mDir/$$fileName"
        message(defineReplace attempting $$attempt)
        exists($$clean_path($$attempt)) {
            message(defineReplace found $$attempt)
            return($$system_path($$attempt))
        }
    }
    return()
}

## For each platform defining the following variables may be needed
## PREFIX, QMAKE_LIBDIR, INCLUDEPATH, RS_INCLUDE_DIR, RS_DATA_DIR, RS_PLUGIN_DIR
## RS_BIN_DIR

linux-* {
    isEmpty(PREFIX)        : PREFIX         = "/usr"
    isEmpty(BIN_DIR)       : BIN_DIR        = "$${PREFIX}/bin"
    isEmpty(RS_INCLUDE_DIR): RS_INCLUDE_DIR = "$${PREFIX}/include"
    isEmpty(RS_DATA_DIR)   : RS_DATA_DIR    = "$${PREFIX}/share/retroshare"
    isEmpty(RS_PLUGIN_DIR) : RS_PLUGIN_DIR  = "$${LIB_DIR}/retroshare/extensions6"

    INCLUDEPATH *= "$$RS_INCLUDE_DIR"
    QMAKE_LIBDIR *= "$${PREFIX}/lib"

    rs_autologin {
        DEFINES *= HAS_GNOME_KEYRING
        PKGCONFIG *= gnome-keyring-1
    }
}

android-* {
    isEmpty(NATIVE_LIBS_TOOLCHAIN_PATH) {
        NATIVE_LIBS_TOOLCHAIN_PATH = $$(NATIVE_LIBS_TOOLCHAIN_PATH)
    }
    retroshare_qml_app {
        CONFIG -= no_retroshare_android_notify_service
        CONFIG *= retroshare_android_notify_service
    }
    CONFIG *= no_libresapihttpserver upnp_libupnp
    CONFIG -= libresapihttpserver upnp_miniupnpc
    QT *= androidextras
    INCLUDEPATH += $$NATIVE_LIBS_TOOLCHAIN_PATH/sysroot/usr/include
    LIBS *= -L$$NATIVE_LIBS_TOOLCHAIN_PATH/sysroot/usr/lib/
}

win32-g++ {
    PREFIX_MSYS2 = $$(MINGW_PREFIX)
    isEmpty(PREFIX_MSYS2) {
        message("MINGW_PREFIX is not set, attempting MSYS2 autodiscovery.")

        TEMPTATIVE_MSYS2=$$system_path(C:\\msys32\\mingw32)
        exists($$clean_path($${TEMPTATIVE_MSYS2}/include)) {
            PREFIX_MSYS2=$${TEMPTATIVE_MSYS2}
        }

        TEMPTATIVE_MSYS2=$$system_path(C:\\msys64\\mingw32)
        exists($$clean_path($${TEMPTATIVE_MSYS2}/include)) {
            PREFIX_MSYS2=$${TEMPTATIVE_MSYS2}
        }

        isEmpty(PREFIX_MSYS2) {
            error(Cannot find MSYS2 please set MINGW_PREFIX)
        } else {
            message(Found MSYS2: $${PREFIX_MSYS2})
        }
    }

    isEmpty(PREFIX) {
        PREFIX = $$system_path($${PREFIX_MSYS2}/usr)
    }

    INCLUDEPATH *= $$system_path($${PREFIX}/include)
    INCLUDEPATH *= $$system_path($${PREFIX_MSYS2}/usr/include)

    QMAKE_LIBDIR *= $$system_path($${PREFIX}/lib)
    QMAKE_LIBDIR *= $$system_path($${PREFIX_MSYS2}/usr/lib)

    DEFINES *= WINDOWS_SYS WIN32

    message(***retroshare.pri:Win32 PREFIX $$PREFIX INCLUDEPATH $$INCLUDEPATH QMAKE_LIBDIR $$QMAKE_LIBDIR DEFINES $$DEFINES)
}

macx-* {
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
		QMAKE_CXXFLAGS += -Wno-nullability-completeness
		QMAKE_CFLAGS += -Wno-nullability-completeness
	}

	message(***retroshare.pri:MacOSX)
	BIN_DIR += "/usr/bin"
	INC_DIR += "/usr/include"
	INC_DIR += "/usr/local/include"
	INC_DIR += "/opt/local/include"
	LIB_DIR += "/usr/local/lib"
	LIB_DIR += "/opt/local/lib"
	CONFIG += c++11
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

retrotor {
    CONFIG *= rs_onlyhiddennode
}

rs_onlyhiddennode {
    DEFINES *= RS_ONLYHIDDENNODE
    warning("QMAKE: You have enabled only hidden node.")
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

###########################################################################################################################################################
#
#  V07_NON_BACKWARD_COMPATIBLE_CHANGE_001:
#
#     What: Computes the node id by performing a sha256 hash of the certificate's PGP signature, instead of simply picking up the last 20 bytes of it.
#
#     Why: There is no real risk in forging a certificate with the same ID as the authentication is performed over the PGP signature of the certificate
#           which hashes the full SSL certificate (i.e. the full serialized CERT_INFO structure). However the possibility to
#           create two certificates with the same IDs is a problem, as it can be used to cause disturbance in the software.
#
#     Backward compat: connexions impossible with non patched peers older than Nov 2017, probably because the SSL id that is computed is not the same on both side,
#                    and in particular unpatched peers see a cerficate with ID different (because computed with the old method) than the ID that was
#                    submitted when making friends.
#
#     Note: the advantage of basing the ID on the signature rather than the public key is not very clear, given that the signature is based on a hash
#           of the public key (and the rest of the certificate info).
#
#  V07_NON_BACKWARD_COMPATIBLE_CHANGE_002:
#
#     What: Use RSA+SHA256 instead of RSA+SHA1 for PGP certificate signatures
#
#     Why:  Sha1 is likely to be prone to primary collisions anytime soon, so it is urgent to turn to a more secure solution.
#
#     Backward compat: unpatched peers after Nov 2017 are able to verify signatures since openpgp-sdk already handle it.
#
#  V07_NON_BACKWARD_COMPATIBLE_CHANGE_003:
#
#      What: Do not hash PGP certificate twice when signing
#
#  	 Why: hasing twice is not per se a security issue, but it makes it harder to change the settings for hashing.
#
#  	 Backward compat: patched peers cannot connect to non patched peers older than Nov 2017.
###########################################################################################################################################################

#CONFIG += rs_v07_changes
rs_v07_changes {
	DEFINES += V07_NON_BACKWARD_COMPATIBLE_CHANGE_001
	DEFINES += V07_NON_BACKWARD_COMPATIBLE_CHANGE_002
	DEFINES += V07_NON_BACKWARD_COMPATIBLE_CHANGE_003
}

## Retrocompatibility assignations, get rid of this ASAP
isEmpty(BIN_DIR)   : BIN_DIR   = $${RS_BIN_DIR}
isEmpty(INC_DIR)   : INC_DIR   = $${RS_INCLUDE_DIR}
isEmpty(LIBDIR)    : LIBDIR    = $${QMAKE_LIBDIR}
isEmpty(DATA_DIR)  : DATA_DIR  = $${RS_DATA_DIR}
isEmpty(PLUGIN_DIR): PLUGIN_DIR= $${RS_PLUGIN_DIR}
