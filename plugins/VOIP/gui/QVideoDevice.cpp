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

#include <QPainter>
#include <QImageReader>
#include <QBuffer>
#include <QCamera>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  include <QMediaDevices>
#  include <QCameraDevice>
#  include <QCameraFormat>
#  include <QMediaCaptureSession>
#  include <QVideoSink>
#  include <QVideoFrameFormat>
#  include <QPointer>
#  if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#    include <QPermissions>
#    include <QCoreApplication>
#  endif
#  if defined(Q_OS_MACOS)
#    include "MacCameraPermission.h"
#    include "MacCameraCapture.h"
#  endif
#else
#  include <QCameraInfo>
#endif
#include <util/rsdebug.h>
#include "QVideoDevice.h"
#include "VideoProcessor.h"

// #define DEBUG_QVIDEODEVICE 1

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
// Period between two grabbed frames, in ms, used to sample the continuous frame
// stream of the Qt6 camera pipeline (and of the macOS AVFoundation capture).
static const int VIDEO_GRAB_PERIOD_MS = 50 ;	// 20 images per second.
#endif

QVideoInputDevice::QVideoInputDevice(QWidget *parent)
  :QObject(parent)
{
	_capture_device = NULL ;
	_video_processor = NULL ;
	_echo_output_device = NULL ;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	_capture_session = NULL ;
	_video_sink = NULL ;
	_camera_permission_granted = false ;
#  if defined(Q_OS_MACOS)
	_mac_capture = NULL ;
#  endif
#else
    _video_surface = NULL;
#endif
}

QVideoInputDevice::~QVideoInputDevice()
{
    stop() ;             // releases _capture_device and the capture pipeline
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && defined(Q_OS_MACOS)
    delete _mac_capture ;
    _mac_capture = NULL ;
#endif
    _video_processor = NULL ;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

bool QVideoInputDevice::stopped() const
{
#if defined(Q_OS_MACOS)
    return _mac_capture == NULL || !_mac_capture->isRunning() ;
#else
    return _capture_device == NULL ;
#endif
}

void QVideoInputDevice::stop()
{
    _current_camera_description.clear();
    _frame_throttle.invalidate() ;

#if defined(Q_OS_MACOS)
    // Keep the AVFoundation object around (cheap) so it can be restarted.
    if(_mac_capture != NULL)
        _mac_capture->stop() ;
#endif

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

    if(_echo_output_device != NULL)
        _echo_output_device->showFrameOff() ;
}

void QVideoInputDevice::getAvailableDevices(QList<QString>& device_desc)
{
    device_desc.clear();

#if defined(Q_OS_MACOS)
    MacCameraCapture::availableDevices(device_desc);
#else
    const QList<QCameraDevice> dev_list = QMediaDevices::videoInputs();

    for(auto& cam:dev_list)
        device_desc.push_back(cam.description());
#endif
}

void QVideoInputDevice::start(const QString& description)
{
	// make sure everything is re-initialised
	//
	stop() ;

#if defined(Q_OS_MACOS)
    // Qt6's QPermission API never contacts the OS for the camera on macOS
    // (qApp->requestPermission(QCameraPermission) answers Denied without a TCC
    // request, so the system prompt never shows). Ask AVFoundation directly,
    // which is what the microphone path already does, then re-enter start()
    // once granted. The callback may fire later, so guard against this object
    // having been deleted in between.
    if(!_camera_permission_granted)
    {
        QPointer<QVideoInputDevice> self(this);

        requestMacCameraAccess([self, description](bool granted)
        {
            if(!self)
                return;

            if(granted)
            {
                self->_camera_permission_granted = true ;
                self->start(description) ;	// re-enter, now authorized
            }
            else
                RsDbg() << "DISTANT_VOIP: [mac] camera access not granted, no video." ;
        });
        return;
    }

    // Authorized: capture straight from AVFoundation. Qt6's QMediaCaptureSession/
    // QVideoSink pipeline delivers no frames on macOS; frames arrive here as
    // QImage on the GUI thread and go through the same encoder + echo path as on
    // every other platform.
    if(_mac_capture == NULL)
        _mac_capture = new MacCameraCapture();

    _current_camera_description = description;

    QObject::connect(this,SIGNAL(cameraCaptureInfo(CameraStatus,QCamera::Error)),this,SLOT(errorHandling(CameraStatus,QCamera::Error)));

    if(_mac_capture->start(description, [this](const QImage& image){ handleMacFrame(image); }))
        emit cameraCaptureInfo(CAMERA_IS_READY, QCamera::NoError);
    else
        emit cameraCaptureInfo(CANNOT_INITIALIZE_CAMERA, QCamera::NoError);

    return;
#else

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    // Since Qt 6.5 the camera must be authorized through the QPermission API
    // before QCamera::start(). checkPermission() can wrongly report Denied, so
    // always request until an explicit Granted, then re-enter start().
    if(!_camera_permission_granted)
    {
        QCameraPermission cameraPermission;

        qApp->requestPermission(cameraPermission, this,
            [this, description](const QPermission& perm)
            {
                if(perm.status() == Qt::PermissionStatus::Granted)
                {
                    _camera_permission_granted = true ;
                    start(description) ;	// re-enter, now authorized
                }
                else
                    RsDbg() << "DISTANT_VOIP: [ERROR] camera permission not granted, no video." ;
            });
        return;
    }
#endif

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
        RsDbg() << "DISTANT_VOIP: [CRITICAL] No video camera available in this system!";
        return ;
    }
    _current_camera_description = caminfo.description();
    RsDbg() << "DISTANT_VOIP: Initializing camera: " << caminfo.description().toStdString() << " (ID: " << caminfo.id().toStdString() << ")";

    _capture_device = new QCamera(caminfo);

    if(_capture_device->error() != QCamera::NoError)
    {
        emit cameraCaptureInfo(CANNOT_INITIALIZE_CAMERA,_capture_device->error());
        RsDbg() << "DISTANT_VOIP: [ERROR] Cannot initialise camera. Error code: " << (int)_capture_device->error();
        return;
    }

    // QCamera can silently stay inactive when no capture format is selected.
    // Pick one explicitly, preferring a 640-1280 wide format, else the first.
    {
        const QList<QCameraFormat> formats = caminfo.videoFormats();

        if(!formats.isEmpty())
        {
            QCameraFormat chosen = formats.first();

            for(const QCameraFormat& f : formats)
                if(f.resolution().width() >= 640 && f.resolution().width() <= 1280)
                {
                    chosen = f;
                    break;
                }

            _capture_device->setCameraFormat(chosen);
        }
    }

    // Qt6 grabs frames through a QVideoSink fed by a QMediaCaptureSession
    // (QAbstractVideoSurface + setViewfinder were removed). videoFrameChanged()
    // is delivered on the GUI thread; handleSurfaceFrame() samples them.
    _video_sink = new QVideoSink(this);
    _capture_session = new QMediaCaptureSession(this);
    _capture_session->setCamera(_capture_device);
    _capture_session->setVideoSink(_video_sink);

    QObject::connect(_video_sink, &QVideoSink::videoFrameChanged, this, &QVideoInputDevice::handleSurfaceFrame);

    // QCamera reports start failures asynchronously through errorOccurred(), not
    // through error() right after construction.
    QObject::connect(_capture_device, &QCamera::errorOccurred, this,
        [](QCamera::Error err, const QString& msg)
        { RsDbg() << "DISTANT_VOIP: [Qt6] QCamera error " << (int)err << ": " << msg.toStdString(); });

    QObject::connect(this,SIGNAL(cameraCaptureInfo(CameraStatus,QCamera::Error)),this,SLOT(errorHandling(CameraStatus,QCamera::Error)));

    emit cameraCaptureInfo(CAMERA_IS_READY,QCamera::NoError);

    _capture_device->start();
#endif // Q_OS_MACOS
}

bool QVideoInputDevice::passFrameThrottle()
{
    // The camera runs at its native rate; keep a fixed grab period so the
    // encoder sees a bounded frame rate.
    if(_frame_throttle.isValid() && _frame_throttle.elapsed() < VIDEO_GRAB_PERIOD_MS)
        return false;

    _frame_throttle.start();
    return true;
}

void QVideoInputDevice::handleSurfaceFrame(const QVideoFrame& f)
{
    if(!passFrameThrottle())
        return;

    static int p_id = 0;
    grabFrame(p_id++, f);
}

#if defined(Q_OS_MACOS)
void QVideoInputDevice::handleMacFrame(const QImage& image)
{
    if(!passFrameThrottle() || image.isNull())
        return;

    // Mirror grabFrame()'s policy: keep VOIP frames modest (<= 640 wide).
    deliverImage((image.width() > 640) ? image.scaled(640, 480, Qt::KeepAspectRatio) : image) ;
}
#endif

#else // Qt5

bool QVideoInputDevice::stopped() const
{
    return _capture_device == NULL ;
}

void QVideoInputDevice::stop()
{
    _capture_device_info = QCameraInfo();

	if(_capture_device != NULL)
	{
        _capture_device->stop() ;
        delete _capture_device ;     // detaches and releases the viewfinder surface
        _capture_device = NULL ;
    }
    // Delete the surface only after the camera that referenced it is gone.
    delete _video_surface ;
    _video_surface = NULL ;

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

    _capture_device->setCaptureMode(QCamera::CaptureVideo);

    // Grab frames through a viewfinder surface. This works on every backend,
    // unlike QVideoProbe which attaches but never delivers frames on macOS
    // (AVFoundation) -> camera LED on, but no image.
    _video_surface = new RsCameraVideoSurface(this);
    QObject::connect(_video_surface,SIGNAL(frameAvailable(QVideoFrame)),this,SLOT(handleSurfaceFrame(QVideoFrame)));
    _capture_device->setViewfinder(_video_surface);
    RsDbg() << "DISTANT_VOIP: Viewfinder surface attached to camera.";

    QObject::connect(this,SIGNAL(cameraCaptureInfo(CameraStatus,QCamera::Error)),this,SLOT(errorHandling(CameraStatus,QCamera::Error)));

    if(_capture_device->error() == QCamera::NoError)
    {
        RsDbg() << "DISTANT_VOIP: Camera object created and mode set to CaptureVideo.";
        emit cameraCaptureInfo(CAMERA_IS_READY,QCamera::NoError);
    }

    RsDbg() << "DISTANT_VOIP: Finalizing camera start(). LED should turn on now.";
    _capture_device->start();
}

void QVideoInputDevice::handleSurfaceFrame(const QVideoFrame& f)
{
    static int p_id = 0;
    grabFrame(p_id++, f);
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

    QImage image;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Check if it's already a JPEG (common on some Windows cams)
    if (frame.pixelFormat() == QVideoFrameFormat::Format_Jpeg) {
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
    } else {
        // Qt6's QVideoFrame::toImage() handles YUV/RGB conversion robustly.
        image = frame.toImage();

        if (!image.isNull() && (image.width() > 640))
            image = image.scaled(640, 480, Qt::KeepAspectRatio);
    }

    if (image.isNull()) {
        RsDbg() << "DISTANT_VOIP: [ERROR] Failed to convert QVideoFrame to QImage. Format=" << (int)frame.pixelFormat();
        return;
    }
#else
    frame.map(QAbstractVideoBuffer::ReadOnly);

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
#endif

#ifdef DEBUG_QVIDEODEVICE
    std::cerr << "Frame " << id << ". Pixel format: " << frame.pixelFormat() << ". Size: " << image.size().width() << " x " << image.size().height() << std::endl; // if(frame.pixelFormat() != QVideoFrame::Format_Jpeg)
#else
    Q_UNUSED(id);
#endif

    deliverImage(image) ;
}

void QVideoInputDevice::deliverImage(const QImage& image)
{
    if(_video_processor != NULL)
    {
        _video_processor->processImage(image) ;

        emit networkPacketReady() ;
    }
    if(_echo_output_device != NULL)
        // Self-view is mirrored (horizontal flip), like every mainstream video
        // app, so it feels natural. The frame sent to the peer (processImage
        // above) stays un-mirrored, so the remote sees us the right way round.
        _echo_output_device->showFrame(image.mirrored(true, false)) ;
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
