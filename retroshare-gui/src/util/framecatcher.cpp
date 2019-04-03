/*******************************************************************************
 * util/framecatcher.cpp                                                       *
 *                                                                             *
 * Copyright (c) 2006 Raul E.                                                  *
 * Copyright (c) 2010 Chris Evi-Parker <retroshare.project@gmail.com>          *
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

#include "framecatcher.h"

#include <iostream>

framecatcher::framecatcher(): xine(NULL), stream(NULL), vo_port(NULL) , length(0){

	// start up drivers
	std::string vo_driver = "auto";
	xine= xine_new();
	xine_init(xine);

	if((vo_port=xine_open_video_driver(xine,vo_driver.c_str(),XINE_VISUAL_TYPE_NONE, NULL))==NULL)
		std::cerr << "framecatcher::framecatcher() " << "ERROR OPENING VIDEO DRIVER";

}

framecatcher::~framecatcher(){

	if(stream != NULL){
		close();
	}

	xine_close_video_driver(xine,vo_port);

}

std::string framecatcher::getCodec(){
	return codec;
}

int framecatcher::getLength(){
	return length;
}

int framecatcher::open(const std::string& fileName){

	int errorCode = 0;


	/* ensure resources used previous stream have been released */
	if(stream != NULL)
		close();


	stream = xine_stream_new(xine,NULL,vo_port);

	if(stream == NULL)
		return 0;


	// open stream
	if(!xine_open(stream,fileName.c_str())){
		errorCode= xine_get_error(stream);
		return errorCode;
	}

	// get length of video file stream is attached to
	if(!xine_get_pos_length(stream,0,0,&length)){
		errorCode = xine_get_error(stream);
		return errorCode;
	}

	xine_play(stream,0,0);
	const char* temp_codec=xine_get_meta_info(stream, XINE_META_INFO_VIDEOCODEC);

	if(temp_codec == NULL)
		return 0;

	codec = temp_codec;
	return 1;
}

void framecatcher::close(){

	if(stream != NULL){

		xine_close(stream);
		xine_dispose(stream);
		stream = NULL;
	}

}


int framecatcher::getRGBImage(int frame, unsigned char*& rgb32BitData, int& width, int& height){

	if(stream == NULL)
		return 0;

	int errCode = 0;
	xine_play(stream,0,0);
	xine_play(stream,0,frame); // 1 frame 2 mseconds

	uint8_t   *yuv = NULL, *y = NULL, *u = NULL, *v =NULL ;
	int ratio = 0, format = 0;

	if (!xine_get_current_frame(stream, &width, &height, &ratio, &format, NULL)){
		errCode= xine_get_error(stream);
		return errCode;
	}

	yuv = new uint8_t[((width+8) * (height+1) * 2)];
	if (yuv == NULL){
		std::cerr << "Out of Memory!";
		return 0;
	}

	if (!xine_get_current_frame(stream, &width, &height, &ratio, &format, yuv)){
		errCode= xine_get_error(stream);

		if(yuv != NULL)
					delete[] yuv;

		return errCode;
	}

	/*
	 * convert to yv12 if necessary
	 */
	switch (format){
		case XINE_IMGFMT_YUY2:
		{
			uint8_t *yuy2 = yuv;

			yuv = new uint8_t[(width * height * 2)];
			if (yuv == NULL){
				std::cerr << "Not enough memory to make screenshot!" << std::endl;
				return 0;
			}

			y = yuv;
			u = yuv + width * height;
			v = yuv + width * height * 5 / 4;

			yuy2Toyv12 (y, u, v, yuy2, width, height);

			delete [] yuy2;
		}
		break;
		case XINE_IMGFMT_YV12:
		y = yuv;
		u = yuv + width * height;
		v = yuv + width * height * 5 / 4;

		break;
		default:
		{
			std::cerr << "Format Not Supported" << std::endl;

			if(yuv != NULL)
				delete [] yuv;

			return 0;
		}
	}

	rgb32BitData = yv12ToRgb (y, u, v, width, height);
	delete [] yuv;
	return 1;
}




void framecatcher::yuy2Toyv12 (uint8_t *y, uint8_t *u, uint8_t *v, uint8_t *input, int width, int height){

	int    i, j, w2;

	w2 = width / 2;

	for (i = 0; i < height; i += 2){
		for (j = 0; j < w2; j++){
			/*
			 * packed YUV 422 is: Y[i] U[i] Y[i+1] V[i]
			 */
			*(y++) = *(input++);
			*(u++) = *(input++);
			*(y++) = *(input++);
			*(v++) = *(input++);
		}

		/*
		 * down sampling
		 */

		for (j = 0; j < w2; j++){
			/*
			 * skip every second line for U and V
			 */
			*(y++) = *(input++);
			input++;
			*(y++) = *(input++);
			input++;
		}
	}
}

unsigned char * framecatcher::yv12ToRgb (uint8_t *src_y, uint8_t *src_u, uint8_t *src_v, int width, int height){
	/*
	 *   Create rgb data from yv12
	 */

#define clip_8_bit(val)              \
{                                    \
   if (val < 0)                      \
      val = 0;                       \
   else                              \
      if (val > 255) val = 255;      \
}

	int     i, j;

	int     y, u, v;
	int     r, g, b;

	int     sub_i_uv;
	int     sub_j_uv;

	int     uv_width, uv_height;

	unsigned char *rgb;

	uv_width  = width / 2;
	uv_height = height / 2;

	rgb = new unsigned char[(width * height * 4)]; //qt needs a 32bit align
	if (!rgb)   //qDebug ("Not enough memory!");
		return NULL;
	

	for (i = 0; i < height; ++i){
		/*
		 * calculate u & v rows
		 */
		sub_i_uv = ((i * uv_height) / height);

		for (j = 0; j < width; ++j){
			/*
			 * calculate u & v columns
			 */
			sub_j_uv = ((j * uv_width) / width);

			/***************************************************
			 *
			 *  Colour conversion from
			 *    http://www.inforamp.net/~poynton/notes/colour_and_gamma/ColorFAQ.html#RTFToC30
			 *
			 *  Thanks to Billy Biggs <vektor@dumbterm.net>
			 *  for the pointer and the following conversion.
			 *
			 *   R' = [ 1.1644         0    1.5960 ]   ([ Y' ]   [  16 ])
			 *   G' = [ 1.1644   -0.3918   -0.8130 ] * ([ Cb ] - [ 128 ])
			 *   B' = [ 1.1644    2.0172         0 ]   ([ Cr ]   [ 128 ])
			 *
			 *  Where in xine the above values are represented as
			 *
			 *   Y' == image->y
			 *   Cb == image->u
			 *   Cr == image->v
			 *
			 ***************************************************/

			y = src_y[(i * width) + j] - 16;
			u = src_u[(sub_i_uv * uv_width) + sub_j_uv] - 128;
			v = src_v[(sub_i_uv * uv_width) + sub_j_uv] - 128;

			r = (int)((1.1644 * (double)y) + (1.5960 * (double)v));
			g = (int)((1.1644 * (double)y) - (0.3918 * (double)u) - (0.8130 * (double)v));
			b = (int)((1.1644 * (double)y) + (2.0172 * (double)u));

			clip_8_bit (r);
			clip_8_bit (g);
			clip_8_bit (b);

			rgb[(i * width + j) * 4 + 0] = b;
			rgb[(i * width + j) * 4 + 1] = g;
			rgb[(i * width + j) * 4 + 2] = r;
			rgb[(i * width + j) * 4 + 3] = 0;

		}
	}

	return rgb;
}

void framecatcher::getError(int errorCode, std::string& errorStr){

	switch (errorCode){
		case XINE_ERROR_NO_INPUT_PLUGIN:
		case XINE_ERROR_NO_DEMUX_PLUGIN: {
			errorStr = "No plugin found to handle this resource";
			break;
		}
		case XINE_ERROR_DEMUX_FAILED: {
			errorStr = "Resource seems to be broken";
			break;
		}
		case XINE_ERROR_MALFORMED_MRL: {
			errorStr = "Requested resource does not exist";
			break;
		}
		case XINE_ERROR_INPUT_FAILED: {
			errorStr = "Resource can not be opened";
			break;
		}
		default: {
			errorStr = "Unknown error";
			break;
		}
	}

	return;
}
