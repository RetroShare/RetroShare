################################################################################
# retroshare.pri                                                               #
# Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>               #
#                                                                              #
# This program is free software: you can redistribute it and/or modify         #
# it under the terms of the GNU Affero General Public License as               #
# published by the Free Software Foundation, either version 3 of the           #
# License, or (at your option) any later version.                              #
#                                                                              #
# This program is distributed in the hope that it will be useful,              #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                #
# GNU Lesser General Public License for more details.                          #
#                                                                              #
# You should have received a copy of the GNU Lesser General Public License     #
# along with this program.  If not, see <https://www.gnu.org/licenses/>.       #
################################################################################

################################################################################
## Documented build options (CONFIG) goes here as all the rest depend on them ##
## CONFIG must not be edited in other .pro files, aka if CONFIG need do be #####
## programatically modified depending on platform or from CONFIG itself it #####
## can be done ONLY inside this file (retroshare.pri) ##########################
################################################################################

# To disable RetroShare-gui append the following
# assignation to qmake command line "CONFIG+=no_retroshare_gui"
CONFIG *= retroshare_gui
no_retroshare_gui:CONFIG -= retroshare_gui

CONFIG *= gxsdistsync

# disabled by the time we fix compilation
CONFIG *= no_cmark

# To disable RetroShare-nogui append the following
# assignation to qmake command line "CONFIG+=no_retroshare_nogui"
CONFIG *= retroshare_nogui
no_retroshare_nogui:CONFIG -= retroshare_nogui

# To disable cmark append the following 
# assignation to qmake command line "CONFIG+=no_cmark"
CONFIG *= cmark
no_cmark:CONFIG -= cmark

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

# To enable RetroShare service append the following assignation to
# qmake command line "CONFIG+=retroshare_service"
CONFIG *= no_retroshare_service
retroshare_service:CONFIG -= no_retroshare_service

# To disable libresapi append the following assignation to qmake command line
#"CONFIG+=no_libresapi"
CONFIG *= libresapi
no_libresapi:CONFIG -= libresapi

# To enable libresapi via local socket (unix domain socket or windows named
# pipes) append the following assignation to qmake command line
#"CONFIG+=libresapilocalserver"
CONFIG *= no_libresapilocalserver
libresapilocalserver:CONFIG -= no_libresapilocalserver

# To enable libresapi settings handler in libresapi append the following
# assignation to qmake command line "CONFIG+=libresapi_settings"
CONFIG *= no_libresapi_settings
libresapi_settings:CONFIG -= no_libresapi_settings

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

# To disable GXS distrubuting all available posts independed of the "sync"
# settings append the following assignation to qmake command line
# "CONFIG+=no_rs_gxs_send_all"
CONFIG *= rs_gxs_send_all
no_rs_gxs_send_all:CONFIG -= rs_gxs_send_all

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

# To disable direct chat which has been deprecated since RetroShare 0.6.5 append
# the following assignation to qmake command line "CONFIG+=no_direct_chat"
CONFIG *= direct_chat
no_direct_chat:CONFIG -= direct_chat

# To disable bitdht append the following assignation to qmake command line
# "CONFIG+=no_bitdht"
CONFIG *= bitdht
no_bitdht:CONFIG -= bitdht

# To select your MacOsX version append the following assignation to qmake
# command line "CONFIG+=rs_macos10.11" where 10.11 depends your version
macx:CONFIG *= rs_macos10.11
rs_macos10.8:CONFIG -= rs_macos10.11
rs_macos10.9:CONFIG -= rs_macos10.11
rs_macos10.10:CONFIG -= rs_macos10.11
rs_macos10.12:CONFIG -= rs_macos10.11
rs_macos10.13:CONFIG -= rs_macos10.11
rs_macos10.14:CONFIG -= rs_macos10.11

# To enable JSON API append the following assignation to qmake command line
# "CONFIG+=rs_jsonapi"
CONFIG *= no_rs_jsonapi
rs_jsonapi:CONFIG -= no_rs_jsonapi

# To disable deep search append the following assignation to qmake command line
CONFIG *= no_rs_deep_search
rs_deep_search:CONFIG -= no_rs_deep_search

# To enable native dialogs append the following assignation to qmake command
#line "CONFIG+=rs_use_native_dialogs"
CONFIG *= no_rs_use_native_dialogs
rs_use_native_dialogs:CONFIG -= no_rs_use_native_dialogs

# Specify host precompiled jsonapi-generator path, appending the following
# assignation to qmake command line
# 'JSONAPI_GENERATOR_EXE=/myBuildDir/jsonapi-generator'. Required for JSON API
# cross-compiling
#JSONAPI_GENERATOR_EXE=/myBuildDir/jsonapi-generator

# Specify RetroShare major version (must be a number) appending the following
# assignation to qmake command line 'RS_MAJOR_VERSION=0'
#RS_MAJOR_VERSION=0

# Specify RetroShare minor version (must be a number) appending the following
# assignation to qmake command line 'RS_MINOR_VERSION=6'
#RS_MINOR_VERSION=6

# Specify RetroShare mini version (must be a number) appending the following
# assignation to qmake command line 'RS_MINI_VERSION=4'
#RS_MINI_VERSION=4

# Specify RetroShare extra version (must be a string) appending the following
# assignation to qmake command line 'RS_EXTRA_VERSION=""'
#RS_EXTRA_VERSION=git

# Specify threading library to use appending the following assignation to qmake
# commandline 'RS_THREAD_LIB=pthread' the name of the multi threading library to
# use (pthread, "") usually depends on platform.
isEmpty(RS_THREAD_LIB):RS_THREAD_LIB = pthread

# Specify UPnP library to use appending the following assignation to qmake
# command line 'RS_UPNP_LIB=miniupnpc' the name of the UPNP library to use
# (miniupnpc, "upnp ixml threadutil") usually depends on platform.
isEmpty(RS_UPNP_LIB):RS_UPNP_LIB = upnp ixml threadutil

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

################################################################################
## RetroShare qmake functions goes here as all the rest may use them. ##########
################################################################################

## Qt versions older the 5 are not supported anymore, check if the user is
## attempting use them and fail accordingly with a proper error message
lessThan(QT_MAJOR_VERSION, 5) {
    error(Qt 5.0.0 or newer is needed to build RetroShare)
}

## This function is useful to look for the location of a file in a list of paths
## like the which command on linux, first paramether is the file name,
## second parameter is the name of a variable containing the list of folders
## where to look for. First match is returned.
defineReplace(findFileInPath) {
    fileName=$$1
    pathList=$$2

    for(mDir, $$pathList) {
        attempt = $$clean_path($$mDir/$$fileName)
        exists($$attempt) {
            return($$system_path($$attempt))
        }
    }
    return()
}

## This function return linker option to link statically the libraries contained
## in the variable given as paramether.
## Be carefull static library are very susceptible to order
defineReplace(linkStaticLibs) {
    libsVarName = $$1
    retSlib =

    for(mLib, $$libsVarName) {
        attemptPath=$$findFileInPath(lib$${mLib}.a, QMAKE_LIBDIR)
        isEmpty(attemptPath):error(lib$${mLib}.a not found in [$${QMAKE_LIBDIR}])

        retSlib += -L$$dirname(attemptPath) -l$$mLib
    }

    return($$retSlib)
}

## This function return pretarget deps for the static the libraries contained in
## the variable given as paramether.
defineReplace(pretargetStaticLibs) {
    libsVarName = $$1

    retPreTarget =

    for(mLib, $$libsVarName) {
        attemptPath=$$findFileInPath(lib$${mLib}.a, QMAKE_LIBDIR)
        isEmpty(attemptPath):error(lib$${mLib}.a not found in [$${QMAKE_LIBDIR}])

        retPreTarget += $$attemptPath
    }

    return($$retPreTarget)
}

## This function return linker option to link dynamically the libraries
## contained in the variable given as paramether.
defineReplace(linkDynamicLibs) {
    libsVarName = $$1
    retDlib =

    for(mLib, $$libsVarName) {
        retDlib += -l$$mLib
    }

    return($$retDlib)
}

################################################################################
## Statements and variables that depends on build options (CONFIG) goes here ###
################################################################################
##
## Defining the following variables may be needed depending on platform and
## build options (CONFIG)
##
## PREFIX String variable containing the directory considered as prefix set
##  with = operator.
## QMAKE_LIBDIR, INCLUDEPATH Lists variables where qmake will look for includes
##   and libraries. Add values using *= operator.
## RS_BIN_DIR, RS_LIB_DIR, RS_INCLUDE_DIR, RS_DATA_DIR, RS_PLUGIN_DIR String
##   variables of directories where RetroShare components will be installed, on
##   most platforms they are automatically calculated from PREFIX or in other
##   ways.
## RS_SQL_LIB String viariable containing the name of the SQL library to use
##   ("sqlcipher sqlite3", sqlite3) it is usually precalculated depending on
##   CONFIG.

isEmpty(QMAKE_HOST_SPEC):QMAKE_HOST_SPEC=$$[QMAKE_SPEC]
isEmpty(QMAKE_TARGET_SPEC):QMAKE_TARGET_SPEC=$$[QMAKE_XSPEC]
equals(QMAKE_HOST_SPEC, $$QMAKE_TARGET_SPEC) {
    CONFIG *= no_rs_cross_compiling
    CONFIG -= rs_cross_compiling
} else {
    CONFIG *= rs_cross_compiling
    CONFIG -= no_rs_cross_compiling
    message(Cross-compiling detected QMAKE_HOST_SPEC: $$QMAKE_HOST_SPEC \
QMAKE_TARGET_SPEC: $$QMAKE_TARGET_SPEC)
}

defined(RS_MAJOR_VERSION,var):\
defined(RS_MINOR_VERSION,var):\
defined(RS_MINI_VERSION,var):\
defined(RS_EXTRA_VERSION,var) {
    message("RetroShare version\
$${RS_MAJOR_VERSION}.$${RS_MINOR_VERSION}.$${RS_MINI_VERSION}$${RS_EXTRA_VERSION}\
defined in command line")
    DEFINES += RS_MAJOR_VERSION=$${RS_MAJOR_VERSION}
    DEFINES += RS_MINOR_VERSION=$${RS_MINOR_VERSION}
    DEFINES += RS_MINI_VERSION=$${RS_MINI_VERSION}
    DEFINES += RS_EXTRA_VERSION=\\\"$${RS_EXTRA_VERSION}\\\"
} else {
    RS_GIT_DESCRIBE = $$system(git describe)
    contains(RS_GIT_DESCRIBE, ^v\d+\.\d+\.\d+.*) {
        RS_GIT_DESCRIBE_SPLIT = $$split(RS_GIT_DESCRIBE, v)
        RS_GIT_DESCRIBE_SPLIT = $$split(RS_GIT_DESCRIBE_SPLIT, .)

        RS_MAJOR_VERSION = $$member(RS_GIT_DESCRIBE_SPLIT, 0)
        RS_MINOR_VERSION = $$member(RS_GIT_DESCRIBE_SPLIT, 1)

        RS_GIT_DESCRIBE_SPLIT = $$member(RS_GIT_DESCRIBE_SPLIT, 2)
        RS_GIT_DESCRIBE_SPLIT = $$split(RS_GIT_DESCRIBE_SPLIT, -)

        RS_MINI_VERSION = $$member(RS_GIT_DESCRIBE_SPLIT, 0)

        RS_GIT_DESCRIBE_SPLIT = $$member(RS_GIT_DESCRIBE_SPLIT, 1, -1)

        RS_EXTRA_VERSION = $$join(RS_GIT_DESCRIBE_SPLIT,-,-)

        message("RetroShare version\
$${RS_MAJOR_VERSION}.$${RS_MINOR_VERSION}.$${RS_MINI_VERSION}$${RS_EXTRA_VERSION}\
determined via git")

        DEFINES += RS_MAJOR_VERSION=$${RS_MAJOR_VERSION}
        DEFINES += RS_MINOR_VERSION=$${RS_MINOR_VERSION}
        DEFINES += RS_MINI_VERSION=$${RS_MINI_VERSION}
        DEFINES += RS_EXTRA_VERSION=\\\"$${RS_EXTRA_VERSION}\\\"
    } else {
        warning("Determining RetroShare version via git failed plese specify it\
trough qmake command line arguments!")
    }
}

gxsdistsync:DEFINES *= RS_USE_GXS_DISTANT_SYNC
wikipoos:DEFINES *= RS_USE_WIKI
rs_gxs:DEFINES *= RS_ENABLE_GXS
rs_gxs_send_all:DEFINES *= RS_GXS_SEND_ALL
libresapilocalserver:DEFINES *= LIBRESAPI_LOCAL_SERVER
libresapi_settings:DEFINES *= LIBRESAPI_SETTINGS
libresapihttpserver:DEFINES *= ENABLE_WEBUI

sqlcipher {
    DEFINES -= NO_SQLCIPHER
    RS_SQL_LIB = sqlcipher 
}
no_sqlcipher {
    DEFINES *= NO_SQLCIPHER
    RS_SQL_LIB = sqlite3
}

rs_autologin {
    DEFINES *= RS_AUTOLOGIN
    RS_AUTOLOGIN_WARNING_MSG = \
        You have enabled RetroShare auto-login, this is discouraged. The usage \
        of auto-login on some linux distributions may allow someone having \
        access to your session to steal the SSL keys of your node location and \
        therefore compromise your security
    warning("$${RS_AUTOLOGIN_WARNING_MSG}")
}

rs_onlyhiddennode {
    DEFINES *= RS_ONLYHIDDENNODE
    CONFIG -= bitdht
    CONFIG *= no_bitdht
    message("QMAKE: You have enabled only hidden node.")
}

no_rs_deprecatedwarning {
    QMAKE_CXXFLAGS += -Wno-deprecated
    QMAKE_CXXFLAGS += -Wno-deprecated-declarations
    DEFINES *= RS_NO_WARN_DEPRECATED
    message("QMAKE: You have disabled deprecated warnings.")
}

no_rs_cppwarning {
    QMAKE_CXXFLAGS += -Wno-cpp
    QMAKE_CXXFLAGS += -Wno-inconsistent-missing-override

    DEFINES *= RS_NO_WARN_CPP
    message("QMAKE: You have disabled C preprocessor warnings.")
}

rs_gxs_trans {
    DEFINES *= RS_GXS_TRANS
    greaterThan(QT_MAJOR_VERSION, 4) {
        CONFIG += c++11
    } else {
        QMAKE_CXXFLAGS += -std=c++0x
    }
}

bitdht {
    DEFINES *= RS_USE_BITDHT
}

direct_chat {
    warning("You have enabled RetroShare direct chat which is deprecated!")
    DEFINES *= RS_DIRECT_CHAT
}

rs_async_chat {
    DEFINES *= RS_ASYNC_CHAT
}

rs_chatserver {
    DEFINES *= RS_CHATSERVER
}

rs_jsonapi {
    rs_cross_compiling:!exists($$JSONAPI_GENERATOR_EXE):error("Inconsistent \
build configuration, cross-compiling JSON API requires JSONAPI_GENERATOR_EXE \
to contain the path to an host executable jsonapi-generator")

    DEFINES *= RS_JSONAPI
}

rs_deep_search {
    DEFINES *= RS_DEEP_SEARCH

	linux {
	 exists("/usr/include/xapian-1.3") {
	 	INCLUDEPATH += /usr/include/xapian-1.3
	 }
	}
}

rs_use_native_dialogs:DEFINES *= RS_NATIVEDIALOGS

debug {
    QMAKE_CXXFLAGS -= -O2 -fomit-frame-pointer
    QMAKE_CFLAGS -= -O2 -fomit-frame-pointer

    QMAKE_CXXFLAGS *= -O0 -g -fno-omit-frame-pointer
    QMAKE_CFLAGS *= -O0 -g -fno-omit-frame-pointer
}

profiling {
    QMAKE_CXXFLAGS -= -fomit-frame-pointer
    QMAKE_CFLAGS -= -fomit-frame-pointer

    QMAKE_CXXFLAGS *= -pg -g -fno-omit-frame-pointer
    QMAKE_CFLAGS *= -pg -g -fno-omit-frame-pointer

    QMAKE_LFLAGS *= -pg
}

################################################################################
## Last goes platform specific statements common to all RetroShare subprojects #
################################################################################

linux-* {
    isEmpty(PREFIX)        : PREFIX         = "/usr"
    isEmpty(RS_BIN_DIR)    : RS_BIN_DIR     = "$${PREFIX}/bin"
    isEmpty(RS_INCLUDE_DIR): RS_INCLUDE_DIR = "$${PREFIX}/include"
    isEmpty(RS_LIB_DIR)    : RS_LIB_DIR     = "$${PREFIX}/lib"
    isEmpty(RS_DATA_DIR)   : RS_DATA_DIR    = "$${PREFIX}/share/retroshare"
    isEmpty(RS_PLUGIN_DIR) : RS_PLUGIN_DIR  = "$${PREFIX}/lib/retroshare/extensions6"

    QMAKE_LIBDIR *= "$$RS_LIB_DIR"

    rs_autologin {
        # try libsecret first since it is not limited to gnome keyring and libgnome-keyring is deprecated
        LIBSECRET_AVAILABLE = $$system(pkg-config --exists libsecret-1 && echo yes)
        isEmpty(LIBSECRET_AVAILABLE) {
            message("using libgnome-keyring for auto login")
            DEFINES *= HAS_GNOME_KEYRING
            PKGCONFIG *= gnome-keyring-1
        } else {
            message("using libsecret for auto login")
            DEFINES *= HAS_LIBSECRET
            PKGCONFIG *= libsecret-1
        }
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
    CONFIG *= no_libresapihttpserver
    CONFIG -= libresapihttpserver
    QT *= androidextras
    INCLUDEPATH *= $$NATIVE_LIBS_TOOLCHAIN_PATH/sysroot/usr/include
    QMAKE_LIBDIR *= "$$NATIVE_LIBS_TOOLCHAIN_PATH/sysroot/usr/lib/"

    # The android libc, bionic, provides built-in support for pthreads,
    # additional linking (-lpthreads) break linking.
    # See https://stackoverflow.com/a/31277163
    RS_THREAD_LIB =
}

win32-g++ {
    !isEmpty(EXTERNAL_LIB_DIR) {
        message(Use pre-compiled libraries in $${EXTERNAL_LIB_DIR}.)
        PREFIX = $$system_path($$EXTERNAL_LIB_DIR)
    }

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

        !isEmpty(PREFIX_MSYS2):message(Found MSYS2: $${PREFIX_MSYS2})
    }

    isEmpty(PREFIX):!isEmpty(PREFIX_MSYS2) {
        PREFIX = $$system_path($${PREFIX_MSYS2})
    }

    isEmpty(PREFIX) {
        error(PREFIX is not set. Set either EXTERNAL_LIB_DIR or PREFIX_MSYS2.)
    }

    INCLUDEPATH *= $$system_path($${PREFIX}/include)
    !isEmpty(PREFIX_MSYS2) : INCLUDEPATH *= $$system_path($${PREFIX_MSYS2}/include)

    QMAKE_LIBDIR *= $$system_path($${PREFIX}/lib)
    !isEmpty(PREFIX_MSYS2) : QMAKE_LIBDIR *= $$system_path($${PREFIX_MSYS2}/lib)

    RS_BIN_DIR     = $$system_path($${PREFIX}/bin)
    RS_INCLUDE_DIR = $$system_path($${PREFIX}/include)
    RS_LIB_DIR     = $$system_path($${PREFIX}/lib)

    RS_UPNP_LIB = miniupnpc

    DEFINES *= NOGDI WIN32 WIN32_LEAN_AND_MEAN WINDOWS_SYS

    # This defines the platform to be WinXP or later and is needed for
    #  getaddrinfo (_WIN32_WINNT_WINXP)
    DEFINES *= WINVER=0x0501

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
	rs_macos10.13 {
		message(***retroshare.pri: Set Target and SDK to MacOS 10.13 )
		QMAKE_MACOSX_DEPLOYMENT_TARGET=10.13
		QMAKE_MAC_SDK = macosx10.13
		QMAKE_CXXFLAGS += -Wno-nullability-completeness
		QMAKE_CFLAGS += -Wno-nullability-completeness
	}
	rs_macos10.14 {
		message(***retroshare.pri: Set Target and SDK to MacOS 10.14 )
		QMAKE_MACOSX_DEPLOYMENT_TARGET=10.14
		QMAKE_MAC_SDK = macosx10.14
		QMAKE_CXXFLAGS += -Wno-nullability-completeness
		QMAKE_CFLAGS += -Wno-nullability-completeness
	}



	message(***retroshare.pri:MacOSX)
	# BIN_DIR += "/usr/bin"
	# INC_DIR += "/usr/include"
	# INC_DIR += "/usr/local/include"
	# INC_DIR += "/opt/local/include"
	# LIB_DIR += "/usr/local/lib"
	# LIB_DIR += "/opt/local/lib"
	BIN_DIR += "/Applications/Xcode.app/Contents/Developer/usr/bin"
	INC_DIR += "/usr/local/Cellar/miniupnpc/2.1/include"
	INC_DIR += "/usr/local/Cellar/libmicrohttpd/0.9.62_1/include"
	INC_DIR += "/usr/local/Cellar/sqlcipher/4.0.1/include"
	LIB_DIR += "/usr/local/opt/openssl/lib/"
	LIB_DIR += "/usr/local/Cellar/libmicrohttpd/0.9.62_1/lib"
	LIB_DIR += "/usr/local/Cellar/sqlcipher/4.0.1/lib"
	LIB_DIR += "/usr/local/Cellar/miniupnpc/2.1/lib"
	CONFIG += c++11
	INCLUDEPATH += "/usr/local/include"
	RS_UPNP_LIB = miniupnpc
	QT += macextras
}


## Retrocompatibility assignations, get rid of this ASAP
isEmpty(BIN_DIR)   : BIN_DIR   = $${RS_BIN_DIR}
isEmpty(INC_DIR)   : INC_DIR   = $${RS_INCLUDE_DIR}
isEmpty(LIBDIR)    : LIBDIR    = $${QMAKE_LIBDIR}
isEmpty(DATA_DIR)  : DATA_DIR  = $${RS_DATA_DIR}
isEmpty(PLUGIN_DIR): PLUGIN_DIR= $${RS_PLUGIN_DIR}
