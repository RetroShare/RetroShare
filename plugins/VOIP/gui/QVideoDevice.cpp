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

bool QVideoInputDevice::stopped()
{
    return _timer == NULL ;
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
		// the camera will be deinitialized automatically in VideoCapture destructor
		_capture_device->release();
		delete _capture_device ;
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
	_capture_device = new cv::VideoCapture(cam_id);

	if(!_capture_device->isOpened())
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
    if(!_timer)
        return ;

    cv::Mat frame;
    if(!_capture_device->read(frame))
    {
        std::cerr << "(EE) Cannot capture image from camera. Something's wrong." << std::endl;
        return ;
    }

    // get the image data

    if(frame.channels() != 3)
    {
        std::cerr << "(EE) expected 3 channels. Got " << frame.channels() << std::endl;
        return ;
    }

    // convert to RGB and copy to new buffer, because cvQueryFrame tells us to not modify the buffer
    cv::Mat img_rgb;
    cv::cvtColor(frame, img_rgb, CV_BGR2RGB);
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

QVideoInputDevice::~QVideoInputDevice()
{
    stop() ;
    _video_processor = NULL ;
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

