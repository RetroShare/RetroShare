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
#include "taskGraphPainterWidget.h"

TaskGraphPainterWidget::TaskGraphPainterWidget(QWidget * parent, Qt::WFlags f )
{
    this->fileSize = 0;
    this->blockSize = 100*1024;
    notDownloadList.clear();

    QRect TaskGraphRect = geometry();
    maxWidth = TaskGraphRect.width();
    pixmap = QPixmap(size());
    pixmap.fill(this, 0, 0);

    downloadedPixmap.load(":images/graph-downloaded.png");
    downloadingPixmap.load(":images/graph-downloading.png");
    notDownloadPixmap.load(":images/graph-notdownload.png");

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
void TaskGraphPainterWidget::setData( qint64 fileSize, qint64 blockSize )
{
    this->fileSize = fileSize;
    this->blockSize = blockSize;

    int blocks = fileSize/blockSize;
    if (fileSize%blockSize == 0) blocks--;
#ifdef WIN32
    if (blocks>=5000)
    {
        this->blockSize=this->fileSize/5000;
    }
#else
    if (blocks>=3000)
    {
        this->blockSize=this->fileSize/3000;
    }
#endif
}
void TaskGraphPainterWidget::setBlockSizeData( qint64 blockSize )
{
    this->blockSize = blockSize;

    int blocks = fileSize/blockSize;
    if (fileSize%blockSize == 0) blocks--;
#ifdef WIN32
    if (blocks>=5000)
    {
        this->blockSize=this->fileSize/5000;
    }
#else
    if (blocks>=3000)
    {
        this->blockSize=this->fileSize/3000;
    }
#endif
    refreshAll();
}
void TaskGraphPainterWidget::setNotDownloadListClear()
{
    notDownloadList.clear();
}
void TaskGraphPainterWidget::setNotDownloadList( int taskThreadListId, qint64 startPosition, qint64 endPosition)
{
    NotDownload notDownload;
    notDownload.taskThreadListId = taskThreadListId;
    notDownload.startPosition = startPosition;
    notDownload.endPosition = endPosition;
    notDownloadList.append(notDownload);
}
void TaskGraphPainterWidget::newReceivedListClear()
{
    newTaskReceivedList.clear();
}
void TaskGraphPainterWidget::setNewReceived(int taskThreadListId, QList <qint64> newReceivedList)
{
    NewReceived newReceived;
    newReceived.taskThreadListId = taskThreadListId;
    newReceived.newThreadReceivedList = newReceivedList;
    newTaskReceivedList.append(newReceived);
}
void TaskGraphPainterWidget::resizeEvent(QResizeEvent *event)
{
    QRect TaskGraphRect = geometry();
    maxWidth = TaskGraphRect.width();

    refreshAll();
}
void TaskGraphPainterWidget::refreshAll()
{
    int blocks = fileSize/blockSize;
    if (fileSize%blockSize >0) blocks++;
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
    if (fileSize>0)
    {
        drawDownloaded(&painter);
        drawNotDownload(&painter);
    }
    else
    {
        drawNoSizeFile(&painter);
    }

    pixmap2 = pixmap;
    update();
}
void TaskGraphPainterWidget::refreshPixmap()
{
    pixmap2 = pixmap;
    QPainter painter(&pixmap2);

    drawNewReceivedData(&painter);

    update();
}
void TaskGraphPainterWidget::refreshThreadLastBlock(int newThreadReceivedListId)
{
    pixmap2 = pixmap;
    QPainter painter(&pixmap2);

    drawThreadLastBlock(&painter, newThreadReceivedListId);

    repaint ();
}
void TaskGraphPainterWidget::paintEvent(QPaintEvent *event)
{
    QStylePainter painter(this);

    painter.drawPixmap(0, 0, pixmap2);
    pixmap = pixmap2;
}
void TaskGraphPainterWidget::drawNotDownload(QPainter *painter)
{
    if (this->fileSize == 0) return;
    int columns = maxWidth/13;
    int blocks = fileSize/blockSize;
    if (fileSize%blockSize == 0) blocks--;

    QRectF source(0.0, 0.0, 12.0, 12.0);
    for (int taskThreadListId=0;taskThreadListId<notDownloadList.size();taskThreadListId++)
    {
        qint64 startPosition = notDownloadList.at(taskThreadListId).startPosition;
        qint64 endPosition = notDownloadList.at(taskThreadListId).endPosition;
        int startPositionBlocks = startPosition/blockSize;
        if (startPosition%blockSize > 0) startPositionBlocks++;
        int endPositionBlocks = endPosition/blockSize;
        if (endPositionBlocks != blocks) endPositionBlocks--;

        for (int block=startPositionBlocks;block<=endPositionBlocks;block++)
        {
            y = block/columns*13;
            x = block%columns*13;
            QRectF target(x, y, 12.0, 12.0);
            painter->drawPixmap(target, notDownloadPixmap, source);
        }
    }
}
void TaskGraphPainterWidget::drawDownloaded(QPainter *painter)
{
    x=0;
    y=0;
    int blocks = fileSize/blockSize;
    if (fileSize%blockSize == 0) blocks--;
    QRectF source(0.0, 0.0, 12.0, 12.0);

    for (int i=0;i<=blocks;i++)
    {
        if (x > maxWidth - 13)
        {
            x = 0;
            y += 13;
        }
        QRectF target(x, y, 12.0, 12.0);
        painter->drawPixmap(target, downloadedPixmap, source);
        x += 13;
    }
    maxHeight = y+15;
}
void TaskGraphPainterWidget::drawNoSizeFile(QPainter *painter)
{
    x=0;
    y=0;
    QList <qint64> newThreadReceivedList;
    if (newTaskReceivedList.isEmpty()) return;
    newThreadReceivedList = newTaskReceivedList.at(0).newThreadReceivedList;
    if (newThreadReceivedList.isEmpty()) return;
    qint64 startPosition = newThreadReceivedList.at(newThreadReceivedList.size()-1);
    int blocks = startPosition/blockSize;
    if (fileSize%blockSize == 0) blocks--;
    QRectF source(0.0, 0.0, 12.0, 12.0);

    for (int i=0;i<=blocks;i++)
    {
        if (x > maxWidth - 13)
        {
            x = 0;
            y += 13;
        }
        QRectF target(x, y, 12.0, 12.0);
        painter->drawPixmap(target, downloadedPixmap, source);
        x += 13;
    }

    maxHeight = y+15;
}
void TaskGraphPainterWidget::drawNewReceivedData(QPainter *painter)
{
    for (int newTaskReceivedListId=0;newTaskReceivedListId<newTaskReceivedList.size();newTaskReceivedListId++)
    {
        QList <qint64> newThreadReceivedList;
        newThreadReceivedList = newTaskReceivedList.at(newTaskReceivedListId).newThreadReceivedList;
        if (newThreadReceivedList.size() < 1) continue;

        int columns = maxWidth/13;
        QRectF source(0.0, 0.0, 12.0, 12.0);

        qint64 startPosition = newThreadReceivedList.at(0);
        qint64 endPosition = newThreadReceivedList.at(newThreadReceivedList.size()-1);
        int startPositionBlocks = startPosition/blockSize;
        int endPositionBlocks = endPosition/blockSize;
        if (endPosition%blockSize == 0) endPositionBlocks--;

        for (int block=startPositionBlocks;block<=endPositionBlocks;block++)
        {
            y = block/columns*13;
            x = block%columns*13;
            QRectF target(x, y, 12.0, 12.0);
            if (block==endPositionBlocks)
            {
                if (newTaskReceivedList.at(newTaskReceivedListId).taskThreadListId>=0)
                    painter->drawPixmap(target, downloadingPixmap, source);
                else
                    painter->drawPixmap(target, downloadedPixmap, source);
            }
            else
                painter->drawPixmap(target, downloadedPixmap, source);
        }
    }
}
void TaskGraphPainterWidget::drawThreadLastBlock(QPainter *painter, int newThreadReceivedListId)
{
    QList <qint64> newThreadReceivedList;
    newThreadReceivedList = newTaskReceivedList.at(newThreadReceivedListId).newThreadReceivedList;
    if (newThreadReceivedList.size() < 1) return;

    int columns = maxWidth/13;
    QRectF source(0.0, 0.0, 12.0, 12.0);

    qint64 startPosition = newThreadReceivedList.at(0);
    qint64 endPosition = newThreadReceivedList.at(newThreadReceivedList.size()-1);
    int startPositionBlocks = startPosition/blockSize;
    int endPositionBlocks = endPosition/blockSize;
    if (endPosition%blockSize == 0) endPositionBlocks--;

    for (int block=startPositionBlocks;block<=endPositionBlocks;block++)
    {
        y = block/columns*13;
        x = block%columns*13;
        QRectF target(x, y, 12.0, 12.0);
        painter->drawPixmap(target, downloadedPixmap, source);
    }
}
