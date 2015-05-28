/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2015
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/
#include <QToolButton>
#include <QPropertyAnimation>
#include <QIcon>
#include <QLayout>

#include <gui/audiodevicehelper.h>
#include "interface/rsVOIP.h"
#include "gui/SoundManager.h"
#include "util/HandleRichText.h"
#include "gui/common/StatusDefs.h"
#include "gui/chat/ChatWidget.h"

#include "VOIPChatWidgetHolder.h"
#include "VideoProcessor.h"
#include "QVideoDevice.h"

#include <retroshare/rsstatus.h>
#include <retroshare/rspeers.h>

#define CALL_START ":/images/call-start.png"
#define CALL_STOP  ":/images/call-stop.png"
#define CALL_HOLD  ":/images/call-hold.png"


VOIPChatWidgetHolder::VOIPChatWidgetHolder(ChatWidget *chatWidget, VOIPNotify *notify)
  : QObject(), ChatWidgetHolder(chatWidget), mVOIPNotify(notify)
{
	QIcon icon ;
	icon.addPixmap(QPixmap(":/images/audio-volume-muted.png")) ;
	icon.addPixmap(QPixmap(":/images/audio-volume-high.png"),QIcon::Normal,QIcon::On) ;
	icon.addPixmap(QPixmap(":/images/audio-volume-high.png"),QIcon::Disabled,QIcon::On) ;
	icon.addPixmap(QPixmap(":/images/audio-volume-high.png"),QIcon::Active,QIcon::On) ;
	icon.addPixmap(QPixmap(":/images/audio-volume-high.png"),QIcon::Selected,QIcon::On) ;

	audioListenToggleButton = new QToolButton ;
	audioListenToggleButton->setIcon(icon) ;
	audioListenToggleButton->setIconSize(QSize(42,42)) ;
	audioListenToggleButton->setAutoRaise(true) ;
	audioListenToggleButton->setCheckable(true);
	audioListenToggleButton->setMinimumSize(QSize(44,44)) ;
	audioListenToggleButton->setMaximumSize(QSize(44,44)) ;
	audioListenToggleButton->setText(QString()) ;
	audioListenToggleButton->setToolTip(tr("Mute"));

	QIcon icon2 ;
	icon2.addPixmap(QPixmap(":/images/call-start.png")) ;
	icon2.addPixmap(QPixmap(":/images/call-hold.png"),QIcon::Normal,QIcon::On) ;
	icon2.addPixmap(QPixmap(":/images/call-hold.png"),QIcon::Disabled,QIcon::On) ;
	icon2.addPixmap(QPixmap(":/images/call-hold.png"),QIcon::Active,QIcon::On) ;
	icon2.addPixmap(QPixmap(":/images/call-hold.png"),QIcon::Selected,QIcon::On) ;

	audioCaptureToggleButton = new QToolButton ;
	audioCaptureToggleButton->setMinimumSize(QSize(44,44)) ;
	audioCaptureToggleButton->setMaximumSize(QSize(44,44)) ;
	audioCaptureToggleButton->setText(QString()) ;
	audioCaptureToggleButton->setToolTip(tr("Start Call"));
	audioCaptureToggleButton->setIcon(icon2) ;
	audioCaptureToggleButton->setIconSize(QSize(42,42)) ;
	audioCaptureToggleButton->setAutoRaise(true) ;
	audioCaptureToggleButton->setCheckable(true) ;
	
	hangupButton = new QToolButton ;
	hangupButton->setIcon(QIcon(":/images/call-stop.png")) ;
	hangupButton->setIconSize(QSize(42,42)) ;
	hangupButton->setMinimumSize(QSize(44,44)) ;
	hangupButton->setMaximumSize(QSize(44,44)) ;
	hangupButton->setCheckable(false) ;
	hangupButton->setAutoRaise(true) ;		
	hangupButton->setText(QString()) ;
	hangupButton->setToolTip(tr("Hangup Call"));
	hangupButton->hide();	

	QIcon icon3 ;
	icon3.addPixmap(QPixmap(":/images/video-icon-on.png")) ;
	icon3.addPixmap(QPixmap(":/images/video-icon-off.png"),QIcon::Normal,QIcon::On) ;
	icon3.addPixmap(QPixmap(":/images/video-icon-off.png"),QIcon::Disabled,QIcon::On) ;
	icon3.addPixmap(QPixmap(":/images/video-icon-off.png"),QIcon::Active,QIcon::On) ;
	icon3.addPixmap(QPixmap(":/images/video-icon-off.png"),QIcon::Selected,QIcon::On) ;

	videoCaptureToggleButton = new QToolButton ;
	videoCaptureToggleButton->setMinimumSize(QSize(44,44)) ;
	videoCaptureToggleButton->setMaximumSize(QSize(44,44)) ;
	videoCaptureToggleButton->setText(QString()) ;
	videoCaptureToggleButton->setToolTip(tr("Start Video Call"));
	videoCaptureToggleButton->setIcon(icon3) ;
	videoCaptureToggleButton->setIconSize(QSize(42,42)) ;
	videoCaptureToggleButton->setAutoRaise(true) ;
	videoCaptureToggleButton->setCheckable(true) ;

	connect(videoCaptureToggleButton, SIGNAL(clicked()), this , SLOT(toggleVideoCapture()));
	connect(audioListenToggleButton, SIGNAL(clicked()), this , SLOT(toggleAudioListen()));
	connect(audioCaptureToggleButton, SIGNAL(clicked()), this , SLOT(toggleAudioCapture()));
	connect(hangupButton, SIGNAL(clicked()), this , SLOT(hangupCall()));

	mChatWidget->addVOIPBarWidget(audioListenToggleButton) ;
	mChatWidget->addVOIPBarWidget(audioCaptureToggleButton) ;
	mChatWidget->addVOIPBarWidget(hangupButton) ;
	mChatWidget->addVOIPBarWidget(videoCaptureToggleButton) ;

	outputAudioProcessor = NULL ;
	outputAudioDevice = NULL ;
	inputAudioProcessor = NULL ;
	inputAudioDevice = NULL ;

	inputVideoDevice = new QVideoInputDevice(mChatWidget) ; // not started yet ;-)
	inputVideoProcessor = new JPEGVideoEncoder ;
	outputVideoProcessor = new JPEGVideoDecoder ;

	// Make a widget with two video devices, one for echo, and one for the talking peer.
	videoWidget = new QWidget(mChatWidget) ;
	videoWidget->setLayout(new QVBoxLayout()) ;
	videoWidget->layout()->addWidget(outputVideoDevice = new QVideoOutputDevice(videoWidget)) ;
	videoWidget->layout()->addWidget(echoVideoDevice = new QVideoOutputDevice(videoWidget)) ;
	videoWidget->hide();

	connect(inputVideoDevice, SIGNAL(networkPacketReady()), this, SLOT(sendVideoData()));

	echoVideoDevice->setMinimumSize(320,256) ;
	outputVideoDevice->setMinimumSize(320,256) ;
	
	echoVideoDevice->setStyleSheet("border: 4px solid #CCCCCC; border-radius: 4px;");
	outputVideoDevice->setStyleSheet("border: 4px solid #CCCCCC; border-radius: 4px;");

	mChatWidget->addChatHorizontalWidget(videoWidget) ;

	inputVideoDevice->setEchoVideoTarget(echoVideoDevice) ;
	inputVideoDevice->setVideoEncoder(inputVideoProcessor) ;
	outputVideoProcessor->setDisplayTarget(outputVideoDevice) ;
}

VOIPChatWidgetHolder::~VOIPChatWidgetHolder()
{
	if(inputAudioDevice != NULL)
		inputAudioDevice->stop() ;

	delete inputVideoDevice ;
	delete inputVideoProcessor ;
	delete outputVideoProcessor ;

	button_map::iterator it = buttonMapTakeVideo.begin();
	while (it != buttonMapTakeVideo.end()) {
		it = buttonMapTakeVideo.erase(it);
  }
}

void VOIPChatWidgetHolder::toggleAudioListen()
{
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
	disconnect(inputAudioProcessor, SIGNAL(networkPacketReady()), this, SLOT(sendAudioData()));
	if (inputAudioDevice) {
		inputAudioDevice->stop();
	}        
	if (outputAudioDevice) {
		outputAudioDevice->stop();
	}

	if (mChatWidget) {
		mChatWidget->addChatMsg(true, tr("VoIP Status"), QDateTime::currentDateTime(), QDateTime::currentDateTime(), tr("Outgoing Call stopped."), ChatWidget::MSGTYPE_SYSTEM);
	}
	
	audioListenToggleButton->setChecked(false);
	audioCaptureToggleButton->setChecked(false);
	hangupButton->hide();
}

void VOIPChatWidgetHolder::startAudioCapture()
{
	audioCaptureToggleButton->setChecked(true);
	toggleAudioCapture();
}

void VOIPChatWidgetHolder::toggleAudioCapture()
{
    if (audioCaptureToggleButton->isChecked()) {
        //activate audio output
        audioListenToggleButton->setChecked(true);
        audioCaptureToggleButton->setToolTip(tr("Hold Call"));
        hangupButton->show();

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
        
        button_map::iterator it = buttonMapTakeVideo.begin();
        while (it != buttonMapTakeVideo.end()) {
        RSButtonOnText *button = it.value();
        delete button;
        it = buttonMapTakeVideo.erase(it);
        }
        
    } else {
        disconnect(inputAudioProcessor, SIGNAL(networkPacketReady()), this, SLOT(sendAudioData()));
        if (inputAudioDevice) {
            inputAudioDevice->stop();
        }
        audioCaptureToggleButton->setToolTip(tr("Resume Call"));
        hangupButton->hide();
    }
}

void VOIPChatWidgetHolder::startVideoCapture()
{
	videoCaptureToggleButton->setChecked(true);
	toggleVideoCapture();
}

void VOIPChatWidgetHolder::toggleVideoCapture()
{
	if (videoCaptureToggleButton->isChecked()) 
	{
		//activate video input
		//
		videoWidget->show();
		inputVideoDevice->start() ;

		videoCaptureToggleButton->setToolTip(tr("Shut camera off"));

		if (mChatWidget) 
			mChatWidget->addChatMsg(true, tr("VoIP Status"), QDateTime::currentDateTime(), QDateTime::currentDateTime()
			                        , tr("You're now sending video..."), ChatWidget::MSGTYPE_SYSTEM);

		button_map::iterator it = buttonMapTakeVideo.begin();
		while (it != buttonMapTakeVideo.end()) {
			RSButtonOnText *button = it.value();
			delete button;
			it = buttonMapTakeVideo.erase(it);
	  }
	} 
	else 
	{
		inputVideoDevice->stop() ;
		videoCaptureToggleButton->setToolTip(tr("Activate camera"));
		outputVideoDevice->showFrameOff();
		videoWidget->hide();
		
		if (mChatWidget) 
			mChatWidget->addChatMsg(true, tr("VoIP Status"), QDateTime::currentDateTime(), QDateTime::currentDateTime()
			                        , tr("Video call stopped"), ChatWidget::MSGTYPE_SYSTEM);
	}
}

void VOIPChatWidgetHolder::addVideoData(const RsPeerId &peer_id, QByteArray* array)
{
	if (!videoCaptureToggleButton->isChecked()) {
		if (mChatWidget) {
			QString buttonName = QString::fromUtf8(rsPeers->getPeerName(peer_id).c_str());
			if (buttonName.isEmpty()) buttonName = "VoIP";//TODO maybe change all with GxsId
			button_map::iterator it = buttonMapTakeVideo.find(buttonName);
			if (it == buttonMapTakeVideo.end()){
				mChatWidget->addChatMsg(true, tr("VoIP Status"), QDateTime::currentDateTime(), QDateTime::currentDateTime()
				                        , tr("%1 inviting you to start a video conversation. do you want Accept or Decline the invitation?").arg(buttonName), ChatWidget::MSGTYPE_SYSTEM);
				RSButtonOnText *button = mChatWidget->getNewButtonOnTextBrowser(tr("Accept Video Call"));
				button->setToolTip(tr("Activate camera"));
				button->setStyleSheet(QString("border: 1px solid #199909;")
                              .append("font-size: 12pt;  color: white;")
                              .append("min-width: 128px; min-height: 24px;")
				                      .append("border-radius: 6px;")
				                      .append("background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 0.67, "
                                        "stop: 0 #22c70d, stop: 1 #116a06);")

				                      );

				button->updateImage();

				connect(button,SIGNAL(clicked()),this,SLOT(startVideoCapture()));
				connect(button,SIGNAL(mouseEnter()),this,SLOT(botMouseEnter()));
				connect(button,SIGNAL(mouseLeave()),this,SLOT(botMouseLeave()));

				buttonMapTakeVideo.insert(buttonName, button);
			}
		}

		//TODO make a sound for the incoming call
//        soundManager->play(VOIP_SOUND_INCOMING_CALL);
		if (mVOIPNotify) mVOIPNotify->notifyReceivedVoipVideoCall(peer_id);

	} else {
		outputVideoProcessor->receiveEncodedData((unsigned char *)array->data(),array->size()) ;
	}
}

void VOIPChatWidgetHolder::botMouseEnter()
{
	RSButtonOnText *source = qobject_cast<RSButtonOnText *>(QObject::sender());
	if (source){
		source->setStyleSheet(QString("border: 1px solid #333333;")
                          .append("font-size: 12pt; color: white;")
                          .append("min-width: 128px; min-height: 24px;")
                          .append("border-radius: 6px;")
                          .append("background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 0.67, "
                                  "stop: 0 #444444, stop: 1 #222222);")

                          );
		//source->setDown(true);
	}
}

void VOIPChatWidgetHolder::botMouseLeave()
{
	RSButtonOnText *source = qobject_cast<RSButtonOnText *>(QObject::sender());
	if (source){
		source->setStyleSheet(QString("border: 1px solid #199909;")
                          .append("font-size: 12pt; color: white;")
                          .append("min-width: 128px; min-height: 24px;")
				                  .append("border-radius: 6px;")
				                  .append("background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 0.67, "
                                   "stop: 0 #22c70d, stop: 1 #116a06);")

				                   );
		//source->setDown(false);
	}
}

void VOIPChatWidgetHolder::setAcceptedBandwidth(uint32_t bytes_per_sec)
{
	inputVideoProcessor->setMaximumFrameRate(bytes_per_sec) ;
}

void VOIPChatWidgetHolder::addAudioData(const RsPeerId &peer_id, QByteArray* array)
{
    if (!audioCaptureToggleButton->isChecked()) {
        //launch an animation. Don't launch it if already animating
        if (!audioCaptureToggleButton->graphicsEffect() ||
            (audioCaptureToggleButton->graphicsEffect()->inherits("QGraphicsOpacityEffect") &&
                ((QGraphicsOpacityEffect*)audioCaptureToggleButton->graphicsEffect())->opacity() == 1)
            ) {
            QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(audioListenToggleButton);
            audioCaptureToggleButton->setGraphicsEffect(effect);
            QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity", effect);
            anim->setStartValue(1);
            anim->setKeyValueAt(0.5,0);
            anim->setEndValue(1);
            anim->setDuration(400);
            anim->start();
        }
        
        if (mChatWidget) {
        QString buttonName = QString::fromUtf8(rsPeers->getPeerName(peer_id).c_str());
        if (buttonName.isEmpty()) buttonName = "VoIP";//TODO maybe change all with GxsId
        button_map::iterator it = buttonMapTakeVideo.find(buttonName);
        if (it == buttonMapTakeVideo.end()){
				mChatWidget->addChatMsg(true, tr("VoIP Status"), QDateTime::currentDateTime(), QDateTime::currentDateTime()
				                        , tr("%1 inviting you to start a audio conversation. do you want Accept or Decline the invitation?").arg(buttonName), ChatWidget::MSGTYPE_SYSTEM);
				RSButtonOnText *button = mChatWidget->getNewButtonOnTextBrowser(tr("Accept Call"));
				button->setToolTip(tr("Activate audio"));
				button->setStyleSheet(QString("border: 1px solid #199909;")
				                      .append("font-size: 12pt;  color: white;")
				                      .append("min-width: 128px; min-height: 24px;")
				                      .append("border-radius: 6px;")
				                      .append("background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 0.67, "
                                        "stop: 0 #22c70d, stop: 1 #116a06);")

				                      );
                                       

				button->updateImage();

				connect(button,SIGNAL(clicked()),this,SLOT(startAudioCapture()));
				connect(button,SIGNAL(mouseEnter()),this,SLOT(botMouseEnter()));
				connect(button,SIGNAL(mouseLeave()),this,SLOT(botMouseLeave()));

				buttonMapTakeVideo.insert(buttonName, button);
        }
        }

        audioCaptureToggleButton->setToolTip(tr("Answer"));

        //TODO make a sound for the incoming call
//        soundManager->play(VOIP_SOUND_INCOMING_CALL);
        if (mVOIPNotify) mVOIPNotify->notifyReceivedVoipAudioCall(peer_id);

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
    outputAudioProcessor->putNetworkPacket(QString::fromStdString(peer_id.toStdString()), *array);

    //check the input device for errors
    if (inputAudioDevice && inputAudioDevice->error() != QAudio::NoError) {
        std::cerr << "Restarting input device. Error before reset " << inputAudioDevice->error() << std::endl;
        inputAudioDevice->stop();
        inputAudioDevice->reset();
        inputAudioDevice->start(inputAudioProcessor);
    }
}

void VOIPChatWidgetHolder::sendVideoData()
{
	RsVOIPDataChunk chunk ;

	while(inputVideoDevice && inputVideoDevice->getNextEncodedPacket(chunk))
        rsVOIP->sendVoipData(mChatWidget->getChatId().toPeerId(),chunk) ;
}

void VOIPChatWidgetHolder::sendAudioData()
{
    while(inputAudioProcessor && inputAudioProcessor->hasPendingPackets()) {
        QByteArray qbarray = inputAudioProcessor->getNetworkPacket();
        RsVOIPDataChunk chunk;
        chunk.size = qbarray.size();
        chunk.data = (void*)qbarray.constData();
		  chunk.type = RsVOIPDataChunk::RS_VOIP_DATA_TYPE_AUDIO ;
        rsVOIP->sendVoipData(mChatWidget->getChatId().toPeerId(),chunk);
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
