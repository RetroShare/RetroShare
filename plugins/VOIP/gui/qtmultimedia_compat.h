/*******************************************************************************
 * plugins/VOIP/gui/qtmultimedia_compat.h                                      *
 *                                                                             *
 * Copyright (C) 2012 by Retroshare Team <retroshare.project@gmail.com>        *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#pragma once

// QtMultimedia compatibility shim so the VOIP plugin builds against both Qt5
// and Qt6.
//
// In Qt6 the pull-style audio I/O classes were renamed:
//   Qt5 QAudioInput  (reads a capture device into a QIODevice) -> Qt6 QAudioSource
//   Qt5 QAudioOutput (writes a QIODevice to a playback device) -> Qt6 QAudioSink
// Qt6 reuses the names QAudioInput/QAudioOutput for volume-control objects, so we
// cannot alias to those names. The runtime API the plugin actually uses --
// start(QIODevice*), stop(), error() returning QAudio::Error -- is identical
// between the Qt5 and Qt6 classes, so the call sites are unchanged; only the type
// names differ, which we bridge here via RsAudioInput / RsAudioOutput.
//
// Device enumeration also moved: Qt5 QAudioDeviceInfo -> Qt6 QAudioDevice
// enumerated through QMediaDevices (handled directly in audiodevicehelper.cpp).

#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  include <QAudioSource>
#  include <QAudioSink>
#  include <QAudioDevice>
#  include <QMediaDevices>
using RsAudioInput  = QAudioSource;
using RsAudioOutput = QAudioSink;
#else
#  include <QAudioInput>
#  include <QAudioOutput>
#  include <QAudioDeviceInfo>
using RsAudioInput  = QAudioInput;
using RsAudioOutput = QAudioOutput;
#endif
