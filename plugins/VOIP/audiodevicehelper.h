#ifndef AUDIODEVICEHELPER_H
#define AUDIODEVICEHELPER_H
#include <QtMultimediaKit/QAudioInput>
#include <QtMultimediaKit/QAudioOutput>

class AudioDeviceHelper
{
public:
    AudioDeviceHelper();
    static QAudioInput* getDefaultInputDevice();
    static QAudioInput* getPreferedInputDevice();
    //static list getInputDeviceList();

    static QAudioOutput* getDefaultOutputDevice();
    static QAudioOutput* getPreferedOutputDevice();
    //static list getOutputDeviceList();
};

#endif // AUDIODEVICEHELPER_H
