/*******************************************************************************
 * plugins/VOIP/gui/VideoProcessor.h                                           *
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

#pragma once

#include <stdint.h>
#include <QImage>
#include "interface/rsVOIP.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

class QVideoOutputDevice ;

class VideoCodec
{
public:
    virtual bool encodeData(const QImage& Image, uint32_t size_hint, RsVOIPDataChunk& chunk) = 0;
    virtual bool decodeData(const RsVOIPDataChunk& chunk,QImage& image) = 0;
    
protected:
    static const uint32_t HEADER_SIZE = 0x04 ;
};

// Now derive various image encoding/decoding algorithms.
//

class JPEGVideo: public VideoCodec
{
public:
    JPEGVideo() ;
    
protected:
    virtual bool encodeData(const QImage& Image, uint32_t target_encoding_bitrate, RsVOIPDataChunk& chunk) ;
    virtual bool decodeData(const RsVOIPDataChunk& chunk,QImage& image) ;

    static const uint32_t JPEG_VIDEO_FLAGS_DIFFERENTIAL_FRAME = 0x0001 ;
private:
    QImage _decoded_reference_frame ;
    QImage _encoded_reference_frame ;

    uint32_t _encoded_ref_frame_max_distance ;	// max distance between two reference frames.
    uint32_t _encoded_ref_frame_count ;
};

struct AVCodec ;
struct AVCodecContext ;
struct AVFrame ;
struct AVPacket ;

class FFmpegVideo: public VideoCodec
{
public:
    FFmpegVideo() ;
    ~FFmpegVideo() ;

protected:
    virtual bool encodeData(const QImage& Image, uint32_t target_encoding_bitrate, RsVOIPDataChunk& chunk) ;
    virtual bool decodeData(const RsVOIPDataChunk& chunk,QImage& image) ;
    
private:
    AVCodec *encoding_codec;
    AVCodec *decoding_codec;
    AVCodecContext *encoding_context;
    AVCodecContext *decoding_context;
    AVFrame *encoding_frame_buffer ;
    AVFrame *decoding_frame_buffer ;
    AVPacket decoding_buffer;
    uint64_t encoding_frame_count ;
    
#ifdef DEBUG_MPEG_VIDEO
    FILE *encoding_debug_file ;
#endif
};

// This class decodes video from a stream. It keeps a queue of
// decoded frame that needs to be retrieved using the getNextImage() method.
//
class VideoProcessor
{
	public:
		VideoProcessor() ;
		virtual ~VideoProcessor() ;

        	enum CodecId {
                		VIDEO_PROCESSOR_CODEC_ID_UNKNOWN    = 0x0000,
                		VIDEO_PROCESSOR_CODEC_ID_JPEG_VIDEO = 0x0001,
                		VIDEO_PROCESSOR_CODEC_ID_DDWT_VIDEO = 0x0002,
                		VIDEO_PROCESSOR_CODEC_ID_MPEG_VIDEO = 0x0003
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
		uint32_t currentBandwidthIn() const { return _estimated_bandwidth_in ;  }

	private:
		QVideoOutputDevice *_decoded_output_device ;
		std::list<QImage> _decoded_image_queue ;
		//time_t _lastTimeToShowFrame ;

// =====================================================================================
// =------------------------------------ ENCODING -------------------------------------=
// =====================================================================================
        
	public:
		// Takes the next image to be encoded. 
		//
		bool processImage(const QImage& Image) ;
		bool encodedPacketReady() const { return !_encoded_out_queue.empty() ; }
		bool nextEncodedPacket(RsVOIPDataChunk& ) ;

		// Used to tweak the compression ratio so that the video can stream ok.
		//
		void setMaximumBandwidth(uint32_t bytes_per_second) ;
        	void setInternalFrameSize(QSize) ;
            
        	// returns the current encoding frame rate in bytes per second.
        	//
        	uint32_t currentBandwidthOut() const { return _estimated_bandwidth_out ; }
            
	protected:
		std::list<RsVOIPDataChunk> _encoded_out_queue ;
        	QSize _encoded_frame_size ;
            
// =====================================================================================
// =------------------------------------- Codecs --------------------------------------=
// =====================================================================================
        
	    JPEGVideo    _jpeg_video_codec ;
            FFmpegVideo  _mpeg_video_codec ;
            
            uint16_t _encoding_current_codec ;
            
	    time_t _last_bw_estimate_in_TS;
	    time_t _last_bw_estimate_out_TS;
        
	    uint32_t _total_encoded_size_in ;
	    uint32_t _total_encoded_size_out ;
        
            float _estimated_bandwidth_in ;
            float _estimated_bandwidth_out ;
            
            float _target_bandwidth_out ;
            
            RsMutex vpMtx ;
};

