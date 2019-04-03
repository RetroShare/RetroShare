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

QAudioInput* AudioDeviceHelper::getDefaultInputDevice() 
{
    QAudioFormat fmt;
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
    fmt.setSampleRate(16000);
    fmt.setChannelCount(1);
#else
    fmt.setFrequency(16000);
    fmt.setChannels(1);
#endif
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
    return new QAudioInput(dev, fmt);
}
QAudioInput* AudioDeviceHelper::getPreferedInputDevice() {
    return AudioDeviceHelper::getDefaultInputDevice();
}

QAudioOutput* AudioDeviceHelper::getDefaultOutputDevice() {
    QAudioFormat fmt;
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
    fmt.setSampleRate(16000);
    fmt.setChannelCount(1);
#else
    fmt.setFrequency(16000);
    fmt.setChannels(1);
#endif
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
    return new QAudioOutput(dev, fmt);
}
QAudioOutput* AudioDeviceHelper::getPreferedOutputDevice() {
    return AudioDeviceHelper::getDefaultOutputDevice();
}
