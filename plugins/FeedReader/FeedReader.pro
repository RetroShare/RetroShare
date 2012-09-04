!include("../Common/retroshare_plugin.pri"): error("Could not include file ../Common/retroshare_plugin.pri")

CONFIG += qt uic qrc resources

SOURCES =	FeedReaderPlugin.cpp \
			services/p3FeedReader.cc \
			services/p3FeedReaderThread.cc \
			services/rsFeedReaderItems.cc \
			gui/FeedReaderDialog.cpp \
			gui/AddFeedDialog.cpp \
			gui/PreviewFeedDialog.cpp \
			gui/FeedReaderNotify.cpp \
			gui/FeedReaderConfig.cpp \
			gui/FeedReaderStringDefs.cpp \
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
			gui/AddFeedDialog.h \
			gui/PreviewFeedDialog.h \
			gui/FeedReaderNotify.h \
			gui/FeedReaderConfig.h \
			gui/FeedReaderStringDefs.h \
			util/CURLWrapper.h \
			util/XMLWrapper.h \
			util/HTMLWrapper.h \
			util/XPathWrapper.h

FORMS =		gui/FeedReaderDialog.ui \
			gui/AddFeedDialog.ui \
			gui/PreviewFeedDialog.ui \
			gui/FeedReaderConfig.ui

TARGET = FeedReader

RESOURCES = gui/FeedReader_images.qrc \
			lang/lang.qrc

linux-* {
	LIBXML2_DIR = /usr/include/libxml2

	INCLUDEPATH += $${LIBXML2_DIR}

	LIBS += -lcurl -lxml2
}

win32 {
	DEFINES += CURL_STATICLIB LIBXML_STATIC

	CURL_DIR = ../../../curl-7.26.0
	LIBXML2_DIR = ../../../libxml2-2.8.0
	LIBICONV_DIR = ../../../libiconv-1.14

	INCLUDEPATH += $${CURL_DIR}/include $${LIBXML2_DIR}/include $${LIBICONV_DIR}/include

	LIBS += -lcurl -lxml2 -lws2_32 -lwldap32
}
