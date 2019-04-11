/*******************************************************************************
 * retroshare-gui/src/gui/FileTransfer/xprogressbar.cpp                        *
 *                                                                             *
 * Copyright (c) xEsk (Xesc & Technology 2008)                                 *
 * Copyright (c) 2010 Retroshare Tram           <retroshare.project@gmail.com> *
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

#include <math.h>
#include <retroshare/rstypes.h>
#include "xprogressbar.h"

bool FileProgressInfo::operator<(const FileProgressInfo &other) const
{
	return progress < other.progress;
}

bool FileProgressInfo::operator>(const FileProgressInfo &other) const
{
	return progress > other.progress;
}

xProgressBar::xProgressBar(const FileProgressInfo& pinfo,QRect rect, QPainter *painter, int schemaIndex)
	: _pinfo(pinfo)
{
	// assign internal data
	this->schemaIndex = schemaIndex;
	this->rect = rect;
	this->painter = painter;
	displayText = false;
	// set the progress bar colors
	setColor();
	// configure span
	vSpan = 0;
	hSpan = 0;
	// text color
	textColor = QColor("black");
}

void xProgressBar::setColor()
{
	/* TEMPORAL SCHEMA DEFINITION */
	
	switch (schemaIndex)
	{
		/* blue schema */
		case 0:
			// background
			backgroundBorderColor.setRgb(143, 180, 219);
			backgroundColor.setRgb(198, 209, 221);
			// progress
			gradBorderColor.setRgb(35, 96, 167);
			gradColor1.setRgb(100, 136, 252);
			gradColor2.setRgb(165, 183, 240);
			// ok
			break;

		/* green schema */
		case 1:
			// background
			backgroundBorderColor.setRgb(53, 194, 26);
			backgroundColor.setRgb(176, 214, 93);
			// progress
			gradBorderColor.setRgb(8, 77, 16);
			gradColor1.setRgb(0, 137, 16);
			gradColor2.setRgb(78, 194, 81);
			// ok
			break;
			
		/* red schema */
		case 2:
			// background
			backgroundBorderColor.setRgb(255, 62, 62);
			backgroundColor.setRgb(248, 175, 175);
			// progress
			gradBorderColor.setRgb(151, 0, 0);
			gradColor1.setRgb(251, 54, 54);
			gradColor2.setRgb(246, 118, 118);
			// ok
			break;
			
		/* gray schema */
		case 3:
			// background
			backgroundBorderColor.setRgb(116, 177, 160);
			backgroundColor.setRgb(178, 215, 205);
			// progress
			gradBorderColor.setRgb(106, 106, 106);
			gradColor1.setRgb(168, 168, 168);
			gradColor2.setRgb(197, 197, 197);
			// ok
			break;
			
		/* yellow schema */
		case 4:
			// background
			backgroundBorderColor.setRgb(227, 204, 79);
			backgroundColor.setRgb(255, 236, 130);
			// progress
			gradBorderColor.setRgb(215, 182, 0);
			gradColor1.setRgb(233, 197, 0);
			gradColor2.setRgb(255, 236, 130);
			// ok
			break;
			
		/* black schema */
		case 5:
			// background
			backgroundBorderColor.setRgb(99, 99, 99);
			backgroundColor.setRgb(134, 134, 134);
			// progress
			gradBorderColor.setRgb(0, 0, 0);
			gradColor1.setRgb(38, 38, 38);
			gradColor2.setRgb(113, 113, 113);
			// ok
			break;
			
		/* purple schema */
		case 6:
			// background
			backgroundBorderColor.setRgb(234, 127, 223);
			backgroundColor.setRgb(255, 164, 246);
			// progress
			gradBorderColor.setRgb(150, 0, 134);
			gradColor1.setRgb(218, 0, 195);
			gradColor2.setRgb(255, 121, 241);
			// ok
			break;
			
		/* maroon schema */
		case 7:
			// background
			backgroundBorderColor.setRgb(255, 174, 49);
			backgroundColor.setRgb(255, 204, 132);
			// progress
			gradBorderColor.setRgb(159, 94, 0);
			gradColor1.setRgb(223, 134, 6);
			gradColor2.setRgb(248, 170, 59);
			// ok
			break;
			
		/* clean schema */
		case 8:
			// background
			backgroundBorderColor.setRgb(143, 180, 219);
			backgroundColor.setRgb(198, 209, 221);
			// progress
			gradBorderColor.setRgb(209, 128, 24);
			gradColor1.setRgb(246, 199, 138);
			gradColor2.setRgb(255, 227, 190);
			// ok
			break;

		/* light gray */
		case 9:
			// background
			backgroundBorderColor.setRgb(194, 194, 194);
			backgroundColor.setRgb(232, 233, 233);
			// progress
			gradBorderColor.setRgb(176, 176, 176);
			gradColor1.setRgb(201, 201, 201);
			gradColor2.setRgb(223, 223, 223);
			// set text color (white is not a good option)
			textColor = QColor(58, 58, 58);
			// ok
			break;	
	}
}

void xProgressBar::overPaintSelectedChunks(const std::vector<uint32_t>& chunks,const QColor& gradColor_a1,const QColor& gradColor_a2, int width,uint32_t ss) const
{
	QLinearGradient linearGrad(rect.x(), rect.y(), rect.x(), rect.y() + rect.height() - 1);

	linearGrad.setColorAt(0.00, gradColor_a1);
	linearGrad.setColorAt(0.16, gradColor_a2);
	linearGrad.setColorAt(1.00, gradColor_a1);

	painter->setBrush(linearGrad);

	if(chunks.empty())
		return ;

	int last_i = chunks[0] ;

	for(uint32_t i=1;i<chunks.size();++i)
		if(chunks[i] > chunks[i-1]+1)
		{
			int nb_consecutive_chunks = chunks[i-1] - last_i + 1 ;

			painter->drawRect(rect.x() + hSpan+(int)rint(last_i*width/(float)ss), rect.y() + vSpan, (int)ceil(nb_consecutive_chunks*width/(float)ss), rect.height() - 1 - vSpan * 2);
			last_i = chunks[i] ;
		}
	int nb_consecutive_chunks = chunks.back() - last_i + 1 ;
	painter->drawRect(rect.x() + hSpan+(int)rint(last_i*width/(float)ss), rect.y() + vSpan, (int)ceil(nb_consecutive_chunks*width/(float)ss), rect.height() - 1 - vSpan * 2);
}

void xProgressBar::paint()
{
	// paint the progressBar background
	painter->setBrush(backgroundColor);
	painter->setPen(backgroundBorderColor);
	painter->drawRect(rect.x() + hSpan, rect.y() + vSpan, rect.width() - 1 - hSpan, rect.height() - 1 - vSpan * 2);

	// define gradient
	QLinearGradient linearGrad(rect.x(), rect.y(), rect.x(), rect.y() + rect.height() - 1);

	linearGrad.setColorAt(0.00, gradColor1);
	linearGrad.setColorAt(0.16, gradColor2);
	linearGrad.setColorAt(1.00, gradColor1);
	painter->setPen(gradBorderColor);

	uint32_t width = static_cast<uint32_t>(rect.width()-1-2*hSpan) ;

	painter->setBrush(linearGrad);

	uint32_t ss = _pinfo.nb_chunks ;

	QRect bounding = painter->boundingRect(rect, Qt::AlignCenter, QLocale().toString(_pinfo.progress, 'f', 2) + "%");

#ifdef TO_BE_REMOVED
	if((ss > 1) && (rect.width() > (1.5*bounding.width())))	// for small files we use a more progressive display
	{
#endif
		if(!_pinfo.cmap._map.empty())
		{
			if (ss > width)
				for(uint32_t i=0;i<width;++i)
				{
					int first_chunk_in_pixel = (int)rint(ss*i/width);
					int last_chunk_in_pixel = (int)rint(ss*(i+1)/width)-1; 
					
					if (last_chunk_in_pixel<first_chunk_in_pixel)
						last_chunk_in_pixel=first_chunk_in_pixel;

					int c = 0;
					bool all_done = true; // will be set to false when the first incomplete chunk is encountered
					int j;

					//loop through all chunks represented by the pixel at position i and count the completed chunks
					//if no incomplete chunk is encountered in this range, continue to count the row of completed chunks
					//to see whether more than 1 pixel can be drawn at once.
					for(j=first_chunk_in_pixel;((j<=last_chunk_in_pixel) || (all_done && (j<(int)ss)));++j)
						if (_pinfo.cmap[j])
							++c;
						else
							all_done = false;
					//after the loop has completed, we either need c (for opacity) or j (for width), because either:
					// I: if the first range was completely completed, j > last_chunk_in_pixel
					//II: if not, c < last_chunk_in_pixel-first_chunk_in_pixel

					if (c==0) // not even a single chunk found
						continue;

					//we now check for case I.
					//if the loop above has checked more than 1 pixel, increment i as long as j is in its range
					int x0 = i;
					while (j >= rint(ss*(i+2)/width)-1)
						++i;

					//case  I.: c >= last_chunk_in_pixel - first_chunk_in_pixel    ===> o  = 1
					//case II.: c <= last_chunk_in_pixel - first_chunk_in_pixel    ===> o <= 1
					float o = std::min(1.0f,(float)c / (last_chunk_in_pixel - first_chunk_in_pixel + 1));
					
					if (o>0)
					{
						painter->setOpacity(o);
						painter->drawRect(rect.x() + hSpan + x0, rect.y() + vSpan, i - x0 + 1, rect.height() - 1 - vSpan * 2);
					}
				}
			else
			for(uint32_t i=0;i<ss;++i)
			{
				uint32_t j=0 ;
				while(i+j<ss && _pinfo.cmap[i+j])
					++j ;

				float o = std::min(1.0f,j/(float)ss*width) ;

				if(j>0 && o >= 1.0f)	// limits the number of regions drawn
				{
					painter->setOpacity(o) ;
					painter->drawRect(rect.x() + hSpan+(int)rint(i*width/(float)ss), rect.y() + vSpan, (int)ceil(j*width/(float)ss), rect.height() - 1 - vSpan * 2);
				}

				i += j ;
			}
		}

		overPaintSelectedChunks( _pinfo.chunks_in_progress , QColor(170, 20,9), QColor(223,121,123), width,ss) ;
		overPaintSelectedChunks( _pinfo.chunks_in_checking , QColor(186,143,0), QColor(223,196, 61), width,ss) ;
#ifdef TO_BE_REMOVED
	}
	else if ((rect.width() < bounding.width()) || !displayText)
	{
		// calculate progress value
		int preWidth = static_cast<int>((rect.width() - 1 - hSpan)*(_pinfo.progress/100.0f));
		int progressWidth = rect.width() - preWidth;
		if (progressWidth == rect.width() - hSpan) return;

		// paint the progress
		painter->setBrush(linearGrad);
		painter->drawRect(rect.x() + hSpan, rect.y() + vSpan, rect.width() - progressWidth - hSpan, rect.height() - 1 - vSpan * 2);
	}
#endif
	painter->setOpacity(1.0f) ;

	
	// paint text?
	if (displayText && (rect.width() >= bounding.width()))
	{
		QColor colorInt (255-textColor.red(), 255-textColor.green(), 255-textColor.blue(), 127);
		QColor colorBor (255-textColor.red(), 255-textColor.green(), 255-textColor.blue(), 63);
		QRadialGradient radialGrad(bounding.x()+(bounding.width()/2), bounding.y()+(bounding.height()/2),bounding.width()/2);
		radialGrad.setColorAt(0.0, colorInt);
		radialGrad.setColorAt(1.0, colorBor);
		radialGrad.setSpread(QGradient::ReflectSpread);
		painter->setPen(colorBor);
		painter->setBrush(radialGrad);
		painter->drawRoundedRect(bounding.adjusted(-2,2,2,-3),4.0,4.0);

		painter->setPen(textColor);
		painter->drawText(rect, Qt::AlignCenter, QLocale().toString(_pinfo.progress, 'f', 2) + "%");
	}

	backgroundColor.setRgb(255, 255, 255);
	painter->setBrush(backgroundColor);
	backgroundBorderColor.setRgb(0, 0, 0);
	painter->setPen(backgroundBorderColor);
}

void xProgressBar::setColorSchema(const int value)
{
	schemaIndex = value;
	// set the progress bar colors
	setColor();
}

void xProgressBar::setDisplayText(const bool display)
{
	displayText = display;
}

void xProgressBar::setVerticalSpan(const int value)
{
	vSpan = value;
}

void xProgressBar::setHorizontalSpan(const int value)
{
	hSpan = value;
}
