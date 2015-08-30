#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <QTimer>
#include <QPainter>
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

void QVideoInputDevice::stop()
{
	if(_timer != NULL)
	{
		QObject::disconnect(_timer,SIGNAL(timeout()),this,SLOT(grabFrame())) ;
		_timer->stop() ;
		delete _timer ;
		_timer = NULL ;
	}
	if(_capture_device != NULL)
	{
		cvReleaseCapture(&_capture_device) ;
		_capture_device = NULL ;
	}
}
void QVideoInputDevice::start()
{
	// make sure everything is re-initialised 
	//
	stop() ;

	// Initialise la capture  
	static const int cam_id = 0 ;
	_capture_device = cvCaptureFromCAM(cam_id);

	if(_capture_device == NULL)
	{
		std::cerr << "Cannot initialise camera. Something's wrong." << std::endl;
		return ;
	}

	_timer = new QTimer ;
	QObject::connect(_timer,SIGNAL(timeout()),this,SLOT(grabFrame())) ;

	_timer->start(50) ;	// 10 images per second.
}

void QVideoInputDevice::grabFrame()
{
    IplImage *img=cvQueryFrame(_capture_device);    

    if(img == NULL)
    {
	    std::cerr << "(EE) Cannot capture image from camera. Something's wrong." << std::endl;
	    return ;
    }
    // get the image data

    if(img->nChannels != 3)
    {
	    std::cerr << "(EE) expected 3 channels. Got " << img->nChannels << std::endl;
	    return ;
    }

    // convert to RGB and copy to new buffer, because cvQueryFrame tells us to not modify the buffer
    cv::Mat img_rgb;
    cv::cvtColor(cv::Mat(img), img_rgb, CV_BGR2RGB);

    QImage image = QImage(img_rgb.data,img_rgb.cols,img_rgb.rows,QImage::Format_RGB888);

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
  return _video_processor->currentBandwidthOut() ;  
}

QVideoInputDevice::~QVideoInputDevice()
{
	stop() ;
}


QVideoOutputDevice::QVideoOutputDevice(QWidget *parent)
	: QLabel(parent)
{
	showFrameOff() ;
}

void QVideoOutputDevice::showFrameOff()
{
	setPixmap(QPixmap(":/images/video-icon-big.png").scaled(320,256,Qt::KeepAspectRatio,Qt::SmoothTransformation)) ;
}

void QVideoOutputDevice::showFrame(const QImage& img)
{
    std::cerr << "img.size = " << img.width() << " x " << img.height() << std::endl;
	setPixmap(QPixmap::fromImage(img).scaled( QSize(height()*640/480,height()),Qt::IgnoreAspectRatio,Qt::SmoothTransformation)) ;
}

