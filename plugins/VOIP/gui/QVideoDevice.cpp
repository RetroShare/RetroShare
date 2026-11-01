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
#  include <QCameraViewfinderSettings>
#endif
#include <cstdlib>
#include "QVideoDevice.h"
#include "VideoProcessor.h"

// #define DEBUG_QVIDEODEVICE 1

// Period between two grabbed frames, in ms. Qt5 drives a still-image capture
// with it; Qt6 uses it to sample the continuous frame stream of the camera.
static const int VIDEO_GRAB_PERIOD_MS = 50 ;	// 20 images per second.

QVideoInputDevice::QVideoInputDevice(QWidget *parent)
  :QObject(parent)
{
	_capture_device = NULL ;
	_video_processor = NULL ;
	_echo_output_device = NULL ;
	_frames_received = 0 ;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	_capture_session = NULL ;
	_video_sink = NULL ;
	_camera_permission_granted = false ;
#  if defined(Q_OS_MACOS)
	_mac_capture = NULL ;
#  endif
#else
	_grabber = NULL ;
#endif
}

QVideoInputDevice::~QVideoInputDevice()
{
    stop() ;	// releases the camera and the capture pipeline
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
    _frames_received = 0 ;

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
                std::cerr << "VOIP: [mac] camera access not granted, no video." << std::endl;
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
                    std::cerr << "VOIP: camera permission not granted, no video." << std::endl;
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
        std::cerr << "No video camera available in this system!" << std::endl;
        emit cameraErrorString(tr("No camera found on this system."));
        return ;
    }
    _current_camera_description = caminfo.description();
    _capture_device = new QCamera(caminfo);

    if(_capture_device->error() != QCamera::NoError)
    {
        emit cameraCaptureInfo(CANNOT_INITIALIZE_CAMERA,_capture_device->error());
        std::cerr << "Cannot initialise camera. Something's wrong." << std::endl;
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

    // Qt6 has no capture-to-buffer still-image API: the camera feeds a
    // QVideoSink through a QMediaCaptureSession and videoFrameChanged() delivers
    // every frame on the GUI thread; handleSinkFrame() samples them.
    _video_sink = new QVideoSink(this);
    _capture_session = new QMediaCaptureSession(this);
    _capture_session->setCamera(_capture_device);
    _capture_session->setVideoSink(_video_sink);

    QObject::connect(_video_sink, &QVideoSink::videoFrameChanged, this, &QVideoInputDevice::handleSinkFrame);

    // QCamera reports start failures asynchronously through errorOccurred(), not
    // through error() right after construction.
    QObject::connect(_capture_device, &QCamera::errorOccurred, this,
        [this](QCamera::Error err, const QString& msg)
        {
            std::cerr << "VOIP: camera error " << (int)err << ": " << msg.toStdString() << std::endl;
            emit cameraErrorString(msg.isEmpty() ? tr("Camera error %1").arg((int)err) : msg);
        });

    QObject::connect(this,SIGNAL(cameraCaptureInfo(CameraStatus,QCamera::Error)),this,SLOT(errorHandling(CameraStatus,QCamera::Error)));

    emit cameraCaptureInfo(CAMERA_IS_READY,QCamera::NoError);

    _capture_device->start();
#endif // Q_OS_MACOS
}

bool QVideoInputDevice::passFrameThrottle()
{
    // The camera runs at its native rate; keep the grab period of the Qt5 timer
    // so the encoder sees the same frame rate on both Qt versions.
    if(_frame_throttle.isValid() && _frame_throttle.elapsed() < VIDEO_GRAB_PERIOD_MS)
        return false;

    _frame_throttle.start();
    return true;
}

void QVideoInputDevice::handleSinkFrame(const QVideoFrame& f)
{
    if(!passFrameThrottle())
        return;

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

#if defined(Q_OS_MACOS)
void QVideoInputDevice::handleMacFrame(const QImage& image)
{
    if(!passFrameThrottle() || image.isNull())
        return;

    processImage((image.width() > 640) ? image.scaled(640, 480, Qt::KeepAspectRatio) : image) ;
}
#endif

#else // Qt5

// =====================================================================================
//                              VideoFrameGrabber (Qt5)
// =====================================================================================

VideoFrameGrabber::VideoFrameGrabber(QObject *parent)
    : QAbstractVideoSurface(parent)
{
}

QList<QVideoFrame::PixelFormat> VideoFrameGrabber::supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const
{
    if(type != QAbstractVideoBuffer::NoHandle)
        return QList<QVideoFrame::PixelFormat>() ;

    // Formats that convert directly to QImage first, so that the backend picks them if it can.
    QList<QVideoFrame::PixelFormat> formats ;
    formats << QVideoFrame::Format_RGB32
            << QVideoFrame::Format_ARGB32
            << QVideoFrame::Format_ARGB32_Premultiplied
            << QVideoFrame::Format_RGB24
            << QVideoFrame::Format_RGB565
            << QVideoFrame::Format_RGB555
            << QVideoFrame::Format_BGR32
            << QVideoFrame::Format_BGRA32
            << QVideoFrame::Format_BGR24
            << QVideoFrame::Format_Jpeg ;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    // QVideoFrame::image() (Qt >= 5.15) also converts YUV formats.
    formats << QVideoFrame::Format_YUYV
            << QVideoFrame::Format_UYVY
            << QVideoFrame::Format_YUV420P
            << QVideoFrame::Format_YV12
            << QVideoFrame::Format_NV12
            << QVideoFrame::Format_NV21 ;
#endif
    return formats ;
}

bool VideoFrameGrabber::present(const QVideoFrame& input)
{
    if(!input.isValid())
        return false ;

    // The camera runs at its native rate; keep one frame per grab period.
    if(_last_frame_time.isValid() && _last_frame_time.elapsed() < VIDEO_GRAB_PERIOD_MS)
        return true ;

    _last_frame_time.restart() ;

    QVideoFrame frame(input) ;
    QImage img ;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    img = frame.image() ;	// deep copy; handles RGB, YUV and JPEG
#else
    if(!frame.map(QAbstractVideoBuffer::ReadOnly))
        return true ;

    if(frame.pixelFormat() == QVideoFrame::Format_Jpeg)
        img.loadFromData(frame.bits(), frame.mappedBytes(), "JPG") ;
    else
    {
        QImage::Format f = QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat()) ;

        if(f != QImage::Format_Invalid)
            img = QImage(frame.bits(), frame.width(), frame.height(), frame.bytesPerLine(), f).copy() ;
    }
    frame.unmap() ;
#endif

    if(img.isNull())
    {
#ifdef DEBUG_QVIDEODEVICE
        std::cerr << "Cannot convert camera frame to image. Pixel format: " << (int)frame.pixelFormat() << std::endl;
#endif
        return true ;
    }

    // Some backends (e.g. DirectShow) deliver images bottom-up.
    if(surfaceFormat().scanLineDirection() == QVideoSurfaceFormat::BottomToTop)
        img = img.mirrored(false,true) ;

    emit frameAvailable(img) ;
    return true ;
}

// =====================================================================================
//                              QVideoInputDevice (Qt5)
// =====================================================================================

bool QVideoInputDevice::stopped() const
{
    return _capture_device == NULL ;
}

void QVideoInputDevice::stop()
{
    _capture_device_info = QCameraInfo();
    _frames_received = 0 ;

    if(_grabber != NULL)
        _grabber->disconnect(this) ;	// no more frames to us from now on

	if(_capture_device != NULL)
	{
        _capture_device->disconnect(this) ;
        _capture_device->stop();

        // The QCamera destructor unloads the camera and releases the viewfinder.
        delete _capture_device ;
		_capture_device = NULL ;
    }
    if(_grabber != NULL)
    {
        _grabber->deleteLater() ;	// a queued frame may still reference it
        _grabber = NULL ;
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

    if(description.isEmpty())
        caminfo = QCameraInfo::defaultCamera();
    else
    {
        auto cam_list = QCameraInfo::availableCameras();

        for(auto& s:cam_list)
            if(s.deviceName() == description)
                caminfo = s;

        if(caminfo.isNull())
        {
            std::cerr << "Camera \"" << description.toStdString() << "\" not found. Using default camera." << std::endl;
            caminfo = QCameraInfo::defaultCamera();
        }
    }

    if(caminfo.isNull())
    {
        std::cerr << "No video camera available in this system!" << std::endl;
        emit cameraErrorString(tr("No camera found on this system."));
        return ;
    }
    _capture_device_info = caminfo;
    _capture_device = new QCamera(caminfo);

    if(_capture_device->error() != QCamera::NoError)
    {
        QCamera::Error err = _capture_device->error() ;
        emit cameraErrorString(_capture_device->errorString());
        stop() ;
        emit cameraCaptureInfo(CANNOT_INITIALIZE_CAMERA,err);
        std::cerr << "Cannot initialise camera. Something's wrong." << std::endl;
        return;
    }

    QObject::connect(_capture_device,SIGNAL(error(QCamera::Error)),this,SLOT(cameraError(QCamera::Error)));
    QObject::connect(this,SIGNAL(cameraCaptureInfo(CameraStatus,QCamera::Error)),this,SLOT(errorHandling(CameraStatus,QCamera::Error)),Qt::UniqueConnection);

    _grabber = new VideoFrameGrabber ;
    QObject::connect(_grabber,SIGNAL(frameAvailable(QImage)),this,SLOT(grabImage(QImage)),Qt::QueuedConnection);

    _capture_device->setViewfinder(_grabber);
    _capture_device->setCaptureMode(QCamera::CaptureViewfinder);

    // Pick a resolution close to 640x480: cameras default to their highest
    // resolution, which is a waste of CPU since the encoder downscales anyway.
    // (load() is synchronous with the DirectShow backend; with others the list may
    // still be empty here, then the default resolution is kept.)
    _capture_device->load();

    const QList<QSize> resolutions = _capture_device->supportedViewfinderResolutions();
    if(!resolutions.isEmpty())
    {
        QSize best ;
        int best_score = -1 ;

        for(const QSize& sz:resolutions)
        {
            int score = std::abs(sz.width() - 640) + std::abs(sz.height() - 480) ;
            if(best_score < 0 || score < best_score)
            {
                best = sz ;
                best_score = score ;
            }
        }
        QCameraViewfinderSettings settings = _capture_device->viewfinderSettings();
        settings.setResolution(best);
        _capture_device->setViewfinderSettings(settings);
    }

    _capture_device->start();

    if(_capture_device->error() != QCamera::NoError)
    {
        QCamera::Error err = _capture_device->error() ;
        emit cameraErrorString(_capture_device->errorString());
        stop() ;
        emit cameraCaptureInfo(CANNOT_INITIALIZE_CAMERA,err);
        return ;
    }

    emit cameraCaptureInfo(CAMERA_IS_READY,QCamera::NoError);
}

void QVideoInputDevice::grabImage(const QImage& image)
{
    if(_capture_device == NULL || image.isNull())	// frame arrived after stop()
        return ;

    processImage((image.width() > 640) ? image.scaled(640, 480, Qt::KeepAspectRatio) : image) ;
}

void QVideoInputDevice::cameraError(QCamera::Error error)
{
    if(error == QCamera::NoError)
        return ;

    QString s = _capture_device ? _capture_device->errorString() : QString() ;
    std::cerr << "VOIP: camera error " << (int)error << ": " << s.toStdString() << std::endl;

    emit cameraErrorString(s.isEmpty() ? tr("Camera error %1").arg((int)error) : s);
    emit cameraCaptureInfo(CANNOT_INITIALIZE_CAMERA,error);
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
    ++_frames_received ;

    if(_video_processor != NULL)
    {
        _video_processor->processImage(image) ;

        emit networkPacketReady() ;
    }
    if(_echo_output_device != NULL)
        _echo_output_device->showFrame(image) ;
}

// =====================================================================================
//                                   Camera selection
// =====================================================================================

void QVideoInputDevice::getAvailableDevices(QList<QPair<QString,QString> >& devices)
{
    devices.clear();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt6 (and macOS) identify cameras by their description (see start()).
    QList<QString> names;
    getAvailableDevices(names);

    for(const QString& n : names)
        devices.push_back(qMakePair(n, n));
#else
    for(const QCameraInfo& cam : QCameraInfo::availableCameras())
        devices.push_back(qMakePair(cam.deviceName(), cam.description().isEmpty() ? cam.deviceName() : cam.description()));
#endif
}

QString QVideoInputDevice::defaultCameraLabel()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  if defined(Q_OS_MACOS)
    QList<QString> names;
    getAvailableDevices(names);
    return names.isEmpty() ? QString() : names.front();
#  else
    return QMediaDevices::defaultVideoInput().description();
#  endif
#else
    return QCameraInfo::defaultCamera().description();
#endif
}

bool QVideoInputDevice::cameraExists(const QString& id)
{
    QList<QString> ids;
    getAvailableDevices(ids);
    return ids.contains(id);
}

QString QVideoInputDevice::preferredCamera()
{
    if(!rsVOIP)
        return QString();

    const QString id = QString::fromStdString(rsVOIP->getVoipVideoInputDevice());

    if(id.isEmpty())
        return QString();	// null string: default camera

    if(cameraExists(id))
        return id;

    std::cerr << "VOIP: selected camera \"" << id.toStdString() << "\" is not available. Using default camera." << std::endl;
    return QString();
}

QString QVideoInputDevice::currentCameraHumanName() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return _current_camera_description;
#else
    return _capture_device_info.description().isEmpty() ? _capture_device_info.deviceName() : _capture_device_info.description();
#endif
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
    if(stopped() || _video_processor == NULL)
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
