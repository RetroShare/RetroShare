!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TEMPLATE = lib
CONFIG += staticlib
CONFIG += create_prl
CONFIG -= qt
TARGET = resapi
TARGET_PRL = libresapi
DESTDIR = lib

INCLUDEPATH += ../../libretroshare/src

retroshare_android_service {
    win32 {
        OBJECTS_DIR = temp/obj

        LIBS_DIR = $$PWD/../../libs/lib
        LIBS += $$OUT_PWD/../../libretroshare/src/lib/libretroshare.a
        LIBS += $$OUT_PWD/../../openpgpsdk/src/lib/libops.a

        for(lib, LIB_DIR):LIBS += -L"$$lib"
        for(bin, BIN_DIR):LIBS += -L"$$bin"


        LIBS += -lssl -lcrypto -lpthread -lminiupnpc -lz -lws2_32
        LIBS += -luuid -lole32 -liphlpapi -lcrypt32 -lgdi32
        LIBS += -lwinmm

        DEFINES *= WINDOWS_SYS WIN32_LEAN_AND_MEAN _USE_32BIT_TIME_T

        DEPENDPATH += . $$INC_DIR
        INCLUDEPATH += . $$INC_DIR

        greaterThan(QT_MAJOR_VERSION, 4) {
                # Qt 5
                RC_INCLUDEPATH += $$_PRO_FILE_PWD_/../../libretroshare/src
        } else {
                # Qt 4
                QMAKE_RC += --include-dir=$$_PRO_FILE_PWD_/../../libretroshare/src
        }
    }

    DEPENDPATH += . ../../libretroshare/src/
    INCLUDEPATH += ../../libretroshare/src/
}

libresapihttpserver {
    CONFIG += libmicrohttpd

    unix {

        webui_files.path = "$${DATA_DIR}/webui"
        webui_files.files = webui/app.js webui/app.css webui/index.html
        INSTALLS += webui_files

        webui_img_files.path = "$${DATA_DIR}/webui/img"
        webui_img_files.files = ../../retroshare-gui/src/gui/images/logo/logo_splash.png
        INSTALLS += webui_img_files

        # create dummy files, we need it to include files on first try
        system(webui-src/make-src/build.sh .)

        WEBUI_SRC_SCRIPT = webui-src/make-src/build.sh

        WEBUI_SRC_HTML   = $$WEBUI_SRC_SCRIPT
        WEBUI_SRC_HTML  += webui-src/app/assets/index.html

        WEBUI_SRC_JS  = $$WEBUI_SRC_SCRIPT
        WEBUI_SRC_JS += webui-src/app/accountselect.js
        WEBUI_SRC_JS += webui-src/app/adddownloads.js
        WEBUI_SRC_JS += webui-src/app/addidentity.js
        WEBUI_SRC_JS += webui-src/app/addpeer.js
        WEBUI_SRC_JS += webui-src/app/chat.js
        WEBUI_SRC_JS += webui-src/app/createlogin.js
        WEBUI_SRC_JS += webui-src/app/downloads.js
        WEBUI_SRC_JS += webui-src/app/forums.js
        WEBUI_SRC_JS += webui-src/app/home.js
        WEBUI_SRC_JS += webui-src/app/identities.js
        WEBUI_SRC_JS += webui-src/app/main.js
        WEBUI_SRC_JS += webui-src/app/menudef.js
        WEBUI_SRC_JS += webui-src/app/menu.js
        WEBUI_SRC_JS += webui-src/app/mithril.js
        WEBUI_SRC_JS += webui-src/app/mithril.min.js
        WEBUI_SRC_JS += webui-src/app/peers.js
        WEBUI_SRC_JS += webui-src/app/retroshare.js
        WEBUI_SRC_JS += webui-src/app/search.js
        WEBUI_SRC_JS += webui-src/app/searchresult.js
        WEBUI_SRC_JS += webui-src/app/servicecontrol.js
        WEBUI_SRC_JS += webui-src/app/settings.js
        WEBUI_SRC_JS += webui-src/app/waiting.js

        WEBUI_SRC_CSS  = $$WEBUI_SRC_SCRIPT
        WEBUI_SRC_CSS += webui-src/app/green-black.scss
        WEBUI_SRC_CSS += webui-src/app/_reset.scss
        WEBUI_SRC_CSS += webui-src/app/_chat.sass
        WEBUI_SRC_CSS += webui-src/app/main.sass


        create_webfiles_html.output = webui/index.html
        create_webfiles_html.input = WEBUI_SRC_HTML
        create_webfiles_html.commands = sh $$_PRO_FILE_PWD_/webui-src/make-src/build.sh $$_PRO_FILE_PWD_ index.html .
        create_webfiles_html.variable_out = JUNK
        create_webfiles_html.CONFIG = combine no_link

        create_webfiles_js.output = webui/app.js
        create_webfiles_js.input = WEBUI_SRC_JS
        create_webfiles_js.commands = sh $$_PRO_FILE_PWD_/webui-src/make-src/build.sh $$_PRO_FILE_PWD_ app.js .
        create_webfiles_js.variable_out = JUNK
        create_webfiles_js.CONFIG = combine no_link

        create_webfiles_css.output = webui/app.css
        create_webfiles_css.input = WEBUI_SRC_CSS
        create_webfiles_css.commands = sh $$_PRO_FILE_PWD_/webui-src/make-src/build.sh $$_PRO_FILE_PWD_ app.css .
        create_webfiles_css.variable_out = JUNK
        create_webfiles_css.CONFIG = combine no_link


        QMAKE_EXTRA_COMPILERS += create_webfiles_html create_webfiles_js create_webfiles_css
    }

    win32 {
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

        #create_webfiles.commands = $$MAKE_SRC\\build.bat $$PRO_PATH
        #QMAKE_EXTRA_TARGETS += create_webfiles
        #PRE_TARGETDEPS += create_webfiles
        QMAKE_POST_LINK=$$MAKE_SRC\\build.bat $$PRO_PATH

	# create dummy files
	system($$MAKE_SRC\\init.bat .)
    }

	linux {
		CONFIG += link_pkgconfig
		PKGCONFIG *= libmicrohttpd
	} else {
		mac {
			INCLUDEPATH += . $$INC_DIR
			#for(lib, LIB_DIR):exists($$lib/libmicrohttpd.a){ LIBS *= $$lib/libmicrohttpd.a}
			LIBS *= -lmicrohttpd
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
    api/ChannelsHandler.cpp \
    api/StatsHandler.cpp

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
    api/ChannelsHandler.h \
    api/StatsHandler.h

libresapilocalserver {
    CONFIG *= qt
    QT *= network
    SOURCES *= api/ApiServerLocal.cpp
    HEADERS *= api/ApiServerLocal.h
}

qt_dependencies {
    CONFIG *= qt
    QT *= core

    SOURCES += api/SettingsHandler.cpp
    HEADERS += api/SettingsHandler.h
}
