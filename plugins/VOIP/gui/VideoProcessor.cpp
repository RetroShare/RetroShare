#include <iostream>

#include <QByteArray>
#include <QBuffer>
#include <QImage>

#include "VideoProcessor.h"
#include "QVideoDevice.h"

//bool VideoDecoder::getNextImage(QImage& image)
//{
//	if(_image_queue.empty())
//		return false ;
//
//	image = _image_queue.front() ;
//	_image_queue.pop_front() ;
//
//	return true ;
//}

bool VideoEncoder::addImage(const QImage& img)
{
	std::cerr << "VideoEncoder: adding image." << std::endl;

	encodeData(img) ;

	if(_echo_output_device != NULL)
		_echo_output_device->showFrame(img) ;

	return true ;
}

void VideoDecoder::receiveEncodedData(const unsigned char *data,uint32_t size)
{
	_output_device->showFrame(decodeData(data,size)) ;
}

QImage JPEGVideoDecoder::decodeData(const unsigned char *encoded_image_data,uint32_t size)
{
	QByteArray qb((char*)encoded_image_data,size) ;
	QImage image ;
	if(image.loadFromData(qb))
		return image ;
	else
		return QImage() ;
}

void JPEGVideoEncoder::encodeData(const QImage& image)
{
	QByteArray qb ;
	
	QBuffer buffer(&qb) ;
	buffer.open(QIODevice::WriteOnly) ;
	image.save(&buffer,"JPEG") ;

	//destination_decoder->receiveEncodedData((unsigned char *)qb.data(),qb.size()) ;

	std::cerr <<"sending encoded data. size = " << qb.size() << std::endl;
}

