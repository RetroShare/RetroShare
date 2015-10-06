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
#include <QIcon>
#include <QLayout>
#include <QPropertyAnimation>
#include <QToolButton>

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
	int S = QFontMetricsF(chatWidget->font()).height() ;
	QSize iconSize = QSize(3*S,3*S);
	QSize buttonSize = QSize(iconSize + QSize(3,3));

	QIcon iconaudioListenToggleButton ;
	iconaudioListenToggleButton.addPixmap(QPixmap(":/images/audio-volume-muted.png")) ;
	iconaudioListenToggleButton.addPixmap(QPixmap(":/images/audio-volume-high.png"),QIcon::Normal,QIcon::On) ;
	iconaudioListenToggleButton.addPixmap(QPixmap(":/images/audio-volume-high.png"),QIcon::Disabled,QIcon::On) ;
	iconaudioListenToggleButton.addPixmap(QPixmap(":/images/audio-volume-high.png"),QIcon::Active,QIcon::On) ;
	iconaudioListenToggleButton.addPixmap(QPixmap(":/images/audio-volume-high.png"),QIcon::Selected,QIcon::On) ;

	audioListenToggleButton = new QToolButton ;
	audioListenToggleButton->setIcon(iconaudioListenToggleButton) ;
	audioListenToggleButton->setIconSize(iconSize) ;
	audioListenToggleButton->setMinimumSize(buttonSize) ;
	audioListenToggleButton->setMaximumSize(buttonSize) ;
	audioListenToggleButton->setCheckable(true);
	audioListenToggleButton->setAutoRaise(true) ;
	audioListenToggleButton->setText(QString()) ;
	audioListenToggleButton->setToolTip(tr("Mute"));

	QIcon iconaudioCaptureToggleButton ;
	iconaudioCaptureToggleButton.addPixmap(QPixmap(":/images/call-start.png")) ;
	iconaudioCaptureToggleButton.addPixmap(QPixmap(":/images/call-hold.png"),QIcon::Normal,QIcon::On) ;
	iconaudioCaptureToggleButton.addPixmap(QPixmap(":/images/call-hold.png"),QIcon::Disabled,QIcon::On) ;
	iconaudioCaptureToggleButton.addPixmap(QPixmap(":/images/call-hold.png"),QIcon::Active,QIcon::On) ;
	iconaudioCaptureToggleButton.addPixmap(QPixmap(":/images/call-hold.png"),QIcon::Selected,QIcon::On) ;

	audioCaptureToggleButton = new QToolButton ;
	audioCaptureToggleButton->setIcon(iconaudioCaptureToggleButton) ;
	audioCaptureToggleButton->setIconSize(iconSize) ;
	audioCaptureToggleButton->setMinimumSize(buttonSize) ;
	audioCaptureToggleButton->setMaximumSize(buttonSize) ;
	audioCaptureToggleButton->setCheckable(true) ;
	audioCaptureToggleButton->setAutoRaise(true) ;
	audioCaptureToggleButton->setText(QString()) ;
	audioCaptureToggleButton->setToolTip(tr("Start Call"));

	QIcon iconvideoCaptureToggleButton ;
	iconvideoCaptureToggleButton.addPixmap(QPixmap(":/images/video-icon-on.png")) ;
	iconvideoCaptureToggleButton.addPixmap(QPixmap(":/images/video-icon-off.png"),QIcon::Normal,QIcon::On) ;
	iconvideoCaptureToggleButton.addPixmap(QPixmap(":/images/video-icon-off.png"),QIcon::Disabled,QIcon::On) ;
	iconvideoCaptureToggleButton.addPixmap(QPixmap(":/images/video-icon-off.png"),QIcon::Active,QIcon::On) ;
	iconvideoCaptureToggleButton.addPixmap(QPixmap(":/images/video-icon-off.png"),QIcon::Selected,QIcon::On) ;

	videoCaptureToggleButton = new QToolButton ;
	videoCaptureToggleButton->setIcon(iconvideoCaptureToggleButton) ;
	videoCaptureToggleButton->setIconSize(iconSize) ;
	videoCaptureToggleButton->setMinimumSize(buttonSize) ;
	videoCaptureToggleButton->setMaximumSize(buttonSize) ;
	videoCaptureToggleButton->setCheckable(true) ;
	videoCaptureToggleButton->setAutoRaise(true) ;
	videoCaptureToggleButton->setText(QString()) ;
	videoCaptureToggleButton->setToolTip(tr("Start Video Call"));

	hangupButton = new QToolButton ;
	hangupButton->setIcon(QIcon(":/images/call-stop.png")) ;
	hangupButton->setIconSize(iconSize) ;
	hangupButton->setMinimumSize(buttonSize) ;
	hangupButton->setMaximumSize(buttonSize) ;
	hangupButton->setCheckable(false) ;
	hangupButton->setAutoRaise(true) ;
	hangupButton->setText(QString()) ;
	hangupButton->setToolTip(tr("Hangup Call"));
	hangupButton->hide();

	QIcon iconhideChatTextToggleButton ;
	iconhideChatTextToggleButton.addPixmap(QPixmap(":/images/orange-bubble-64.png")) ;
	iconhideChatTextToggleButton.addPixmap(QPixmap(":/images/white-bubble-64.png"),QIcon::Normal,QIcon::On) ;
	iconhideChatTextToggleButton.addPixmap(QPixmap(":/images/white-bubble-64.png"),QIcon::Disabled,QIcon::On) ;
	iconhideChatTextToggleButton.addPixmap(QPixmap(":/images/white-bubble-64.png"),QIcon::Active,QIcon::On) ;
	iconhideChatTextToggleButton.addPixmap(QPixmap(":/images/white-bubble-64.png"),QIcon::Selected,QIcon::On) ;

	hideChatTextToggleButton = new QToolButton ;
	hideChatTextToggleButton->setIcon(iconhideChatTextToggleButton) ;
	hideChatTextToggleButton->setIconSize(iconSize) ;
	hideChatTextToggleButton->setMinimumSize(buttonSize) ;
	hideChatTextToggleButton->setMaximumSize(buttonSize) ;
	hideChatTextToggleButton->setCheckable(true) ;
	hideChatTextToggleButton->setAutoRaise(true) ;
	hideChatTextToggleButton->setText(QString()) ;
	hideChatTextToggleButton->setToolTip(tr("Hide Chat Text"));
	hideChatTextToggleButton->setEnabled(false) ;

	QIcon iconfullscreenToggleButton ;
	iconfullscreenToggleButton.addPixmap(QPixmap(":/images/channels32.png")) ;
	iconfullscreenToggleButton.addPixmap(QPixmap(":/images/folder-draft24.png"),QIcon::Normal,QIcon::On) ;
	iconfullscreenToggleButton.addPixmap(QPixmap(":/images/folder-draft24.png"),QIcon::Disabled,QIcon::On) ;
	iconfullscreenToggleButton.addPixmap(QPixmap(":/images/folder-draft24.png"),QIcon::Active,QIcon::On) ;
	iconfullscreenToggleButton.addPixmap(QPixmap(":/images/folder-draft24.png"),QIcon::Selected,QIcon::On) ;

	fullscreenToggleButton = new QToolButton ;
	fullscreenToggleButton->setIcon(iconfullscreenToggleButton) ;
	fullscreenToggleButton->setIconSize(iconSize) ;
	fullscreenToggleButton->setMinimumSize(buttonSize) ;
	fullscreenToggleButton->setMaximumSize(buttonSize) ;
	fullscreenToggleButton->setCheckable(true) ;
	fullscreenToggleButton->setAutoRaise(true) ;
	fullscreenToggleButton->setText(QString()) ;
	fullscreenToggleButton->setToolTip(tr("Fullscreen mode"));
	fullscreenToggleButton->setEnabled(false) ;

	connect(audioListenToggleButton, SIGNAL(clicked()), this , SLOT(toggleAudioListen()));
	connect(audioCaptureToggleButton, SIGNAL(clicked()), this , SLOT(toggleAudioCapture()));
	connect(videoCaptureToggleButton, SIGNAL(clicked()), this , SLOT(toggleVideoCapture()));
	connect(hangupButton, SIGNAL(clicked()), this , SLOT(hangupCall()));
	connect(hideChatTextToggleButton, SIGNAL(clicked()), this , SLOT(toggleHideChatText()));
	connect(fullscreenToggleButton, SIGNAL(clicked()), this , SLOT(toggleFullScreen()));

	mChatWidget->addTitleBarWidget(audioListenToggleButton) ;
	mChatWidget->addTitleBarWidget(audioCaptureToggleButton) ;
	mChatWidget->addTitleBarWidget(videoCaptureToggleButton) ;
	mChatWidget->addTitleBarWidget(hangupButton) ;
	mChatWidget->addTitleBarWidget(hideChatTextToggleButton) ;
	mChatWidget->addTitleBarWidget(fullscreenToggleButton) ;

	outputAudioProcessor = NULL ;
	outputAudioDevice = NULL ;
	inputAudioProcessor = NULL ;
	inputAudioDevice = NULL ;

	inputVideoDevice = new QVideoInputDevice(mChatWidget) ; // not started yet ;-)
	videoProcessor = new VideoProcessor ;

	// Make a widget with two video devices, one for echo, and one for the talking peer.
	videoWidget = new QWidget(mChatWidget) ;
	videoWidget->setLayout(new QVBoxLayout()) ;
	videoWidget->layout()->addWidget(outputVideoDevice = new QVideoOutputDevice(videoWidget)) ;
	videoWidget->layout()->addWidget(echoVideoDevice = new QVideoOutputDevice(videoWidget)) ;
	videoWidget->hide();

	connect(inputVideoDevice, SIGNAL(networkPacketReady()), this, SLOT(sendVideoData()));

	echoVideoDevice->setMinimumSize(320,240) ;//4/3
	outputVideoDevice->setMinimumSize(320,240) ;//4/3

	echoVideoDevice->showFrameOff();
	outputVideoDevice->showFrameOff();
	
	echoVideoDevice->setStyleSheet("border: 4px solid #CCCCCC; border-radius: 4px;");
	outputVideoDevice->setStyleSheet("border: 4px solid #CCCCCC; border-radius: 4px;");

	/// FULLSCREEN ///
	fullScreenFrame = new QFrame();

	outputVideoDeviceFS = new QVideoOutputDevice(fullScreenFrame);
	outputVideoDeviceFS->setGeometry(QRect(QPoint(0,0),fullScreenFrame->geometry().size()));
	outputVideoDeviceFS->showFrameOff();

	echoVideoDeviceFS = new QVideoOutputDevice(fullScreenFrame);
	echoVideoDeviceFS->setGeometry(QRect(QPoint(fullScreenFrame->width(), fullScreenFrame->height()) - QPoint(320,240), QSize(320,240)));
	echoVideoDeviceFS->showFrameOff();

	toolBarFS = new QFrame(fullScreenFrame);
	QHBoxLayout *toolBarFSLayout = new QHBoxLayout(toolBarFS);

	audioListenToggleButtonFS = new QToolButton(fullScreenFrame) ;
	audioListenToggleButtonFS->setIcon(iconaudioListenToggleButton) ;
	audioListenToggleButtonFS->setIconSize(iconSize*2) ;
	audioListenToggleButtonFS->setMinimumSize(buttonSize*2) ;
	audioListenToggleButtonFS->setMaximumSize(buttonSize*2) ;
	audioListenToggleButtonFS->setCheckable(true);
	audioListenToggleButtonFS->setAutoRaise(true) ;
	audioListenToggleButtonFS->setText(QString()) ;
	audioListenToggleButtonFS->setToolTip(tr("Mute"));

	audioCaptureToggleButtonFS = new QToolButton(fullScreenFrame) ;
	audioCaptureToggleButtonFS->setIcon(iconaudioCaptureToggleButton) ;
	audioCaptureToggleButtonFS->setIconSize(iconSize*2) ;
	audioCaptureToggleButtonFS->setMinimumSize(buttonSize*2) ;
	audioCaptureToggleButtonFS->setMaximumSize(buttonSize*2) ;
	audioCaptureToggleButtonFS->setCheckable(true) ;
	audioCaptureToggleButtonFS->setAutoRaise(true) ;
	audioCaptureToggleButtonFS->setText(QString()) ;
	audioCaptureToggleButtonFS->setToolTip(tr("Start Call"));

	videoCaptureToggleButtonFS = new QToolButton(fullScreenFrame) ;
	videoCaptureToggleButtonFS->setIcon(iconvideoCaptureToggleButton) ;
	videoCaptureToggleButtonFS->setIconSize(iconSize*2) ;
	videoCaptureToggleButtonFS->setMinimumSize(buttonSize*2) ;
	videoCaptureToggleButtonFS->setMaximumSize(buttonSize*2) ;
	videoCaptureToggleButtonFS->setCheckable(true) ;
	videoCaptureToggleButtonFS->setAutoRaise(true) ;
	videoCaptureToggleButtonFS->setText(QString()) ;
	videoCaptureToggleButtonFS->setToolTip(tr("Start Video Call"));

	hangupButtonFS = new QToolButton(fullScreenFrame) ;
	hangupButtonFS->setIcon(QIcon(":/images/call-stop.png")) ;
	hangupButtonFS->setIconSize(iconSize*2) ;
	hangupButtonFS->setMinimumSize(buttonSize*2) ;
	hangupButtonFS->setMaximumSize(buttonSize*2) ;
	hangupButtonFS->setCheckable(false) ;
	hangupButtonFS->setAutoRaise(true) ;
	hangupButtonFS->setText(QString()) ;
	hangupButtonFS->setToolTip(tr("Hangup Call"));
	hangupButtonFS->hide();

	fullscreenToggleButtonFS = new QToolButton(fullScreenFrame);
	fullscreenToggleButtonFS->setIcon(iconfullscreenToggleButton);
	fullscreenToggleButtonFS->setIconSize(iconSize*2);
	fullscreenToggleButtonFS->setMinimumSize(buttonSize*2);
	fullscreenToggleButtonFS->setMaximumSize(buttonSize*2);
	fullscreenToggleButtonFS->setCheckable(true);
	fullscreenToggleButtonFS->setAutoRaise(true);
	fullscreenToggleButtonFS->setText(QString());
	fullscreenToggleButtonFS->setToolTip(tr("Fullscreen mode"));
	fullscreenToggleButtonFS->setEnabled(false);

	connect(audioListenToggleButtonFS, SIGNAL(clicked()), this , SLOT(toggleAudioListenFS()));
	connect(audioCaptureToggleButtonFS, SIGNAL(clicked()), this , SLOT(toggleAudioCaptureFS()));
	connect(videoCaptureToggleButtonFS, SIGNAL(clicked()), this , SLOT(toggleVideoCaptureFS()));
	connect(hangupButtonFS, SIGNAL(clicked()), this , SLOT(hangupCall()));
	connect(fullscreenToggleButtonFS, SIGNAL(clicked()), this , SLOT(toggleFullScreenFS()));

	toolBarFSLayout->setDirection(QBoxLayout::LeftToRight);
	toolBarFSLayout->setSpacing(2);
	toolBarFSLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
	toolBarFSLayout->addWidget(audioListenToggleButtonFS);
	toolBarFSLayout->addWidget(audioCaptureToggleButtonFS);
	toolBarFSLayout->addWidget(videoCaptureToggleButtonFS);
	toolBarFSLayout->addWidget(hangupButtonFS);
	toolBarFSLayout->addWidget(fullscreenToggleButtonFS);
	toolBarFS->setLayout(toolBarFSLayout);

	fullScreenFrame->setParent(0);
	fullScreenFrame->setWindowFlags(Qt::WindowStaysOnTopHint);
	fullScreenFrame->setFocusPolicy(Qt::StrongFocus);
	fullScreenFrame->setWindowState(Qt::WindowFullScreen);
	fullScreenFrame->hide();
	fullScreenFrame->installEventFilter(this);

	mChatWidget->addChatHorizontalWidget(videoWidget) ;

	inputVideoDevice->setEchoVideoTarget(echoVideoDevice) ;
	inputVideoDevice->setVideoProcessor(videoProcessor) ;
	videoProcessor->setDisplayTarget(outputVideoDevice) ;
}

VOIPChatWidgetHolder::~VOIPChatWidgetHolder()
{
	if(inputAudioDevice != NULL)
		inputAudioDevice->stop() ;

	delete inputVideoDevice ;
	delete videoProcessor ;

	button_map::iterator it = buttonMapTakeVideo.begin();
	while (it != buttonMapTakeVideo.end()) {
		it = buttonMapTakeVideo.erase(it);
  }
}

bool VOIPChatWidgetHolder::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == fullScreenFrame) {
		if (event->type() == QEvent::Close || event->type() == QEvent::MouseButtonDblClick) {
			showNormalView();
		}
		if (event->type() == QEvent::Resize) {
			replaceFullscreenWidget();
		}

	}
	// pass the event on to the parent class
	return QObject::eventFilter(obj, event);
}

void VOIPChatWidgetHolder::hangupCall()
{
	if (audioCaptureToggleButton->isChecked()) {
		audioCaptureToggleButton->setChecked(false);
		toggleAudioCapture();
	}
	if (videoCaptureToggleButton->isChecked()) {
		videoCaptureToggleButton->setChecked(false);
		toggleVideoCapture();
	}
	if (fullscreenToggleButton->isChecked()) {
		fullscreenToggleButton->setChecked(false);
		toggleFullScreen();
	}
	if (hideChatTextToggleButton->isChecked()) {
		hideChatTextToggleButton->setChecked(false);
		toggleHideChatText();
	}
	hangupButton->hide();
	hangupButtonFS->hide();
}

void VOIPChatWidgetHolder::toggleAudioListenFS()
{
	audioListenToggleButton->setChecked(audioListenToggleButtonFS->isChecked());
	toggleAudioListen();
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
    audioListenToggleButtonFS->setChecked(audioListenToggleButton->isChecked());
    audioListenToggleButtonFS->setToolTip(audioListenToggleButton->toolTip());
}

void VOIPChatWidgetHolder::startAudioCapture()
{
	audioCaptureToggleButton->setChecked(true);
	toggleAudioCapture();
}

void VOIPChatWidgetHolder::toggleAudioCaptureFS()
{
	audioCaptureToggleButton->setChecked(audioCaptureToggleButtonFS->isChecked());
	toggleAudioCapture();
}

void VOIPChatWidgetHolder::toggleAudioCapture()
{
    if (audioCaptureToggleButton->isChecked()) {
        //activate audio output
        audioListenToggleButton->setChecked(true);
        audioListenToggleButtonFS->setChecked(true);
        audioCaptureToggleButton->setToolTip(tr("Hold Call"));
        hangupButton->show();
        hangupButtonFS->show();

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
        if (mChatWidget) {
            mChatWidget->addChatMsg(true, tr("VoIP Status"), QDateTime::currentDateTime(), QDateTime::currentDateTime(), tr("Outgoing Audio Call stopped."), ChatWidget::MSGTYPE_SYSTEM);
        }
        audioCaptureToggleButton->setToolTip(tr("Resume Call"));
        hangupButton->hide();
        hangupButtonFS->hide();
    }
    audioCaptureToggleButtonFS->setChecked(audioCaptureToggleButton->isChecked());
    audioCaptureToggleButtonFS->setToolTip(audioCaptureToggleButton->toolTip());
}

void VOIPChatWidgetHolder::startVideoCapture()
{
	videoCaptureToggleButton->setChecked(true);
	toggleVideoCapture();
}

void VOIPChatWidgetHolder::toggleVideoCaptureFS()
{
	videoCaptureToggleButton->setChecked(videoCaptureToggleButtonFS->isChecked());
	toggleVideoCapture();
}

void VOIPChatWidgetHolder::toggleVideoCapture()
{
	if (videoCaptureToggleButton->isChecked()) 
	{
		hideChatTextToggleButton->setEnabled(true);
		fullscreenToggleButton->setEnabled(true);
		fullscreenToggleButtonFS->setEnabled(true);
		hangupButton->show();
    hangupButtonFS->show();
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
		hideChatTextToggleButton->setEnabled(false);
		hideChatTextToggleButton->setChecked(false);
		toggleHideChatText();
		fullscreenToggleButton->setEnabled(false);
		fullscreenToggleButton->setChecked(false);
		fullscreenToggleButtonFS->setEnabled(false);
		fullscreenToggleButtonFS->setChecked(false);
		toggleFullScreen();
		hangupButton->hide();
    hangupButtonFS->hide();

		inputVideoDevice->stop() ;
		videoCaptureToggleButton->setToolTip(tr("Activate camera"));
		outputVideoDevice->showFrameOff();
		videoWidget->hide();
		
		if (mChatWidget) 
			mChatWidget->addChatMsg(true, tr("VoIP Status"), QDateTime::currentDateTime(), QDateTime::currentDateTime()
			                        , tr("Video call stopped"), ChatWidget::MSGTYPE_SYSTEM);
	}
	videoCaptureToggleButtonFS->setChecked(videoCaptureToggleButton->isChecked());
	videoCaptureToggleButtonFS->setToolTip(videoCaptureToggleButton->toolTip());
}

void VOIPChatWidgetHolder::addVideoData(const RsPeerId &peer_id, QByteArray* array)
{
    if (!videoCaptureToggleButton->isChecked()) 
    {
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

    } 
    else 
    {
	    RsVOIPDataChunk chunk ;
	    chunk.type = RsVOIPDataChunk::RS_VOIP_DATA_TYPE_VIDEO ;
	    chunk.size = array->size() ;
	    chunk.data = array->data() ;

	    videoProcessor->receiveEncodedData(chunk) ;
    }
}

void VOIPChatWidgetHolder::toggleHideChatText()
{
	QBoxLayout *layout = static_cast<QBoxLayout*>(videoWidget->layout());

	if (hideChatTextToggleButton->isChecked()) {
		mChatWidget->hideChatText(true);
		if (layout) layout->setDirection(QBoxLayout::LeftToRight);
		hideChatTextToggleButton->setToolTip(tr("Show Chat Text"));
	} else {
		mChatWidget->hideChatText(false);
		if (layout) layout->setDirection(QBoxLayout::TopToBottom);
		hideChatTextToggleButton->setToolTip(tr("Hide Chat Text"));
		fullscreenToggleButton->setChecked(false);
		toggleFullScreen();
	}
}

void VOIPChatWidgetHolder::toggleFullScreenFS()
{
	fullscreenToggleButton->setChecked(fullscreenToggleButtonFS->isChecked());
	toggleFullScreen();
}

void VOIPChatWidgetHolder::toggleFullScreen()
{
	if (fullscreenToggleButton->isChecked()) {
		fullscreenToggleButton->setToolTip(tr("Return to normal view."));
		inputVideoDevice->setEchoVideoTarget(echoVideoDeviceFS) ;
		videoProcessor->setDisplayTarget(outputVideoDeviceFS) ;
		fullScreenFrame->show();
	} else {
		mChatWidget->hideChatText(false);
		fullscreenToggleButton->setToolTip(tr("Fullscreen mode"));
		inputVideoDevice->setEchoVideoTarget(echoVideoDevice) ;
		videoProcessor->setDisplayTarget(outputVideoDevice) ;
		fullScreenFrame->hide();
	}
	fullscreenToggleButtonFS->setChecked(fullscreenToggleButton->isChecked());
	fullscreenToggleButtonFS->setToolTip(fullscreenToggleButton->toolTip());
}

void VOIPChatWidgetHolder::replaceFullscreenWidget()
{
	if (QSize(toolBarFS->geometry().size() - fullScreenFrame->geometry().size()).isValid()){
		QRect fsRect = fullScreenFrame->geometry();
		fsRect.setSize(toolBarFS->geometry().size());
		fullScreenFrame->setGeometry(fsRect);
	}

	outputVideoDeviceFS->setGeometry(QRect(QPoint(0,0),fullScreenFrame->geometry().size()));
	echoVideoDeviceFS->setGeometry(QRect(QPoint(fullScreenFrame->width(), fullScreenFrame->height()) - QPoint(320,240), QSize(320,240)));
	QRect toolBarFSGeo = QRect( (fullScreenFrame->width() - toolBarFS->geometry().width()) / 2
	                            , fullScreenFrame->height() - toolBarFS->geometry().height()
	                            , toolBarFS->geometry().width(), toolBarFS->geometry().height());
	toolBarFS->setGeometry(toolBarFSGeo);

	if (!videoCaptureToggleButton->isChecked()) {
		outputVideoDeviceFS->showFrameOff();
		echoVideoDeviceFS->showFrameOff();
	}
}

void VOIPChatWidgetHolder::showNormalView()
{
	hideChatTextToggleButton->setChecked(false);
	toggleHideChatText();
	fullscreenToggleButton->setChecked(false);
	fullscreenToggleButtonFS->setChecked(false);
	toggleFullScreen();
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
	videoProcessor->setMaximumBandwidth(bytes_per_sec) ;
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
	bool enabled = (status != RS_STATUS_OFFLINE);

	audioListenToggleButton->setEnabled(enabled);
	audioListenToggleButtonFS->setEnabled(enabled);
	audioCaptureToggleButton->setEnabled(enabled);
	audioCaptureToggleButtonFS->setEnabled(enabled);
	videoCaptureToggleButton->setEnabled(enabled);
	videoCaptureToggleButtonFS->setEnabled(enabled);
	hideChatTextToggleButton->setEnabled(videoCaptureToggleButton->isChecked() && enabled);
	fullscreenToggleButton->setEnabled(videoCaptureToggleButton->isChecked() && enabled);
	fullscreenToggleButtonFS->setEnabled(videoCaptureToggleButton->isChecked() && enabled);
	hangupButton->setEnabled(enabled);
	hangupButtonFS->setEnabled(enabled);
}
