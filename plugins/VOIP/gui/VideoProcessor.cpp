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

	return true ;
}

bool VideoEncoder::nextPacket(RsVoipDataChunk& chunk)
{
	if(_out_queue.empty())
		return false ;

	chunk = _out_queue.front() ;
	_out_queue.pop_front() ;

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
	if(image.loadFromData(qb,"JPEG"))
	{
		std::cerr << "image decoded successfully" << std::endl;
		return image ;
	}
	else
	{
		std::cerr << "image.loadFromData(): returned an error.: " << std::endl;
		return QImage() ;
	}
}

void JPEGVideoEncoder::encodeData(const QImage& image)
{
	QByteArray qb ;
	
	QBuffer buffer(&qb) ;
	buffer.open(QIODevice::WriteOnly) ;
	image.save(&buffer,"JPEG") ;

	//destination_decoder->receiveEncodedData((unsigned char *)qb.data(),qb.size()) ;

	RsVoipDataChunk voip_chunk ;
	voip_chunk.data = malloc(qb.size());
	memcpy(voip_chunk.data,qb.data(),qb.size()) ;
	voip_chunk.size = qb.size() ;
	voip_chunk.type = RsVoipDataChunk::RS_VOIP_DATA_TYPE_VIDEO ;

	_out_queue.push_back(voip_chunk) ;

	std::cerr << "sending encoded data. size = " << std::dec << qb.size() << std::endl;
}

