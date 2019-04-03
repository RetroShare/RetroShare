/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/PhotoDialog.h                             *
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
