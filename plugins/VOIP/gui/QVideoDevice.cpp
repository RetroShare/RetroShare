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
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  include <QMediaDevices>
#  include <QCameraDevice>
#  include <QMediaCaptureSession>
#  include <QVideoSink>
#  include <QVideoFrameFormat>
#else
#  include <QCameraInfo>
#  include <QCameraImageCapture>
#endif
#include "QVideoDevice.h"
#include "VideoProcessor.h"

// #define DEBUG_QVIDEODEVICE 1

// Period between two grabbed frames, in ms. Qt5 drives a still-image capture
// with it; Qt6 uses it to sample the continuous frame stream of the video sink.
static const int VIDEO_GRAB_PERIOD_MS = 50 ;	// 20 images per second.

QVideoInputDevice::QVideoInputDevice(QWidget *parent)
  :QObject(parent)
{
	_capture_device = NULL ;
	_video_processor = NULL ;
	_echo_output_device = NULL ;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	_capture_session = NULL ;
	_video_sink = NULL ;
#else
	_timer = NULL ;
	_image_capture = NULL;
#endif
}

QVideoInputDevice::~QVideoInputDevice()
{
    stop() ;	// releases the camera and the capture pipeline
    _video_processor = NULL ;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

bool QVideoInputDevice::stopped() const
{
    return _capture_device == NULL ;
}

void QVideoInputDevice::stop()
{
    _capture_device_info = QCameraDevice();

    if(_capture_device != NULL)
    {
        _capture_device->stop() ;
        delete _capture_device ;
        _capture_device = NULL ;
    }
    // The capture session wires camera -> sink; tear it down after the camera.
    delete _capture_session ;
    _capture_session = NULL ;
    delete _video_sink ;
    _video_sink = NULL ;
    _frame_throttle.invalidate() ;

    if(_echo_output_device != NULL)
        _echo_output_device->showFrameOff() ;
}

void QVideoInputDevice::getAvailableDevices(QList<QString>& device_desc)
{
    device_desc.clear();

    const QList<QCameraDevice> dev_list = QMediaDevices::videoInputs();

    for(auto& cam:dev_list)
        device_desc.push_back(cam.description());
}

void QVideoInputDevice::start(const QString& description)
{
	// make sure everything is re-initialised
	//
	stop() ;

    QCameraDevice caminfo ;

    if(description.isNull())
        caminfo = QMediaDevices::defaultVideoInput();
    else
    {
        const auto cam_list = QMediaDevices::videoInputs();

        for(auto& s:cam_list)
            if(s.description() == description)
                caminfo = s;
    }

    if(caminfo.isNull())
    {
        std::cerr << "No video camera available in this system!" << std::endl;
        return ;
    }
    _capture_device_info = caminfo;
    _capture_device = new QCamera(caminfo);

    if(_capture_device->error() != QCamera::NoError)
    {
        emit cameraCaptureInfo(CANNOT_INITIALIZE_CAMERA,_capture_device->error());
        std::cerr << "Cannot initialise camera. Something's wrong." << std::endl;
        return;
    }

    // Qt6 has no capture-to-buffer still-image API: the camera feeds a
    // QVideoSink through a QMediaCaptureSession and videoFrameChanged() delivers
    // every frame on the GUI thread; handleSinkFrame() samples them.
    _video_sink = new QVideoSink(this);
    _capture_session = new QMediaCaptureSession(this);
    _capture_session->setCamera(_capture_device);
    _capture_session->setVideoSink(_video_sink);

    QObject::connect(_video_sink,SIGNAL(videoFrameChanged(QVideoFrame)),this,SLOT(handleSinkFrame(QVideoFrame)));
    QObject::connect(this,SIGNAL(cameraCaptureInfo(CameraStatus,QCamera::Error)),this,SLOT(errorHandling(CameraStatus,QCamera::Error)));

    emit cameraCaptureInfo(CAMERA_IS_READY,QCamera::NoError);

    _capture_device->start();
}

void QVideoInputDevice::handleSinkFrame(const QVideoFrame& f)
{
    // The sink runs at the camera's native rate; keep the grab period of the
    // Qt5 timer so the encoder sees the same frame rate on both Qt versions.
    if(_frame_throttle.isValid() && _frame_throttle.elapsed() < VIDEO_GRAB_PERIOD_MS)
        return;
    _frame_throttle.start();

    if(!f.isValid() || f.size().isEmpty())
    {
        std::cerr << "Empty frame!" ;
        return;
    }

    QImage image;

    if(f.pixelFormat() == QVideoFrameFormat::Format_Jpeg)
    {
        // Some cameras (Windows in particular) hand out compressed frames.
        QVideoFrame frame(f);

        if(!frame.map(QVideoFrame::ReadOnly))
            return;

        QByteArray data((const char *)frame.bits(0), frame.mappedBytes(0));
        frame.unmap();

        QBuffer buffer;
        buffer.setData(data);
        buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer, "JPG");
        reader.setScaledSize(QSize(640,480));
        image = reader.read();
    }
    else
    {
        // toImage() converts YUV/RGB frames whatever the backend. The encoder
        // rescales to its own frame size; just bound the work on large frames.
        image = f.toImage();

        if(!image.isNull() && image.width() > 640)
            image = image.scaled(640, 480, Qt::KeepAspectRatio);
    }

    if(image.isNull())
    {
        std::cerr << "Cannot convert camera frame to image. Pixel format: " << (int)f.pixelFormat() << std::endl;
        return;
    }

#ifdef DEBUG_QVIDEODEVICE
    std::cerr << "Frame. Pixel format: " << (int)f.pixelFormat() << ". Size: " << image.size().width() << " x " << image.size().height() << std::endl;
#endif

    processImage(image) ;
}

#else // Qt5

bool QVideoInputDevice::stopped() const
{
    return _timer == NULL ;
}

void QVideoInputDevice::stop()
{
    _capture_device_info = QCameraInfo();

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
        std::cerr << "No video camera available in this system!" << std::endl;
        return ;
    }
    _capture_device_info = caminfo;
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

    _timer->start(VIDEO_GRAB_PERIOD_MS) ;

    _capture_device->start();
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

#ifdef DEBUG_QVIDEODEVICE
    std::cerr << "Frame " << id << ". Pixel format: " << frame.pixelFormat() << ". Size: " << image.size().width() << " x " << image.size().height() << std::endl; // if(frame.pixelFormat() != QVideoFrame::Format_Jpeg)
#else
    Q_UNUSED(id);
#endif

    processImage(image) ;
}

#endif // Qt5

void QVideoInputDevice::errorHandling(CameraStatus status,QCamera::Error error)
{
#ifdef DEBUG_QVIDEODEVICE
    std::cerr << "Received msg from camera capture: status=" << (int)status << " error=" << (int)error << std::endl;
#else
    Q_UNUSED(error);
#endif
    if(status == CANNOT_INITIALIZE_CAMERA)
    {
        std::cerr << "Cannot initialize camera. Make sure the QtMultimedia camera backend is installed (package libqt5multimedia5-plugins on Qt5), as this is a common cause for camera not being found." << std::endl;
    }
}

void QVideoInputDevice::processImage(const QImage& image)
{
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
    if(stopped())
        return false ;

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
    setPixmap(QPixmap::fromImage(img).scaled( QSize(height()*4/3,height()),Qt::KeepAspectRatio,Qt::SmoothTransformation)) ;
}
