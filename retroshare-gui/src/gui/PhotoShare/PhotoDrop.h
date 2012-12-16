/*
 * Retroshare Photo Plugin.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#ifndef MRK_PHOTO_DROP_H
#define MRK_PHOTO_DROP_H

#include <QList>
#include <QPoint>
#include <QPixmap>
#include <QWidget>
#include <QSet>

QT_BEGIN_NAMESPACE
class QDragEnterEvent;
class QDropEvent;
class QMouseEvent;
QT_END_NAMESPACE

#include "gui/PhotoShare/PhotoItem.h"
#include "PhotoShareItemHolder.h"

#define PHOTO_SHIFT_NO_BUTTONS		0
#define PHOTO_SHIFT_LEFT_ONLY		1
#define PHOTO_SHIFT_RIGHT_ONLY		2
#define PHOTO_SHIFT_BOTH		3


class PhotoDrop : public QWidget
{
    Q_OBJECT

public:

    PhotoDrop(QWidget *parent = 0);
    void clear();
    PhotoItem *getSelectedPhotoItem();
    int	getPhotoCount();
    PhotoItem *getPhotoIdx(int idx);
    void getPhotos(QSet<PhotoItem*>& photos);

    void 	addPhotoItem(PhotoItem *item);
    void setPhotoItemHolder(PhotoShareItemHolder* holder);

    /*!
     * delete photo item held by photo drop
     * @param item photo item to delete
     * @return false if item could not be found
     */
    bool deletePhoto(PhotoItem* item);

public slots:
	void moveLeft();
	void moveRight();
	void checkMoveButtons();
	void clearPhotos();

signals:
    void buttonStatus(uint32_t status);
    void photosChanged();

protected:

	void resizeEvent(QResizeEvent *event);
	void mousePressEvent(QMouseEvent *event);

	void dragEnterEvent(QDragEnterEvent *event);
	void dragLeaveEvent(QDragLeaveEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dropEvent(QDropEvent *event);


private:
	void reorderPhotos();

	PhotoItem *mSelected;
	int mColumns;
        PhotoShareItemHolder* mHolder;
        QSet<PhotoItem*> mPhotos;
};

#endif
