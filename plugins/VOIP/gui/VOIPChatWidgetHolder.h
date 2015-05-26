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

#pragma once

#include "gui/VOIPNotify.h"

#include <QObject>
#include <QGraphicsEffect>
#include <gui/SpeexProcessor.h>
#include <gui/chat/ChatWidget.h>
#include <gui/common/RsButtonOnText.h>

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
	VOIPChatWidgetHolder(ChatWidget *chatWidget, VOIPNotify *notify);
	virtual ~VOIPChatWidgetHolder();

	virtual void updateStatus(int status);

	void addAudioData(const RsPeerId &peer_id, QByteArray* array) ;
	void addVideoData(const RsPeerId &peer_id, QByteArray* array) ;
	void setAcceptedBandwidth(uint32_t bytes_per_sec) ;

public slots:
	void sendAudioData();
	void sendVideoData();
	void startAudioCapture();
	void startVideoCapture();

private slots:
	void toggleAudioListen();
	void toggleAudioCapture();
	void toggleVideoCapture();
	void hangupCall() ;
	void botMouseEnter();
	void botMouseLeave();

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

	typedef QMap<QString, RSButtonOnText*> button_map;
	button_map buttonMapTakeVideo;

	VOIPNotify *mVOIPNotify;
};

