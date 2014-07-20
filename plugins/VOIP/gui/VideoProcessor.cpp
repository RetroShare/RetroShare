#include <iostream>

#include <QByteArray>
#include <QBuffer>
#include <QImage>

#include "VideoProcessor.h"
#include "QVideoDevice.h"

VideoDecoder::VideoDecoder()
{
	_output_device = NULL ;
}

bool VideoEncoder::addImage(const QImage& img)
{
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
		return image ;
	else
	{
		std::cerr << "image.loadFromData(): returned an error.: " << std::endl;
		return QImage() ;
	}
}

void VideoEncoder::setMaximumFrameRate(uint32_t bytes_per_sec)
{
	std::cerr << "Video Encoder: maximum frame rate is set to " << bytes_per_sec << " Bps" << std::endl;
}


void JPEGVideoEncoder::encodeData(const QImage& image)
{
	QByteArray qb ;
	
	QBuffer buffer(&qb) ;
	buffer.open(QIODevice::WriteOnly) ;
	image.save(&buffer,"JPEG") ;

	RsVoipDataChunk voip_chunk ;
	voip_chunk.data = malloc(qb.size());
	memcpy(voip_chunk.data,qb.data(),qb.size()) ;
	voip_chunk.size = qb.size() ;
	voip_chunk.type = RsVoipDataChunk::RS_VOIP_DATA_TYPE_VIDEO ;

	_out_queue.push_back(voip_chunk) ;
}

