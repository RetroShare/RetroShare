/*******************************************************************************
 * plugins/VOIP/gui/audiodevicehelper.cpp                                      *
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

#include "audiodevicehelper.h"
#include <iostream>

AudioDeviceHelper::AudioDeviceHelper()
{
}

RsAudioInput* AudioDeviceHelper::getDefaultInputDevice()
{
    QAudioFormat fmt;
    fmt.setSampleRate(16000);
    fmt.setChannelCount(1);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt6 replaced setSampleSize()+setSampleType() (and dropped byteOrder/codec)
    // with a single sample-format enum; 16-bit signed PCM is QAudioFormat::Int16.
    fmt.setSampleFormat(QAudioFormat::Int16);

    // Qt6: QAudioDeviceInfo -> QAudioDevice, enumerated via QMediaDevices. The old
    // "pulse"/"null" deviceName() heuristic was PulseAudio-backend specific and has
    // no Qt6 equivalent (QAudioDevice exposes description()/id(), not the backend
    // node name), so we take the system default input directly.
    QAudioDevice dev = QMediaDevices::defaultAudioInput();
    std::cerr << "input device : " << dev.description().toStdString() << std::endl;
#else
    fmt.setSampleSize(16);
    fmt.setSampleType(QAudioFormat::SignedInt);
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setCodec("audio/pcm");

    QAudioDeviceInfo it, dev;
    QList<QAudioDeviceInfo> input_list = QAudioDeviceInfo::availableDevices(QAudio::AudioInput) ;

    dev = QAudioDeviceInfo::defaultInputDevice();
    if (dev.deviceName() != "pulse") {
        foreach(it, input_list) {
            if(it.deviceName() == "pulse") {
                    dev = it;
                    qDebug("Ok.");
                    break;
            }
        }
    }
    if (dev.deviceName() == "null") {
        foreach(it, input_list) {
            if(it.deviceName() != "null") {
                    dev = it;
                    break;
            }
        }
    }
    std::cerr << "input device : " << dev.deviceName().toStdString() << std::endl;
#endif
    return new RsAudioInput(dev, fmt);
}
RsAudioInput* AudioDeviceHelper::getPreferedInputDevice() {
    return AudioDeviceHelper::getDefaultInputDevice();
}

RsAudioOutput* AudioDeviceHelper::getDefaultOutputDevice() {
    QAudioFormat fmt;
    fmt.setSampleRate(16000);
    fmt.setChannelCount(1);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    fmt.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice dev = QMediaDevices::defaultAudioOutput();
    std::cerr << "output device : " << dev.description().toStdString() << std::endl;
#else
    fmt.setSampleSize(16);
    fmt.setSampleType(QAudioFormat::SignedInt);
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setCodec("audio/pcm");

    QList<QAudioDeviceInfo> list_output = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput) ;

    QAudioDeviceInfo it, dev;
    dev = QAudioDeviceInfo::defaultOutputDevice();

    if (dev.deviceName() != "pulse") {
        foreach(it, list_output) {
                if(it.deviceName() == "pulse") {
                        dev = it;
                        break;
                }
        }
    }
    if (dev.deviceName() == "null") {
        foreach(it, list_output) {
                if(it.deviceName() != "null") {
                        dev = it;
                        break;
                }
        }
    }
    std::cerr << "output device : " << dev.deviceName().toStdString() << std::endl;
#endif
    return new RsAudioOutput(dev, fmt);
}
RsAudioOutput* AudioDeviceHelper::getPreferedOutputDevice() {
    return AudioDeviceHelper::getDefaultOutputDevice();
}
