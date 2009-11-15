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

#ifndef __TASKGRAPHPAINTERWIDGET_H__
#define __TASKGRAPHPAINTERWIDGET_H__

#include <QWidget>
#include <QPainter>
#include <QBitmap>

class TaskGraphPainterWidget : public QWidget
{
    Q_OBJECT

public:
    TaskGraphPainterWidget(QWidget * parent = 0, Qt::WFlags f = 0 );

    void setData( qint64 fileSize, qint64 blockSize );
    void setBlockSizeData( qint64 blockSize );
    void setNotDownloadListClear();
    void setNotDownloadList( int taskThreadListId, qint64 startPosition, qint64 endPosition);
    void newReceivedListClear();
    void setNewReceived(int taskThreadListId, QList <qint64> newReceivedList);
    void refreshAll();
    void refreshPixmap();
    void refreshThreadLastBlock(int newThreadReceivedListId);

    qint64 fileSize;
    qint64 blockSize;

protected:
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *event);

private:
    void drawNotDownload(QPainter *painter);
    void drawDownloaded(QPainter *painter);
    void drawNoSizeFile(QPainter *painter);
    void drawNewReceivedData(QPainter *painter);
    void drawThreadLastBlock(QPainter *painter, int newThreadReceivedListId);


    int x;
    int y;
    int maxWidth;
    int maxHeight;
    QPixmap pixmap;
    QPixmap pixmap2;
    QPixmap downloadedPixmap;
    QPixmap downloadingPixmap;
    QPixmap notDownloadPixmap;
    int taskThreadListId;
    struct _NotDownload
    {
        int taskThreadListId;
        qint64 startPosition;
        qint64 endPosition;
    };
    typedef struct _NotDownload NotDownload;
    QList <NotDownload> notDownloadList;
    struct _NewReceived
    {
        int taskThreadListId;
        QList <qint64> newThreadReceivedList;
    };
    typedef struct _NewReceived NewReceived;
    QList <NewReceived> newTaskReceivedList;
};
#endif // __TASKGRAPHPAINTERWIDGET_H__
