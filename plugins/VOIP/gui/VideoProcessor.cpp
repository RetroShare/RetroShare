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

VideoEncoder::VideoEncoder()
    :_frame_size(128,128) 
{
}

bool VideoEncoder::addImage(const QImage& img,uint32_t size_hint,uint32_t& encoded_size)
{
    std::cerr << "reducing to " << _frame_size.width() << " x " << _frame_size.height() << std::endl;
	encodeData(img.scaled(_frame_size,Qt::IgnoreAspectRatio,Qt::SmoothTransformation),size_hint,encoded_size) ;

	return true ;
}

bool VideoEncoder::nextPacket(RsVOIPDataChunk& chunk)
{
	if(_out_queue.empty())
		return false ;

	chunk = _out_queue.front() ;
	_out_queue.pop_front() ;

	return true ;
}

void VideoDecoder::receiveEncodedData(const unsigned char *data,uint32_t size)
{
    if(_output_device)
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

void VideoEncoder::setInternalFrameSize(QSize s)
{
    _frame_size = s ;
}

JPEGVideoEncoder::JPEGVideoEncoder()
	: _ref_frame_max_distance(10),_ref_frame_count(10) 
{
}

void JPEGVideoEncoder::encodeData(const QImage& image,uint32_t /* size_hint */,uint32_t& encoded_size)
{
    // check if we make a diff image, or if we use the full frame.

    QImage encoded_frame ;

    if(_ref_frame_count++ < _ref_frame_max_distance && image.size() == _reference_frame.size())
    {
	    // compute difference with reference frame.
	    encoded_frame = image ;

	    for(uint32_t i=0;i<image.byteCount();++i)
		    encoded_frame.bits()[i] = image.bits()[i] - _reference_frame.bits()[i] + 128 ;
    }
    else
    {
	    _ref_frame_count = 0 ;
	    _reference_frame = image ;
	    encoded_frame = image ;
    }

    QByteArray qb ;

    QBuffer buffer(&qb) ;
    buffer.open(QIODevice::WriteOnly) ;
    encoded_frame.save(&buffer,"JPEG") ;

    RsVOIPDataChunk voip_chunk ;
    voip_chunk.data = malloc(qb.size());
    memcpy(voip_chunk.data,qb.data(),qb.size()) ;
    voip_chunk.size = qb.size() ;
    voip_chunk.type = RsVOIPDataChunk::RS_VOIP_DATA_TYPE_VIDEO ;

    _out_queue.push_back(voip_chunk) ;

    encoded_size = voip_chunk.size ;
}

