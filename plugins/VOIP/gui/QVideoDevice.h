#pragma once 

#include <QLabel>
#include "interface/rsvoip.h"

class VideoEncoder ;
class CvCapture ;

// Responsible from displaying the video. The source of the video is
// a VideoDecoder object, which uses a codec.
//
class QVideoOutputDevice: public QLabel
{
	public:
		QVideoOutputDevice(QWidget *parent) ;
		
		void showFrame(const QImage&) ;
};

// Responsible for grabbing the video from the webcam and sending it to the 
// VideoEncoder object.
//
class QVideoInputDevice: public QObject
{
	Q_OBJECT

	public:
		QVideoInputDevice(QWidget *parent) ;
		~QVideoInputDevice() ;

		// Captured images are sent to this encoder. Can be NULL.
		//
		void setVideoEncoder(VideoEncoder *venc) { _video_encoder = venc ; }

		// All images received will be echoed to this target. We could use signal/slots, but it's
		// probably faster this way. Can be NULL.
		//
		void setEchoVideoTarget(QVideoOutputDevice *odev) { _echo_output_device = odev ; }
	
		// get the next encoded video data chunk.
		//
		bool getNextEncodedPacket(RsVoipDataChunk&) ;

		void start() ;
		void stop() ;

	protected slots:
		void grabFrame() ;

	signals:
		void networkPacketReady() ;

	private:
		VideoEncoder *_video_encoder ;
		QTimer *_timer ;
		CvCapture *_capture_device ;

		QVideoOutputDevice *_echo_output_device ;

		std::list<RsVoipDataChunk> _out_queue ;
};

