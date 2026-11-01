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
#include "interface/rsVOIP.h"

#include <QSet>
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
    QString key;
    if(rsVOIP)
        key = QString::fromStdString(rsVOIP->getVoipAudioInputDevice());
    return createInputDevice(key);
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
    QString key;
    if(rsVOIP)
        key = QString::fromStdString(rsVOIP->getVoipAudioOutputDevice());
    return createOutputDevice(key);
}

// =====================================================================================
//                                   Device selection
// =====================================================================================

QAudioFormat AudioDeviceHelper::voipAudioFormat()
{
    QAudioFormat fmt;
    fmt.setSampleRate(16000);
    fmt.setChannelCount(1);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    fmt.setSampleFormat(QAudioFormat::Int16);
#else
    fmt.setSampleSize(16);
    fmt.setSampleType(QAudioFormat::SignedInt);
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setCodec("audio/pcm");
#endif
    return fmt;
}

QString AudioDeviceHelper::deviceKey(const RsAudioDevice& dev)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return QString::fromUtf8(dev.id());
#else
    return dev.deviceName();
#endif
}

QString AudioDeviceHelper::deviceLabel(const RsAudioDevice& dev)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return dev.description();
#else
    return dev.deviceName();
#endif
}

static QList<RsAudioDevice> systemDevices(bool input)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return input ? QMediaDevices::audioInputs() : QMediaDevices::audioOutputs();
#else
    return QAudioDeviceInfo::availableDevices(input ? QAudio::AudioInput : QAudio::AudioOutput);
#endif
}

static QList<RsAudioDevice> filteredDevices(bool input)
{
    QList<RsAudioDevice> result;
    QSet<QString> seen;

    for(const RsAudioDevice& dev : systemDevices(input))
    {
        const QString key = AudioDeviceHelper::deviceKey(dev);

        if(dev.isNull() || key.isEmpty() || key == "null" || seen.contains(key))
            continue;

        seen.insert(key);
        result.push_back(dev);
    }
    return result;
}

QList<RsAudioDevice> AudioDeviceHelper::availableInputDevices()  { return filteredDevices(true);  }
QList<RsAudioDevice> AudioDeviceHelper::availableOutputDevices() { return filteredDevices(false); }

static RsAudioDevice findDevice(bool input, const QString& key)
{
    if(!key.isEmpty())
        for(const RsAudioDevice& dev : systemDevices(input))
            if(AudioDeviceHelper::deviceKey(dev) == key)
                return dev;

    return RsAudioDevice();
}

RsAudioDevice AudioDeviceHelper::inputDeviceInfo(const QString& key)
{
    RsAudioDevice dev = findDevice(true, key);

    if(dev.isNull())
    {
        // the default device, chosen the same way as getDefaultInputDevice()
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        dev = QMediaDevices::defaultAudioInput();
#else
        dev = QAudioDeviceInfo::defaultInputDevice();
        for(const RsAudioDevice& d : systemDevices(true))
            if(d.deviceName() == "pulse") { dev = d; break; }
#endif
    }
    return dev;
}

RsAudioDevice AudioDeviceHelper::outputDeviceInfo(const QString& key)
{
    RsAudioDevice dev = findDevice(false, key);

    if(dev.isNull())
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        dev = QMediaDevices::defaultAudioOutput();
#else
        dev = QAudioDeviceInfo::defaultOutputDevice();
        for(const RsAudioDevice& d : systemDevices(false))
            if(d.deviceName() == "pulse") { dev = d; break; }
#endif
    }
    return dev;
}

RsAudioInput* AudioDeviceHelper::createInputDevice(const QString& key, QObject *parent)
{
    RsAudioDevice dev = findDevice(true, key);
    RsAudioInput *res;

    if(dev.isNull())
    {
        if(!key.isEmpty())
            std::cerr << "VOIP: audio input device \"" << key.toStdString() << "\" not found, using default." << std::endl;
        res = getDefaultInputDevice();
    }
    else
    {
        std::cerr << "input device : " << deviceLabel(dev).toStdString() << std::endl;
        res = new RsAudioInput(dev, voipAudioFormat());
    }
    res->setParent(parent);
    return res;
}

RsAudioOutput* AudioDeviceHelper::createOutputDevice(const QString& key, QObject *parent)
{
    RsAudioDevice dev = findDevice(false, key);
    RsAudioOutput *res;

    if(dev.isNull())
    {
        if(!key.isEmpty())
            std::cerr << "VOIP: audio output device \"" << key.toStdString() << "\" not found, using default." << std::endl;
        res = getDefaultOutputDevice();
    }
    else
    {
        std::cerr << "output device : " << deviceLabel(dev).toStdString() << std::endl;
        res = new RsAudioOutput(dev, voipAudioFormat());
    }
    res->setParent(parent);
    return res;
}
