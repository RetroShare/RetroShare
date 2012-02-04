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

#ifndef MRK_PHOTO_PHOTO_ITEM_H
#define MRK_PHOTO_PHOTO_ITEM_H

#include "ui_PhotoItem.h"

#include <retroshare/rsphoto.h>

class PhotoItem;

class PhotoHolder
{
	public:
virtual void deletePhotoItem(PhotoItem *, uint32_t ptype) = 0;
virtual void notifySelection(PhotoItem *item, int ptype) = 0;
};


#define PHOTO_ITEM_TYPE_ALBUM	0x0001
#define PHOTO_ITEM_TYPE_PHOTO	0x0002
#define PHOTO_ITEM_TYPE_NEW	0x0003

class PhotoItem : public QWidget, private Ui::PhotoItem
{
  Q_OBJECT

public:
	PhotoItem(PhotoHolder *parent, const RsPhotoAlbum &album, const RsPhotoThumbnail &thumbnail);
	PhotoItem(PhotoHolder *parent, const RsPhotoPhoto &photo, const RsPhotoThumbnail &thumbnail);
	PhotoItem(PhotoHolder *parent, std::string url); // for new photos.

	bool getPhotoThumbnail(RsPhotoThumbnail &nail);

	void removeItem();

	void setSelected(bool on);
	bool isSelected();

	const QPixmap *getPixmap();

	// details are public - so that can be easily edited.
	RsPhotoPhoto mDetails;

//private slots:


protected:
	void mousePressEvent(QMouseEvent *event);

private:
	void updateAlbumText(const RsPhotoAlbum &album);
	void updatePhotoText(const RsPhotoPhoto &photo);
	void updateImage(const RsPhotoThumbnail &thumbnail);

	PhotoHolder *mParent;
	uint32_t     mType;


        bool mSelected;
};


#endif

