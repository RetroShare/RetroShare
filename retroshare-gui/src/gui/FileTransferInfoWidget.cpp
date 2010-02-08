/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2009, defnax
 * Copyright (c) 2009, lsn752 
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <math.h>
#include <QStylePainter>
#include <QDebug>
#include <rsiface/rsfiles.h>
#include <rsiface/rstypes.h>
#include "FileTransferInfoWidget.h"

// Variables to decide of display behaviour. Should be adapted to window size.
//
static const int chunk_square_size       = 13 ;
static const int text_height             = 10 ;	// should be set according to the font size
static const int block_sep               = 3 ;	// separator between blocks
static const int ch_num_size             = 50 ;	// size of field for chunk number
static const int availability_map_size_X = 400 ;// length of availability bar
static const int availability_map_size_Y = 20 ;	// height of availability bar
static const int tab_size                = 200 ;// size between tabulated entries

FileTransferInfoWidget::FileTransferInfoWidget(QWidget * parent, Qt::WFlags f )
{
	QRect TaskGraphRect = geometry();
	maxWidth = TaskGraphRect.width();
	pixmap = QPixmap(size());
	pixmap.fill(this, 0, 0);

	downloadedPixmap.load(":images/graph-downloaded.png");
	downloadingPixmap.load(":images/graph-downloading.png");
	notDownloadPixmap.load(":images/graph-notdownload.png");

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void FileTransferInfoWidget::resizeEvent(QResizeEvent *event)
{
    QRect TaskGraphRect = geometry();
    maxWidth = TaskGraphRect.width();

    updateDisplay();
}
void FileTransferInfoWidget::updateDisplay()
{
        //std::cout << "In TaskGraphPainterWidget::updateDisplay()" << std::endl ;

	bool ok=true ;
	FileInfo nfo ;
	if(!rsFiles->FileDetails(_file_hash, RS_FILE_HINTS_DOWNLOAD, nfo)) 
		ok = false ;
	FileChunksInfo info ;
	if(!rsFiles->FileDownloadChunksDetails(_file_hash, info)) 
		ok = false ;

        //std::cout << "got details for file " << nfo.fname << std::endl ;

	pixmap = QPixmap(size());
	pixmap.fill(this, 0, 0);
	pixmap = QPixmap(maxWidth, maxHeight);
	pixmap.fill(this, 0, 0);
	setFixedHeight(maxHeight);

	QPainter painter(&pixmap);
	painter.initFrom(this);

	if(ok)
	{
		int blocks = info.chunks.size() ;
		int columns = maxWidth/chunk_square_size;
		y = blocks/columns*chunk_square_size;
		x = blocks%columns*chunk_square_size;
		maxHeight = y+150+info.active_chunks.size()*(block_sep+text_height);	// warning: this should be computed from the different size parameter and the number of objects drawn, otherwise the last objects to be displayed will be truncated.

		draw(info,&painter) ;
	}

	pixmap2 = pixmap;
}

void FileTransferInfoWidget::paintEvent(QPaintEvent *event)
{
	std::cout << "In paint event" << std::endl ;
    QStylePainter painter(this);

    painter.drawPixmap(0, 0, pixmap2);
    pixmap = pixmap2;
}

void FileTransferInfoWidget::draw(const FileChunksInfo& info,QPainter *painter)
{
    x=0;
    y=0;
    int blocks = info.chunks.size() ;
	 uint64_t fileSize = info.file_size ;
	 uint32_t blockSize = info.chunk_size ;

    if (fileSize%blockSize == 0) blocks--;
    QRectF source(0.0, 0.0, 12.0, 12.0);

	 painter->setPen(QColor::fromRgb(0,0,0)) ;
	 y += text_height ;
	 painter->drawText(0,y,tr("Chunk map:")) ;
	 y += block_sep ;

	 // draw the chunk map
	 //
    for (int i=0;i<blocks;i++)
    {
        if (x > maxWidth - chunk_square_size)
        {
            x = 0;
            y += chunk_square_size;
        }
        QRectF target(x, y, 12.0, 12.0);

		  switch(info.chunks[i])
		  {
			  case FileChunksInfo::CHUNK_DONE: 			painter->drawPixmap(target, downloadedPixmap, source);
																	break ;

			  case FileChunksInfo::CHUNK_ACTIVE: 		painter->drawPixmap(target, downloadingPixmap, source);
																	break ;

			  case FileChunksInfo::CHUNK_OUTSTANDING: painter->drawPixmap(target, notDownloadPixmap, source);
																	break ;
			  default: ;
		  }
        x += chunk_square_size;
    }
	 y += chunk_square_size ;

	 // draw the currently downloaded chunks
	 //
	 painter->setPen(QColor::fromRgb(70,70,70)) ;
	 painter->drawLine(0,y,maxWidth,y) ;

	 uint32_t sizeX = 100 ;
	 uint32_t sizeY = 10 ;
	 y += block_sep ;
	 y += text_height ;
	 painter->setPen(QColor::fromRgb(0,0,0)) ;
	 painter->drawText(0,y,tr("Active chunks:")) ;
	 y += block_sep ;

	 for(uint i=0;i<info.active_chunks.size();++i)
	 {
		 painter->setPen(QColor::fromRgb(0,0,0)) ;
		 painter->drawText(5,y+text_height,QString::number(info.active_chunks[i].first)) ;

		 uint32_t s = (uint32_t)rint(sizeX*(info.chunk_size - info.active_chunks[i].second)/(float)info.chunk_size) ;

		 painter->fillRect(ch_num_size,y,s,sizeY,QColor::fromHsv(200,200,255)) ;
		 painter->fillRect(ch_num_size+s,y,sizeX-s,sizeY,QColor::fromHsv(200,50,255)) ;

		 painter->setPen(QColor::fromRgb(0,0,0)) ;
		 float percent = (info.chunk_size - info.active_chunks[i].second)*100.0/info.chunk_size ;

		 painter->drawText(sizeX+55,y+text_height,QString::number(percent,'g',2) + " %") ;

		 y += sizeY+block_sep ;
	 }

	 // draw the availability map
	 //
	 painter->setPen(QColor::fromRgb(70,70,70)) ;
	 painter->drawLine(0,y,maxWidth,y) ;

	 y += block_sep ;
	 y += text_height ;
	 painter->setPen(QColor::fromRgb(0,0,0)) ;
	 painter->drawText(0,y,tr("Availability map (")+QString::number(info.compressed_peer_availability_maps.size())+ tr(" sources")+")") ;
	 y += block_sep ;

	 // Note (for non geeks): the !! operator transforms anything positive into 1 and 0 into 0.
	 //
	 int nb_chunks = info.file_size/info.chunk_size + !!(info.file_size % info.chunk_size);

	 for(uint i=0;i<availability_map_size_X;++i)
	 {
		 int nb_src = 0 ;
		 int chunk_num = (int)floor(i/float(availability_map_size_X)*(nb_chunks-1)) ;

		 for(std::map<std::string,CompressedChunkMap>::const_iterator it(info.compressed_peer_availability_maps.begin());it!=info.compressed_peer_availability_maps.end();++it)
			 nb_src += it->second[chunk_num] ;

		 painter->setPen(QColor::fromHsv(200,50*nb_src,200)) ; // the more sources, the more saturated
		 painter->drawLine(i,y,i,y+availability_map_size_Y) ;
	 }
	 
	 y += block_sep + availability_map_size_Y ;
	 painter->setPen(QColor::fromRgb(70,70,70)) ;
	 painter->drawLine(0,y,maxWidth,y) ;
	 y += block_sep ;

	 // various info:
	 //

	 painter->setPen(QColor::fromRgb(0,0,0)) ;
	 y += text_height ; painter->drawText(0,y,tr("File info:")) ;
	 y += block_sep ;
	 y += text_height ; painter->drawText(20,y,tr("File size: ")) ; painter->drawText(tab_size,y,QString::number(info.file_size)) ;
	 y += block_sep ;
	 y += text_height ; painter->drawText(20,y,tr("Chunk size: ")) ; painter->drawText(tab_size,y,QString::number(info.chunk_size)) ;
	 y += block_sep ;
	 y += text_height ; painter->drawText(20,y,tr("Number of chunks: ")) ; painter->drawText(tab_size,y,QString::number(info.chunks.size())) ;
	 y += block_sep ;
	 y += text_height ; painter->drawText(20,y,tr("Number of sources: ")) ; painter->drawText(tab_size,y,QString::number(info.compressed_peer_availability_maps.size())) ;
	 y += block_sep ;
	 y += text_height ; painter->drawText(20,y,tr("Chunk strategy: ")) ; painter->drawText(tab_size,y,(info.strategy==FileChunksInfo::CHUNK_STRATEGY_RANDOM)?"Random":"Streaming") ;
	 y += block_sep ;
	 y += text_height ; painter->drawText(20,y,tr("Transfer type: ")) ; 
	 if(info.flags & RS_FILE_HINTS_NETWORK_WIDE) painter->drawText(tab_size,y,"Anonymous F2F") ;
	 if(info.flags & RS_FILE_HINTS_ASSUME_AVAILABILITY) painter->drawText(tab_size,y,"Direct friend transfer / Availability assumed") ;
	 y += text_height ;
	 y += block_sep ;

    maxHeight = y+15;
}


