#pragma once
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

#include <QWidget>
#include <QPainter>
#include <QBitmap>
#include "RsAutoUpdatePage.h"

class FileChunksInfo ;
class FileInfo ;

class FileTransferInfoWidget : public RsAutoUpdatePage
{
    Q_OBJECT

public:
    FileTransferInfoWidget(QWidget * parent = 0, Qt::WFlags f = 0 );

	 void setFileHash(const std::string& hash) { _file_hash = hash ; }

	 virtual void updateDisplay() ;	// update from RsAutoUpdateWidget
protected:
	 void draw(const FileInfo& nfo,const FileChunksInfo& details,QPainter *painter) ;

    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *event);

private:
    int x;
    int y;
    int maxWidth;
    int maxHeight;
    QPixmap pixmap;
    QPixmap pixmap2;
    QPixmap downloadedPixmap;
    QPixmap downloadingPixmap;
    QPixmap notDownloadPixmap;
    QPixmap checkingPixmap;

	 std::string _file_hash ;
};

