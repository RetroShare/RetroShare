#include <QObject>
#include <QGraphicsEffect>
#include <gui/SpeexProcessor.h>
#include <gui/chat/PopupChatDialog.h>
#include <gui/audiodevicehelper.h>

class QPushButton;

class AudioPopupChatDialog: public PopupChatDialog
{
	Q_OBJECT

	public:
		AudioPopupChatDialog(QWidget *parent = NULL); 

		virtual ~AudioPopupChatDialog()
		{
			if(inputDevice != NULL)
				inputDevice->stop() ;
		}

	private slots:
		void toggleAudioListen();
		void toggleAudioMuteCapture();

	public slots:
		void sendAudioData();

	protected:
		QAudioInput* inputDevice;
		QAudioOutput* outputDevice;
		QtSpeex::SpeexInputProcessor* inputProcessor;
		QtSpeex::SpeexOutputProcessor* outputProcessor;

		virtual void updateStatus(const QString& peer_id,int status) ;

		void addAudioData(const QString name, QByteArray* array) ;

		QPushButton *audioListenToggleButton ;
		QPushButton *audioMuteCaptureToggleButton ;
};

