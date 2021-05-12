/*******************************************************************************
 * plugins/VOIP/gui/QVideoDevice.h                                             *
 *                                                                             *
 * Copyright (C) 2012 by Retroshare Team <retroshare.project@gmail.com>        *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#pragma once

#include <QLabel>
#include <QCamera>
#include "interface/rsVOIP.h"

#include "gui/VideoProcessor.h"

class VideoEncoder ;
class QCameraImageCapture;

// Responsible from displaying the video. The source of the video is
// a VideoDecoder object, which uses a codec.
//
class QVideoOutputDevice: public QLabel
{
	public:
		QVideoOutputDevice(QWidget *parent = 0) ;
		
		void showFrame(const QImage&) ;
		void showFrameOff() ;
};

// Responsible for grabbing the video from the webcam and sending it to the 
// VideoEncoder object.
//
class QVideoInputDevice: public QObject
{
	Q_OBJECT

	public:
		QVideoInputDevice(QWidget *parent = 0) ;
		~QVideoInputDevice() ;

		// Captured images are sent to this encoder. Can be NULL.
		//
		void setVideoProcessor(VideoProcessor *venc) { _video_processor = venc ; }

		// All images received will be echoed to this target. We could use signal/slots, but it's
		// probably faster this way. Can be NULL.
		//
		void setEchoVideoTarget(QVideoOutputDevice *odev) { _echo_output_device = odev ; }
	
		// get the next encoded video data chunk.
		//
		bool getNextEncodedPacket(RsVOIPDataChunk&) ;

        	// gets the estimated current bandwidth required to transmit the encoded data, in B/s
        	//
        	uint32_t currentBandwidth() const ;
        
        	// control
        
		void start() ;
		void stop() ;
		bool stopped();

        enum CameraStatus {
            CAMERA_IS_READY           = 0x00,
            CANNOT_INITIALIZE_CAMERA  = 0x01,
            CAMERA_CANNOT_GRAB_FRAMES = 0x02
        };

protected slots:
        void grabFrame(int id, QVideoFrame f) ;
        void errorHandling(CameraStatus status,QCamera::Error error);

	signals:
		void networkPacketReady() ;
        void cameraCaptureInfo(CameraStatus status,QCamera::Error qt_cam_err_code);

	private:
		VideoProcessor *_video_processor ;
		QTimer *_timer ;
        QCamera *_capture_device;
        QCameraImageCapture *_image_capture;

		QVideoOutputDevice *_echo_output_device ;

		std::list<RsVOIPDataChunk> _out_queue ;
};

