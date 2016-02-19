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

#include "gui/TheWire/PulseAddDialog.h"

#include "gui/PhotoShare/PhotoDrop.h"

#include <iostream>

/** Constructor */
PulseAddDialog::PulseAddDialog(QWidget *parent)
: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.post_PB, SIGNAL( clicked( void ) ), this, SLOT( postPulse( void ) ) );
	connect(ui.addURL_PB, SIGNAL( clicked( void ) ), this, SLOT( addURL( void ) ) );
	connect(ui.addTo_PB, SIGNAL( clicked( void ) ), this, SLOT( addTo( void ) ) );
	connect(ui.cancel_PB, SIGNAL( clicked( void ) ), this, SLOT( cancelPulse( void ) ) );
#if 0
	connect(ui.photoSAreaWC, SIGNAL( buttonStatus( uint32_t ) ), this, SLOT( updateMoveButtons( uint32_t ) ) );
	connect(ui.shiftRight_PB, SIGNAL( clicked( void ) ), ui.addSAreaWC, SLOT( moveRight( void ) ) );
	connect(ui.editPhotoDetails_PB, SIGNAL( clicked( void ) ), this, SLOT( showPhotoDetails( void ) ) );

	connect(ui.publish_PB, SIGNAL( clicked( void ) ), this, SLOT( publishAlbum( void ) ) );
#endif

	mPhotoDetails = NULL;

}


void PulseAddDialog::addURL()
{
        std::cerr << "PulseAddDialog::addURL()";
        std::cerr << std::endl;

	return;
}


void PulseAddDialog::addTo()
{
        std::cerr << "PulseAddDialog::addTo()";
        std::cerr << std::endl;

	return;
}


void PulseAddDialog::cancelPulse()
{
        std::cerr << "PulseAddDialog::cancelPulse()";
        std::cerr << std::endl;

	clearDialog();
	hide();

	return;
}



void PulseAddDialog::updateMoveButtons(uint32_t status)
{
        std::cerr << "PulseAddDialog::updateMoveButtons(" << status << ")";
        std::cerr << std::endl;

#if 0
	switch(status)
	{
		case PHOTO_SHIFT_NO_BUTTONS:
                	ui.shiftLeft_PB->setEnabled(false);
                	ui.shiftRight_PB->setEnabled(false);
			break;
		case PHOTO_SHIFT_LEFT_ONLY:
                	ui.shiftLeft_PB->setEnabled(true);
                	ui.shiftRight_PB->setEnabled(false);
			break;
		case PHOTO_SHIFT_RIGHT_ONLY:
                	ui.shiftLeft_PB->setEnabled(false);
                	ui.shiftRight_PB->setEnabled(true);
			break;
		case PHOTO_SHIFT_BOTH:
                	ui.shiftLeft_PB->setEnabled(true);
                	ui.shiftRight_PB->setEnabled(true);
			break;
	}
#endif
}


void PulseAddDialog::showPhotoDetails()
{

#if 0
        std::cerr << "PulseAddDialog::showPhotoDetails()";
        std::cerr << std::endl;

	if (!mPhotoDetails)
	{
		mPhotoDetails = new PhotoDetailsDialog(NULL);
	}

	PhotoItem *item = ui.photoSAreaWC->getSelectedPhotoItem();

	mPhotoDetails->setPhotoItem(item);
	mPhotoDetails->show();
#endif
}




void PulseAddDialog::postPulse()
{
        std::cerr << "PulseAddDialog::postPulse()";
        std::cerr << std::endl;

#if 0
	/* we need to iterate through each photoItem, and extract the details */


	RsPhotoAlbum album;
	RsPhotoThumbnail albumThumb;

	album.mShareOptions.mShareType = 0;
	album.mShareOptions.mShareGroupId = "unknown";
	album.mShareOptions.mPublishKey = "unknown";
	album.mShareOptions.mCommentMode = 0;
	album.mShareOptions.mResizeMode = 0;

	album.mTitle = ui.titleLEdit->text().toStdString();
	album.mCategory = "Unknown";
	album.mCaption = ui.captionLEdit->text().toStdString();
	album.mWhere = ui.whereLEdit->text().toStdString();
	album.mWhen = ui.whenLEdit->text().toStdString();

	if (rsPhoto->submitAlbumDetails(album, albumThumb))
	{
		/* now have path and album id */
		int photoCount = ui.photoSAreaWC->getPhotoCount();

		for(int i = 0; i < photoCount; ++i)
		{
			RsPhotoPhoto photo;
			RsPhotoThumbnail thumbnail;
			PhotoItem *item = ui.photoSAreaWC->getPhotoIdx(i);
			photo = item->mDetails;
			item->getPhotoThumbnail(thumbnail);
	
			photo.mAlbumId = album.mAlbumId;
			photo.mOrder = i;

			/* scale photo if needed */
			if (album.mShareOptions.mResizeMode)
			{
				/* */

			}
			/* save image to album path */
			photo.path = "unknown";

			rsPhoto->submitPhoto(photo, thumbnail);
		}
	}

#endif
	clearDialog();

	hide();
}


void PulseAddDialog::clearDialog()
{

	ui.pulseTEdit->setPlainText("");
#if 0
	ui.titleLEdit->setText(tr("Title"));
	ui.captionLEdit->setText(tr("Caption"));
	ui.whereLEdit->setText(tr("Where"));
	ui.whenLEdit->setText(tr("When"));

	ui.photoSAreaWC->clearPhotos();
#endif
}

	
