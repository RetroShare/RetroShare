#pragma once 

#include <QLabel>
#include "interface/rsVOIP.h"

class VideoEncoder ;
class CvCapture ;

// Responsible from displaying the video. The source of the video is
// a VideoDecoder object, which uses a codec.
//
class QVideoOutputDevice: public QLabel
{
	public:
		QVideoOutputDevice(QWidget *parent = 0) ;
		
		void showFrame(const QImage&) ;
		void showFrameOff() ;
};

// Responsible for grabbing the video from the webcam and sending it to the 
// VideoEncoder object.
//
class QVideoInputDevice: public QObject
{
	Q_OBJECT

	public:
		QVideoInputDevice(QWidget *parent = 0) ;
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
		bool getNextEncodedPacket(RsVOIPDataChunk&) ;

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

		std::list<RsVOIPDataChunk> _out_queue ;
};

