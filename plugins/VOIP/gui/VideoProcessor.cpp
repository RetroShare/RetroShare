#include <iostream>
#include <assert.h>
#include <malloc.h>

#include <QByteArray>
#include <QBuffer>
#include <QImage>

#include "VideoProcessor.h"
#include "QVideoDevice.h"
#include "DaubechyWavelets.h"

#include <math.h>


extern "C" {
#include <libavcodec/avcodec.h>

#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
}
#define DEBUG_MPEG_VIDEO 1

VideoProcessor::VideoProcessor()
    :_encoded_frame_size(176,144) 
{
	_decoded_output_device = NULL ;
  //_encoding_current_codec = VIDEO_PROCESSOR_CODEC_ID_JPEG_VIDEO;
  //_encoding_current_codec = VIDEO_PROCESSOR_CODEC_ID_DDWT_VIDEO;
    _encoding_current_codec = VIDEO_PROCESSOR_CODEC_ID_MPEG_VIDEO;
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
    case VIDEO_PROCESSOR_CODEC_ID_MPEG_VIDEO: codec = &_mpeg_video_codec ;
	    break ;
    default:
	    codec = NULL ;
    }

    //    std::cerr << "reducing to " << _frame_size.width() << " x " << _frame_size.height() << std::endl;

    encoded_size = 0 ;

    if(codec)
    {
	    RsVOIPDataChunk chunk ;

	    if(codec->encodeData(img.scaled(_encoded_frame_size,Qt::IgnoreAspectRatio,Qt::SmoothTransformation),size_hint,chunk) && chunk.size > 0)
	{
		encoded_size = chunk.size ;
		_encoded_out_queue.push_back(chunk) ;
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
    case VIDEO_PROCESSOR_CODEC_ID_DDWT_VIDEO: codec = &_ddwt_video_codec ;
	    break ;
    case VIDEO_PROCESSOR_CODEC_ID_MPEG_VIDEO: codec = &_mpeg_video_codec ;
	    break ;
    default:
	    codec = NULL ;
    }
    QImage img ;

    if(codec == NULL)
    {
        std::cerr << "Unknown decoding codec: " << codid << std::endl;
        return ;
    }
    
    if(!codec->decodeData(chunk,img)) 
    {
        std::cerr << "No image decoded. Probably in the middle of something..." << std::endl;
        return ;
    }

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

    if(_encoded_ref_frame_count++ < _encoded_ref_frame_max_distance && image.size() == _encoded_reference_frame.size())
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

    memcpy(&((unsigned char*)voip_chunk.data)[HEADER_SIZE],qb.data(),qb.size()) ;

    voip_chunk.size = HEADER_SIZE + qb.size() ;
    voip_chunk.type = RsVOIPDataChunk::RS_VOIP_DATA_TYPE_VIDEO ;

    return true ;
}


bool WaveletVideo::encodeData(const QImage& image,uint32_t size_hint,RsVOIPDataChunk& voip_chunk)
{
    static const int   WAVELET_IMG_SIZE =  128 ;
    static const float W_THRESHOLD      = 0.005 ;	// low quality
    //static const float W_THRESHOLD      = 0.0001;	// high quality
    //static const float W_THRESHOLD      = 0.0005;	// medium quality

    static const int W2 = WAVELET_IMG_SIZE ;
    static const int H2 = WAVELET_IMG_SIZE ;

    assert(image.width() == W2) ;
    assert(image.height() == H2) ;
    
    float *temp = new float[W2*H2] ;

    std::cerr << "  codec type: wavelets." << std::endl;

    // We should perform some interpolation here ;-)
    //
    for(int i=0;i<W2*H2;++i)
		    temp[i] = (0.3*image.constBits()[4*i+1] + 0.59*image.constBits()[4*i+2] + 0.11*image.constBits()[4*i+3]) / 255.0 ;
	    
    std::cerr << "  resized image to B&W " << W2 << "x" << H2 << std::endl;

    DaubechyWavelets<float>::DWT2D(temp,W2,H2,DaubechyWavelets<float>::DWT_DAUB12,DaubechyWavelets<float>::DWT_FORWARD) ;

    // Now estimate the max energy in the W coefs, and only keep the largest.

    float mx = 0.0f ;
    for(int i=0;i<W2*H2;++i)
	    if(mx < fabsf(temp[i]))
		    mx = fabs(temp[i]) ;
    
    mx *= 1.1;	// This avoids quantisation problems with wavelet coefs when they get too close to mx.

    std::cerr << "  max wavelet coef : " << mx << std::endl;

    std::vector<uint16_t> compressed_values ;
    compressed_values.reserve(W2*H2) ;

    for(int i=0;i<W2*H2;++i)
	    if(fabs(temp[i]) >= W_THRESHOLD*mx)	// This needs to be improved. Wavelets do not all have the same visual impact.
	    {
		    // add one value, using 16 bits for coordinates and 16 bits for the value.

		    compressed_values.push_back((uint16_t)i) ;
		    compressed_values.push_back(quantize_16b(temp[i],mx)) ;

		    //float f2 = from_quantized_16b(quantize_16b(temp[i],mx),mx) ;

		    //if(fabs(f2 - temp[i]) >= 0.01*(fabs(temp[i])+fabs(f2)))
		    //std::cerr << "     before: " << temp[i] << ", quantised=" << quantize_16b(temp[i],mx)<< ", after: " << f2 << std::endl;
	    }
    delete[] temp ;

    // Serialise all values into a memory buffer. This needs to be taken care of because of endian issues.

    int compressed_size = 4 + compressed_values.size()*2 ;

    std::cerr << "  threshold  : " << W_THRESHOLD << std::endl;
    std::cerr << "  values kept: " << compressed_values.size()/2 << std::endl;
    std::cerr << "  compression: " << compressed_size/float(W2*H2*3)*100 << " %" << std::endl;

    voip_chunk.data = malloc(HEADER_SIZE + compressed_size) ;

    // build header
    uint32_t flags = 0 ;

    ((unsigned char *)voip_chunk.data)[0] =  VideoProcessor::VIDEO_PROCESSOR_CODEC_ID_DDWT_VIDEO       & 0xff ;
    ((unsigned char *)voip_chunk.data)[1] = (VideoProcessor::VIDEO_PROCESSOR_CODEC_ID_DDWT_VIDEO >> 8) & 0xff ;
    ((unsigned char *)voip_chunk.data)[2] =  flags       & 0xff ;
    ((unsigned char *)voip_chunk.data)[3] = (flags >> 8) & 0xff ;

    unsigned char *compressed_mem = &((unsigned char *)voip_chunk.data)[HEADER_SIZE] ;
    serialise_ufloat(compressed_mem,mx) ;

    for(uint32_t i=0;i<compressed_values.size();++i)
    {
	    compressed_mem[4 + 2*i+0] = compressed_values[i] & 0xff ;
	    compressed_mem[4 + 2*i+1] = compressed_values[i] >> 8   ;
    }
    
    voip_chunk.type = RsVOIPDataChunk::RS_VOIP_DATA_TYPE_VIDEO ;
    voip_chunk.size = HEADER_SIZE + compressed_size ;
    
    return true ;
}

bool WaveletVideo::decodeData(const RsVOIPDataChunk& chunk,QImage& image)
{
    static const int   WAVELET_IMG_SIZE =  128 ;

    static const int W2 = WAVELET_IMG_SIZE ;
    static const int H2 = WAVELET_IMG_SIZE ;

    float *temp = new float[W2*H2] ;

    const unsigned char *compressed_mem = &static_cast<const unsigned char *>(chunk.data)[HEADER_SIZE] ;
    int compressed_size = chunk.size - HEADER_SIZE;

    memset(temp,0,W2*H2*sizeof(float)) ;
    float M = deserialise_ufloat(compressed_mem);

#ifdef VOIP_CODEC_DEBUG
    std::cerr << "  codec type: wavelets." << std::endl;
    std::cerr << "  max coef: " << M << std::endl;
#endif

    for(int i=4;i<compressed_size;i+=4)
    {
	    // read all values. first 2 bytes: image coordinates.
	    // next two bytes: value.
	    //
	    uint16_t indx = compressed_mem[i+0] + (compressed_mem[i+1] << 8) ;
	    uint16_t encv = compressed_mem[i+2] + (compressed_mem[i+3] << 8) ;

	    float f = from_quantized_16b(encv,M) ;

	    temp[indx] = f ;
    }
#ifdef VOIP_CODEC_DEBUG
    std::cerr << "  values read: " << compressed_size/4-1 << std::endl;
#endif

    DaubechyWavelets<float>::DWT2D(temp,W2,H2,DaubechyWavelets<float>::DWT_DAUB12,DaubechyWavelets<float>::DWT_BACKWARD) ;

#ifdef VOIP_CODEC_DEBUG
    std::cerr << "  resizing image to: " << w << "x" << h << std::endl;
#endif

    image = QImage(W2,H2,QImage::Format_RGB32) ;

    int indx = 0 ;

    for(int j=0;j<H2;++j)
	    for(int i=0;i<W2;++i,++indx)
	    {
		    uint32_t val = std::min(255,std::max(0,(int)(255*temp[indx]))) ;
            
		    QRgb rgb = (0xff << 24) + (val << 16) + (val << 8) + val ;

		    image.setPixel(i,j,rgb); 
	    }

    delete[] temp ;
    return true ;
}

uint16_t WaveletVideo::quantize_16b(float x,float M)
{
	// Do the quantization into 
	//   x = M * (m * 2^{-p} / 2^10)
	//
	// where m is coded on 10 bits (0->1023), and p is coded on 6 bits (0->63).
	// Packing [mp] into a 16bit uint16_t. M is the maximum coefficient over the quantization
	// process. 
	//
	// So this represents numbers from M * 1 * 2^{-73} to M
	//
	// All calculatoins are performed on x/M*2^10
	//
	static const float LOG2 = log(2.0f) ;

	int m,p ;

	if(fabs(x) < 1e-8*M)
	{
		m = 0 ;
		p = 0 ;
	}
	else
	{
		float log2f = log(fabsf(x)/M)/LOG2 ;
		int mexp = (int)floor(MANTISSE_BITS - log2f) ;

		m = (int)floor(pow(2.0f,mexp+log2f)) ;
		p = mexp ;

		if(p > (1<<EXPONENT_BITS)-1)
			m=0 ;
	}

	return (uint16_t)(p & ((1<<EXPONENT_BITS)-1)) + (uint16_t)((m & ((1<<MANTISSE_BITS)-1)) << EXPONENT_BITS) + ((x<0.0)?32768:0);
}

float WaveletVideo::from_quantized_16b(uint16_t n,float M)
{
	M *= (n&32768)?-1:1 ;

	n &= 32767 ;
	uint32_t p = n & ((1<<EXPONENT_BITS)-1) ;
	uint32_t m = (n & (((1<<MANTISSE_BITS)-1) << EXPONENT_BITS)) >> EXPONENT_BITS ;

	if(p > 10)
		return M * m / 1024.0f / (float)(1 << (p-10)) ;
	else
		return M * m / (float)(1 << p) ;
}

void WaveletVideo::serialise_ufloat(unsigned char *mem, float f)
{
	if(f < 0.0f)
	{
		std::cerr << "(EE) Cannot serialise invalid negative float value " << f << " in " << __PRETTY_FUNCTION__ << std::endl;
		return ;
	}
	// This serialisation is quite accurate. The max relative error is approx.
	// 0.01% and most of the time less than 1e-05% The error is well distributed
	// over numbers also.
	//
	uint32_t n = (f < 1e-7)?(~(uint32_t)0): ((uint32_t)( (1.0f/(1.0f+f) * (~(uint32_t)0)))) ;

	mem[0] = n & 0xff ; n >>= 8 ;
	mem[1] = n & 0xff ; n >>= 8 ;
	mem[2] = n & 0xff ; n >>= 8 ;
	mem[3] = n & 0xff ; 
}
float WaveletVideo::deserialise_ufloat(const unsigned char *mem)
{
	uint32_t n  = mem[3] ;
	n = (n << 8) + mem[2] ;
	n = (n << 8) + mem[1] ;
	n = (n << 8) + mem[0] ;

	return 1.0f/ ( n/(float)(~(uint32_t)0)) - 1.0f ;
}

FFmpegVideo::FFmpegVideo()
{
    // Encoding   
     
    encoding_codec = NULL ;
    encoding_frame_buffer = NULL ;
    encoding_context = NULL ;
    
    //AVCodecID codec_id = AV_CODEC_ID_H264 ; 
    AVCodecID codec_id = AV_CODEC_ID_MPEG1VIDEO;
    
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    /* find the mpeg1 video encoder */
    encoding_codec = avcodec_find_encoder(codec_id);
    
    if (!encoding_codec)  throw("AV codec not found for codec id ") ;

    encoding_context = avcodec_alloc_context3(encoding_codec);
    
    if (!encoding_context)  throw std::runtime_error("AV: Could not allocate video codec encoding context");

    /* put sample parameters */
    encoding_context->bit_rate = 200000;
    /* resolution must be a multiple of two */
    encoding_context->width = 352;//176;
    encoding_context->height = 288;//144;
    /* frames per second */
    encoding_context->time_base = (AVRational){1,25};
    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    encoding_context->gop_size = 10;
    encoding_context->max_b_frames = 1;
    //context->pix_fmt = AV_PIX_FMT_RGB24;
    encoding_context->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codec_id == AV_CODEC_ID_H264)
        av_opt_set(encoding_context->priv_data, "preset", "slow", 0);

    /* open it */
    if (avcodec_open2(encoding_context, encoding_codec, NULL) < 0) 
        throw std::runtime_error( "AV: Could not open codec context. Something's wrong.");

    encoding_frame_buffer = avcodec_alloc_frame() ;//(AVFrame*)malloc(sizeof(AVFrame)) ;
    
    if(!encoding_frame_buffer)
        throw std::runtime_error("AV: could not allocate frame buffer.") ;
    
    encoding_frame_buffer->format = encoding_context->pix_fmt;
    encoding_frame_buffer->width  = encoding_context->width;
    encoding_frame_buffer->height = encoding_context->height;

    /* the image can be allocated by any means and av_image_alloc() is
     * just the most convenient way if av_malloc() is to be used */
    
    int ret = av_image_alloc(encoding_frame_buffer->data, encoding_frame_buffer->linesize,
                             encoding_context->width, encoding_context->height, encoding_context->pix_fmt, 32);
    
    if (ret < 0) 
       throw std::runtime_error("AV: Could not allocate raw picture buffer");
    
    encoding_frame_count = 0 ;
    
    // Decoding
    
    decoding_codec = avcodec_find_decoder(codec_id);
    
    if (!decoding_codec)  
        throw("AV codec not found for codec id ") ;
    
    decoding_context = avcodec_alloc_context3(decoding_codec);
    
    if(!decoding_context)  
        throw std::runtime_error("AV: Could not allocate video codec decoding context");
    
    decoding_context->width = encoding_context->width;
    decoding_context->height = encoding_context->height;
    decoding_context->pix_fmt = AV_PIX_FMT_YUV420P;
    
    if(decoding_codec->capabilities & CODEC_CAP_TRUNCATED)
         decoding_context->flags |= CODEC_FLAG_TRUNCATED; // we do not send complete frames
    
    if(avcodec_open2(decoding_context,decoding_codec,NULL) < 0)
        throw("AV codec open action failed! ") ;
    
    decoding_frame_buffer = avcodec_alloc_frame() ;//(AVFrame*)malloc(sizeof(AVFrame)) ;
    
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
    avcodec_close(encoding_context);
    avcodec_close(decoding_context);
    av_free(encoding_context);
    av_free(decoding_context);
    av_freep(&encoding_frame_buffer->data[0]);
    av_freep(&decoding_frame_buffer->data[0]);
    free(encoding_frame_buffer);
    free(decoding_frame_buffer);
}


bool FFmpegVideo::encodeData(const QImage& image,uint32_t size_hint,RsVOIPDataChunk& voip_chunk)
{
#ifdef DEBUG_MPEG_VIDEO 
	std::cerr << "Encoding frame of size " << image.width() << "x" << image.height() << ", resized to " << encoding_frame_buffer->width << "x" << encoding_frame_buffer->height << " : "; 
#endif
	QImage input ;

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

	AVFrame *frame = encoding_frame_buffer ;

	AVPacket pkt ;
	av_init_packet(&pkt);
	pkt.data = NULL;    // packet data will be allocated by the encoder
	pkt.size = 0;

	//    do
	//    {
	int ret = avcodec_encode_video2(encoding_context, &pkt, frame, &got_output) ;

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
		voip_chunk.data = malloc(pkt.size + HEADER_SIZE) ;
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
#endif
	unsigned char *buff ;

	if(decoding_buffer.data != NULL)
	{
#ifdef DEBUG_MPEG_VIDEO
		std::cerr << "Completing buffer with size " << chunk.size - HEADER_SIZE + decoding_buffer.size << ": copying existing " 
                  << decoding_buffer.size << " bytes. Adding new " << chunk.size  - HEADER_SIZE<< " bytes " << std::endl;
#endif

		uint32_t s =  chunk.size - HEADER_SIZE + decoding_buffer.size ;
		unsigned char *tmp = (unsigned char*)memalign(16,s + FF_INPUT_BUFFER_PADDING_SIZE) ;
		memset(tmp,0,s+FF_INPUT_BUFFER_PADDING_SIZE) ;

		memcpy(tmp,decoding_buffer.data,decoding_buffer.size) ;

		free(decoding_buffer.data) ;

		buff = &tmp[decoding_buffer.size] ;
		decoding_buffer.size = s ;
		decoding_buffer.data = tmp ;
	}
	else
	{
#ifdef DEBUG_MPEG_VIDEO
		std::cerr << "Allocating new buffer of size " << chunk.size - HEADER_SIZE << std::endl;
#endif

		decoding_buffer.data = (unsigned char *)memalign(16,chunk.size - HEADER_SIZE + FF_INPUT_BUFFER_PADDING_SIZE) ;
		decoding_buffer.size = chunk.size - HEADER_SIZE ;
		memset(decoding_buffer.data,0,decoding_buffer.size + FF_INPUT_BUFFER_PADDING_SIZE) ;

		buff = decoding_buffer.data ;
	}

	memcpy(buff,chunk.data+HEADER_SIZE,chunk.size - HEADER_SIZE) ;

	int got_frame = 0 ;
	int len = avcodec_decode_video2(decoding_context,decoding_frame_buffer,&got_frame,&decoding_buffer) ;

	if(len < 0)
	{
		std::cerr << "Error decodign frame!" << std::endl;
		return false ;
	}
#ifdef DEBUG_MPEG_VIDEO
	std::cerr << "Used " << len << " bytes out of " << decoding_buffer.size << ". got_frame = " << got_frame << std::endl;
#endif

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

				int R = std::min(255,std::max(0,(int)(1.164*(Y - 16) + 1.596*(V - 128)))) ;
				int G = std::min(255,std::max(0,(int)(1.164*(Y - 16) - 0.813*(V - 128) - 0.391*(U - 128)))) ;
				int B = std::min(255,std::max(0,(int)(1.164*(Y - 16)                   + 2.018*(U - 128)))) ;

				image.setPixel(QPoint(x,y),QRgb( 0xff000000 + (R << 16) + (G << 8) + B)) ;
			}
	}

	if(len == decoding_buffer.size)
	{
		free(decoding_buffer.data) ;
		decoding_buffer.data  = NULL;
		decoding_buffer.size  = 0;
	}
	else if(len != 0)
	{
#ifdef DEBUG_MPEG_VIDEO
		std::cerr << "Moving remaining data (" << decoding_buffer.size - len << " bytes) back to 0" << std::endl;
#endif

		memmove(decoding_buffer.data,decoding_buffer.data+len,decoding_buffer.size - len) ;
		decoding_buffer.size -= len ;
	}

	return true ;
}

