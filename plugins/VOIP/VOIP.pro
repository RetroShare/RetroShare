!include("../Common/retroshare_plugin.pri"): error("Could not include file ../Common/retroshare_plugin.pri")

CONFIG += qt uic qrc resources
CONFIG += mobility
#/QT += multimedia
MOBILITY = multimedia

QMAKE_CXXFLAGS *= -Wall

SOURCES = p3Voip.cpp VOIPPlugin.cpp AudioInputConfig.cpp  AudioStats.cpp  AudioWizard.cpp  SpeexProcessor.cpp  audiodevicehelper.cpp
HEADERS = p3Voip.h AudioInputConfig.h  AudioStats.h  AudioWizard.h  SpeexProcessor.h  audiodevicehelper.h
FORMS   = AudioInputConfig.ui AudioStats.ui AudioWizard.ui

TARGET = VOIP

RESOURCES = VOIP_images.qrc 

LIBS += -lspeex -lspeexdsp
