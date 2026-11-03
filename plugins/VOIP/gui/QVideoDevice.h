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

// Load the version macros directly before moc evaluates the camera guards.
#include <QtGlobal>
#include <QLabel>
#include <QCamera>
#include <QVideoFrame>
#include <QPair>
#include "interface/rsVOIP.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  include <QCameraDevice>
#  include <QMediaDevices>
#  include <QMediaCaptureSession>
#  include <QVideoSink>
#  include <QElapsedTimer>
#else
#  include <QCameraInfo>
#  include <QAbstractVideoSurface>
#  include <QVideoSurfaceFormat>
#  include <QElapsedTimer>
#endif

#include "gui/VideoProcessor.h"

class VideoEncoder ;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && defined(Q_OS_MACOS)
class MacCameraCapture ;
#endif

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

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
// Qt5: viewfinder surface of the QCamera. Receives the continuous frame stream,
// keeps one frame per grab period and converts it to a QImage.
// (Replaces the former QCameraImageCapture still-image grabbing, which does not
// work with the DirectShow backend on Windows: no capture-to-buffer support, and
// buffers that are not always JPEG.)
class VideoFrameGrabber: public QAbstractVideoSurface
{
	Q_OBJECT

	public:
		VideoFrameGrabber(QObject *parent = 0) ;

		virtual QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType type = QAbstractVideoBuffer::NoHandle) const override ;
		virtual bool present(const QVideoFrame& frame) override ;

	signals:
		void frameAvailable(QImage) ;

	private:
		QElapsedTimer _last_frame_time ;
};
#endif

// Responsible for grabbing the video from the webcam and sending it to the 
// VideoEncoder object.
//
// Qt5 grabs the frames of the camera viewfinder (VideoFrameGrabber).
// Qt6 removed QAbstractVideoSurface, so there the camera feeds a
// QVideoSink through a QMediaCaptureSession and we sample its frames at the
// same period as the Qt5 timer. On macOS with Qt6 that pipeline delivers no
// frames at all, so the camera is driven straight from AVFoundation instead
// (see MacCameraCapture).
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

        // ---- Camera selection ----
        // Pairs of (camera id usable with start(), human readable name).
        static void getAvailableDevices(QList<QPair<QString,QString> >& devices);
        // Name of the system default camera (empty if there is no camera).
        static QString defaultCameraLabel();
        static bool cameraExists(const QString& id);
        // Camera chosen by the user (rsVOIP setting), or a null string (= default
        // camera) if nothing was chosen or the chosen camera is not plugged in.
        static QString preferredCamera();

        // Human readable name of the running camera.
        QString currentCameraHumanName() const;
        // Number of frames received since start().
        uint32_t framesReceived() const { return _frames_received ; }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QString currentCameraDescriptionString() const { return _current_camera_description; }
#else
        QString currentCameraDescriptionString() const { return _capture_device_info.deviceName(); }
#endif
protected slots:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        void handleSinkFrame(const QVideoFrame& f) ;
#else
        void grabImage(const QImage& image) ;
        void cameraError(QCamera::Error error) ;
#endif
        void errorHandling(CameraStatus status,QCamera::Error error);

	signals:
		void networkPacketReady() ;
        void cameraCaptureInfo(CameraStatus status,QCamera::Error qt_cam_err_code);
        // Human readable camera problem (camera not found, cannot start...)
        void cameraErrorString(QString);

	private:
        // Common tail of the capture path: encode the image and echo it locally.
        void processImage(const QImage& image) ;

		VideoProcessor *_video_processor ;
        QCamera *_capture_device;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        // Qt6 (and macOS AVFoundation) deliver frames at the camera's native rate;
        // this keeps only one frame per grab period. Returns true when the frame
        // is to be used.
        bool passFrameThrottle() ;

        QMediaCaptureSession *_capture_session;
        QVideoSink *_video_sink;
        QString _current_camera_description;
        QElapsedTimer _frame_throttle;

        // Once the camera authorization has been granted we stop re-requesting it,
        // otherwise every start() would fire another request.
        bool _camera_permission_granted ;
#  if defined(Q_OS_MACOS)
        // Frames from the AVFoundation capture (already QImage, on the GUI thread).
        void handleMacFrame(const QImage& image) ;

        // Owns the AVCaptureSession. NULL until the first start().
        MacCameraCapture *_mac_capture ;
#  endif
#else
        VideoFrameGrabber *_grabber;
        QCameraInfo _capture_device_info;
#endif

		QVideoOutputDevice *_echo_output_device ;
        uint32_t _frames_received ;

		std::list<RsVOIPDataChunk> _out_queue ;
};
