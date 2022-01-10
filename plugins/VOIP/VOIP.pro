################################################################################
# VOIP.pro                                                                     #
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
TARGET = VOIP
TARGET_PRL = VOIP
DESTDIR = lib

target.files = lib/libVOIP.so

DEPENDPATH += $$PWD/../../retroshare-gui/src/temp/ui
INCLUDEPATH += $$PWD/../../retroshare-gui/src/temp/ui

# when rapidjson is mainstream on all distribs, we will not need the sources anymore
# in the meantime, they are part of the RS directory so that it is always possible to find them

INCLUDEPATH += ../../rapidjson-1.1.0


#################################### Linux #####################################

linux-* {
	CONFIG += link_pkgconfig

	PKGCONFIG += libavcodec libavutil
	PKGCONFIG += speex speexdsp
} else {
	LIBS += -lspeex -lspeexdsp -lavcodec -lavutil
}

#################################### Windows #####################################

win32 {

	DEPENDPATH += . $$INC_DIR
	INCLUDEPATH += . $$INC_DIR
}

#################################### MacOSX #####################################

macx {

	DEPENDPATH += . $$INC_DIR
	INCLUDEPATH += . $$INC_DIR
}


# ffmpeg (and libavutil: https://github.com/ffms/ffms2/issues/11)
QMAKE_CXXFLAGS += -D__STDC_CONSTANT_MACROS

SOURCES = VOIPPlugin.cpp               \
          gui/VOIPConfigPanel.cpp \
          services/p3VOIP.cc           \
          services/rsVOIPItems.cc      \
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
          gui/VOIPConfigPanel.h \
          services/p3VOIP.h            \
          services/rsVOIPItems.h       \
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

FORMS   =      \
          gui/AudioStats.ui            \
          gui/AudioWizard.ui           \
          gui/VOIPConfigPanel.ui \
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
