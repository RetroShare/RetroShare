#include <QObject>
#include <QGraphicsEffect>
#include <gui/SpeexProcessor.h>
#include <gui/chat/PopupChatDialog.h>
#include <gui/audiodevicehelper.h>

class QToolButton;

#define VOIP_SOUND_INCOMING_CALL "VOIP_incoming_call"

class AudioPopupChatDialogWidgetsHolder: public QObject, public PopupChatDialog_WidgetsHolder
{
	Q_OBJECT

	public:
        AudioPopupChatDialogWidgetsHolder();

        virtual void init(const std::string &peerId, const QString &title, ChatWidget* chatWidget);
        virtual std::vector<QWidget*> getWidgets();
        virtual void updateStatus(int status);

        virtual ~AudioPopupChatDialogWidgetsHolder()
		{
			if(inputDevice != NULL)
				inputDevice->stop() ;
		}

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

        std::string peerId;
        ChatWidget* chatWidget;

		QToolButton *audioListenToggleButton ;
		QToolButton *audioMuteCaptureToggleButton ;
		QToolButton *hangupButton ;

};

