!include("../Common/retroshare_plugin.pri"): error("Could not include file ../Common/retroshare_plugin.pri")

greaterThan(QT_MAJOR_VERSION, 4) {
	# Qt 5
	QT += widgets
}

exists($$[QMAKE_MKSPECS]/features/mobility.prf) {
  CONFIG += mobility
} else {
  QT += multimedia
}
CONFIG += qt uic qrc resources
MOBILITY = multimedia

DEPENDPATH += $$PWD/../../retroshare-gui/src/temp/ui
INCLUDEPATH += $$PWD/../../retroshare-gui/src/temp/ui

#################################### Linux #####################################

linux-* {
	CONFIG += link_pkgconfig

	PKGCONFIG += libavcodec libavutil
	PKGCONFIG += speex speexdsp
	PKGCONFIG += opencv
} else {
	LIBS += -lspeex -lspeexdsp -lavcodec -lavutil
}

#################################### Windows #####################################

win32 {

	DEPENDPATH += . $$INC_DIR
	INCLUDEPATH += . $$INC_DIR

	OPENCV_VERSION = "320"
	USE_PRECOMPILED_LIBS =
	for(lib, LIB_DIR) {
#message(Scanning $$lib)
		exists( $$lib/opencv/libopencv_core$${OPENCV_VERSION}.a) {
			isEmpty(USE_PRECOMPILED_LIBS) {
				message(Get pre-compiled opencv $$OPENCV_VERSION libraries here:)
				message($$lib)
				LIBS += -L"$$lib/opencv"
				LIBS += -lopencv_core$$OPENCV_VERSION -lopencv_highgui$$OPENCV_VERSION -lopencv_imgproc$$OPENCV_VERSION -lopencv_videoio$$OPENCV_VERSION -lopencv_imgcodecs$$OPENCV_VERSION -llibwebp
				USE_PRECOMPILED_LIBS = 1
			}
		}
		exists( $$lib/opencv/libopencv_core.a) {
			isEmpty(USE_PRECOMPILED_LIBS) {
				message(Get pre-compiled opencv libraries here:)
				message($$lib)
				LIBS += -L"$$lib/opencv"
				LIBS += -lopencv_core -lopencv_highgui -lopencv_imgproc
				USE_PRECOMPILED_LIBS = 1
			}
		}
		exists( $$lib/libopencv_core.dll.a) {
			isEmpty(USE_PRECOMPILED_LIBS) {
				message(Get pre-compiled opencv libraries here:)
				message($$lib)
				LIBS += -L"$$lib/opencv"
				LIBS += -lopencv_core -lopencv_highgui -lopencv_imgproc
				USE_PRECOMPILED_LIBS = 1
			}
		}
		exists( $$lib/libopencv_videoio.dll.a) {
			message(videoio found in opencv libraries.)
			message($$lib)
			LIBS += -lopencv_videoio
		}
	}
	isEmpty(USE_PRECOMPILED_LIBS) {
		message(Use system opencv libraries.)
		LIBS += -lopencv_core -lopencv_highgui -lopencv_imgproc
	}
	LIBS += -lz -lole32 -loleaut32 -luuid -lvfw32 -llibjpeg -llibtiff -llibpng -llibjasper -lIlmImf
	LIBS += -lavifil32 -lavicap32 -lavcodec -lavutil -lswresample
}

#################################### MacOSX #####################################

macx {

	DEPENDPATH += . $$INC_DIR
	INCLUDEPATH += . $$INC_DIR

	#OPENCV_VERSION = "249"
	USE_PRECOMPILED_LIBS =
	for(lib, LIB_DIR) {
#message(Scanning $$lib)
		exists( $$lib/opencv/libopencv_core*.dylib) {
			isEmpty(USE_PRECOMPILED_LIBS) {
				message(Get pre-compiled opencv libraries here:)
				message($$lib)
				LIBS += -L"$$lib/opencv"
				LIBS += -lopencv_core -lopencv_highgui -lopencv_imgproc
				USE_PRECOMPILED_LIBS = 1
			}
		}
		exists( $$lib/libopencv_videoio*.dylib) {
			message(videoio found in opencv libraries.)
			message($$lib)
			LIBS += -lopencv_videoio
		}
	}
	isEmpty(USE_PRECOMPILED_LIBS) {
		message(Use system opencv libraries.)
		LIBS += -lopencv_core -lopencv_highgui -lopencv_imgproc
	}
}


# ffmpeg (and libavutil: https://github.com/ffms/ffms2/issues/11)
QMAKE_CXXFLAGS += -D__STDC_CONSTANT_MACROS

QMAKE_CXXFLAGS *= -Wall

SOURCES = VOIPPlugin.cpp               \
          services/p3VOIP.cc           \
          services/rsVOIPItems.cc      \
          gui/AudioInputConfig.cpp     \
          gui/AudioStats.cpp           \
          gui/AudioWizard.cpp          \
          gui/SpeexProcessor.cpp       \
          gui/audiodevicehelper.cpp    \
          gui/VideoProcessor.cpp       \
          gui/QVideoDevice.cpp         \
          gui/VOIPChatWidgetHolder.cpp \
          gui/VOIPGUIHandler.cpp       \
          gui/VOIPNotify.cpp           \
          gui/VOIPToasterItem.cpp      \
          gui/VOIPToasterNotify.cpp

HEADERS = VOIPPlugin.h                 \
          services/p3VOIP.h            \
          services/rsVOIPItems.h       \
          gui/AudioInputConfig.h       \
          gui/AudioStats.h             \
          gui/AudioWizard.h            \
          gui/SpeexProcessor.h         \
          gui/audiodevicehelper.h      \
          gui/VideoProcessor.h         \
          gui/QVideoDevice.h           \
          gui/VOIPChatWidgetHolder.h   \
          gui/VOIPGUIHandler.h         \
          gui/VOIPNotify.h             \
          gui/VOIPToasterItem.h        \
          gui/VOIPToasterNotify.h     \
          interface/rsVOIP.h

FORMS   = gui/AudioInputConfig.ui      \
          gui/AudioStats.ui            \
          gui/AudioWizard.ui           \
          gui/VOIPToasterItem.ui

TARGET = VOIP

RESOURCES = gui/VOIP_images.qrc lang/VOIP_lang.qrc qss/VOIP_qss.qrc

TRANSLATIONS +=  \
            lang/VOIP_ca_ES.ts \
            lang/VOIP_cs.ts \
            lang/VOIP_da.ts \
            lang/VOIP_de.ts \
            lang/VOIP_el.ts \
            lang/VOIP_en.ts \
            lang/VOIP_es.ts \
            lang/VOIP_fi.ts \
            lang/VOIP_fr.ts \
            lang/VOIP_hu.ts \
            lang/VOIP_it.ts \
            lang/VOIP_ja_JP.ts \
            lang/VOIP_ko.ts \
            lang/VOIP_nl.ts \
            lang/VOIP_pl.ts \
            lang/VOIP_ru.ts \
            lang/VOIP_sv.ts \
            lang/VOIP_tr.ts \
            lang/VOIP_zh_CN.ts
