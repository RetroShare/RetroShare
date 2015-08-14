#pragma once

#include <stdint.h>
#include <QImage>
#include "interface/rsVOIP.h"

class QVideoOutputDevice ;

class VideoCodec
{
public:
    virtual bool encodeData(const QImage& Image, uint32_t size_hint, RsVOIPDataChunk& chunk) = 0;
    virtual bool decodeData(const RsVOIPDataChunk& chunk,QImage& image) = 0;
};

// Now derive various image encoding/decoding algorithms.
//

class JPEGVideo: public VideoCodec
{
public:
    JPEGVideo() ;
    
protected:
    virtual bool encodeData(const QImage& Image, uint32_t size_hint, RsVOIPDataChunk& chunk) ;
    virtual bool decodeData(const RsVOIPDataChunk& chunk,QImage& image) ;

    static const uint32_t HEADER_SIZE = 0x04 ;
    static const uint32_t JPEG_VIDEO_FLAGS_DIFFERENTIAL_FRAME = 0x0001 ;
private:
    QImage _decoded_reference_frame ;
    QImage _encoded_reference_frame ;

    uint32_t _encoded_ref_frame_max_distance ;	// max distance between two reference frames.
    uint32_t _encoded_ref_frame_count ;
};

class DifferentialWaveletVideo: public VideoCodec
{
public:
	DifferentialWaveletVideo() {}

protected:
    virtual bool encodeData(const QImage& Image, uint32_t size_hint, RsVOIPDataChunk& chunk) { return true ; }
    virtual bool decodeData(const RsVOIPDataChunk& chunk,QImage& image) { return true ; }

private:
	QImage _last_reference_frame ;
};

// This class decodes video from a stream. It keeps a queue of
// decoded frame that needs to be retrieved using the getNextImage() method.
//
class VideoProcessor
{
	public:
		VideoProcessor() ;
		virtual ~VideoProcessor() {}

        	enum CodecId {
                		VIDEO_PROCESSOR_CODEC_ID_UNKNOWN    = 0x0000,
                		VIDEO_PROCESSOR_CODEC_ID_JPEG_VIDEO = 0x0001,
                		VIDEO_PROCESSOR_CODEC_ID_DDWT_VIDEO = 0x0002
            };
            
// =====================================================================================
// =------------------------------------ DECODING -------------------------------------=
// =====================================================================================
        
		// Gets the next image to be displayed. Once returned, the image should
		// be cleared from the incoming queue.
		//
		void setDisplayTarget(QVideoOutputDevice *odev) { _decoded_output_device = odev ; }
		virtual void receiveEncodedData(const RsVOIPDataChunk& chunk) ;

		// returns the current (measured) frame rate in bytes per second.
		//
		uint32_t currentDecodingFrameRate() const; 

	private:
		QVideoOutputDevice *_decoded_output_device ;
		std::list<QImage> _decoded_image_queue ;

// =====================================================================================
// =------------------------------------ ENCODING -------------------------------------=
// =====================================================================================
        
	public:
		// Takes the next image to be encoded. 
		//
		bool processImage(const QImage& Image, uint32_t size_hint, uint32_t &encoded_size) ;
		bool encodedPacketReady() const { return !_encoded_out_queue.empty() ; }
		bool nextEncodedPacket(RsVOIPDataChunk& ) ;

		// Used to tweak the compression ratio so that the video can stream ok.
		//
		void setMaximumFrameRate(uint32_t bytes_per_second) ;
        	void setInternalFrameSize(QSize) ;
            
	protected:
		std::list<RsVOIPDataChunk> _encoded_out_queue ;
        	QSize _encoded_frame_size ;
            
// =====================================================================================
// =------------------------------------- Codecs --------------------------------------=
// =====================================================================================
        
	    JPEGVideo                _jpeg_video_codec ;
            DifferentialWaveletVideo _ddwt_video_codec ;
            
            uint16_t _encoding_current_codec ;
};

