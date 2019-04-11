/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/AlbumItem.h                               *
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

#ifndef ALBUMITEM_H
#define ALBUMITEM_H

#include <QWidget>
#include "string.h"
#include "retroshare/rsphoto.h"
#include "PhotoShareItemHolder.h"

namespace Ui {
    class AlbumItem;
}

class AlbumItem : public QWidget, public PhotoShareItem
{
    Q_OBJECT

public:
    explicit AlbumItem(const RsPhotoAlbum& album, PhotoShareItemHolder* albumHolder, QWidget *parent = 0);
    virtual ~AlbumItem();

    const RsPhotoAlbum& getAlbum();

    bool isSelected() {  return mSelected ;}
    void setSelected(bool selected);

protected:
    void mousePressEvent(QMouseEvent *event);

private:
    void setUp();
private:
    Ui::AlbumItem *ui;
    RsPhotoAlbum mAlbum;
    PhotoShareItemHolder* mAlbumHolder;
    bool mSelected;
};

#endif // ALBUMITEM_H
