/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/PhotoItem.h                               *
 *                                                                             *
 * Copyright (C) 2012 by Robert Fernie       <retroshare.project@gmail.com>    *
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

#ifndef PHOTOITEM_H
#define PHOTOITEM_H

#include <QWidget>
#include <QLabel>
#include "PhotoShareItemHolder.h"
#include "retroshare/rsphoto.h"

namespace Ui {
    class PhotoItem;
}

class PhotoItem : public QWidget, public PhotoShareItem
{
    Q_OBJECT

public:

    enum State { New, Existing, Modified, Deleted };

    PhotoItem(PhotoShareItemHolder *holder, const RsPhotoPhoto& photo, QWidget* parent = 0);
    PhotoItem(PhotoShareItemHolder *holder, const QString& path,  uint32_t order, QWidget* parent = 0); // for new photos.
    ~PhotoItem();
    void setSelected(bool selected);
    bool isSelected(){ return mSelected; }
    const RsPhotoPhoto& getPhotoDetails();
    bool getLowResImage(RsGxsImage &image);
    void markForDeletion();
    State getState() { return mState; }

protected:
        void mousePressEvent(QMouseEvent *event);

private:
        void updateImage(const RsGxsImage &image);
        void setUp();

    private slots:
        void setTitle();
        void setPhotoGrapher();

private:
    Ui::PhotoItem *ui;

    QPixmap mLowResImage;

    QPixmap getPixmap() { return mLowResImage; }

    bool mSelected;
    State mState;
    PhotoShareItemHolder* mHolder;
    RsPhotoPhoto mPhotoDetails;

    QLabel *mTitleLabel, *mPhotoGrapherLabel;
};

#endif // PHOTOITEM_H
