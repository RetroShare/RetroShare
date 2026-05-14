/*******************************************************************************
 * plugins/VOIP/gui/QVideoDevice.cpp                                           *
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

#include <QTimer>
#include <QPainter>
#include <QImageReader>
#include <QBuffer>
#include <QCamera>
#include <QCameraInfo>
#include <QCameraImageCapture>
#include <util/rsdebug.h>
#include "QVideoDevice.h"
#include "VideoProcessor.h"

// #define DEBUG_QVIDEODEVICE 1

QVideoInputDevice::QVideoInputDevice(QWidget *parent)
  :QObject(parent)
{
	_timer = NULL ;
	_capture_device = NULL ;
	_video_processor = NULL ;
	_echo_output_device = NULL ;
	_image_capture = NULL;
    _video_probe = NULL;
}

QVideoInputDevice::~QVideoInputDevice()
{
    stop() ;
    _video_processor = NULL ;

    delete _video_probe;
    delete _image_capture;
    delete _capture_device;
    delete _timer;
}

bool QVideoInputDevice::stopped() const
{
    return _timer == NULL ;
}

void QVideoInputDevice::stop()
{
    _capture_device_info = QCameraInfo();

	if(_timer != NULL)
	{
        if (_capture_device) _capture_device->stop();
		_timer->stop() ;
		delete _timer ;
		_timer = NULL ;
	}
	if(_capture_device != NULL)
	{
		// the camera will be deinitialized automatically in VideoCapture destructor
        delete _video_probe;
        delete _image_capture ;
        delete _capture_device ;

        _video_probe = NULL;
		_capture_device = NULL ;
        _image_capture = NULL ;
    }
    if(_echo_output_device != NULL)
        _echo_output_device->showFrameOff() ;
}
void QVideoInputDevice::getAvailableDevices(QList<QString>& device_desc)
{
    device_desc.clear();

    QList<QCameraInfo> dev_list = QCameraInfo::availableCameras();

    for(auto& cam:dev_list)
        device_desc.push_back(cam.deviceName());
}

void QVideoInputDevice::start(const QString& description)
{
	// make sure everything is re-initialised
	//
	stop() ;

    QCameraInfo caminfo ;

    if(description.isNull())
        caminfo = QCameraInfo::defaultCamera();
    else
    {
        auto cam_list = QCameraInfo::availableCameras();

        for(auto& s:cam_list)
            if(s.deviceName() == description)
                caminfo = s;
    }

    if(caminfo.isNull())
    {
        RsDbg() << "DISTANT_VOIP: [CRITICAL] No video camera available in this system!";
        return ;
    }
    _capture_device_info = caminfo;
    RsDbg() << "DISTANT_VOIP: Initializing camera: " << caminfo.description().toStdString() << " (ID: " << caminfo.deviceName().toStdString() << ")";
    
    _capture_device = new QCamera(caminfo);

    if(_capture_device->error() != QCamera::NoError)
    {
        emit cameraCaptureInfo(CANNOT_INITIALIZE_CAMERA,_capture_device->error());
        RsDbg() << "DISTANT_VOIP: [ERROR] Cannot initialise camera. Error code: " << (int)_capture_device->error();
        return;
    }

    _video_probe = new QVideoProbe(this);
    if (_video_probe->setSource(_capture_device)) {
        RsDbg() << "DISTANT_VOIP: Video Probe successfully attached to camera.";
        QObject::connect(_video_probe, SIGNAL(videoFrameProbed(QVideoFrame)), this, SLOT(handleProbedFrame(QVideoFrame)));
    } else {
        RsDbg() << "DISTANT_VOIP: [WARNING] Video Probe could NOT be attached. Falling back to StillImage capture.";
    }

    _capture_device->setCaptureMode(QCamera::CaptureVideo);

    if(_capture_device->error() == QCamera::NoError)
    {
        RsDbg() << "DISTANT_VOIP: Camera object created and mode set to CaptureVideo.";
        emit cameraCaptureInfo(CAMERA_IS_READY,QCamera::NoError);
    }

    _image_capture = new QCameraImageCapture(_capture_device);
    _image_capture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
    QObject::connect(_image_capture,SIGNAL(imageAvailable(int,QVideoFrame)),this,SLOT(grabFrame(int,QVideoFrame)));
    QObject::connect(this,SIGNAL(cameraCaptureInfo(CameraStatus,QCamera::Error)),this,SLOT(errorHandling(CameraStatus,QCamera::Error)));

    // Fallback timer only if probe failed
    if (!_video_probe->isActive()) {
        _timer = new QTimer ;
        QObject::connect(_timer,SIGNAL(timeout()),_image_capture,SLOT(capture())) ;
        RsDbg() << "DISTANT_VOIP: Starting fallback capture timer (100ms)...";
        _timer->start(100) ;
    }

    RsDbg() << "DISTANT_VOIP: Finalizing camera start(). LED should turn on now.";
    _capture_device->start();
}

void QVideoInputDevice::handleProbedFrame(const QVideoFrame& f)
{
    static int p_id = 0;
    grabFrame(p_id++, f);
}

void QVideoInputDevice::errorHandling(CameraStatus status,QCamera::Error error)
{
#ifdef DEBUG_QVIDEODEVICE
    std::cerr << "Received msg from camera capture: status=" << (int)status << " error=" << (int)error << std::endl;
#else
    Q_UNUSED(error);
#endif
    if(status == CANNOT_INITIALIZE_CAMERA)
    {
        std::cerr << "Cannot initialize camera. Make sure to install package libqt5multimedia5-plugins, as this is a common cause for camera not being found." << std::endl;
    }
}

void QVideoInputDevice::grabFrame(int id,QVideoFrame frame)
{
    if(frame.size().isEmpty())
    {
        RsDbg() << "DISTANT_VOIP: [WARNING] Received an empty frame from camera!";
        return;
    }

    static int frame_count = 0;
    if (frame_count++ % 20 == 0) {
        RsDbg() << "DISTANT_VOIP: Frame received. ID=" << id << " Size=" << frame.width() << "x" << frame.height() << " Format=" << (int)frame.pixelFormat();
    }

    frame.map(QAbstractVideoBuffer::ReadOnly);
    
    QImage image;
    
    // Check if it's already a JPEG (common on some Windows cams)
    if (frame.pixelFormat() == QVideoFrame::Format_Jpeg) {
        QByteArray data((const char *)frame.bits(), frame.mappedBytes());
        QBuffer buffer;
        buffer.setData(data);
        buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer, "JPG");
        reader.setScaledSize(QSize(640,480));
        image = reader.read();
    } else {
        // Use Qt's native conversion which handles YUV, RGB, etc.
        // If your Qt version is 5.15+, frame.image() is available.
        // Otherwise, we create a QImage from the raw bits.
        image = frame.image();
        
        if (image.isNull()) {
            // Manual fallback for common formats if .image() fails
            image = QImage(frame.bits(), frame.width(), frame.height(), frame.bytesPerLine(), QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat()));
        }
        
        if (!image.isNull() && (image.width() > 640)) {
            image = image.scaled(640, 480, Qt::KeepAspectRatio);
        }
    }

    if (image.isNull()) {
        RsDbg() << "DISTANT_VOIP: [ERROR] Failed to convert QVideoFrame to QImage. Format=" << (int)frame.pixelFormat();
        frame.unmap();
        return;
    }
    
    frame.unmap();

#ifdef DEBUG_QVIDEODEVICE
    std::cerr << "Frame " << id << ". Pixel format: " << frame.pixelFormat() << ". Size: " << image.size().width() << " x " << image.size().height() << std::endl; // if(frame.pixelFormat() != QVideoFrame::Format_Jpeg)
#else
    Q_UNUSED(id);
#endif

    if(_video_processor != NULL)
    {
        _video_processor->processImage(image) ;

        emit networkPacketReady() ;
    }
    if(_echo_output_device != NULL)
        _echo_output_device->showFrame(image) ;
}

bool QVideoInputDevice::getNextEncodedPacket(RsVOIPDataChunk& chunk)
{
    if(_video_processor)
        return _video_processor->nextEncodedPacket(chunk) ;
    else
        return false ;
}

uint32_t QVideoInputDevice::currentBandwidth() const
{
    if(stopped())
        return 0;
    else
        return _video_processor->currentBandwidthOut() ;
}

QVideoOutputDevice::QVideoOutputDevice(QWidget *parent)
    : QLabel(parent)
{
	showFrameOff() ;
}

void QVideoOutputDevice::showFrameOff()
{
	setPixmap(QPixmap(":/images/video-icon-big.png").scaled(QSize(height()*4/3,height()),Qt::KeepAspectRatio,Qt::SmoothTransformation)) ;
	setAlignment(Qt::AlignCenter);
}

void QVideoOutputDevice::showFrame(const QImage& img)
{
#ifdef DEBUG_QVIDEODEVICE
    std::cerr << "img.size = " << img.width() << " x " << img.height() << std::endl;
#endif
    setPixmap(QPixmap::fromImage(img).scaled( QSize(height()*4/3,height()),Qt::IgnoreAspectRatio,Qt::SmoothTransformation)) ;
}

