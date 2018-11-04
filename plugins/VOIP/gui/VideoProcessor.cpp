/*******************************************************************************
 * plugins/VOIP/gui/VideoProcessor.cpp                                         *
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

#include <iostream>
#include <assert.h>
#ifdef __MACH__
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

#include <QByteArray>
#include <QBuffer>
#include <QImage>

#include "util/rsmemory.h"

#include "VideoProcessor.h"
#include "QVideoDevice.h"

#include <math.h>
#include <time.h>

extern "C" {
#include <libavcodec/avcodec.h>

#include <libavutil/opt.h>
//#include <libavutil/channel_layout.h>
#include <libavutil/mem.h>
#include <libavutil/imgutils.h>
//#include <libavutil/mathematics.h>
//#include <libavutil/samplefmt.h>
}
//#define DEBUG_MPEG_VIDEO 1

#ifndef AV_INPUT_BUFFER_PADDING_SIZE
#ifndef FF_INPUT_BUFFER_PADDING_SIZE
#define AV_INPUT_BUFFER_PADDING_SIZE 32
#else
#define AV_INPUT_BUFFER_PADDING_SIZE FF_INPUT_BUFFER_PADDING_SIZE
#endif
#endif

#ifndef MINGW
#if (LIBAVUTIL_VERSION_MAJOR == 54) && (LIBAVUTIL_VERSION_MINOR == 3) && (LIBAVUTIL_VERSION_MICRO == 0)
//Ubuntu Vivid use other version of rational.h than GIT with LIBAVUTIL_VERSION_MICRO  == 0
#define VIVID_RATIONAL_H_VERSION 1
#endif

#if (VIVID_RATIONAL_H_VERSION) || (LIBAVUTIL_VERSION_MAJOR < 52) || ((LIBAVUTIL_VERSION_MAJOR == 52) && (LIBAVUTIL_VERSION_MINOR < 63))
//since https://github.com/FFmpeg/FFmpeg/commit/3532dd52c51f3d4b95f31d1b195e64a04a8aea5d
static inline AVRational av_make_q(int num, int den)
{
    AVRational r = { num, den };
    return r;
}
#endif

#if (LIBAVUTIL_VERSION_MAJOR < 55) || ((LIBAVUTIL_VERSION_MAJOR == 55) && (LIBAVUTIL_VERSION_MINOR < 52))
//since https://github.com/FFmpeg/FFmpeg/commit/fd056029f45a9f6d213d9fce8165632042511d4f
void avcodec_free_context(AVCodecContext **pavctx)
{
    AVCodecContext *avctx = *pavctx;

    if (!avctx)
        return;

    avcodec_close(avctx);

    av_freep(&avctx->extradata);
    av_freep(&avctx->subtitle_header);

    av_freep(pavctx);
}
#endif


#if (LIBAVUTIL_VERSION_MAJOR < 57) || ((LIBAVUTIL_VERSION_MAJOR == 57) && (LIBAVUTIL_VERSION_MINOR < 52))
//Since https://github.com/FFmpeg/FFmpeg/commit/7ecc2d403ce5c7b6ea3b1f368dccefd105209c7e
static void get_frame_defaults(AVFrame *frame)
{
    if (frame->extended_data != frame->data) {
        av_freep(&frame->extended_data);
        return;
    }

    frame->extended_data = NULL;
    get_frame_defaults(frame);

    return;
}

AVFrame *av_frame_alloc(void)
{
    AVFrame *frame = (AVFrame *)av_mallocz(sizeof(*frame));

    if (!frame)
        return NULL;

    get_frame_defaults(frame);

    return frame;
}


void av_frame_free(AVFrame **frame)
{
    if (!frame || !*frame)
        return;

    av_freep(frame);
}
#endif
#endif // MINGW

VideoProcessor::VideoProcessor()
    :_encoded_frame_size(640,480) , vpMtx("VideoProcessor")
{
    //_lastTimeToShowFrame = time(NULL);
    _decoded_output_device = NULL ;

  //_encoding_current_codec = VIDEO_PROCESSOR_CODEC_ID_JPEG_VIDEO;
    _encoding_current_codec = VIDEO_PROCESSOR_CODEC_ID_MPEG_VIDEO;

    _estimated_bandwidth_in = 0 ;
    _estimated_bandwidth_out = 0 ;
    _target_bandwidth_out = 30*1024 ;	// 30 KB/s

    _total_encoded_size_in = 0 ;
    _total_encoded_size_out = 0 ;

    _last_bw_estimate_in_TS = time(NULL) ;
    _last_bw_estimate_out_TS = time(NULL) ;
}

VideoProcessor::~VideoProcessor()
{
    // clear encoding queue

    RS_STACK_MUTEX(vpMtx) ;

    while(!_encoded_out_queue.empty())
    {
        _encoded_out_queue.back().clear() ;
        _encoded_out_queue.pop_back() ;
    }
}

bool VideoProcessor::processImage(const QImage& img)
{
    VideoCodec *codec ;

    switch(_encoding_current_codec)
    {
    case VIDEO_PROCESSOR_CODEC_ID_JPEG_VIDEO: codec = &_jpeg_video_codec ;
	    break ;
    case VIDEO_PROCESSOR_CODEC_ID_MPEG_VIDEO: codec = &_mpeg_video_codec ;
	    break ;
    default:
	    codec = NULL ;
    }

    //    std::cerr << "reducing to " << _frame_size.width() << " x " << _frame_size.height() << std::endl;

    if(codec)
    {
	    RsVOIPDataChunk chunk ;

	    if(codec->encodeData(img.scaled(_encoded_frame_size,Qt::IgnoreAspectRatio,Qt::SmoothTransformation),_target_bandwidth_out,chunk) && chunk.size > 0)
	    {
		    RS_STACK_MUTEX(vpMtx) ;
		    _encoded_out_queue.push_back(chunk) ;
		    _total_encoded_size_out += chunk.size ;
	    }

	    time_t now = time(NULL) ;

	    if(now > _last_bw_estimate_out_TS)
	    {
		    RS_STACK_MUTEX(vpMtx) ;

		    _estimated_bandwidth_out = uint32_t(0.75*_estimated_bandwidth_out + 0.25 * (_total_encoded_size_out / (float)(now - _last_bw_estimate_out_TS))) ;

		    _total_encoded_size_out = 0 ;
		    _last_bw_estimate_out_TS = now ;

#ifdef DEBUG_VIDEO_OUTPUT_DEVICE
		    std::cerr << "new bw estimate: " << _estimated_bw << std::endl;
#endif
	    }

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
	RS_STACK_MUTEX(vpMtx) ;
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
    //uint16_t flags = ((unsigned char *)chunk.data)[2] + (((unsigned char *)chunk.data)[3] << 8) ;

    VideoCodec *codec ;

    switch(codid)
    {
    case VIDEO_PROCESSOR_CODEC_ID_JPEG_VIDEO: codec = &_jpeg_video_codec ;
	    break ;
    case VIDEO_PROCESSOR_CODEC_ID_MPEG_VIDEO: codec = &_mpeg_video_codec ;
	    break ;
    default:
	    codec = NULL ;
    }

    if(codec == NULL)
    {
        std::cerr << "Unknown decoding codec: " << codid << std::endl;
        return ;
    }

    {
	    RS_STACK_MUTEX(vpMtx) ;
	    _total_encoded_size_in += chunk.size ;

	    time_t now = time(NULL) ;

	    if(now > _last_bw_estimate_in_TS)
	    {
		    _estimated_bandwidth_in = uint32_t(0.75*_estimated_bandwidth_in + 0.25 * (_total_encoded_size_in / (float)(now - _last_bw_estimate_in_TS))) ;

		    _total_encoded_size_in = 0 ;
		    _last_bw_estimate_in_TS = now ;

#ifdef DEBUG_VIDEO_OUTPUT_DEVICE
		    std::cerr << "new bw estimate (in): " << _estimated_bandwidth_in << std::endl;
#endif
	    }
    }

    QImage img ;
    if(!codec->decodeData(chunk,img))
    {
        std::cerr << "No image decoded. Probably in the middle of something..." << std::endl;
        return ;
    }

    if(_decoded_output_device)
//        if (time(NULL) > _lastTimeToShowFrame)
//        {
            _decoded_output_device->showFrame(img) ;
//            _lastTimeToShowFrame = time(NULL) ;//+ 1000/25;
#warning \plugins\VOIP\gui\VideoProcessor.cpp:210 Phenom: TODO: Get CPU usage to pass image.
//        }
}

void VideoProcessor::setMaximumBandwidth(uint32_t bytes_per_sec)
{
    std::cerr << "Video Encoder: maximum frame rate is set to " << bytes_per_sec << " Bps" << std::endl;
    _target_bandwidth_out = bytes_per_sec ;
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

	    for(int i=0;i<image.byteCount();++i)
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

    if (_encoded_ref_frame_count++ < _encoded_ref_frame_max_distance
        && image.size() == _encoded_reference_frame.size()
        && image.byteCount() == _encoded_reference_frame.byteCount())
	{
	    // compute difference with reference frame.
	    encoded_frame = image ;

	    for(int i=0;i<image.byteCount();++i)
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
	    _encoded_reference_frame = image.copy() ;
	    encoded_frame = image ;

	    differential_frame = false ;
    }

    QByteArray qb ;

    QBuffer buffer(&qb) ;
    buffer.open(QIODevice::WriteOnly) ;
    encoded_frame.save(&buffer,"JPEG") ;

    voip_chunk.data = rs_malloc(HEADER_SIZE + qb.size());
    
    if(!voip_chunk.data)
        return false ;

    // build header
    uint32_t flags = differential_frame ? JPEG_VIDEO_FLAGS_DIFFERENTIAL_FRAME : 0x0 ;

    ((unsigned char *)voip_chunk.data)[0] =  VideoProcessor::VIDEO_PROCESSOR_CODEC_ID_JPEG_VIDEO       & 0xff ;
    ((unsigned char *)voip_chunk.data)[1] = (VideoProcessor::VIDEO_PROCESSOR_CODEC_ID_JPEG_VIDEO >> 8) & 0xff ;
    ((unsigned char *)voip_chunk.data)[2] = flags & 0xff ;
    ((unsigned char *)voip_chunk.data)[3] = (flags >> 8) & 0xff ;

    memcpy(&((unsigned char*)voip_chunk.data)[HEADER_SIZE],qb.data(),qb.size()) ;

    voip_chunk.size = HEADER_SIZE + qb.size() ;
    voip_chunk.type = RsVOIPDataChunk::RS_VOIP_DATA_TYPE_VIDEO ;

    return true ;
}

FFmpegVideo::FFmpegVideo()
{
    avcodec_register_all();
    // Encoding

    encoding_codec = NULL ;
    encoding_frame_buffer = NULL ;
    encoding_context = NULL ;

    //AVCodecID codec_id = AV_CODEC_ID_H264 ;
    //AVCodecID codec_id = AV_CODEC_ID_MPEG2VIDEO;
#if LIBAVCODEC_VERSION_MAJOR < 54
    CodecID codec_id = CODEC_ID_MPEG4;
#else
    AVCodecID codec_id = AV_CODEC_ID_MPEG4;
#endif

    /* find the video encoder */
    encoding_codec = avcodec_find_encoder(codec_id);

    if (!encoding_codec) std::cerr << "AV codec not found for codec id " << std::endl;
    if (!encoding_codec) throw std::runtime_error("AV codec not found for codec id ") ;

    encoding_context = avcodec_alloc_context3(encoding_codec);

    if (!encoding_context) std::cerr << "AV: Could not allocate video codec encoding context" << std::endl;
    if (!encoding_context) throw std::runtime_error("AV: Could not allocate video codec encoding context");

    /* put sample parameters */
    encoding_context->bit_rate = 10*1024 ; // default bitrate is 30KB/s
    encoding_context->bit_rate_tolerance = encoding_context->bit_rate ;

#ifdef USE_VARIABLE_BITRATE
    encoding_context->rc_min_rate = 0;
    encoding_context->rc_max_rate = 10*1024;//encoding_context->bit_rate;
    encoding_context->rc_buffer_size = 10*1024*1024;
    encoding_context->rc_initial_buffer_occupancy = (int) ( 0.9 * encoding_context->rc_buffer_size);
    encoding_context->rc_max_available_vbv_use = 1.0;
    encoding_context->rc_min_vbv_overflow_use = 0.0;
#else
    encoding_context->rc_min_rate = 0;
    encoding_context->rc_max_rate = 0;
    encoding_context->rc_buffer_size = 0;
#endif
    if (encoding_codec->capabilities & CODEC_CAP_TRUNCATED)
        encoding_context->flags |= CODEC_FLAG_TRUNCATED;
    encoding_context->flags |= CODEC_FLAG_PSNR;//Peak signal-to-noise ratio
    encoding_context->flags |= CODEC_CAP_PARAM_CHANGE;
    encoding_context->i_quant_factor = 0.769f;
    encoding_context->b_quant_factor = 1.4f;
    encoding_context->time_base.num = 1;
    encoding_context->time_base.den = 15;//framesPerSecond;
    encoding_context->qmin =  1;
    encoding_context->qmax = 51;
    encoding_context->max_qdiff = 4;

    //encoding_context->me_method = ME_HEX;
    //encoding_context->max_b_frames = 4;
    //encoding_context->flags |= CODEC_FLAG_LOW_DELAY;	// MPEG2 only
    //encoding_context->partitions = X264_PART_I4X4 | X264_PART_I8X8 | X264_PART_P8X8 | X264_PART_P4X4 | X264_PART_B8X8;
    //encoding_context->crf = 0.0f;
    //encoding_context->cqp = 26;

    /* resolution must be a multiple of two */
    encoding_context->width = 640;//176;
    encoding_context->height = 480;//144;
    /* frames per second */
    encoding_context->time_base = av_make_q(1, 25);
    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    encoding_context->gop_size = 100;
    //encoding_context->max_b_frames = 1;
#if LIBAVCODEC_VERSION_MAJOR < 54
    encoding_context->pix_fmt = PIX_FMT_YUV420P; //context->pix_fmt = PIX_FMT_RGB24;
    if (codec_id == CODEC_ID_H264) {
#else
    encoding_context->pix_fmt = AV_PIX_FMT_YUV420P; //context->pix_fmt = AV_PIX_FMT_RGB24;
    if (codec_id == AV_CODEC_ID_H264) {
#endif
        av_opt_set(encoding_context->priv_data, "preset", "slow", 0);
    }

    /* open it */
    if (avcodec_open2(encoding_context, encoding_codec, NULL) < 0)
    {
        std::cerr << "AV: Could not open codec context. Something's wrong." << std::endl;
        throw std::runtime_error( "AV: Could not open codec context. Something's wrong.");
    }

#if (LIBAVCODEC_VERSION_MAJOR < 57) | (LIBAVCODEC_VERSION_MAJOR == 57 && LIBAVCODEC_VERSION_MINOR <3 )
    encoding_frame_buffer = avcodec_alloc_frame() ;//(AVFrame*)malloc(sizeof(AVFrame)) ;
#else
    encoding_frame_buffer = av_frame_alloc() ;
#endif

    if(!encoding_frame_buffer) std::cerr << "AV: could not allocate frame buffer." << std::endl;
    if(!encoding_frame_buffer)
        throw std::runtime_error("AV: could not allocate frame buffer.") ;

    encoding_frame_buffer->format = encoding_context->pix_fmt;
    encoding_frame_buffer->width  = encoding_context->width;
    encoding_frame_buffer->height = encoding_context->height;

    /* the image can be allocated by any means and av_image_alloc() is
     * just the most convenient way if av_malloc() is to be used */

    int ret = av_image_alloc(encoding_frame_buffer->data, encoding_frame_buffer->linesize,
                             encoding_context->width, encoding_context->height, encoding_context->pix_fmt, 32);

    if (ret < 0) std::cerr << "AV: Could not allocate raw picture buffer" << std::endl;
    if (ret < 0)
        throw std::runtime_error("AV: Could not allocate raw picture buffer");

    encoding_frame_count = 0 ;

    // Decoding
    decoding_codec = avcodec_find_decoder(codec_id);

    if (!decoding_codec) std::cerr << "AV codec not found for codec id " << std::endl;
    if (!decoding_codec)
        throw("AV codec not found for codec id ") ;

    decoding_context = avcodec_alloc_context3(decoding_codec);

    if(!decoding_context) std::cerr << "AV: Could not allocate video codec decoding context" << std::endl;
    if(!decoding_context)
        throw std::runtime_error("AV: Could not allocate video codec decoding context");

    decoding_context->width = encoding_context->width;
    decoding_context->height = encoding_context->height;
#if LIBAVCODEC_VERSION_MAJOR < 54
    decoding_context->pix_fmt = PIX_FMT_YUV420P;
#else
    decoding_context->pix_fmt = AV_PIX_FMT_YUV420P;
#endif

    if(decoding_codec->capabilities & CODEC_CAP_TRUNCATED)
        decoding_context->flags |= CODEC_FLAG_TRUNCATED; // we do not send complete frames
    //we can receive truncated frames
    decoding_context->flags2 |= CODEC_FLAG2_CHUNKS;

    AVDictionary* dictionary = NULL;
    if(avcodec_open2(decoding_context, decoding_codec, &dictionary) < 0)
    {
        std::cerr << "AV codec open action failed! " << std::endl;
        throw("AV codec open action failed! ") ;
    }

    //decoding_frame_buffer = avcodec_alloc_frame() ;//(AVFrame*)malloc(sizeof(AVFrame)) ;
    decoding_frame_buffer = av_frame_alloc() ;

    av_init_packet(&decoding_buffer);
    decoding_buffer.data = NULL ;
    decoding_buffer.size = 0 ;

    //ret = av_image_alloc(decoding_frame_buffer->data, decoding_frame_buffer->linesize, decoding_context->width, decoding_context->height, decoding_context->pix_fmt, 32);

    //if (ret < 0)
    //throw std::runtime_error("AV: Could not allocate raw picture buffer");

    // debug
#ifdef DEBUG_MPEG_VIDEO
    std::cerr << "Dumping captured data to file tmpvideo.mpg" << std::endl;
    encoding_debug_file = fopen("tmpvideo.mpg","w") ;
#endif
}

FFmpegVideo::~FFmpegVideo()
{
    avcodec_free_context(&encoding_context);
    avcodec_free_context(&decoding_context);
    av_frame_free(&encoding_frame_buffer);
    av_frame_free(&decoding_frame_buffer);
}

#define MAX_FFMPEG_ENCODING_BITRATE 81920

bool FFmpegVideo::encodeData(const QImage& image, uint32_t target_encoding_bitrate, RsVOIPDataChunk& voip_chunk)
{
#ifdef DEBUG_MPEG_VIDEO
	std::cerr << "Encoding frame of size " << image.width() << "x" << image.height() << ", resized to " << encoding_frame_buffer->width << "x" << encoding_frame_buffer->height << " : ";
#endif
	QImage input ;

    if(target_encoding_bitrate > MAX_FFMPEG_ENCODING_BITRATE)
    {
        std::cerr << "Max encodign bitrate eexceeded. Capping to " << MAX_FFMPEG_ENCODING_BITRATE << std::endl;
        target_encoding_bitrate = MAX_FFMPEG_ENCODING_BITRATE ;
    }
	//encoding_context->bit_rate = target_encoding_bitrate;
	encoding_context->rc_max_rate = target_encoding_bitrate;
	//encoding_context->bit_rate_tolerance = target_encoding_bitrate;

	if(image.width() != encoding_frame_buffer->width || image.height() != encoding_frame_buffer->height)
		input = image.scaled(QSize(encoding_frame_buffer->width,encoding_frame_buffer->height),Qt::IgnoreAspectRatio,Qt::SmoothTransformation) ;
	else
		input = image ;

	/* prepare a dummy image */
	/* Y */
	for (int y = 0; y < encoding_context->height/2; y++)
		for (int x = 0; x < encoding_context->width/2; x++)
		{
			QRgb pix00 = input.pixel(QPoint(2*x+0,2*y+0)) ;
			QRgb pix01 = input.pixel(QPoint(2*x+0,2*y+1)) ;
			QRgb pix10 = input.pixel(QPoint(2*x+1,2*y+0)) ;
			QRgb pix11 = input.pixel(QPoint(2*x+1,2*y+1)) ;

			int R00 = (pix00 >> 16) & 0xff ; int G00 = (pix00 >>  8) & 0xff ; int B00 = (pix00 >>  0) & 0xff ;
			int R01 = (pix01 >> 16) & 0xff ; int G01 = (pix01 >>  8) & 0xff ; int B01 = (pix01 >>  0) & 0xff ;
			int R10 = (pix10 >> 16) & 0xff ; int G10 = (pix10 >>  8) & 0xff ; int B10 = (pix10 >>  0) & 0xff ;
			int R11 = (pix11 >> 16) & 0xff ; int G11 = (pix11 >>  8) & 0xff ; int B11 = (pix11 >>  0) & 0xff ;

			int Y00 =  (0.257 * R00) + (0.504 * G00) + (0.098 * B00) + 16  ;
			int Y01 =  (0.257 * R01) + (0.504 * G01) + (0.098 * B01) + 16  ;
			int Y10 =  (0.257 * R10) + (0.504 * G10) + (0.098 * B10) + 16  ;
			int Y11 =  (0.257 * R11) + (0.504 * G11) + (0.098 * B11) + 16  ;

			float R = 0.25*(R00+R01+R10+R11) ;
			float G = 0.25*(G00+G01+G10+G11) ;
			float B = 0.25*(B00+B01+B10+B11) ;

			int U =  (0.439 * R) - (0.368 * G) - (0.071 * B) + 128 ;
			int V = -(0.148 * R) - (0.291 * G) + (0.439 * B) + 128 ;

			encoding_frame_buffer->data[0][(2*y+0) * encoding_frame_buffer->linesize[0] + 2*x+0] = std::min(255,std::max(0,Y00)); // Y
			encoding_frame_buffer->data[0][(2*y+0) * encoding_frame_buffer->linesize[0] + 2*x+1] = std::min(255,std::max(0,Y01)); // Y
			encoding_frame_buffer->data[0][(2*y+1) * encoding_frame_buffer->linesize[0] + 2*x+0] = std::min(255,std::max(0,Y10)); // Y
			encoding_frame_buffer->data[0][(2*y+1) * encoding_frame_buffer->linesize[0] + 2*x+1] = std::min(255,std::max(0,Y11)); // Y

			encoding_frame_buffer->data[1][y * encoding_frame_buffer->linesize[1] + x] = std::min(255,std::max(0,U));// Cr
			encoding_frame_buffer->data[2][y * encoding_frame_buffer->linesize[2] + x] = std::min(255,std::max(0,V));// Cb
		}


	encoding_frame_buffer->pts = encoding_frame_count++;

	/* encode the image */

	int got_output = 0;

	AVPacket pkt ;
	av_init_packet(&pkt);
#if LIBAVCODEC_VERSION_MAJOR < 54
	pkt.size = avpicture_get_size(encoding_context->pix_fmt, encoding_context->width, encoding_context->height);
	pkt.data = (uint8_t*)av_malloc(pkt.size);

	//    do
	//    {
	int ret = avcodec_encode_video(encoding_context, pkt.data, pkt.size, encoding_frame_buffer) ;
	if (ret > 0) {
		got_output = ret;
	}
#else
	pkt.data = NULL;    // packet data will be allocated by the encoder
	pkt.size = 0;

	//    do
	//    {
	int ret = avcodec_encode_video2(encoding_context, &pkt, encoding_frame_buffer, &got_output) ;
#endif

	if (ret < 0)
	{
		std::cerr << "Error encoding frame!" << std::endl;
		return false ;
	}
	//        frame = NULL ;	// next attempts: do not encode anything. Do this to just flush the buffer
	//
	//    } while(got_output) ;

	if(got_output)
	{
		voip_chunk.data = rs_malloc(pkt.size + HEADER_SIZE) ;
		
		if(!voip_chunk.data)
			return false ;
        
		uint32_t flags = 0;

		((unsigned char *)voip_chunk.data)[0] =  VideoProcessor::VIDEO_PROCESSOR_CODEC_ID_MPEG_VIDEO       & 0xff ;
		((unsigned char *)voip_chunk.data)[1] = (VideoProcessor::VIDEO_PROCESSOR_CODEC_ID_MPEG_VIDEO >> 8) & 0xff ;
		((unsigned char *)voip_chunk.data)[2] = flags & 0xff ;
		((unsigned char *)voip_chunk.data)[3] = (flags >> 8) & 0xff ;

		memcpy(&((unsigned char*)voip_chunk.data)[HEADER_SIZE],pkt.data,pkt.size) ;

		voip_chunk.size = pkt.size + HEADER_SIZE;
		voip_chunk.type = RsVOIPDataChunk::RS_VOIP_DATA_TYPE_VIDEO ;

#ifdef DEBUG_MPEG_VIDEO
		std::cerr << "Output : " << pkt.size << " bytes." << std::endl;
		fwrite(pkt.data,1,pkt.size,encoding_debug_file) ;
		fflush(encoding_debug_file) ;
#endif
		av_free_packet(&pkt);

		return true ;
	}
	else
	{
		voip_chunk.data = NULL;
		voip_chunk.size = 0;
		voip_chunk.type = RsVOIPDataChunk::RS_VOIP_DATA_TYPE_VIDEO ;

		std::cerr << "No output produced." << std::endl;
		return false ;
	}

}

bool FFmpegVideo::decodeData(const RsVOIPDataChunk& chunk,QImage& image)
{
#ifdef DEBUG_MPEG_VIDEO
	std::cerr << "Decoding data of size " << chunk.size << std::endl;
	std::cerr << "Allocating new buffer of size " << chunk.size - HEADER_SIZE << std::endl;
#endif

	uint32_t s = chunk.size - HEADER_SIZE ;
#if defined(__MINGW32__)
	unsigned char *tmp = (unsigned char*)_aligned_malloc(s + AV_INPUT_BUFFER_PADDING_SIZE, 16) ;
#else
#ifdef __MACH__
	//Mac OS X appears to be 16-byte mem aligned.
	unsigned char *tmp = (unsigned char*)malloc(s + AV_INPUT_BUFFER_PADDING_SIZE) ;
#else //MAC
	unsigned char *tmp = (unsigned char*)memalign(16, s + AV_INPUT_BUFFER_PADDING_SIZE) ;
#endif //MAC
#endif //MINGW
	if (tmp == NULL) {
		std::cerr << "FFmpegVideo::decodeData() Unable to allocate new buffer of size " << s << std::endl;
		return false;
	}
	/* copy chunk data without header to new buffer */
	memcpy(tmp, &((unsigned char*)chunk.data)[HEADER_SIZE], s);

	/* set end of buffer to 0 (this ensures that no overreading happens for damaged mpeg streams) */
	memset(&tmp[s], 0, AV_INPUT_BUFFER_PADDING_SIZE) ;

	decoding_buffer.size = s ;
	decoding_buffer.data = tmp;
	int got_frame = 1 ;

	while (decoding_buffer.size > 0 || (!decoding_buffer.data && got_frame)) {
		int len = avcodec_decode_video2(decoding_context,decoding_frame_buffer,&got_frame,&decoding_buffer) ;

		if (len < 0)
		{
			std::cerr << "Error decoding frame! Return=" << len << std::endl;
			return false ;
		}

		decoding_buffer.data += len;
		decoding_buffer.size -= len;

		if(got_frame)
		{
			image = QImage(QSize(decoding_frame_buffer->width,decoding_frame_buffer->height),QImage::Format_ARGB32) ;

#ifdef DEBUG_MPEG_VIDEO
			std::cerr << "Decoded frame. Size=" << image.width() << "x" << image.height() << std::endl;
#endif

			for (int y = 0; y < decoding_frame_buffer->height; y++)
				for (int x = 0; x < decoding_frame_buffer->width; x++)
				{
					int Y  = decoding_frame_buffer->data[0][y * decoding_frame_buffer->linesize[0] + x] ;
					int U  = decoding_frame_buffer->data[1][(y/2) * decoding_frame_buffer->linesize[1] + x/2] ;
					int V  = decoding_frame_buffer->data[2][(y/2) * decoding_frame_buffer->linesize[2] + x/2] ;

					int B = std::min(255,std::max(0,(int)(1.164*(Y - 16) + 1.596*(V - 128)))) ;
					int G = std::min(255,std::max(0,(int)(1.164*(Y - 16) - 0.813*(V - 128) - 0.391*(U - 128)))) ;
					int R = std::min(255,std::max(0,(int)(1.164*(Y - 16)                   + 2.018*(U - 128)))) ;

					image.setPixel(QPoint(x,y),QRgb( 0xff000000 + (R << 16) + (G << 8) + B)) ;
				}
		}
	}
	/* flush the decoder */
	decoding_buffer.data  = NULL;
	decoding_buffer.size  = 0;
	//avcodec_decode_video2(decoding_context,decoding_frame_buffer,&got_frame,&decoding_buffer) ;

	return true ;
}
