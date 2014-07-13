#include <QObject>
#include <QGraphicsEffect>
#include <gui/SpeexProcessor.h>
#include <gui/chat/ChatWidget.h>

class QToolButton;
class QAudioInput;
class QAudioOutput;
class QVideoInputDevice ;
class QVideoOutputDevice ;
class VideoEncoder ;
class VideoDecoder ;

#define VOIP_SOUND_INCOMING_CALL "VOIP_incoming_call"

class VOIPChatWidgetHolder : public QObject, public ChatWidgetHolder
{
	Q_OBJECT

public:
	VOIPChatWidgetHolder(ChatWidget *chatWidget);
	virtual ~VOIPChatWidgetHolder();

	virtual void updateStatus(int status);

	void addAudioData(const QString name, QByteArray* array) ;
	void addVideoData(const QString name, QByteArray* array) ;

private slots:
	void toggleAudioListen();
	void toggleAudioCapture();
	void toggleVideoCapture();
	void hangupCall() ;

public slots:
	void sendAudioData();

protected:
	// Audio input/output
	QAudioInput* inputAudioDevice;
	QAudioOutput* outputAudioDevice;

	QtSpeex::SpeexInputProcessor* inputAudioProcessor;
	QtSpeex::SpeexOutputProcessor* outputAudioProcessor;

	// Video input/output
	QVideoOutputDevice *outputVideoDevice;
	QVideoOutputDevice *echoVideoDevice;
	QVideoInputDevice *inputVideoDevice;

	QWidget *videoWidget ;	// pointer to call show/hide

	VideoEncoder *inputVideoProcessor;
	VideoDecoder *outputVideoProcessor;

	// Additional buttons to the chat bar
	QToolButton *audioListenToggleButton ;
	QToolButton *audioCaptureToggleButton ;
	QToolButton *videoCaptureToggleButton ;
	QToolButton *hangupButton ;
};

