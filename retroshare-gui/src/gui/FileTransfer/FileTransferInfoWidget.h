/*******************************************************************************
 * retroshare-gui/src/gui/FileTransfer/FileTransferInfoWidget.h                *
 *                                                                             *
 * Copyright (c) 2009, defnax        <retroshare.project@gmail.com>            *
 * Copyright (c) 2009, lsn752                                                  *
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

#pragma once

#include <QWidget>
#include <QPainter>
#include <QBitmap>

#include <retroshare-gui/RsAutoUpdatePage.h>
#include <retroshare/rstypes.h>

struct FileChunksInfo ;
struct FileInfo ;

class FileTransferInfoWidget : public RsAutoUpdatePage
{
    Q_OBJECT

public:
    FileTransferInfoWidget(QWidget * parent = 0, Qt::WindowFlags f = 0 );

     void setFileHash(const RsFileHash& hash) { _file_hash = hash ; }

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

     RsFileHash _file_hash ;
};

