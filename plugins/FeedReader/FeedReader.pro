!include("../Common/retroshare_plugin.pri"): error("Could not include file ../Common/retroshare_plugin.pri")

CONFIG += qt uic qrc resources

greaterThan(QT_MAJOR_VERSION, 4) {
	# Qt 5
	QT += widgets
}

SOURCES =	FeedReaderPlugin.cpp \
			services/p3FeedReader.cc \
			services/p3FeedReaderThread.cc \
			services/rsFeedReaderItems.cc \
			gui/FeedReaderDialog.cpp \
			gui/FeedReaderMessageWidget.cpp \
			gui/AddFeedDialog.cpp \
			gui/PreviewFeedDialog.cpp \
			gui/FeedReaderNotify.cpp \
			gui/FeedReaderConfig.cpp \
			gui/FeedReaderStringDefs.cpp \
			gui/FeedReaderFeedNotify.cpp \
			gui/FeedReaderUserNotify.cpp \
			gui/FeedReaderFeedItem.cpp \
			util/CURLWrapper.cpp \
			util/XMLWrapper.cpp \
			util/HTMLWrapper.cpp \
			util/XPathWrapper.cpp

HEADERS =	FeedReaderPlugin.h \
			interface/rsFeedReader.h \
			services/p3FeedReader.h \
			services/p3FeedReaderThread.h \
			services/rsFeedReaderItems.h \
			gui/FeedReaderDialog.h \
			gui/FeedReaderMessageWidget.h \
			gui/AddFeedDialog.h \
			gui/PreviewFeedDialog.h \
			gui/FeedReaderNotify.h \
			gui/FeedReaderConfig.h \
			gui/FeedReaderStringDefs.h \
			gui/FeedReaderFeedNotify.h \
			gui/FeedReaderUserNotify.h \
			gui/FeedReaderFeedItem.h \
			util/CURLWrapper.h \
			util/XMLWrapper.h \
			util/HTMLWrapper.h \
			util/XPathWrapper.h

FORMS =		gui/FeedReaderDialog.ui \
			gui/FeedReaderMessageWidget.ui \
			gui/AddFeedDialog.ui \
			gui/PreviewFeedDialog.ui \
			gui/FeedReaderConfig.ui \
			gui/FeedReaderFeedItem.ui

TARGET = FeedReader

RESOURCES = gui/FeedReader_images.qrc \
			lang/FeedReader_lang.qrc \
			qss/FeedReader_qss.qrc

TRANSLATIONS +=  \
			lang/FeedReader_ca_ES.ts \
			lang/FeedReader_cs.ts \
			lang/FeedReader_da.ts \
			lang/FeedReader_de.ts \
			lang/FeedReader_el.ts \
			lang/FeedReader_en.ts \
			lang/FeedReader_es.ts \
			lang/FeedReader_fi.ts \
			lang/FeedReader_fr.ts \
			lang/FeedReader_hu.ts \
			lang/FeedReader_it.ts \
			lang/FeedReader_ja_JP.ts \
			lang/FeedReader_ko.ts \
			lang/FeedReader_nl.ts \
			lang/FeedReader_pl.ts \
			lang/FeedReader_ru.ts \
			lang/FeedReader_sv.ts \
			lang/FeedReader_tr.ts \
			lang/FeedReader_zh_CN.ts

linux-* {
	CONFIG += link_pkgconfig

	PKGCONFIG *= libcurl libxml-2.0 libxslt
}

win32 {
	DEFINES += CURL_STATICLIB LIBXML_STATIC LIBXSLT_STATIC LIBEXSLT_STATIC

	#Have to reorder libs, else got /libs/lib/libcrypto.a(bio_lib.o):bio_lib.c:(.text+0x0): multiple definition of `BIO_new'
	LIBS = -lcurl -lxml2 -lz -lxslt -lws2_32 -lwldap32 -lssl -lcrypto -lgdi32 $${LIBS}
	exists(C:/msys32/mingw32/include) {
		message(msys2 xml2 is installed.)
		INC_DIR  += "C:/msys32/mingw32/include/libxml2"
	}
	exists(C:/msys64/mingw32/include) {
		message(msys2 xml2 is installed.)
		INC_DIR  += "C:/msys64/mingw32/include/libxml2"
	}
	DEPENDPATH += . $$INC_DIR
	INCLUDEPATH += . $$INC_DIR
}

macx {
	DEFINES += CURL_STATICLIB LIBXML_STATIC LIBXSLT_STATIC LIBEXSLT_STATIC

	XML2_FOUND =
	for(inc, INC_DIR){
#message(Scanning $$inc)s
		exists($$inc/libxml2){
			isEmpty( XML2_FOUND) {
				message(xml2 is first found here: $$inc .)
				INC_DIR  += "$$inc/libxml2"
				XML2_FOUND = 1
			}
		}
	}
	DEPENDPATH += . $$INC_DIRs
	INCLUDEPATH += . $$INC_DIR

	LIBS = -lcurl -lxml2 -lxslt -lcrypto

error(Missing RetroShare-gui library. Remove Plugins from RetroShare.pro))
}

openbsd-* {
	LIBXML2_DIR = /usr/local/include/libxml2
}

haiku-* {
	LIBXML2_DIR = pkg-config --cflags libxml-2.0

	INCLUDEPATH += $${LIBXML2_DIR}

	LIBS += -lcurl -lxml2 -lxslt
}
