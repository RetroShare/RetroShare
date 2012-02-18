!include("../Common/retroshare_plugin.pri"): error("Could not include file ../Common/retroshare_plugin.pri")

CONFIG += qt uic qrc resources
CONFIG += mobility
QT += multimedia
MOBILITY = multimedia

SOURCES = VOIPPlugin.cpp AudioInputConfig.cpp  AudioStats.cpp  AudioWizard.cpp  SpeexProcessor.cpp  audiodevicehelper.cpp
HEADERS = AudioInputConfig.h  AudioStats.h  AudioWizard.h  SpeexProcessor.h  audiodevicehelper.h
FORMS   = AudioInputConfig.ui AudioStats.ui AudioWizard.ui

TARGET = VOIP

RESOURCES = VOIP_images.qrc 

LIBS += -lspeex -lspeexdsp
