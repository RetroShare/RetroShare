/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/AlbumItem.cpp                             *
 *                                                                             *
 * Copyright (C) 2018 by Retroshare Team     <retroshare.project@gmail.com>    *
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

#include "AlbumItem.h"
#include "ui_AlbumItem.h"
#include "gui/common/FilesDefs.h"

#include <iostream>

#include <QMouseEvent>

AlbumItem::AlbumItem(const RsPhotoAlbum &album, PhotoShareItemHolder *albumHolder, QWidget *parent) :
    QWidget(NULL),
    ui(new Ui::AlbumItem), mAlbum(album), mAlbumHolder(albumHolder)
{
    ui->setupUi(this);
    setUp();
}

AlbumItem::~AlbumItem()
{
    delete ui;
}

void AlbumItem::setUp()
{
    ui->label_AlbumTitle->setText(QString::fromStdString(mAlbum.mMeta.mGroupName));
    ui->label_Photographer->setText(QString::fromStdString(mAlbum.mPhotographer));
    
    if(mAlbum.mThumbnail.mSize != 0)
    {
        QPixmap qtn;
        qtn.loadFromData(mAlbum.mThumbnail.mData, mAlbum.mThumbnail.mSize, "JPG");
        ui->label_Thumbnail->setPixmap(qtn);
    }
    else
    {
        // display a default Album icon when album has no Thumbnail
        ui->label_Thumbnail->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/album_default_128.png"));
    }
}

void AlbumItem::mousePressEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();

    std::cerr << "AlbumItem::mousePressEvent(" << pos.x() << ", " << pos.y() << ")";
    std::cerr << std::endl;

    if(mAlbumHolder)
        mAlbumHolder->notifySelection(this);
    else
        setSelected(true);

    QWidget::mousePressEvent(event);
}

void AlbumItem::setSelected(bool on)
{
    mSelected = on;
    if (mSelected)
    {
            ui->albumFrame->setStyleSheet("QFrame#albumFrame{border: 2px solid #55CC55;\nbackground: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #55EE55, stop: 1 #CCCCCC);\nborder-radius: 10px}");
    }
    else
    {
            ui->albumFrame->setStyleSheet("");
    }
    update();
}

const RsPhotoAlbum& AlbumItem::getAlbum()
{
    return mAlbum;
}
