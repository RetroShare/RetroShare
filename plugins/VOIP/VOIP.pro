!include("../Common/retroshare_plugin.pri"): error("Could not include file ../Common/retroshare_plugin.pri")

CONFIG += qt uic qrc resources
CONFIG += mobility
MOBILITY = multimedia

QT_VERSION = $$[QT_VERSION]
QT_VERSION = $$split(QT_VERSION, ".")
QT_VER_MAJ = $$member(QT_VERSION, 0)
QT_VER_MIN = $$member(QT_VERSION, 1)
 
lessThan(QT_VER_MAJ, 4) | lessThan(QT_VER_MIN, 7) | win32 {
   QT += multimedia
}

INCLUDEPATH += ../../retroshare-gui/src/temp/ui ../../libretroshare/src

#################################### Windows #####################################

win32 {
	# Speex
	INCLUDEPATH += ../../../speex-1.2rc1/include
}

QMAKE_CXXFLAGS *= -Wall

SOURCES = services/p3vors.cc \
			 services/rsvoipitems.cc \
			 gui/AudioInputConfig.cpp \
			 gui/AudioStats.cpp \
			 gui/AudioWizard.cpp \
			 gui/SpeexProcessor.cpp \
			 gui/audiodevicehelper.cpp \
          gui/VoipStatistics.cpp \
          gui/AudioPopupChatDialog.cpp \
          gui/PluginGUIHandler.cpp \
          gui/PluginNotifier.cpp \
          VOIPPlugin.cpp

HEADERS = services/p3vors.h \
			 services/rsvoipitems.h \
          gui/AudioInputConfig.h \
			 gui/AudioStats.h \
			 gui/AudioWizard.h \
			 gui/SpeexProcessor.h \
			 gui/audiodevicehelper.h \
          gui/VoipStatistics.h \
          gui/AudioPopupChatDialog.h \
          gui/PluginGUIHandler.h \
          gui/PluginNotifier.h \
			 interface/rsvoip.h \
          VOIPPlugin.h

FORMS   = gui/AudioInputConfig.ui \
          gui/AudioStats.ui \
          gui/VoipStatistics.ui \
			 gui/AudioWizard.ui

TARGET = VOIP

RESOURCES = gui/VOIP_images.qrc lang/VOIP_lang.qrc

TRANSLATIONS +=  \
            lang/VOIP_cs.ts \
            lang/VOIP_da.ts \
            lang/VOIP_de.ts \
            lang/VOIP_en.ts \
            lang/VOIP_es.ts \
            lang/VOIP_fi.ts \
            lang/VOIP_fr.ts \
            lang/VOIP_hu.ts \
            lang/VOIP_ja_JP.ts \
            lang/VOIP_ko.ts \
            lang/VOIP_pl.ts \
            lang/VOIP_ru.ts \
            lang/VOIP_sv.ts \
            lang/VOIP_tr.ts \
            lang/VOIP_zh_CN.ts

LIBS += -lspeex -lspeexdsp
