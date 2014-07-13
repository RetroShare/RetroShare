#pragma once

#include <stdint.h>
#include <QImage>

class QVideoOutputDevice ;

// This class decodes video from a stream. It keeps a queue of
// decoded frame that needs to be retrieved using the getNextImage() method.
//
class VideoDecoder
{
	public:
		VideoDecoder() { _output_device = NULL ;}

		// Gets the next image to be displayed. Once returned, the image should
		// be cleared from the incoming queue.
		//
		void setDisplayTarget(QVideoOutputDevice *odev) { _output_device = odev ; }

		virtual void receiveEncodedData(const unsigned char *data,uint32_t size) ;

	private:
		QVideoOutputDevice *_output_device ;

		std::list<QImage> _image_queue ;

		// Incoming data is processed by a video codec and converted into images.
		//
		virtual QImage decodeData(const unsigned char *encoded_image,uint32_t encoded_image_size) = 0 ;

		// This buffer accumulated incoming encoded data, until a full packet is obtained,
		// since the stream might not send images at once. When incoming images are decoded, the
		// data is removed from the buffer.
		//
		unsigned char *buffer ;
		uint32_t buffer_size ;
};

// This class encodes video using a video codec (possibly homemade, or based on existing codecs)
// and produces a data stream that is sent to the network transfer service (e.g. p3VoRs).
//
class VideoEncoder
{
	public:
		VideoEncoder() { _echo_output_device = NULL ;}

		// Takes the next image to be encoded. 
		//
		virtual bool addImage(const QImage& Image) ;

	protected:
		//virtual bool sendEncodedData(unsigned char *mem,uint32_t size) = 0 ;
		virtual void encodeData(const QImage& image) = 0 ;

		unsigned char *buffer ;
		uint32_t buffer_size ;

		QVideoOutputDevice *_echo_output_device ;
};

// Now derive various image encoding/decoding algorithms.
//

class JPEGVideoDecoder: public VideoDecoder
{
	protected:
		virtual QImage decodeData(const unsigned char *encoded_image,uint32_t encoded_image_size) ;
};

class JPEGVideoEncoder: public VideoEncoder
{
	public:
		JPEGVideoEncoder() {}

	protected:
		virtual void encodeData(const QImage& Image) ;
};

