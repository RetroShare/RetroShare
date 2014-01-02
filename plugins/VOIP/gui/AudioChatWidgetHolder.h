#include <QObject>
#include <QGraphicsEffect>
#include <gui/SpeexProcessor.h>
#include <gui/chat/ChatWidget.h>

class QToolButton;
class QAudioInput;
class QAudioOutput;

#define VOIP_SOUND_INCOMING_CALL "VOIP_incoming_call"

class AudioChatWidgetHolder : public QObject, public ChatWidgetHolder
{
	Q_OBJECT

public:
	AudioChatWidgetHolder(ChatWidget *chatWidget);
	virtual ~AudioChatWidgetHolder();

	virtual void updateStatus(int status);

	void addAudioData(const QString name, QByteArray* array) ;

private slots:
	void toggleAudioListen();
	void toggleAudioMuteCapture();
	void hangupCall() ;

public slots:
	void sendAudioData();

protected:
	QAudioInput* inputDevice;
	QAudioOutput* outputDevice;
	QtSpeex::SpeexInputProcessor* inputProcessor;
	QtSpeex::SpeexOutputProcessor* outputProcessor;

	QToolButton *audioListenToggleButton ;
	QToolButton *audioMuteCaptureToggleButton ;
	QToolButton *hangupButton ;
};
