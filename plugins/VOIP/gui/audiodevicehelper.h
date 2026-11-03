/*******************************************************************************
 * plugins/VOIP/gui/audiodevicehelper.h                                        *
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

#include <QList>
#include <QString>
#include "gui/qtmultimedia_compat.h"

class AudioDeviceHelper
{
public:
    AudioDeviceHelper();

    // System default devices.
    static RsAudioInput* getDefaultInputDevice();
    static RsAudioOutput* getDefaultOutputDevice();

    // Devices chosen by the user (rsVOIP settings, see the audio/video wizard).
    // Fall back to the default devices above when nothing was chosen or when the
    // chosen device is not plugged in.
    static RsAudioInput* getPreferedInputDevice();
    static RsAudioOutput* getPreferedOutputDevice();

    // ---- Device selection ----
    // A device is identified by a key string (saved in the settings):
    //   Qt5: QAudioDeviceInfo::deviceName()   Qt6: QAudioDevice::id()
    // An empty key means "system default".

    static QString deviceKey(const RsAudioDevice& dev);
    static QString deviceLabel(const RsAudioDevice& dev);	// human readable name

    // Available devices, without duplicates and without the "null" device.
    static QList<RsAudioDevice> availableInputDevices();
    static QList<RsAudioDevice> availableOutputDevices();

    // Device for a key. Empty or unknown key: the system default device.
    static RsAudioDevice inputDeviceInfo(const QString& key);
    static RsAudioDevice outputDeviceInfo(const QString& key);

    // Create a device from a key (empty/unknown key: same as getDefault*Device()).
    static RsAudioInput* createInputDevice(const QString& key, QObject *parent = nullptr);
    static RsAudioOutput* createOutputDevice(const QString& key, QObject *parent = nullptr);

    // 16 kHz, mono, 16 bit signed: the format used by the speex processors.
    static QAudioFormat voipAudioFormat();
};
