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
#include <QCameraInfo>
#include <QAbstractVideoSurface>
#include "interface/rsVOIP.h"

#include "gui/VideoProcessor.h"

class VideoEncoder ;

// Minimal video surface used as the camera viewfinder to grab live frames.
// We use a viewfinder surface rather than QVideoProbe because, on the macOS
// AVFoundation backend, QVideoProbe::setSource() reports success but never
// delivers a single frame (camera LED turns on, but no image). A viewfinder
// surface reliably receives every frame on all platforms.
class RsCameraVideoSurface: public QAbstractVideoSurface
{
    Q_OBJECT

    public:
        explicit RsCameraVideoSurface(QObject *parent = nullptr) : QAbstractVideoSurface(parent) {}

        QList<QVideoFrame::PixelFormat> supportedPixelFormats(
                QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const override
        {
            Q_UNUSED(handleType);
            // Advertise the formats the camera backends commonly produce; grabFrame()
            // converts whatever we get (frame.image() + manual fallbacks).
            return QList<QVideoFrame::PixelFormat>()
                << QVideoFrame::Format_RGB32   << QVideoFrame::Format_ARGB32
                << QVideoFrame::Format_BGRA32  << QVideoFrame::Format_RGB24
                << QVideoFrame::Format_NV12    << QVideoFrame::Format_NV21
                << QVideoFrame::Format_YUYV    << QVideoFrame::Format_UYVY
                << QVideoFrame::Format_YUV420P << QVideoFrame::Format_Jpeg;
        }

        bool present(const QVideoFrame &frame) override
        {
            // present() may run on the capture thread (AVFoundation queue); emit a
            // signal so the frame is marshalled to the GUI thread (queued connection).
            if(frame.isValid())
                emit frameAvailable(QVideoFrame(frame));
            return true;
        }

    signals:
        void frameAvailable(const QVideoFrame& frame);
};

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
        
        void start(const QString &description = QString()) ;
		void stop() ;
        bool stopped() const;

        enum CameraStatus {
            CAMERA_IS_READY           = 0x00,
            CANNOT_INITIALIZE_CAMERA  = 0x01,
            CAMERA_CANNOT_GRAB_FRAMES = 0x02
        };

        // Gets the list of available devices. The id string for each device can be used when creating a QVideoDevice

        static void getAvailableDevices(QList<QString>& device_desc);

        QString currentCameraDescriptionString() const { return _capture_device_info.deviceName(); }
protected slots:
        void grabFrame(int id, QVideoFrame f) ;
        void handleSurfaceFrame(const QVideoFrame& f) ;
        void errorHandling(CameraStatus status,QCamera::Error error);

	signals:
		void networkPacketReady() ;
        void cameraCaptureInfo(CameraStatus status,QCamera::Error qt_cam_err_code);

	private:
		VideoProcessor *_video_processor ;
        QCamera *_capture_device;
        RsCameraVideoSurface *_video_surface;
        QCameraInfo _capture_device_info;

		QVideoOutputDevice *_echo_output_device ;

		std::list<RsVOIPDataChunk> _out_queue ;
};

