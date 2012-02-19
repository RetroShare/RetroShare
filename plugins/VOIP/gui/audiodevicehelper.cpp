#include "audiodevicehelper.h"
#include <iostream>

AudioDeviceHelper::AudioDeviceHelper()
{
}

QAudioInput* AudioDeviceHelper::getDefaultInputDevice() {
    QAudioFormat fmt;
    fmt.setFrequency(16000);
    fmt.setChannels(1);
    fmt.setSampleSize(16);
    fmt.setSampleType(QAudioFormat::SignedInt);
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setCodec("audio/pcm");

    QAudioDeviceInfo it, dev;

    dev = QAudioDeviceInfo::defaultInputDevice();
    if (dev.deviceName() != "pulse") {
        foreach(it, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
            if(it.deviceName() == "pulse") {
                    dev = it;
                    qDebug("Ok.");
                    break;
            }
        }
    }
    if (dev.deviceName() == "null") {
        foreach(it, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
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
    fmt.setFrequency(16000);
    fmt.setChannels(1);
    fmt.setSampleSize(16);
    fmt.setSampleType(QAudioFormat::SignedInt);
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setCodec("audio/pcm");

    QAudioDeviceInfo it, dev;
    dev = QAudioDeviceInfo::defaultOutputDevice();
    if (dev.deviceName() != "pulse") {
        foreach(it, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
                if(it.deviceName() == "pulse") {
                        dev = it;
                        break;
                }
        }
    }
    if (dev.deviceName() == "null") {
        foreach(it, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
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
