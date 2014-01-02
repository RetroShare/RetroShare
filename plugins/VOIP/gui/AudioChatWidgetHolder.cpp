#include <QToolButton>
#include <QPropertyAnimation>
#include <QIcon>

#include "AudioChatWidgetHolder.h"
#include <gui/audiodevicehelper.h>
#include "interface/rsvoip.h"
#include "gui/SoundManager.h"
#include "util/HandleRichText.h"
#include "gui/common/StatusDefs.h"
#include "gui/chat/ChatWidget.h"

#include <retroshare/rsstatus.h>

#define CALL_START ":/images/call-start-22.png"
#define CALL_STOP  ":/images/call-stop-22.png"
#define CALL_HOLD  ":/images/call-hold-22.png"


AudioChatWidgetHolder::AudioChatWidgetHolder(ChatWidget *chatWidget)
	: QObject(), ChatWidgetHolder(chatWidget)
{
	audioListenToggleButton = new QToolButton ;
	audioListenToggleButton->setMinimumSize(QSize(28,28)) ;
	audioListenToggleButton->setMaximumSize(QSize(28,28)) ;
	audioListenToggleButton->setText(QString()) ;
	audioListenToggleButton->setToolTip(tr("Mute yourself"));

	std::cerr << "****** VOIPLugin: Creating new AudioChatWidgetHolder !!" << std::endl;

	QIcon icon ;
	icon.addPixmap(QPixmap(":/images/audio-volume-muted-22.png")) ;
	icon.addPixmap(QPixmap(":/images/audio-volume-medium-22.png"),QIcon::Normal,QIcon::On) ;
	icon.addPixmap(QPixmap(":/images/audio-volume-medium-22.png"),QIcon::Disabled,QIcon::On) ;
	icon.addPixmap(QPixmap(":/images/audio-volume-medium-22.png"),QIcon::Active,QIcon::On) ;
	icon.addPixmap(QPixmap(":/images/audio-volume-medium-22.png"),QIcon::Selected,QIcon::On) ;

	audioListenToggleButton->setIcon(icon) ;
	audioListenToggleButton->setIconSize(QSize(22,22)) ;
	audioListenToggleButton->setAutoRaise(true) ;
	audioListenToggleButton->setCheckable(true);

	audioMuteCaptureToggleButton = new QToolButton ;
	audioMuteCaptureToggleButton->setMinimumSize(QSize(28,28)) ;
	audioMuteCaptureToggleButton->setMaximumSize(QSize(28,28)) ;
	audioMuteCaptureToggleButton->setText(QString()) ;
	audioMuteCaptureToggleButton->setToolTip(tr("Start Call"));

	QIcon icon2 ;
	icon2.addPixmap(QPixmap(":/images/call-start-22.png")) ;
	icon2.addPixmap(QPixmap(":/images/call-hold-22.png"),QIcon::Normal,QIcon::On) ;
	icon2.addPixmap(QPixmap(":/images/call-hold-22.png"),QIcon::Disabled,QIcon::On) ;
	icon2.addPixmap(QPixmap(":/images/call-hold-22.png"),QIcon::Active,QIcon::On) ;
	icon2.addPixmap(QPixmap(":/images/call-hold-22.png"),QIcon::Selected,QIcon::On) ;

	audioMuteCaptureToggleButton->setIcon(icon2) ;
	audioMuteCaptureToggleButton->setIconSize(QSize(22,22)) ;
	audioMuteCaptureToggleButton->setAutoRaise(true) ;
	audioMuteCaptureToggleButton->setCheckable(true) ;
	
	hangupButton = new QToolButton ;
	hangupButton->setIcon(QIcon(":/images/call-stop-22.png")) ;
	hangupButton->setIconSize(QSize(22,22)) ;
	hangupButton->setMinimumSize(QSize(28,28)) ;
	hangupButton->setMaximumSize(QSize(28,28)) ;
	hangupButton->setCheckable(false) ;
	hangupButton->setAutoRaise(true) ;		
	hangupButton->setText(QString()) ;
	hangupButton->setToolTip(tr("Hangup Call"));

	connect(audioListenToggleButton, SIGNAL(clicked()), this , SLOT(toggleAudioListen()));
	connect(audioMuteCaptureToggleButton, SIGNAL(clicked()), this , SLOT(toggleAudioMuteCapture()));
	connect(hangupButton, SIGNAL(clicked()), this , SLOT(hangupCall()));

	mChatWidget->addChatBarWidget(audioListenToggleButton) ;
	mChatWidget->addChatBarWidget(audioMuteCaptureToggleButton) ;
	mChatWidget->addChatBarWidget(hangupButton) ;

	outputProcessor = NULL ;
	outputDevice = NULL ;
	inputProcessor = NULL ;
	inputDevice = NULL ;
}

AudioChatWidgetHolder::~AudioChatWidgetHolder()
{
	if(inputDevice != NULL)
		inputDevice->stop() ;
}

void AudioChatWidgetHolder::toggleAudioListen()
{
	std::cerr << "******** VOIPLugin: Toggling audio listen!" << std::endl;
    if (audioListenToggleButton->isChecked()) {
        audioListenToggleButton->setToolTip(tr("Mute yourself"));
    } else {
        audioListenToggleButton->setToolTip(tr("Unmute yourself"));
        //audioListenToggleButton->setChecked(false);
        /*if (outputDevice) {
            outputDevice->stop();
        }*/
    }
}

void AudioChatWidgetHolder::hangupCall()
{
	std::cerr << "******** VOIPLugin: Hangup call!" << std::endl;

        disconnect(inputProcessor, SIGNAL(networkPacketReady()), this, SLOT(sendAudioData()));
        if (inputDevice) {
            inputDevice->stop();
        }        
        if (outputDevice) {
            outputDevice->stop();
        }
        audioListenToggleButton->setChecked(false);
        audioMuteCaptureToggleButton->setChecked(false);
}

void AudioChatWidgetHolder::toggleAudioMuteCapture()
{
	std::cerr << "******** VOIPLugin: Toggling audio mute capture!" << std::endl;
    if (audioMuteCaptureToggleButton->isChecked()) {
        //activate audio output
        audioListenToggleButton->setChecked(true);
        audioMuteCaptureToggleButton->setToolTip(tr("Hold Call"));

        //activate audio input
        if (!inputProcessor) {
            inputProcessor = new QtSpeex::SpeexInputProcessor();
            if (outputProcessor) {
                connect(outputProcessor, SIGNAL(playingFrame(QByteArray*)), inputProcessor, SLOT(addEchoFrame(QByteArray*)));
            }
            inputProcessor->open(QIODevice::WriteOnly | QIODevice::Unbuffered);
        }
        if (!inputDevice) {
            inputDevice = AudioDeviceHelper::getPreferedInputDevice();
        }
        connect(inputProcessor, SIGNAL(networkPacketReady()), this, SLOT(sendAudioData()));
        inputDevice->start(inputProcessor);
        
        if (mChatWidget) {
         mChatWidget->addChatMsg(true, tr("VoIP Status"), QDateTime::currentDateTime(), QDateTime::currentDateTime(), tr("Outgoing Call is started..."), ChatWidget::MSGTYPE_SYSTEM);
        }
        
    } else {
        disconnect(inputProcessor, SIGNAL(networkPacketReady()), this, SLOT(sendAudioData()));
        if (inputDevice) {
            inputDevice->stop();
        }
        audioMuteCaptureToggleButton->setToolTip(tr("Resume Call"));
    }
}

void AudioChatWidgetHolder::addAudioData(const QString name, QByteArray* array)
{
    if (!audioMuteCaptureToggleButton->isChecked()) {
        //launch an animation. Don't launch it if already animating
        if (!audioMuteCaptureToggleButton->graphicsEffect() ||
            (audioMuteCaptureToggleButton->graphicsEffect()->inherits("QGraphicsOpacityEffect") &&
                ((QGraphicsOpacityEffect*)audioMuteCaptureToggleButton->graphicsEffect())->opacity() == 1)
            ) {
            QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(audioListenToggleButton);
            audioMuteCaptureToggleButton->setGraphicsEffect(effect);
            QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity");
            anim->setStartValue(1);
            anim->setKeyValueAt(0.5,0);
            anim->setEndValue(1);
            anim->setDuration(400);
            anim->start();
        }

//        soundManager->play(VOIP_SOUND_INCOMING_CALL);

        audioMuteCaptureToggleButton->setToolTip(tr("Answer"));

        //TODO make a toaster and a sound for the incoming call
        return;
    }

    if (!outputDevice) {
        outputDevice = AudioDeviceHelper::getDefaultOutputDevice();
    }

    if (!outputProcessor) {
        //start output audio device
        outputProcessor = new QtSpeex::SpeexOutputProcessor();
        if (inputProcessor) {
            connect(outputProcessor, SIGNAL(playingFrame(QByteArray*)), inputProcessor, SLOT(addEchoFrame(QByteArray*)));
        }
        outputProcessor->open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        outputDevice->start(outputProcessor);
    }

    if (outputDevice && outputDevice->error() != QAudio::NoError) {
        std::cerr << "Restarting output device. Error before reset " << outputDevice->error() << " buffer size : " << outputDevice->bufferSize() << std::endl;
        outputDevice->stop();
        outputDevice->reset();
        if (outputDevice->error() == QAudio::UnderrunError)
            outputDevice->setBufferSize(20);
        outputDevice->start(outputProcessor);
    }
    outputProcessor->putNetworkPacket(name, *array);

    //check the input device for errors
    if (inputDevice && inputDevice->error() != QAudio::NoError) {
        std::cerr << "Restarting input device. Error before reset " << inputDevice->error() << std::endl;
        inputDevice->stop();
        inputDevice->reset();
        inputDevice->start(inputProcessor);
    }
}

void AudioChatWidgetHolder::sendAudioData()
{
    while(inputProcessor && inputProcessor->hasPendingPackets()) {
        QByteArray qbarray = inputProcessor->getNetworkPacket();
        RsVoipDataChunk chunk;
        chunk.size = qbarray.size();
        chunk.data = (void*)qbarray.constData();
        rsVoip->sendVoipData(mChatWidget->getPeerId(),chunk);
    }
}

void AudioChatWidgetHolder::updateStatus(int status)
{
	audioListenToggleButton->setEnabled(true);
	audioMuteCaptureToggleButton->setEnabled(true);
	hangupButton->setEnabled(true);
	
	switch (status) {
	case RS_STATUS_OFFLINE:
		audioListenToggleButton->setEnabled(false);
		audioMuteCaptureToggleButton->setEnabled(false);
		hangupButton->setEnabled(false);
		break;
	}
}
