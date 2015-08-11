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
    :_frame_size(256,256) 
{
}

bool VideoEncoder::addImage(const QImage& img,uint32_t size_hint,uint32_t& encoded_size)
{
//    std::cerr << "reducing to " << _frame_size.width() << " x " << _frame_size.height() << std::endl;
	encodeData(img.scaled(_frame_size,Qt::IgnoreAspectRatio,Qt::SmoothTransformation),size_hint,encoded_size) ;
	//encodeData(img,size_hint,encoded_size) ;

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
    static const int HEADER_SIZE = 4 ;

    // read frame type. Use first 4 bytes to give info about content.

    if(size < HEADER_SIZE)
    {
	    std::cerr << "JPEGVideoDecoder::decodeData(): Too small a data packet. size=" << size << std::endl;
	    return QImage() ;
    }

    uint32_t flags = encoded_image_data[0] + (encoded_image_data[1] << 8) ;

    //  un-compress image data

    QByteArray qb((char*)&encoded_image_data[HEADER_SIZE],(int)size - HEADER_SIZE) ;
    QImage image ;
    if(!image.loadFromData(qb,"JPEG"))
    {
	    std::cerr << "image.loadFromData(): returned an error.: " << std::endl;
	    return QImage() ;
    }

    // now see if the frame is a differential frame, or just a reference frame.

    if(flags & JPEG_VIDEO_FLAGS_DIFFERENTIAL_FRAME)
    {
            if(_reference_frame.size() != image.size())
            {
                std::cerr << "Bad reference frame!" << std::endl;
                return image ;
            }
            
	    QImage res = _reference_frame ;

	    for(uint32_t i=0;i<image.byteCount();++i)
		    res.bits()[i] += (image.bits()[i] - 128) & 0xff ;	// it should be -128, but we're doing modulo 256 arithmetic

	    return res ;
    }
    else
    {
	    _reference_frame = image ;

	    return image ;
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
	: _ref_frame_max_distance(50),_ref_frame_count(50) 
{
}

void JPEGVideoEncoder::encodeData(const QImage& image,uint32_t /* size_hint */,uint32_t& encoded_size)
{
    // check if we make a diff image, or if we use the full frame.

    QImage encoded_frame ;
    bool differential_frame ;

    if(_ref_frame_count++ < _ref_frame_max_distance && image.size() == _reference_frame.size())
    {
	    // compute difference with reference frame.
	    encoded_frame = image ;

	    for(uint32_t i=0;i<image.byteCount();++i)
		    encoded_frame.bits()[i] = (image.bits()[i] - _reference_frame.bits()[i]) + 128;
        
        differential_frame = true ;
    }
    else
    {
	    _ref_frame_count = 0 ;
	    _reference_frame = image ;
	    encoded_frame = image ;
        
        differential_frame = false ;
    }

    QByteArray qb ;

    QBuffer buffer(&qb) ;
    buffer.open(QIODevice::WriteOnly) ;
    encoded_frame.save(&buffer,"JPEG") ;

    RsVOIPDataChunk voip_chunk ;
    voip_chunk.data = malloc(HEADER_SIZE + qb.size());
    
    // build header
    uint32_t flags = differential_frame ? JPEG_VIDEO_FLAGS_DIFFERENTIAL_FRAME : 0x0 ;
    
    ((unsigned char *)voip_chunk.data)[0] = flags & 0xff ;
    ((unsigned char *)voip_chunk.data)[1] = (flags >> 8) & 0xff ;
    ((unsigned char *)voip_chunk.data)[2] = 0 ;
    ((unsigned char *)voip_chunk.data)[3] = 0 ;
    
    memcpy(voip_chunk.data+HEADER_SIZE,qb.data(),qb.size()) ;
    voip_chunk.size = HEADER_SIZE + qb.size() ;
    voip_chunk.type = RsVOIPDataChunk::RS_VOIP_DATA_TYPE_VIDEO ;

    _out_queue.push_back(voip_chunk) ;

    encoded_size = voip_chunk.size ;
}

