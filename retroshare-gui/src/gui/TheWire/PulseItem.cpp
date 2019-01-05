/*******************************************************************************
 * gui/TheWire/PulseItem.cpp                                                   *
 *                                                                             *
 * Copyright (c) 2012 Robert Fernie   <retroshare.project@gmail.com>           *
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

#include <QDateTime>
#include <QMessageBox>
#include <QMouseEvent>
#include <QBuffer>

#include "PulseItem.h"

#include <retroshare/rsphoto.h>

#include <algorithm>
#include <iostream>

/****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
PulseItem::PulseItem(PulseHolder *parent, const RsPhotoAlbum &album, const RsPhotoThumbnail &thumbnail)
:QWidget(NULL), mParent(parent), mType(PHOTO_ITEM_TYPE_ALBUM) 
{
	setupUi(this);

	setAttribute ( Qt::WA_DeleteOnClose, true );

	mDetails = *( (RsPhotoPhoto *) &(album));
	updateAlbumText(album);
	updateImage(thumbnail);

	setSelected(false);
}


PulseItem::PulseItem(PulseHolder *parent, const RsPhotoPhoto &photo, const RsPhotoThumbnail &thumbnail)
:QWidget(NULL), mParent(parent), mType(PHOTO_ITEM_TYPE_PHOTO) 
{
	setupUi(this);

	setAttribute ( Qt::WA_DeleteOnClose, true );

	mDetails = *( (RsPhotoPhoto *) &(photo));

	updatePhotoText(photo);
	updateImage(thumbnail);

	setSelected(false);
}


PulseItem::PulseItem(PulseHolder *parent, std::string path) // for new photos.
:QWidget(NULL), mParent(parent), mType(PHOTO_ITEM_TYPE_NEW) 
{
	setupUi(this);

	setAttribute ( Qt::WA_DeleteOnClose, true );

#if 0
	QString dummyString("dummytext");
	titleLabel->setText(QString("NEW PHOTO"));

	fromBoldLabel->setText(QString("From:"));
	fromLabel->setText(QString("Ourselves"));

	statusBoldLabel->setText(QString("Status:"));
	statusLabel->setText(QString("new photo"));

	dateBoldLabel->setText(QString("Date:"));
	dateLabel->setText(QString("now"));

	int width = 120;
	int height = 120;

	//QPixmap qtn = QPixmap(QString::fromStdString(path)).scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	QPixmap qtn = QPixmap(QString::fromStdString(path)).scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	imgLabel->setPixmap(qtn);
	setSelected(false);
#endif
}

void PulseItem::updateAlbumText(const RsPhotoAlbum &album)
{
#if 0
	QString dummyString("dummytext");
	titleLabel->setText(QString("TITLE"));

	fromBoldLabel->setText(QString("From:"));
	fromLabel->setText(QString("Unknown"));

	statusBoldLabel->setText(QString("Status:"));
	statusLabel->setText(QString("new photo"));

	dateBoldLabel->setText(QString("Date:"));
	dateLabel->setText(QString("now"));

	//QDateTime qtime;
	//qtime.setTime_t(msg.ts);
	//QString timestamp = qtime.toString("dd.MMMM yyyy hh:mm");
	//timestamplabel->setText(timestamp);

	dateBoldLabel->setText(dummyString);
	dateLabel->setText(dummyString);
#endif
}

void PulseItem::updatePhotoText(const RsPhotoPhoto &photo)
{
#if 0
	QString dummyString("dummytext");
	titleLabel->setText(QString("TITLE"));

	fromBoldLabel->setText(QString("From:"));
	fromLabel->setText(QString("Unknown"));

	statusBoldLabel->setText(QString("Status:"));
	statusLabel->setText(QString("new photo"));

	dateBoldLabel->setText(QString("Date:"));
	dateLabel->setText(QString("now"));
#endif
}


void PulseItem::updateImage(const RsPhotoThumbnail &thumbnail)
{
#if 0
	if (thumbnail.data != NULL)
	{
		QPixmap qtn;
		qtn.loadFromData(thumbnail.data, thumbnail.size, thumbnail.type.c_str());
		imgLabel->setPixmap(qtn);
	}
#endif
}

bool PulseItem::getPhotoThumbnail(RsPhotoThumbnail &nail)
{
#if 0
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
#endif
	return false;
}


void PulseItem::removeItem()
{
#if 0
#ifdef DEBUG_ITEM
	std::cerr << "PulseItem::removeItem()";
	std::cerr << std::endl;
#endif
	hide();
	if (mParent)
	{
		mParent->deletePulseItem(this, mType);
	}
#endif
}


void PulseItem::setSelected(bool on)
{
#if 0
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
#endif
}

bool PulseItem::isSelected()
{
	return mSelected;
}


void PulseItem::mousePressEvent(QMouseEvent *event)
{
        /* We can be very cunning here?
	 * grab out position.
	 * flag ourselves as selected.
	 * then pass the mousePressEvent up for handling by the parent
	 */

        QPoint pos = event->pos();

        std::cerr << "PulseItem::mousePressEvent(" << pos.x() << ", " << pos.y() << ")";
        std::cerr << std::endl;

	setSelected(true);

        QWidget::mousePressEvent(event);
}


const QPixmap *PulseItem::getPixmap()
{
#if 0
	return imgLabel->pixmap();
#endif
	return NULL;
}


