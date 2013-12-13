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
			lang/FeedReader_lang.qrc
			
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
	LIBXML2_DIR = /usr/include/libxml2

	INCLUDEPATH += $${LIBXML2_DIR}

	LIBS += -lcurl -lxml2 -lxslt
}

win32 {
	DEFINES += CURL_STATICLIB LIBXML_STATIC LIBXSLT_STATIC LIBEXSLT_STATIC

	CURL_DIR = ../../../curl-7.26.0
	LIBXML2_DIR = ../../../libxml2-2.8.0
	LIBXSLT_DIR = ../../../libxslt-1.1.28

	INCLUDEPATH += $${CURL_DIR}/include $${LIBXML2_DIR}/include $${LIBXSLT_DIR} $${LIBICONV_DIR}/include

	LIBS += -lcurl -lxml2 -lxslt -lws2_32 -lwldap32
}

openbsd-* {
	LIBXML2_DIR = /usr/local/include/libxml2

	INCLUDEPATH += $${LIBXML2_DIR}

	LIBS += -lcurl -lxml2 -lxslt
}

