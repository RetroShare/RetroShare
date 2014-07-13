#include <cv.h>
#include <highgui.h>

#include <QTimer>
#include <QPainter>
#include "QVideoDevice.h"
#include "VideoProcessor.h"

QVideoInputDevice::QVideoInputDevice(QWidget *parent)
{
	_timer = NULL ;
	_capture_device = NULL ;
	_video_encoder = NULL ;
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
		cvReleaseImage(&img) ;
		return ;
	}

	static const int _encoded_width = 128 ;
	static const int _encoded_height = 128 ;

	QImage image = QImage((uchar*)img->imageData,img->width,img->height,QImage::Format_RGB888).scaled(QSize(_encoded_width,_encoded_height),Qt::IgnoreAspectRatio,Qt::SmoothTransformation) ;

	if(_video_encoder != NULL) _video_encoder->addImage(image) ;
	if(_echo_output_device != NULL) _echo_output_device->showFrame(image) ;
}

QVideoInputDevice::~QVideoInputDevice()
{
	stop() ;
}


QVideoOutputDevice::QVideoOutputDevice(QWidget *parent)
	: QLabel(parent)
{
	setPixmap(QPixmap(":/images/video-icon-big.png").scaled(170,128,Qt::KeepAspectRatio,Qt::SmoothTransformation)) ;
}

void QVideoOutputDevice::showFrame(const QImage& img)
{
	//std::cerr << "Displaying frame!!" << std::endl;

	//QPainter painter(this) ;
	//painter.drawImage(QPointF(0,0),img) ;

	setPixmap(QPixmap::fromImage(img).scaled(minimumSize(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation)) ;
}

