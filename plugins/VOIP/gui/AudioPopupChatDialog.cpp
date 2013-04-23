#include <QPushButton>
#include <QPropertyAnimation>
#include <QIcon>
#include "AudioPopupChatDialog.h"
#include "interface/rsvoip.h"
#include "gui/SoundManager.h"

AudioPopupChatDialog::AudioPopupChatDialog(QWidget *parent)
	: PopupChatDialog(parent)
{
	audioListenToggleButton = new QPushButton ;
	audioListenToggleButton->setMinimumSize(QSize(28,28)) ;
	audioListenToggleButton->setMaximumSize(QSize(28,28)) ;
	audioListenToggleButton->setText(QString()) ;
	audioListenToggleButton->setToolTip(tr("Mute yourself"));

	std::cerr << "****** VOIPLugin: Creating new AudioPopupChatDialog !!" << std::endl;

	QIcon icon ;
        icon.addPixmap(QPixmap(":/images/deafened_self.svg")) ;
	icon.addPixmap(QPixmap(":/images/self_undeafened.svg"),QIcon::Normal,QIcon::On) ;
	icon.addPixmap(QPixmap(":/images/self_undeafened.svg"),QIcon::Disabled,QIcon::On) ;
	icon.addPixmap(QPixmap(":/images/self_undeafened.svg"),QIcon::Active,QIcon::On) ;
        icon.addPixmap(QPixmap(":/images/self_undeafened.svg"),QIcon::Selected,QIcon::On) ;

	audioListenToggleButton->setIcon(icon) ;
	audioListenToggleButton->setIconSize(QSize(22,22)) ;
	audioListenToggleButton->setFlat(true) ;
        audioListenToggleButton->setCheckable(true);

	audioMuteCaptureToggleButton = new QPushButton ;
	audioMuteCaptureToggleButton->setMinimumSize(QSize(28,28)) ;
	audioMuteCaptureToggleButton->setMaximumSize(QSize(28,28)) ;
	audioMuteCaptureToggleButton->setText(QString()) ;
	audioMuteCaptureToggleButton->setToolTip(tr("Deafen yourself"));

	QIcon icon2 ;
        icon2.addPixmap(QPixmap(":/images/muted_self.svg")) ;
	icon2.addPixmap(QPixmap(":/images/talking_off.svg"),QIcon::Normal,QIcon::On) ;
	icon2.addPixmap(QPixmap(":/images/talking_off.svg"),QIcon::Disabled,QIcon::On) ;
	icon2.addPixmap(QPixmap(":/images/talking_off.svg"),QIcon::Active,QIcon::On) ;
        icon2.addPixmap(QPixmap(":/images/talking_off.svg"),QIcon::Selected,QIcon::On) ;

	audioMuteCaptureToggleButton->setIcon(icon2) ;
	audioMuteCaptureToggleButton->setIconSize(QSize(22,22)) ;
	audioMuteCaptureToggleButton->setFlat(true) ;
        audioMuteCaptureToggleButton->setCheckable(true) ;

	connect(audioListenToggleButton, SIGNAL(clicked()), this , SLOT(toggleAudioListen()));
	connect(audioMuteCaptureToggleButton, SIGNAL(clicked()), this , SLOT(toggleAudioMuteCapture()));

	addChatBarWidget(audioListenToggleButton) ;
	addChatBarWidget(audioMuteCaptureToggleButton) ;

	//ui.chatWidget->resetStatusBar();

	outputProcessor = NULL ;
	outputDevice = NULL ;
	inputProcessor = NULL ;
	inputDevice = NULL ;
}

void AudioPopupChatDialog::toggleAudioListen() 
{
	std::cerr << "******** VOIPLugin: Toggling audio listen!" << std::endl;
    if (audioListenToggleButton->isChecked()) {
    } else {
        //audioListenToggleButton->setChecked(false);
        /*if (outputDevice) {
            outputDevice->stop();
        }*/
    }
}

void AudioPopupChatDialog::toggleAudioMuteCapture() 
{
	std::cerr << "******** VOIPLugin: Toggling audio mute capture!" << std::endl;
    if (audioMuteCaptureToggleButton->isChecked()) {
        //activate audio output
        audioListenToggleButton->setChecked(true);

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
    } else {
        disconnect(inputProcessor, SIGNAL(networkPacketReady()), this, SLOT(sendAudioData()));
        if (inputDevice) {
            inputDevice->stop();
        }
    }

}

void AudioPopupChatDialog::addAudioData(const QString name, QByteArray* array) 
{
    if (!audioListenToggleButton->isChecked()) {
        //launch an animation. Don't launch it if already animating
        if (!audioListenToggleButton->graphicsEffect() ||
            (audioListenToggleButton->graphicsEffect()->inherits("QGraphicsOpacityEffect") &&
                ((QGraphicsOpacityEffect*)audioListenToggleButton->graphicsEffect())->opacity() == 1)
            ) {
            QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(audioListenToggleButton);
            audioListenToggleButton->setGraphicsEffect(effect);
            QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity");
            anim->setStartValue(1);
            anim->setKeyValueAt(0.5,0);
            anim->setEndValue(1);
            anim->setDuration(400);
            anim->start();
        }

//        soundManager->play(VOIP_SOUND_INCOMING_CALL);

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

void AudioPopupChatDialog::sendAudioData() {
    while(inputProcessor && inputProcessor->hasPendingPackets()) {
        QByteArray qbarray = inputProcessor->getNetworkPacket();
        RsVoipDataChunk chunk;
        chunk.size = qbarray.size();
        chunk.data = (void*)qbarray.constData();
        rsVoip->sendVoipData(peerId,chunk);
    }
}

void AudioPopupChatDialog::updateStatus(int status)
{
	audioListenToggleButton->setEnabled(true);
	audioMuteCaptureToggleButton->setEnabled(true);

	PopupChatDialog::updateStatus(status) ;
}

