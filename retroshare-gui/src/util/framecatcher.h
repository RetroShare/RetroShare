#ifndef FILEFUNCTIONS_H
#define FILEFUNCTIONS_H

#include <xine.h>
#include <xine/xineutils.h>

#include <string>


/*!
 * This can be used to retrieve image frames at a specified
 * time from video files using the xine library
 *
 * In the original implementation this class derived from, attributes were declared  static,
 * Possibly a good reason, as using seperate instances
 * of framecatcher can cause a crash if the xine stream is not correctly closed
 */
class framecatcher{

public:

	/*!
	 * video drivers/ports are started up
	 */
	framecatcher();

	/*!
	 * video drivers/ports closed,
	 * current stream is closed if open
	 */
	~framecatcher();

	std::string getCodec();

	int getLength();

	/*!
	 *
	 * @param frame time in msecs you want frame to be taken
	 * @param rgb32BitData 32 bit aligned rgb image format
	 * @param width of the image data returned
	 * @param height of the image data returned
	 */
	int getRGBImage(int frame, unsigned char*& rgb32BitData, int& width, int& height);

	/*!
	 * attaches a xine stream to the file,
	 * if valid user can then request image frames using RGBImage
	 * please ensure you close this stream before establishing a new one!
	 * Not doing may lead to a seg fault, even if you are using a new instance of framecatcher
	 * @see getRGBImage
	 * @see close
	 */
	int open(const std::string& file);

	/*!
	 * closes and disposes current xine stream if open
	 */
	void close();

private:

	xine_t	*xine;
	static xine_stream_t	*stream;
	xine_video_port_t	*vo_port;
	std::string  vo_driver;


	int length; // length of the stream

	void getXineError(int errCode, std::string& errStr);

	void yuy2Toyv12 (uint8_t *, uint8_t *, uint8_t *, uint8_t *, int , int );
	unsigned char * yv12ToRgb (uint8_t *, uint8_t *, uint8_t *, int , int );


};


#endif
