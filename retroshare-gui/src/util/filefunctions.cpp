/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2006 Raul E.
 * Copyright (c) 2010 Christopher Evi-Parker
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *************************************************************************/


#include "filefunctions.h"

QString  filefunctions::cfilename=NULL;
QTreeWidget *filefunctions::treeObj=NULL;
QStringList filefunctions::tempimages;
QString filefunctions::codec=NULL;
int filefunctions::length=0;
int  filefunctions::fwidth=0;
int  filefunctions::fheight=0;

filefunctions::filefunctions(){

// clear tempfiles
for ( int i=0; i<filefunctions::tempimages.size();i++){
  QFile::remove (filefunctions::tempimages.at(i));
  }
filefunctions::tempimages.clear();
}

filefunctions::~filefunctions(){

}

QString filefunctions::getCodec(){
return filefunctions::codec;
}
QString filefunctions::getImageWidth(){
return QString::number(filefunctions::fwidth);
}
QString filefunctions::getImageHeight(){
return QString::number(filefunctions::fheight);
}
int filefunctions::getLength(){ //ms
return filefunctions::length;
}

void filefunctions::open(){
	vo_driver="auto";
	//ao_driver="auto";
	tempdir=QDir::tempPath ();
	xine= xine_new();
	xine_init(xine);

	if((vo_port=xine_open_video_driver(xine,vo_driver,XINE_VISUAL_TYPE_NONE, NULL))==NULL)	
	     qWarning("ERROR - OPEN VIDEO DRIVER");
	//ao_port=xine_open_audio_driver(xine,ao_driver,NULL);
	//stream = xine_stream_new(xine,ao_port,vo_port);
	stream = xine_stream_new(xine,NULL,vo_port);

if(!xine_open(stream,cfilename.toLocal8Bit())){
const int errorCode= xine_get_error(stream);
QMessageBox::critical(0,"QFrameCatcher","ERROR.\n" + getXineError(errorCode));
return ;
} 
if(xine_get_pos_length(stream,0,0,&length)){
//std::cout << length<< std::endl;
}
xine_play(stream,0,0);


const uint8_t option= 1;

switch (option){
	case 1 : 
		getFrames(); // Capture x Frames
	break;
	case 2 :
		getFramesTime(); // Capture every x seconds
	break;
	default :
		getFrames();
	}

xine_close(stream);
xine_dispose(stream);
xine_close_video_driver(xine,vo_port);
//xine_close_audio_driver(xine,ao_port);
}
void filefunctions::saveRGBImage(uchar *rgb32BitData,int pos,int width,int height){

currImage = new QImage(rgb32BitData, width, height,QImage::Format_RGB32);

//delete rgb32BitData;

}
void filefunctions::savebuffer(QByteArray &ba , QImage *image){
QBuffer buffer(&ba);
buffer.open(QIODevice::WriteOnly);
image->save(&buffer, "PNG");
}
bool filefunctions::saveimage(const QByteArray &ba , const QString &filename){
bool correct=true;
QFile image(filename);
if(!image.open(QIODevice::WriteOnly) )
return false;

if(image.write(ba)==-1)
correct=false;

image.close();
return correct;
}
void filefunctions::getFramesTime(){
xine_play(stream,0,0);
codec=xine_get_meta_info(stream, XINE_META_INFO_VIDEOCODEC);

const uint32_t chunk= (30) * 1000;  // mseconds


// Progress Dialog
QString progresdialogtext =QApplication::translate("xine","Generating Thumbnails...",0,QApplication::UnicodeUTF8);
QString cancelbuttontext=QApplication::translate("xine","&Cancel",0,QApplication::UnicodeUTF8);

progress = new QProgressDialog(progresdialogtext , cancelbuttontext , 0, length,0,Qt::WindowSystemMenuHint);


for (int32_t frame=chunk ; frame<length ; frame=frame+chunk){
xine_play(stream,0,frame); // 1 frame 2 mseconds
SaveFrame(frame);
progress->setValue(frame);
qApp->processEvents();
if(progress->wasCanceled())
	break;
}
progress->setValue(length);
delete progress;
}
void filefunctions::getFrames(){
xine_play(stream,0,0);
codec=xine_get_meta_info(stream, XINE_META_INFO_VIDEOCODEC);

int frame = 20;

xine_play(stream,0,frame); // 1 frame 2 mseconds
SaveFrame(frame);

}

void filefunctions::yuy2Toyv12 (uint8_t *y, uint8_t *u, uint8_t *v, uint8_t *input, int width, int height){

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
uchar * filefunctions::yv12ToRgb (uint8_t *src_y, uint8_t *src_u, uint8_t *src_v, int width, int height){
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

	uchar *rgb;

	uv_width  = width / 2;
	uv_height = height / 2;

	rgb = new uchar[(width * height * 4)]; //qt needs a 32bit align
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
void filefunctions::SaveFrame(int pos){

     uint8_t   *yuv = NULL, *y = NULL, *u = NULL, *v =NULL ;

	int        width, height, ratio, format;

	// double     desired_ratio, image_ratio;

	if (!xine_get_current_frame(stream, &width, &height, &ratio, &format, NULL))
		return;

	yuv = new uint8_t[((width+8) * (height+1) * 2)];
	if (yuv == NULL){
		qWarning("Not enough memory to make screenshot!");
		return;
	}

	xine_get_current_frame(stream, &width, &height, &ratio, &format, yuv);
	fwidth=width;
	fheight=height;

	/*
	 * convert to yv12 if necessary
	 */
	switch (format){
		case XINE_IMGFMT_YUY2:
		{
			uint8_t *yuy2 = yuv;

			yuv = new uint8_t[(width * height * 2)];
			if (yuv == NULL){
				qWarning("Not enough memory to make screenshot!");
				return;
			}
			y = yuv;
			u = yuv + width * height;
			v = yuv + width * height * 5 / 4;

			yuy2Toyv12 (y, u, v, yuy2, width, height);

			delete [] yuy2;
			//free (yuy2);
		}
		break;
		case XINE_IMGFMT_YV12:
		y = yuv;
		u = yuv + width * height;
		v = yuv + width * height * 5 / 4;

		break;
		default:
		{
			qWarning("Screenshot: Format %s not supported!",(char *) &format);
			delete [] yuv;
			return;
		}
}
uchar* rgb32BitData = yv12ToRgb (y, u, v, width, height);
delete [] yuv; 
saveRGBImage(rgb32BitData,pos,width,height);
}
QTime filefunctions::postime(const int itime){
	QTime time(0,0,0);
	time=time.addMSecs(itime);
	return time;
}
QString filefunctions::getXineError(int errorCode){
QString error;
switch (errorCode){
		case XINE_ERROR_NO_INPUT_PLUGIN:
		case XINE_ERROR_NO_DEMUX_PLUGIN: {
			error = QApplication::translate("xine","No plugin found to handle this resource",0,QApplication::UnicodeUTF8);
			break;
		}
		case XINE_ERROR_DEMUX_FAILED: {
			error = QApplication::translate("xine","Resource seems to be broken",0,QApplication::UnicodeUTF8);
			break;
		}
		case XINE_ERROR_MALFORMED_MRL: {
			error = QApplication::translate("xine","Requested resource does not exist",0,QApplication::UnicodeUTF8);
			break;
		}
		case XINE_ERROR_INPUT_FAILED: {
			error = QApplication::translate("xine","Resource can not be opened",0,QApplication::UnicodeUTF8);
			break;
		}
		default: {
			error = QApplication::translate("xine","Unknown error",0,QApplication::UnicodeUTF8);
			break;
		}
	}
return error;
}
