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


#include <QDateTime>
#include <QMessageBox>
#include <QMouseEvent>
#include <QBuffer>

#include "PhotoItem.h"

#include <retroshare/rsphotoV2.h>

#include <algorithm>
#include <iostream>

/****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
PhotoItem::PhotoItem(PhotoHolder *parent, const RsPhotoAlbum &album)
:QWidget(NULL), mParent(parent), mType(PHOTO_ITEM_TYPE_ALBUM) 
{
	setupUi(this);

	setAttribute ( Qt::WA_DeleteOnClose, true );

	mIsPhoto = false;
	setDummyText();
	updateAlbumText(album); // saves: mAlbumDetails = album;
	updateImage(album.mThumbnail);

	setSelected(false);
}


PhotoItem::PhotoItem(PhotoHolder *parent, const RsPhotoPhoto &photo, const RsPhotoAlbum &album)
:QWidget(NULL), mParent(parent), mType(PHOTO_ITEM_TYPE_PHOTO) 
{
	setupUi(this);

	setAttribute ( Qt::WA_DeleteOnClose, true );

	mIsPhoto = true;

	setDummyText();
	updatePhotoText(photo); // saves: mPhotoDetails = photo;
	updateAlbumText(album); // saves: mAlbumDetails = album;

	updateImage(photo.mThumbnail);

	setSelected(false);
}


PhotoItem::PhotoItem(PhotoHolder *parent, std::string path) // for new photos.
:QWidget(NULL), mParent(parent), mType(PHOTO_ITEM_TYPE_NEW) 
{
	setupUi(this);

	setAttribute ( Qt::WA_DeleteOnClose, true );

	setDummyText();
	mIsPhoto = true;

	int width = 120;
	int height = 120;

	//QPixmap qtn = QPixmap(QString::fromStdString(path)).scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	QPixmap qtn = QPixmap(QString::fromStdString(path)).scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	imgLabel->setPixmap(qtn);
	setSelected(false);
}

void PhotoItem::updateParent(PhotoHolder *parent) // for external construction.
{
	mParent = parent;
}


void PhotoItem::setDummyText()
{
	titleLabel->setText(QString("Unknown"));
	fromBoldLabel->setText(QString("By:"));
	fromLabel->setText(QString("Unknown"));
	statusBoldLabel->setText(QString("Where:"));
	statusLabel->setText(QString("Unknown"));
	dateBoldLabel->setText(QString("When:"));
	dateLabel->setText(QString("Unknown"));
}


void PhotoItem::updateAlbumText(const RsPhotoAlbum &album)
{
	mAlbumDetails = album;
	mAlbumDetails.mThumbnail.data = 0;
	updateText();
}

void PhotoItem::updatePhotoText(const RsPhotoPhoto &photo)
{
	// Save new Photo details.
	mPhotoDetails = photo;
	mPhotoDetails.mThumbnail.data = 0;
	updateText();
}



void PhotoItem::updateText()
{
	// SET Album Values first -> then overwrite with Photo Values.
	if (mAlbumDetails.mSetFlags & RSPHOTO_FLAGS_ATTRIB_TITLE)
	{
		titleLabel->setText(QString::fromUtf8(mAlbumDetails.mMeta.mGroupName.c_str()));
	}

	// This needs to be fixed!! TODO
	fromLabel->setText(QString::fromStdString(mAlbumDetails.mMeta.mGroupId));
	if (mAlbumDetails.mSetFlags & RSPHOTO_FLAGS_ATTRIB_AUTHOR)
	{
		// This needs to be fixed!! TODO
		fromLabel->setText(QString::fromStdString(mAlbumDetails.mMeta.mGroupId));
	}

	if (mAlbumDetails.mSetFlags & RSPHOTO_FLAGS_ATTRIB_WHERE)
	{
		statusLabel->setText(QString::fromUtf8(mAlbumDetails.mWhere.c_str()));
	}

	if (mAlbumDetails.mSetFlags & RSPHOTO_FLAGS_ATTRIB_WHEN)
	{
		dateLabel->setText(QString::fromUtf8(mAlbumDetails.mWhen.c_str()));
	}
	
		// NOW Photo Bits.
	if (mIsPhoto)
	{
		if (mPhotoDetails.mSetFlags & RSPHOTO_FLAGS_ATTRIB_TITLE)
		{
			titleLabel->setText(QString::fromUtf8(mPhotoDetails.mMeta.mMsgName.c_str()));
		}
	
		if (mPhotoDetails.mSetFlags & RSPHOTO_FLAGS_ATTRIB_AUTHOR)
		{
			// This needs to be fixed!! TODO
			fromLabel->setText(QString::fromStdString(mPhotoDetails.mMeta.mAuthorId));
		}
	
		if (mPhotoDetails.mSetFlags & RSPHOTO_FLAGS_ATTRIB_WHERE)
		{
			statusLabel->setText(QString::fromUtf8(mPhotoDetails.mWhere.c_str()));
		}
	
		if (mPhotoDetails.mSetFlags & RSPHOTO_FLAGS_ATTRIB_WHEN)
		{
			dateLabel->setText(QString::fromUtf8(mPhotoDetails.mWhen.c_str()));
		}
	}
	
}

void PhotoItem::updateImage(const RsPhotoThumbnail &thumbnail)
{
	if (thumbnail.data != NULL)
	{
		QPixmap qtn;
		qtn.loadFromData(thumbnail.data, thumbnail.size, thumbnail.type.c_str());
		imgLabel->setPixmap(qtn);
	}
}

bool PhotoItem::getPhotoThumbnail(RsPhotoThumbnail &nail)
{
	const QPixmap *tmppix = imgLabel->pixmap();

        QByteArray ba;
        QBuffer buffer(&ba);

        if(!tmppix->isNull())
	{
                // send chan image

                buffer.open(QIODevice::WriteOnly);
                tmppix->save(&buffer, "PNG"); // writes image into ba in PNG format

		RsPhotoThumbnail tmpnail;
		tmpnail.data = (uint8_t *) ba.data();
		tmpnail.size = ba.size();

		nail.copyFrom(tmpnail);

		return true;
        }

	nail.data = NULL;
	nail.size = 0;
	return false;
}


void PhotoItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "PhotoItem::removeItem()";
	std::cerr << std::endl;
#endif
	hide();
	if (mParent)
	{
		mParent->deletePhotoItem(this, mType);
	}
}


void PhotoItem::setSelected(bool on)
{
	mSelected = on;
	if (mSelected)
	{
		mParent->notifySelection(this, mType);
		frame->setStyleSheet("QFrame#frame{border: 2px solid #55CC55;\nbackground: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #55EE55, stop: 1 #CCCCCC);\nborder-radius: 10px}");
	}
	else
	{
		frame->setStyleSheet("QFrame#frame{border: 2px solid #CCCCCC;\nbackground: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #EEEEEE, stop: 1 #CCCCCC);\nborder-radius: 10px}");
	}
	update();
}

bool PhotoItem::isSelected()
{
	return mSelected;
}


void PhotoItem::mousePressEvent(QMouseEvent *event)
{
        /* We can be very cunning here?
	 * grab out position.
	 * flag ourselves as selected.
	 * then pass the mousePressEvent up for handling by the parent
	 */

        QPoint pos = event->pos();

        std::cerr << "PhotoItem::mousePressEvent(" << pos.x() << ", " << pos.y() << ")";
        std::cerr << std::endl;

	setSelected(true);

        QWidget::mousePressEvent(event);
}


const QPixmap *PhotoItem::getPixmap()
{
	return imgLabel->pixmap();
}


