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
#include "QVideoDevice.h"
#include "VideoProcessor.h"

QVideoInputDevice::QVideoInputDevice(QWidget *parent)
  :QObject(parent)
{
	_timer = NULL ;
	_capture_device = NULL ;
	_video_processor = NULL ;
	_echo_output_device = NULL ;
}

QVideoInputDevice::~QVideoInputDevice()
{
    stop() ;
    _video_processor = NULL ;

    delete _image_capture;
    delete _capture_device;
    delete _timer;
}

bool QVideoInputDevice::stopped()
{
    return _timer == NULL ;
}

void QVideoInputDevice::stop()
{
	if(_timer != NULL)
	{
        _capture_device->stop();
		_timer->stop() ;
		delete _timer ;
		_timer = NULL ;
	}
	if(_capture_device != NULL)
	{
		// the camera will be deinitialized automatically in VideoCapture destructor
        delete _image_capture ;
        delete _capture_device ;

		_capture_device = NULL ;
        _image_capture = NULL ;
    }
}
void QVideoInputDevice::start()
{
	// make sure everything is re-initialised
	//
	stop() ;

	// Initialise la capture
    QCameraInfo caminfo = QCameraInfo::defaultCamera();

    if(caminfo.isNull())
    {
        std::cerr << "No video camera available in this system!" << std::endl;
        return ;
    }
    _capture_device = new QCamera(caminfo);

    if(_capture_device->error() != QCamera::NoError)
    {
        emit cameraCaptureInfo(CANNOT_INITIALIZE_CAMERA,_capture_device->error());
        std::cerr << "Cannot initialise camera. Something's wrong." << std::endl;
        return;
    }
    _capture_device->setCaptureMode(QCamera::CaptureStillImage);

    if(_capture_device->error() == QCamera::NoError)
        emit cameraCaptureInfo(CAMERA_IS_READY,QCamera::NoError);

    _image_capture = new QCameraImageCapture(_capture_device);

    if(!_image_capture->isCaptureDestinationSupported(QCameraImageCapture::CaptureToBuffer))
    {
        emit cameraCaptureInfo(CAMERA_IS_READY,QCamera::NoError);

        delete _capture_device;
        delete _image_capture;
        return;
    }

    _image_capture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);

    QObject::connect(_image_capture,SIGNAL(imageAvailable(int,QVideoFrame)),this,SLOT(grabFrame(int,QVideoFrame)));
    QObject::connect(this,SIGNAL(cameraCaptureInfo(CameraStatus,QCamera::Error)),this,SLOT(errorHandling(CameraStatus,QCamera::Error)));

    _timer = new QTimer ;
    QObject::connect(_timer,SIGNAL(timeout()),_image_capture,SLOT(capture())) ;

    _timer->start(50) ;	// 10 images per second.

    _capture_device->start();
}

void QVideoInputDevice::errorHandling(CameraStatus status,QCamera::Error error)
{
    std::cerr << "Received msg from camera capture: status=" << (int)status << " error=" << (int)error << std::endl;

    if(status == CANNOT_INITIALIZE_CAMERA)
    {
        std::cerr << "Cannot initialize camera. Make sure to install package libqt5multimedia5-plugins, as this is a common cause for camera not being found." << std::endl;
    }
}

void QVideoInputDevice::grabFrame(int id,QVideoFrame frame)
{
    if(frame.size().isEmpty())
    {
        std::cerr << "Empty frame!" ;
        return;
    }

    frame.map(QAbstractVideoBuffer::ReadOnly);
    QByteArray data((const char *)frame.bits(), frame.mappedBytes());
    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer, "JPG");
    reader.setScaledSize(QSize(640,480));
    QImage image(reader.read());

    std::cerr << "Frame " << id << ". Pixel format: " << frame.pixelFormat() << ". Size: " << image.size().width() << " x " << image.size().height() << std::endl; // if(frame.pixelFormat() != QVideoFrame::Format_Jpeg)

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
    if(!_timer)
        return false ;

    if(_video_processor)
        return _video_processor->nextEncodedPacket(chunk) ;
    else
        return false ;
}

uint32_t QVideoInputDevice::currentBandwidth() const
{
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
    std::cerr << "img.size = " << img.width() << " x " << img.height() << std::endl;
    setPixmap(QPixmap::fromImage(img).scaled( QSize(height()*4/3,height()),Qt::IgnoreAspectRatio,Qt::SmoothTransformation)) ;
}

