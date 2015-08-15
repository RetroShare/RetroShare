#include <iostream>
#include <assert.h>

#include <QByteArray>
#include <QBuffer>
#include <QImage>

#include "VideoProcessor.h"
#include "QVideoDevice.h"

VideoProcessor::VideoProcessor()
    :_encoded_frame_size(128,128) 
{
	_decoded_output_device = NULL ;
    _encoding_current_codec = VIDEO_PROCESSOR_CODEC_ID_JPEG_VIDEO;
}

bool VideoProcessor::processImage(const QImage& img,uint32_t size_hint,uint32_t& encoded_size)
{
    VideoCodec *codec ;

    switch(_encoding_current_codec)
    {
    case VIDEO_PROCESSOR_CODEC_ID_JPEG_VIDEO: codec = &_jpeg_video_codec ;
	    break ;
    case VIDEO_PROCESSOR_CODEC_ID_DDWT_VIDEO: codec = &_ddwt_video_codec ;
	    break ;
    default:
	    codec = NULL ;
    }

    //    std::cerr << "reducing to " << _frame_size.width() << " x " << _frame_size.height() << std::endl;

    void *data = NULL;
    encoded_size = 0 ;

    if(codec)
    {
	    RsVOIPDataChunk chunk ;

	    codec->encodeData(img.scaled(_encoded_frame_size,Qt::IgnoreAspectRatio,Qt::SmoothTransformation),size_hint,chunk) ;

        	encoded_size = chunk.size ;
            
	    if(chunk.size == 0)		// the codec might be buffering the frame for compression reasons
		    return true ;

	    _encoded_out_queue.push_back(chunk) ;

	    return true ;
    }
    else
    {
        std::cerr << "No codec for codec ID = " << _encoding_current_codec << ". Please call VideoProcessor::setCurrentCodec()" << std::endl;
	    return false ;
    }
}

bool VideoProcessor::nextEncodedPacket(RsVOIPDataChunk& chunk)
{
	if(_encoded_out_queue.empty())
		return false ;

	chunk = _encoded_out_queue.front() ;
	_encoded_out_queue.pop_front() ;

	return true ;
}

void VideoProcessor::setInternalFrameSize(QSize s)
{
    _encoded_frame_size = s ;
}

void VideoProcessor::receiveEncodedData(const RsVOIPDataChunk& chunk)
{
    static const int HEADER_SIZE = 4 ;

    // read frame type. Use first 4 bytes to give info about content.
    // 
    //    Byte       Meaning       Values
    //      00         Codec         CODEC_ID_JPEG_VIDEO      Basic Jpeg codec
    //                               CODEC_ID_DDWT_VIDEO      Differential wavelet compression
    //
    //      01        Unused                      		Might be useful later
    //
    //    0203         Flags                      		Codec specific flags.
    //

    if(chunk.size < HEADER_SIZE)
    {
	    std::cerr << "JPEGVideoDecoder::decodeData(): Too small a data packet. size=" << chunk.size << std::endl;
	    return ;
    }

    uint32_t codid = ((unsigned char *)chunk.data)[0] + (((unsigned char *)chunk.data)[1] << 8) ;
    uint16_t flags = ((unsigned char *)chunk.data)[2] + (((unsigned char *)chunk.data)[3] << 8) ;

    VideoCodec *codec ;

    switch(codid)
    {
    case VIDEO_PROCESSOR_CODEC_ID_JPEG_VIDEO: codec = &_jpeg_video_codec ;
	    break ;
    case VIDEO_PROCESSOR_CODEC_ID_DDWT_VIDEO: codec = &_ddwt_video_codec ;
	    break ;
    default:
	    codec = NULL ;
    }
    QImage img ;

    if(codec != NULL)
	    codec->decodeData(chunk,img) ;

    if(_decoded_output_device)
	    _decoded_output_device->showFrame(img) ;
}

void VideoProcessor::setMaximumFrameRate(uint32_t bytes_per_sec)
{
    std::cerr << "Video Encoder: maximum frame rate is set to " << bytes_per_sec << " Bps" << std::endl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

JPEGVideo::JPEGVideo()
	: _encoded_ref_frame_max_distance(10),_encoded_ref_frame_count(10) 
{
}

bool JPEGVideo::decodeData(const RsVOIPDataChunk& chunk,QImage& image)
{
    // now see if the frame is a differential frame, or just a reference frame.

    uint16_t codec = ((unsigned char *)chunk.data)[0] + (((unsigned char *)chunk.data)[1] << 8) ;
    uint16_t flags = ((unsigned char *)chunk.data)[2] + (((unsigned char *)chunk.data)[3] << 8) ;
    
    assert(codec == VideoProcessor::VIDEO_PROCESSOR_CODEC_ID_JPEG_VIDEO) ;
    
    //  un-compress image data

    QByteArray qb((char*)&((uint8_t*)chunk.data)[HEADER_SIZE],(int)chunk.size - HEADER_SIZE) ;

    if(!image.loadFromData(qb,"JPEG"))
    {
	    std::cerr << "image.loadFromData(): returned an error.: " << std::endl;
	    return false ;
    }
   
    if(flags & JPEG_VIDEO_FLAGS_DIFFERENTIAL_FRAME)
    {
	    if(_decoded_reference_frame.size() != image.size())
	    {
		    std::cerr << "Bad reference frame!" << std::endl;
		    return false ;
	    }

	    QImage res = _decoded_reference_frame ;

	    for(uint32_t i=0;i<image.byteCount();++i)
	    {
		    int new_val = (int)res.bits()[i] + ((int)image.bits()[i] - 128) ;

		    res.bits()[i] = std::max(0,std::min(255,new_val)) ;
	    }

	    image = res ;
    }
    else
        _decoded_reference_frame = image ;
    
    return true ;
}

bool JPEGVideo::encodeData(const QImage& image,uint32_t /* size_hint */,RsVOIPDataChunk& voip_chunk)
{
    // check if we make a diff image, or if we use the full frame.

    QImage encoded_frame ;
    bool differential_frame ;

    if(_encoded_ref_frame_count++ < _encoded_ref_frame_max_distance && image.size() == _encoded_reference_frame.size())
    {
	    // compute difference with reference frame.
	    encoded_frame = image ;

	    for(uint32_t i=0;i<image.byteCount();++i)
	    {
		    // We cannot use basic modulo 256 arithmetic, because the decompressed JPeg frames do not follow the same rules (values are clamped)
		    // and cause color blotches when perturbated by a differential frame.

		    int diff = ( (int)image.bits()[i] - (int)_encoded_reference_frame.bits()[i]) + 128;
		    encoded_frame.bits()[i] = (unsigned char)std::max(0,std::min(255,diff)) ;
	    }

	    differential_frame = true ;
    }
    else
    {
	    _encoded_ref_frame_count = 0 ;
	    _encoded_reference_frame = image ;
	    encoded_frame = image ;

	    differential_frame = false ;
    }

    QByteArray qb ;

    QBuffer buffer(&qb) ;
    buffer.open(QIODevice::WriteOnly) ;
    encoded_frame.save(&buffer,"JPEG") ;

    voip_chunk.data = malloc(HEADER_SIZE + qb.size());

    // build header
    uint32_t flags = differential_frame ? JPEG_VIDEO_FLAGS_DIFFERENTIAL_FRAME : 0x0 ;

    ((unsigned char *)voip_chunk.data)[0] =  VideoProcessor::VIDEO_PROCESSOR_CODEC_ID_JPEG_VIDEO       & 0xff ;
    ((unsigned char *)voip_chunk.data)[1] = (VideoProcessor::VIDEO_PROCESSOR_CODEC_ID_JPEG_VIDEO >> 8) & 0xff ;
    ((unsigned char *)voip_chunk.data)[2] = flags & 0xff ;
    ((unsigned char *)voip_chunk.data)[3] = (flags >> 8) & 0xff ;

    memcpy(voip_chunk.data+HEADER_SIZE,qb.data(),qb.size()) ;

    voip_chunk.size = HEADER_SIZE + qb.size() ;
    voip_chunk.type = RsVOIPDataChunk::RS_VOIP_DATA_TYPE_VIDEO ;

    return true ;
}

