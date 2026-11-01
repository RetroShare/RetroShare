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
    : _encoded_out_queue(8), _encoded_frame_size(640,480),
      _mpeg_video_codec(FFmpegVideo::KIND_MPEG4, VIDEO_PROCESSOR_CODEC_ID_MPEG_VIDEO),
      _h264_video_codec(FFmpegVideo::KIND_H264, VIDEO_PROCESSOR_CODEC_ID_H264_VIDEO)
{
    //_lastTimeToShowFrame = time(NULL);
    _decoded_output_device = NULL ;

    // H.264 gives a much better picture for the same bandwidth. MPEG-4 is the
    // fallback when libavcodec has no usable H.264 encoder.
    _encoding_current_codec = _h264_video_codec.canEncode() ? VIDEO_PROCESSOR_CODEC_ID_H264_VIDEO
                                                            : VIDEO_PROCESSOR_CODEC_ID_MPEG_VIDEO;

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

    while(!_encoded_out_queue.empty())
    {
        _encoded_out_queue.tail().clear();
        _encoded_out_queue.pop() ;
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
    case VIDEO_PROCESSOR_CODEC_ID_H264_VIDEO: codec = &_h264_video_codec ;
	    break ;
    default:
	    codec = NULL ;
    }

    if(codec)
    {
      // Crop to the aspect ratio of the encoded frame (4:3) instead of squashing
      // e.g. 16:9 webcam images.
      QImage src = img ;
      const int w = img.width(), h = img.height() ;
      const int fw = _encoded_frame_size.width(), fh = _encoded_frame_size.height() ;

      if(w > 0 && h > 0 && fw > 0 && fh > 0)
      {
          if((int64_t)w * fh > (int64_t)h * fw)		// too wide
          {
              const int nw = (int)((int64_t)h * fw / fh) ;
              src = img.copy((w - nw) / 2, 0, nw, h) ;
          }
          else if((int64_t)w * fh < (int64_t)h * fw)	// too tall
          {
              const int nh = (int)((int64_t)w * fh / fw) ;
              src = img.copy(0, (h - nh) / 2, w, nh) ;
          }
      }

      codec->encodeData(src.scaled(_encoded_frame_size,Qt::IgnoreAspectRatio,Qt::SmoothTransformation),_target_bandwidth_out, _encoded_out_queue);

	    time_t now = time(NULL) ;

	    if(now > _last_bw_estimate_out_TS)
	    {
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
  return _encoded_out_queue.pop(chunk)
    && ((_total_encoded_size_out += chunk.size), true);
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
    case VIDEO_PROCESSOR_CODEC_ID_H264_VIDEO: codec = _h264_video_codec.canDecode() ? &_h264_video_codec : NULL ;
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

	    for(int i=0;i<image.sizeInBytes();++i)
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

bool JPEGVideo::encodeData(const QImage& image,uint32_t /* size_hint */, NetQueue<RsVOIPDataChunk>& dst)
{
    // check if we make a diff image, or if we use the full frame.

    if(dst.full()) return false;
    RsVOIPDataChunk& voip_chunk = dst.head();

    QImage encoded_frame ;
    bool differential_frame ;

    if (_encoded_ref_frame_count++ < _encoded_ref_frame_max_distance
        && image.size() == _encoded_reference_frame.size()
        && image.sizeInBytes() == _encoded_reference_frame.sizeInBytes())
	{
	    // compute difference with reference frame.
	    encoded_frame = image ;

	    for(int i=0;i<image.sizeInBytes();++i)
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

    dst.push();

    return true ;
}

#if LIBAVCODEC_VERSION_MAJOR < 54
#  define RS_AV_CODEC_ID(x) CODEC_ID_##x
#  define RS_AV_PIX_FMT(x)  PIX_FMT_##x
#else
#  define RS_AV_CODEC_ID(x) AV_CODEC_ID_##x
#  define RS_AV_PIX_FMT(x)  AV_PIX_FMT_##x
#endif

static const int     VIDEO_WIDTH       = 640 ;
static const int     VIDEO_HEIGHT      = 480 ;
static const int     VIDEO_FPS         = 20 ;		// = camera grab period of QVideoInputDevice (50 ms)
static const int64_t MIN_VIDEO_BITRATE = 64*1000 ;	// bits/s
static const int64_t MAX_VIDEO_BITRATE = 4*1000*1000 ;	// bits/s
static const int64_t DEFAULT_VIDEO_BITRATE = 30*1024*8 ;	// bits/s, until a target bandwidth is given

FFmpegVideo::FFmpegVideo(Kind kind, uint16_t header_codec_id)
    : _kind(kind), _header_codec_id(header_codec_id), _encoder_bitrate(0), _last_encoder_reopen(0),
      _encoder_can_change_bitrate(false)
{
#if LIBAVCODEC_VERSION_MAJOR < 58
    avcodec_register_all();
#endif
    encoding_codec = NULL ;
    decoding_codec = NULL ;
    encoding_context = NULL ;
    decoding_context = NULL ;
    encoding_frame_buffer = NULL ;
    decoding_frame_buffer = NULL ;
    encoding_frame_count = 0 ;

    // ------------------------------- Encoder ---------------------------------

    if(kind == KIND_H264)
    {
        // Try the H.264 encoders in order of preference, keep the first one that opens.
        static const char *h264_encoders[] = { "libx264", "libopenh264", "h264_mf", NULL } ;

        for(int i=0; h264_encoders[i] != NULL; ++i)
        {
            encoding_codec = avcodec_find_encoder_by_name(h264_encoders[i]) ;

            if(encoding_codec != NULL && openEncoder(DEFAULT_VIDEO_BITRATE))
            {
                _encoder_can_change_bitrate = (i == 0) ;	// libx264
                break ;
            }
            encoding_codec = NULL ;
        }
        if(encoding_codec == NULL)
            std::cerr << "VOIP: no usable H.264 encoder in libavcodec (libx264/libopenh264/h264_mf). Falling back to MPEG-4." << std::endl;
    }
    else
    {
        encoding_codec = avcodec_find_encoder(RS_AV_CODEC_ID(MPEG4)) ;

        if(encoding_codec == NULL || !openEncoder(DEFAULT_VIDEO_BITRATE))
        {
            std::cerr << "VOIP: cannot open the MPEG-4 video encoder." << std::endl;
            encoding_codec = NULL ;
        }
    }

    if(encoding_codec != NULL)
    {
#if (LIBAVCODEC_VERSION_MAJOR < 57) | (LIBAVCODEC_VERSION_MAJOR == 57 && LIBAVCODEC_VERSION_MINOR <3 )
        encoding_frame_buffer = avcodec_alloc_frame() ;
#else
        encoding_frame_buffer = av_frame_alloc() ;
#endif
        if(encoding_frame_buffer)
        {
            encoding_frame_buffer->format = RS_AV_PIX_FMT(YUV420P) ;
            encoding_frame_buffer->width  = VIDEO_WIDTH ;
            encoding_frame_buffer->height = VIDEO_HEIGHT ;

            if(av_image_alloc(encoding_frame_buffer->data, encoding_frame_buffer->linesize,
                              VIDEO_WIDTH, VIDEO_HEIGHT, RS_AV_PIX_FMT(YUV420P), 32) < 0)
            {
                std::cerr << "AV: Could not allocate raw picture buffer" << std::endl;
                av_frame_free(&encoding_frame_buffer) ;
            }
        }
        if(!encoding_frame_buffer)
        {
            avcodec_free_context(&encoding_context) ;
            encoding_codec = NULL ;
        }
    }

    // ------------------------------- Decoder ---------------------------------

    decoding_codec = avcodec_find_decoder(kind == KIND_H264 ? RS_AV_CODEC_ID(H264) : RS_AV_CODEC_ID(MPEG4)) ;

    if(decoding_codec != NULL)
        decoding_context = avcodec_alloc_context3(decoding_codec) ;

    if(decoding_context != NULL)
    {
        decoding_context->width   = VIDEO_WIDTH ;
        decoding_context->height  = VIDEO_HEIGHT ;
        decoding_context->pix_fmt = RS_AV_PIX_FMT(YUV420P) ;

#if LIBAVCODEC_VERSION_MAJOR < 58
        if(decoding_codec->capabilities & AV_CODEC_CAP_TRUNCATED)
            decoding_context->flags |= AV_CODEC_FLAG_TRUNCATED; // we do not send complete frames
#endif
        decoding_context->flags2 |= AV_CODEC_FLAG2_CHUNKS;	// we can receive truncated frames
        decoding_context->flags  |= AV_CODEC_FLAG_LOW_DELAY;
        // Frame threading delays every picture by one frame per thread: use slice threads only.
        decoding_context->thread_type = FF_THREAD_SLICE ;

        if(avcodec_open2(decoding_context, decoding_codec, NULL) < 0)
        {
            std::cerr << "AV: cannot open the video decoder." << std::endl;
            avcodec_free_context(&decoding_context) ;
        }
    }
    if(decoding_context != NULL)
    {
        decoding_frame_buffer = av_frame_alloc() ;

        av_init_packet(&decoding_buffer);
        decoding_buffer.data = NULL ;
        decoding_buffer.size = 0 ;
    }

#ifdef DEBUG_MPEG_VIDEO
    std::cerr << "Dumping captured data to file tmpvideo.mpg" << std::endl;
    encoding_debug_file = fopen("tmpvideo.mpg","w") ;
#endif
}

bool FFmpegVideo::openEncoder(int64_t bitrate)
{
    if(encoding_codec == NULL)
        return false ;

    if(encoding_context != NULL)
        avcodec_free_context(&encoding_context) ;

    encoding_context = avcodec_alloc_context3(encoding_codec) ;

    if(encoding_context == NULL)
        return false ;

    encoding_context->width   = VIDEO_WIDTH ;
    encoding_context->height  = VIDEO_HEIGHT ;
    encoding_context->pix_fmt = RS_AV_PIX_FMT(YUV420P) ;
    encoding_context->time_base = av_make_q(1, VIDEO_FPS) ;
#if LIBAVCODEC_VERSION_MAJOR >= 57
    encoding_context->framerate = av_make_q(VIDEO_FPS, 1) ;
#endif

    // Bitrate in bits/s, with a ~1 second rate control buffer.
    encoding_context->bit_rate       = bitrate ;
    encoding_context->rc_max_rate    = bitrate ;
    encoding_context->rc_buffer_size = (int)bitrate ;

    // A key frame every 2 seconds: the picture recovers quickly after a lost packet,
    // and a peer that starts receiving later gets a picture quickly.
    encoding_context->gop_size     = 2*VIDEO_FPS ;
    encoding_context->max_b_frames = 0 ;	// B-frames add latency

    if(_kind == KIND_MPEG4)
    {
        encoding_context->qmin = 2 ;
        encoding_context->qmax = 31 ;		// MPEG-4 quantizer range is 1..31
        encoding_context->max_qdiff = 3 ;
        av_opt_set(encoding_context, "mbd", "rd", AV_OPT_SEARCH_CHILDREN) ;	// better macroblock decisions
        av_opt_set(encoding_context, "flags", "+mv4", AV_OPT_SEARCH_CHILDREN) ;
    }
    else
    {
        // libx264: fast, no frame lookahead, one packet per frame. (Options unknown to the
        // other H.264 encoders are simply ignored.)
        av_opt_set(encoding_context->priv_data, "preset", "veryfast", 0) ;
        av_opt_set(encoding_context->priv_data, "tune", "zerolatency", 0) ;
    }

    if(avcodec_open2(encoding_context, encoding_codec, NULL) < 0)
    {
        std::cerr << "AV: Could not open video encoder " << encoding_codec->name << std::endl;
        avcodec_free_context(&encoding_context) ;
        return false ;
    }

    _encoder_bitrate = bitrate ;
    _last_encoder_reopen = time(NULL) ;

    std::cerr << "VOIP: video encoder " << encoding_codec->name << " opened at " << bitrate/1000 << " kbit/s" << std::endl;
    return true ;
}

FFmpegVideo::~FFmpegVideo()
{
    if(encoding_frame_buffer)
        av_freep(&encoding_frame_buffer->data[0]) ;	// allocated by av_image_alloc()

    avcodec_free_context(&encoding_context);
    avcodec_free_context(&decoding_context);
    av_frame_free(&encoding_frame_buffer);
    av_frame_free(&decoding_frame_buffer);
}

static inline uint8_t clip8(int v) { return (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v)) ; }

// RGB -> YUV 4:2:0, ITU-R BT.601, limited range (what H.264/MPEG-4 decoders expect).
static void imageToFrame(AVFrame *frame, const QImage& image)
{
    QImage input = image ;

    if(input.width() != frame->width || input.height() != frame->height)
        input = input.scaled(QSize(frame->width, frame->height), Qt::IgnoreAspectRatio, Qt::SmoothTransformation) ;

    if(input.format() != QImage::Format_RGB32 && input.format() != QImage::Format_ARGB32)
        input = input.convertToFormat(QImage::Format_RGB32) ;

    for(int y = 0; y + 1 < frame->height; y += 2)
    {
        const QRgb *l0 = reinterpret_cast<const QRgb*>(input.constScanLine(y)) ;
        const QRgb *l1 = reinterpret_cast<const QRgb*>(input.constScanLine(y+1)) ;

        uint8_t *Y0 = frame->data[0] + (y+0) * frame->linesize[0] ;
        uint8_t *Y1 = frame->data[0] + (y+1) * frame->linesize[0] ;
        uint8_t *U  = frame->data[1] + (y/2) * frame->linesize[1] ;
        uint8_t *V  = frame->data[2] + (y/2) * frame->linesize[2] ;

        for(int x = 0; x + 1 < frame->width; x += 2)
        {
            const QRgb p[4] = { l0[x], l0[x+1], l1[x], l1[x+1] } ;
            int rs = 0, gs = 0, bs = 0 ;
            uint8_t yv[4] ;

            for(int k=0;k<4;++k)
            {
                const int r = qRed(p[k]), g = qGreen(p[k]), b = qBlue(p[k]) ;
                yv[k] = clip8(((66*r + 129*g + 25*b + 128) >> 8) + 16) ;
                rs += r ; gs += g ; bs += b ;
            }
            Y0[x] = yv[0] ; Y0[x+1] = yv[1] ;
            Y1[x] = yv[2] ; Y1[x+1] = yv[3] ;

            const int r = rs >> 2, g = gs >> 2, b = bs >> 2 ;
            U[x/2] = clip8(((-38*r -  74*g + 112*b + 128) >> 8) + 128) ;	// Cb
            V[x/2] = clip8(((112*r -  94*g -  18*b + 128) >> 8) + 128) ;	// Cr
        }
    }
}

// YUV 4:2:0 (BT.601, limited range) -> RGB
static bool frameToImage(QImage& image, const AVFrame *frame)
{
    if(frame->format != RS_AV_PIX_FMT(YUV420P) && frame->format != RS_AV_PIX_FMT(YUVJ420P))
    {
        std::cerr << "VOIP: unsupported decoded pixel format " << frame->format << std::endl;
        return false ;
    }

    image = QImage(QSize(frame->width,frame->height),QImage::Format_RGB32);

    for(int y = 0; y < frame->height; y++)
    {
        QRgb *line = reinterpret_cast<QRgb*>(image.scanLine(y)) ;
        const uint8_t *Yp = frame->data[0] + y * frame->linesize[0] ;
        const uint8_t *Up = frame->data[1] + (y/2) * frame->linesize[1] ;
        const uint8_t *Vp = frame->data[2] + (y/2) * frame->linesize[2] ;

        for(int x = 0; x < frame->width; x++)
        {
            const int C = 298 * (Yp[x] - 16) ;
            const int D = Up[x/2] - 128 ;	// Cb
            const int E = Vp[x/2] - 128 ;	// Cr

            line[x] = qRgb(clip8((C + 409*E + 128) >> 8),
                           clip8((C - 100*D - 208*E + 128) >> 8),
                           clip8((C + 516*D + 128) >> 8)) ;
        }
    }
    return true ;
}

bool FFmpegVideo::encodeData(const QImage& image, uint32_t target_bytes_per_sec, NetQueue<RsVOIPDataChunk>& dst)
{
  if(encoding_codec == NULL || encoding_frame_buffer == NULL)
    return false ;

#if LIBAVCODEC_VERSION_MAJOR < 58
  if(dst.full()) return false;
#endif

  // ---- follow the target bandwidth (bytes/s -> bits/s) ----
  {
    int64_t wanted = (int64_t)target_bytes_per_sec * 8 ;
    if(wanted < MIN_VIDEO_BITRATE) wanted = MIN_VIDEO_BITRATE ;
    if(wanted > MAX_VIDEO_BITRATE) wanted = MAX_VIDEO_BITRATE ;

    if(encoding_context == NULL)
    {
      if(!openEncoder(wanted))
        return false ;
    }
    else if(wanted != _encoder_bitrate)
    {
      if(_encoder_can_change_bitrate)
      {
        // libx264 applies bitrate changes on the next frame (x264_encoder_reconfig)
        encoding_context->bit_rate       = wanted ;
        encoding_context->rc_max_rate    = wanted ;
        encoding_context->rc_buffer_size = (int)wanted ;
        _encoder_bitrate = wanted ;
      }
      else if((wanted > _encoder_bitrate * 5 / 4 || wanted < _encoder_bitrate * 4 / 5)
              && time(NULL) > _last_encoder_reopen + 2)
      {
        // Other encoders read the bitrate only when opened: reopen (starts with a key frame).
        if(!openEncoder(wanted))
          return false ;
      }
    }
  }

#ifdef DEBUG_MPEG_VIDEO
	std::cerr << "Encoding frame of size " << image.width() << "x" << image.height() << ", resized to " << encoding_frame_buffer->width << "x" << encoding_frame_buffer->height << " : ";
#endif

  imageToFrame(encoding_frame_buffer, image);

	encoding_frame_buffer->pts = encoding_frame_count++;

	/* encode the image */

	AVPacket pkt ;
	av_init_packet(&pkt);
#if LIBAVCODEC_VERSION_MAJOR < 54
	pkt.size = avpicture_get_size(encoding_context->pix_fmt, encoding_context->width, encoding_context->height);
	pkt.data = (uint8_t*)av_malloc(pkt.size);

  int got_output = 0;
	int ret = avcodec_encode_video(encoding_context, pkt.data, pkt.size, encoding_frame_buffer) ;
	if (ret > 0) {
		got_output = ret;
	}
#elif LIBAVCODEC_VERSION_MAJOR < 58
	pkt.data = NULL;    // packet data will be allocated by the encoder
	pkt.size = 0;

	int got_output = 0;
	int ret = avcodec_encode_video2(encoding_context, &pkt, encoding_frame_buffer, &got_output) ;
#else
  pkt.data = NULL;
  pkt.size = 0;

  int ret = avcodec_send_frame(encoding_context, encoding_frame_buffer);
  if(ret == AVERROR(EAGAIN))
    ret = 0;
#endif

	if (ret < 0)
	{
		std::cerr << "Error encoding frame!" << std::endl;
    goto fail;
	}

#if LIBAVCODEC_VERSION_MAJOR >= 58
  while(!dst.full() && !(ret = avcodec_receive_packet(encoding_context, &pkt)))
#else
  if(!got_output) {
		std::cerr << "No output produced." << std::endl;
	}
  else
#endif
	{
    RsVOIPDataChunk& voip_chunk = dst.head();
		voip_chunk.data = rs_malloc(pkt.size + HEADER_SIZE) ;

    if(!voip_chunk.data) {
      std::cerr << "error: failed to allocate RsVOIPDataChunk data"
        << std::endl;
      goto fail;
    }

		uint32_t flags = 0;

		((unsigned char *)voip_chunk.data)[0] =  _header_codec_id       & 0xff ;
		((unsigned char *)voip_chunk.data)[1] = (_header_codec_id >> 8) & 0xff ;
		((unsigned char *)voip_chunk.data)[2] = flags & 0xff ;
		((unsigned char *)voip_chunk.data)[3] = (flags >> 8) & 0xff ;

		memcpy(&((unsigned char*)voip_chunk.data)[HEADER_SIZE],pkt.data,pkt.size) ;

		voip_chunk.size = pkt.size + HEADER_SIZE;
		voip_chunk.type = RsVOIPDataChunk::RS_VOIP_DATA_TYPE_VIDEO ;

    dst.push();

#ifdef DEBUG_MPEG_VIDEO
		std::cerr << "Output : " << pkt.size << " bytes." << std::endl;
		fwrite(pkt.data,1,pkt.size,encoding_debug_file) ;
		fflush(encoding_debug_file) ;
#endif
#if LIBAVCODEC_VERSION_MAJOR >= 58
    av_packet_unref(&pkt);
#endif
	}

#if LIBAVCODEC_VERSION_MAJOR >= 58
  if(ret == AVERROR(EAGAIN))
    ret = 0;
  else if(ret < 0) {
    std::cerr << "error: failed to get packet from encoder: " << ret
      << std::endl;
  }
#endif

  fail:

#if LIBAVCODEC_VERSION_MAJOR < 58
	av_free_packet(&pkt);
#else
  av_packet_unref(&pkt);
#endif

  return ret == 0;
}

bool FFmpegVideo::decodeData(const RsVOIPDataChunk& chunk, QImage& image)
{
#ifdef DEBUG_MPEG_VIDEO
	std::cerr << "Decoding data of size " << chunk.size << std::endl;
	std::cerr << "Allocating new buffer of size " << chunk.size - HEADER_SIZE << std::endl;
#endif

	if(decoding_context == NULL || decoding_frame_buffer == NULL)
		return false ;

	uint32_t s = chunk.size - HEADER_SIZE ;
  unsigned char *tmp =
    (unsigned char *)av_malloc(s + AV_INPUT_BUFFER_PADDING_SIZE);
	if (tmp == NULL) {
		std::cerr << "FFmpegVideo::decodeData() Unable to allocate new buffer of size " << s << std::endl;
		return false;
	}
	/* copy chunk data without header to new buffer */
	memcpy(tmp, &((unsigned char*)chunk.data)[HEADER_SIZE], s);

	/* set end of buffer to 0 (this ensures that no overreading happens for damaged mpeg streams) */
	memset(&tmp[s], 0, AV_INPUT_BUFFER_PADDING_SIZE) ;

  av_packet_from_data(&decoding_buffer, tmp, s);


#if LIBAVCODEC_VERSION_MAJOR >= 58

  int ret = avcodec_send_packet(decoding_context, &decoding_buffer);
  av_packet_unref(&decoding_buffer);
  if(ret) {
    // EAGAIN is an error here because the decoder's buffer should be empty
    std::cerr << "error: failed to send packet to decoder: " << ret
      << std::endl;
  }

  ret = avcodec_receive_frame(decoding_context, decoding_frame_buffer);
  if(ret == AVERROR(EAGAIN)) {
    std::cerr << "warning: "
      << "decoder consumed a packet but didn't generate a frame" << std::endl;
    return false;
  }
  else if(ret) {
    std::cerr << "error: failed to fetch frame from decoder: " << ret
      << std::endl;
    return false ;
  }

  if(!frameToImage(image, decoding_frame_buffer))
    return false ;

#ifdef DEBUG_MPEG_VIDEO
  std::cerr << "debug(MPEG_VIDEO): decoded frame: size="
    << image.width() << "x" << image.height() << std::endl;
#endif

  // Clear out the internal buffer and drop the frames to avoid latency.
  // Video decoders typically only create one frame per packet, so this
  // shouldn't happen under normal circumstances.
  // Ideally, we would render the most recent frame and discard earlier ones,
  // but that would require copying of frames that is not worth it.
  while(!avcodec_receive_frame(decoding_context, decoding_frame_buffer)) {
    std::cerr << "warning: dropping decoded frame" << std::endl;
  }

#else

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
			frameToImage(image, decoding_frame_buffer);

#ifdef DEBUG_MPEG_VIDEO
			std::cerr << "Decoded frame. Size=" << image.width() << "x" << image.height() << std::endl;
#endif
		}
	}
	/* flush the decoder */
	decoding_buffer.data  = NULL;
	decoding_buffer.size  = 0;
	//avcodec_decode_video2(decoding_context,decoding_frame_buffer,&got_frame,&decoding_buffer) ;
#endif

	return true ;
}
