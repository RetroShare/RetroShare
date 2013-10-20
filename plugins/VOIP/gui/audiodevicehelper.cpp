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
