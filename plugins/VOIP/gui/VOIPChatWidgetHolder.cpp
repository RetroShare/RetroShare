#include <QToolButton>
#include <QPropertyAnimation>
#include <QIcon>
#include <QLayout>

#include <gui/audiodevicehelper.h>
#include "interface/rsvoip.h"
#include "gui/SoundManager.h"
#include "util/HandleRichText.h"
#include "gui/common/StatusDefs.h"
#include "gui/chat/ChatWidget.h"

#include "VOIPChatWidgetHolder.h"
#include "VideoProcessor.h"
#include "QVideoDevice.h"

#include <retroshare/rsstatus.h>

#define CALL_START ":/images/call-start-22.png"
#define CALL_STOP  ":/images/call-stop-22.png"
#define CALL_HOLD  ":/images/call-hold-22.png"


VOIPChatWidgetHolder::VOIPChatWidgetHolder(ChatWidget *chatWidget)
	: QObject(), ChatWidgetHolder(chatWidget)
{
	std::cerr << "****** VOIPLugin: Creating new VOIPChatWidgetHolder !!" << std::endl;

	QIcon icon ;
	icon.addPixmap(QPixmap(":/images/audio-volume-muted-22.png")) ;
	icon.addPixmap(QPixmap(":/images/audio-volume-medium-22.png"),QIcon::Normal,QIcon::On) ;
	icon.addPixmap(QPixmap(":/images/audio-volume-medium-22.png"),QIcon::Disabled,QIcon::On) ;
	icon.addPixmap(QPixmap(":/images/audio-volume-medium-22.png"),QIcon::Active,QIcon::On) ;
	icon.addPixmap(QPixmap(":/images/audio-volume-medium-22.png"),QIcon::Selected,QIcon::On) ;

	audioListenToggleButton = new QToolButton ;
	audioListenToggleButton->setIcon(icon) ;
	audioListenToggleButton->setIconSize(QSize(22,22)) ;
	audioListenToggleButton->setAutoRaise(true) ;
	audioListenToggleButton->setCheckable(true);
	audioListenToggleButton->setMinimumSize(QSize(28,28)) ;
	audioListenToggleButton->setMaximumSize(QSize(28,28)) ;
	audioListenToggleButton->setText(QString()) ;
	audioListenToggleButton->setToolTip(tr("Mute"));

	QIcon icon2 ;
	icon2.addPixmap(QPixmap(":/images/call-start-22.png")) ;
	icon2.addPixmap(QPixmap(":/images/call-hold-22.png"),QIcon::Normal,QIcon::On) ;
	icon2.addPixmap(QPixmap(":/images/call-hold-22.png"),QIcon::Disabled,QIcon::On) ;
	icon2.addPixmap(QPixmap(":/images/call-hold-22.png"),QIcon::Active,QIcon::On) ;
	icon2.addPixmap(QPixmap(":/images/call-hold-22.png"),QIcon::Selected,QIcon::On) ;

	audioCaptureToggleButton = new QToolButton ;
	audioCaptureToggleButton->setMinimumSize(QSize(28,28)) ;
	audioCaptureToggleButton->setMaximumSize(QSize(28,28)) ;
	audioCaptureToggleButton->setText(QString()) ;
	audioCaptureToggleButton->setToolTip(tr("Start Call"));
	audioCaptureToggleButton->setIcon(icon2) ;
	audioCaptureToggleButton->setIconSize(QSize(22,22)) ;
	audioCaptureToggleButton->setAutoRaise(true) ;
	audioCaptureToggleButton->setCheckable(true) ;

	QIcon icon3 ;
	icon3.addPixmap(QPixmap(":/images/camera-on.png")) ;
	icon3.addPixmap(QPixmap(":/images/camera-off.png"),QIcon::Normal,QIcon::On) ;
	icon3.addPixmap(QPixmap(":/images/camera-off.png"),QIcon::Disabled,QIcon::On) ;
	icon3.addPixmap(QPixmap(":/images/camera-off.png"),QIcon::Active,QIcon::On) ;
	icon3.addPixmap(QPixmap(":/images/camera-off.png"),QIcon::Selected,QIcon::On) ;

	videoCaptureToggleButton = new QToolButton ;
	videoCaptureToggleButton->setMinimumSize(QSize(28,28)) ;
	videoCaptureToggleButton->setMaximumSize(QSize(28,28)) ;
	videoCaptureToggleButton->setText(QString()) ;
	videoCaptureToggleButton->setToolTip(tr("Start Call"));
	videoCaptureToggleButton->setIcon(icon3) ;
	videoCaptureToggleButton->setIconSize(QSize(22,22)) ;
	videoCaptureToggleButton->setAutoRaise(true) ;
	videoCaptureToggleButton->setCheckable(true) ;
	
	hangupButton = new QToolButton ;
	hangupButton->setIcon(QIcon(":/images/call-stop-22.png")) ;
	hangupButton->setIconSize(QSize(22,22)) ;
	hangupButton->setMinimumSize(QSize(28,28)) ;
	hangupButton->setMaximumSize(QSize(28,28)) ;
	hangupButton->setCheckable(false) ;
	hangupButton->setAutoRaise(true) ;		
	hangupButton->setText(QString()) ;
	hangupButton->setToolTip(tr("Hangup Call"));

	connect(videoCaptureToggleButton, SIGNAL(clicked()), this , SLOT(toggleVideoCapture()));
	connect(audioListenToggleButton, SIGNAL(clicked()), this , SLOT(toggleAudioListen()));
	connect(audioCaptureToggleButton, SIGNAL(clicked()), this , SLOT(toggleAudioCapture()));
	connect(hangupButton, SIGNAL(clicked()), this , SLOT(hangupCall()));

	mChatWidget->addChatBarWidget(audioListenToggleButton) ;
	mChatWidget->addChatBarWidget(audioCaptureToggleButton) ;
	mChatWidget->addChatBarWidget(videoCaptureToggleButton) ;
	mChatWidget->addChatBarWidget(hangupButton) ;

	outputAudioProcessor = NULL ;
	outputAudioDevice = NULL ;
	inputAudioProcessor = NULL ;
	inputAudioDevice = NULL ;

	inputVideoDevice = new QVideoInputDevice(mChatWidget) ; // not started yet ;-)
	inputVideoProcessor = new JPEGVideoEncoder ;
	outputVideoProcessor = new JPEGVideoDecoder ;

	// Make a widget with two video devices, one for echo, and one for the talking peer.
	videoWidget = new QWidget(mChatWidget) ;
	videoWidget->setLayout(new QHBoxLayout()) ;
	videoWidget->layout()->addWidget(echoVideoDevice = new QVideoOutputDevice(videoWidget)) ;
	videoWidget->layout()->addWidget(outputVideoDevice = new QVideoOutputDevice(videoWidget)) ;

	echoVideoDevice->setMinimumSize(128,95) ;
	outputVideoDevice->setMinimumSize(128,95) ;

	mChatWidget->addChatHorizontalWidget(videoWidget) ;

	inputVideoDevice->setEchoVideoTarget(echoVideoDevice) ;
	outputVideoProcessor->setDisplayTarget(outputVideoDevice) ;
}

VOIPChatWidgetHolder::~VOIPChatWidgetHolder()
{
	if(inputAudioDevice != NULL)
		inputAudioDevice->stop() ;

	delete inputVideoDevice ;
	delete inputVideoProcessor ;
	delete outputVideoProcessor ;
}

void VOIPChatWidgetHolder::toggleAudioListen()
{
	std::cerr << "******** VOIPLugin: Toggling audio listen!" << std::endl;
    if (audioListenToggleButton->isChecked()) {
        audioListenToggleButton->setToolTip(tr("Mute yourself"));
    } else {
        audioListenToggleButton->setToolTip(tr("Unmute yourself"));
        //audioListenToggleButton->setChecked(false);
        /*if (outputAudioDevice) {
            outputAudioDevice->stop();
        }*/
    }
}

void VOIPChatWidgetHolder::hangupCall()
{
	std::cerr << "******** VOIPLugin: Hangup call!" << std::endl;

        disconnect(inputAudioProcessor, SIGNAL(networkPacketReady()), this, SLOT(sendAudioData()));
        if (inputAudioDevice) {
            inputAudioDevice->stop();
        }        
        if (outputAudioDevice) {
            outputAudioDevice->stop();
        }
        audioListenToggleButton->setChecked(false);
        audioCaptureToggleButton->setChecked(false);
}

void VOIPChatWidgetHolder::toggleAudioCapture()
{
	std::cerr << "******** VOIPLugin: Toggling audio mute capture!" << std::endl;
    if (audioCaptureToggleButton->isChecked()) {
        //activate audio output
        audioListenToggleButton->setChecked(true);
        audioCaptureToggleButton->setToolTip(tr("Hold Call"));

        //activate audio input
        if (!inputAudioProcessor) {
            inputAudioProcessor = new QtSpeex::SpeexInputProcessor();
            if (outputAudioProcessor) {
                connect(outputAudioProcessor, SIGNAL(playingFrame(QByteArray*)), inputAudioProcessor, SLOT(addEchoFrame(QByteArray*)));
            }
            inputAudioProcessor->open(QIODevice::WriteOnly | QIODevice::Unbuffered);
        }
        if (!inputAudioDevice) {
            inputAudioDevice = AudioDeviceHelper::getPreferedInputDevice();
        }
        connect(inputAudioProcessor, SIGNAL(networkPacketReady()), this, SLOT(sendAudioData()));
        inputAudioDevice->start(inputAudioProcessor);
        
        if (mChatWidget) {
         mChatWidget->addChatMsg(true, tr("VoIP Status"), QDateTime::currentDateTime(), QDateTime::currentDateTime(), tr("Outgoing Call is started..."), ChatWidget::MSGTYPE_SYSTEM);
        }
        
    } else {
        disconnect(inputAudioProcessor, SIGNAL(networkPacketReady()), this, SLOT(sendAudioData()));
        if (inputAudioDevice) {
            inputAudioDevice->stop();
        }
        audioCaptureToggleButton->setToolTip(tr("Resume Call"));
    }
}
void VOIPChatWidgetHolder::toggleVideoCapture()
{
	std::cerr << "******** VOIPLugin: Toggling video capture!" << std::endl;

	if (videoCaptureToggleButton->isChecked()) 
	{
		//activate video input
		//
		inputVideoDevice->start() ;

		videoCaptureToggleButton->setToolTip(tr("Shut camera off"));

		if (mChatWidget) 
			mChatWidget->addChatMsg(true, tr("VoIP Status"), QDateTime::currentDateTime(), QDateTime::currentDateTime(), tr("you're now sending video..."), ChatWidget::MSGTYPE_SYSTEM);
	} 
	else 
	{
		if(inputVideoDevice) 
		{
			delete inputVideoDevice ;
			inputVideoDevice = NULL ;
		}

		videoCaptureToggleButton->setToolTip(tr("Activate camera"));
	}
}

void VOIPChatWidgetHolder::addAudioData(const QString name, QByteArray* array)
{
    if (!audioCaptureToggleButton->isChecked()) {
        //launch an animation. Don't launch it if already animating
        if (!audioCaptureToggleButton->graphicsEffect() ||
            (audioCaptureToggleButton->graphicsEffect()->inherits("QGraphicsOpacityEffect") &&
                ((QGraphicsOpacityEffect*)audioCaptureToggleButton->graphicsEffect())->opacity() == 1)
            ) {
            QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(audioListenToggleButton);
            audioCaptureToggleButton->setGraphicsEffect(effect);
            QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity");
            anim->setStartValue(1);
            anim->setKeyValueAt(0.5,0);
            anim->setEndValue(1);
            anim->setDuration(400);
            anim->start();
        }

//        soundManager->play(VOIP_SOUND_INCOMING_CALL);

        audioCaptureToggleButton->setToolTip(tr("Answer"));

        //TODO make a toaster and a sound for the incoming call
        return;
    }

    if (!outputAudioDevice) {
        outputAudioDevice = AudioDeviceHelper::getDefaultOutputDevice();
    }

    if (!outputAudioProcessor) {
        //start output audio device
        outputAudioProcessor = new QtSpeex::SpeexOutputProcessor();
        if (inputAudioProcessor) {
            connect(outputAudioProcessor, SIGNAL(playingFrame(QByteArray*)), inputAudioProcessor, SLOT(addEchoFrame(QByteArray*)));
        }
        outputAudioProcessor->open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        outputAudioDevice->start(outputAudioProcessor);
    }

    if (outputAudioDevice && outputAudioDevice->error() != QAudio::NoError) {
        std::cerr << "Restarting output device. Error before reset " << outputAudioDevice->error() << " buffer size : " << outputAudioDevice->bufferSize() << std::endl;
        outputAudioDevice->stop();
        outputAudioDevice->reset();
        if (outputAudioDevice->error() == QAudio::UnderrunError)
            outputAudioDevice->setBufferSize(20);
        outputAudioDevice->start(outputAudioProcessor);
    }
    outputAudioProcessor->putNetworkPacket(name, *array);

    //check the input device for errors
    if (inputAudioDevice && inputAudioDevice->error() != QAudio::NoError) {
        std::cerr << "Restarting input device. Error before reset " << inputAudioDevice->error() << std::endl;
        inputAudioDevice->stop();
        inputAudioDevice->reset();
        inputAudioDevice->start(inputAudioProcessor);
    }
}

void VOIPChatWidgetHolder::sendAudioData()
{
    while(inputAudioProcessor && inputAudioProcessor->hasPendingPackets()) {
        QByteArray qbarray = inputAudioProcessor->getNetworkPacket();
        RsVoipDataChunk chunk;
        chunk.size = qbarray.size();
        chunk.data = (void*)qbarray.constData();
        rsVoip->sendVoipData(mChatWidget->getPeerId(),chunk);
    }
}

void VOIPChatWidgetHolder::updateStatus(int status)
{
	audioListenToggleButton->setEnabled(true);
	audioCaptureToggleButton->setEnabled(true);
	hangupButton->setEnabled(true);
	
	switch (status) {
	case RS_STATUS_OFFLINE:
		audioListenToggleButton->setEnabled(false);
		audioCaptureToggleButton->setEnabled(false);
		hangupButton->setEnabled(false);
		break;
	}
}
