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

#include <QStylePainter>
#include <QDebug>
#include <rsiface/rsfiles.h>
#include "FileTransferInfoWidget.h"

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
	std::cout << "In TaskGraphPainterWidget::updateDisplay()" << std::endl ;

	FileInfo nfo ;
	if(!rsFiles->FileDetails(_file_hash, RS_FILE_HINTS_DOWNLOAD, nfo)) 
		return ;
	FileChunksInfo info ;
	if(!rsFiles->FileChunksDetails(_file_hash, info)) 
		return ;

	std::cout << "got details for file " << nfo.fname << std::endl ;

	uint64_t fileSize = info.file_size;
	uint32_t blockSize = info.chunk_size ;
	int blocks = info.chunks.size() ;

	int columns = maxWidth/13;
	y = blocks/columns*13;
	x = blocks%columns*13;
	maxHeight = y+15;
	pixmap = QPixmap(size());
	pixmap.fill(this, 0, 0);
	pixmap = QPixmap(maxWidth, maxHeight);
	pixmap.fill(this, 0, 0);
	setFixedHeight(maxHeight);

	QPainter painter(&pixmap);
	painter.initFrom(this);

	draw(info,&painter) ;

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

    for (int i=0;i<blocks;i++)
    {
        if (x > maxWidth - 13)
        {
            x = 0;
            y += 13;
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
        x += 13;
    }
    maxHeight = y+15;
}

