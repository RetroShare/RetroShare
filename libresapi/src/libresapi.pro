!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TEMPLATE = lib
CONFIG += staticlib
CONFIG += create_prl
CONFIG -= qt
TARGET = resapi
TARGET_PRL = libresapi
DESTDIR = lib

CONFIG += libmicrohttpd

INCLUDEPATH += ../../libretroshare/src

unix {

        webui_files.path = "$${DATA_DIR}/webui"
        webui_files.files = webui/*
	INSTALLS += webui_files        

	webui_img_files.path = "$${DATA_DIR}/webui/img"
	webui_img_files.files = ../../retroshare-gui/src/gui/images/logo/logo_splash.png
	INSTALLS += webui_img_files

        create_webfiles.commands = sh $$_PRO_FILE_PWD_/webui-src/make-src/build.sh $$_PRO_FILE_PWD_
        QMAKE_EXTRA_TARGETS += create_webfiles
        PRE_TARGETDEPS += create_webfiles

        # create dummy files
        system(webui-src/make-src/init.sh .)
}

win32{
	DEFINES *= WINDOWS_SYS
	INCLUDEPATH += . $$INC_DIR

	greaterThan(QT_MAJOR_VERSION, 4) {
		# Qt 5
		PRO_PATH=$$shell_path($$_PRO_FILE_PWD_)
		MAKE_SRC=$$shell_path($$PRO_PATH/webui-src/make-src)
	} else {
		# Qt 4
		PRO_PATH=$$replace(_PRO_FILE_PWD_, /, \\)
		MAKE_SRC=$$PRO_PATH\\webui-src\\make-src
	}

	create_webfiles.commands = $$MAKE_SRC\\build.bat $$PRO_PATH
	QMAKE_EXTRA_TARGETS += create_webfiles
	PRE_TARGETDEPS += create_webfiles

	# create dummy files
	system($$MAKE_SRC\\init.bat .)
}

libmicrohttpd{
	linux {
		CONFIG += link_pkgconfig
		PKGCONFIG *= libmicrohttpd
	} else {
		mac {
			INCLUDEPATH += . $$INC_DIR
			for(lib, LIB_DIR):exists($$lib/libmicrohttpd.a){ LIBS *= $$lib/libmicrohttpd.a}
		} else {
			LIBS *= -lmicrohttpd
		}
	}
	SOURCES += \
		api/ApiServerMHD.cpp

	HEADERS += \
		api/ApiServerMHD.h
}

SOURCES += \
	api/ApiServer.cpp \
	api/json.cpp \
	api/JsonStream.cpp \
	api/ResourceRouter.cpp \
	api/PeersHandler.cpp \
	api/Operators.cpp \
	api/IdentityHandler.cpp \
	api/ForumHandler.cpp \
	api/ServiceControlHandler.cpp \
	api/StateTokenServer.cpp \
	api/GxsResponseTask.cpp \
	api/FileSearchHandler.cpp \
	api/TransfersHandler.cpp	\
	api/RsControlModule.cpp	\
	api/GetPluginInterfaces.cpp \
    api/ChatHandler.cpp \
    api/LivereloadHandler.cpp \
    api/TmpBlobStore.cpp \
    util/ContentTypes.cpp \
    api/ApiPluginHandler.cpp \
    api/ChannelsHandler.cpp

HEADERS += \
	api/ApiServer.h \
	api/json.h \
	api/JsonStream.h \
	api/ApiTypes.h \
	api/ResourceRouter.h \
	api/PeersHandler.h \
	api/Operators.h \
	api/IdentityHandler.h \
	api/ForumHandler.h \
	api/ServiceControlHandler.h \
	api/GxsMetaOperators.h \
	api/StateTokenServer.h \
	api/GxsResponseTask.h \
	api/Pagination.h \
	api/FileSearchHandler.h \
	api/TransfersHandler.h	\
	api/RsControlModule.h	\
	api/GetPluginInterfaces.h \
    api/ChatHandler.h \
    api/LivereloadHandler.h \
    api/TmpBlobStore.h \
    util/ContentTypes.h \
    api/ApiPluginHandler.h \
    api/ChannelsHandler.h
