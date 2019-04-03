/*******************************************************************************
 * gui/TheWire/PulseAddDialog.cpp                                              *
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

#include "gui/TheWire/PulseAddDialog.h"

#include "gui/PhotoShare/PhotoDrop.h"

#include <iostream>

/** Constructor */
PulseAddDialog::PulseAddDialog(QWidget *parent)
: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.pushButton_Post, SIGNAL( clicked( void ) ), this, SLOT( postPulse( void ) ) );
	connect(ui.pushButton_AddURL, SIGNAL( clicked( void ) ), this, SLOT( addURL( void ) ) );
	connect(ui.pushButton_AddTo, SIGNAL( clicked( void ) ), this, SLOT( addTo( void ) ) );
	connect(ui.pushButton_Cancel, SIGNAL( clicked( void ) ), this, SLOT( cancelPulse( void ) ) );
#if 0
	connect(ui.scrollAreaWidgetContents, SIGNAL( buttonStatus( uint32_t ) ), this, SLOT( updateMoveButtons( uint32_t ) ) );
	connect(ui.pushButton_ShiftRight, SIGNAL( clicked( void ) ), ui.scrollAreaWidgetContents, SLOT( moveRight( void ) ) );
	connect(ui.pushButton_EditPhotoDetails, SIGNAL( clicked( void ) ), this, SLOT( showPhotoDetails( void ) ) );

	connect(ui.pushButton_Publish, SIGNAL( clicked( void ) ), this, SLOT( publishAlbum( void ) ) );
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
                	ui.pushButton_ShiftLeft->setEnabled(false);
                	ui.pushButton_ShiftRight->setEnabled(false);
			break;
		case PHOTO_SHIFT_LEFT_ONLY:
                	ui.pushButton_ShiftLeft->setEnabled(true);
                	ui.pushButton_ShiftRight->setEnabled(false);
			break;
		case PHOTO_SHIFT_RIGHT_ONLY:
                	ui.pushButton_ShiftLeft->setEnabled(false);
                	ui.pushButton_ShiftRight->setEnabled(true);
			break;
		case PHOTO_SHIFT_BOTH:
                	ui.pushButton_ShiftLeft->setEnabled(true);
                	ui.pushButton_ShiftRight->setEnabled(true);
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

	PhotoItem *item = ui.scrollAreaWidgetContents->getSelectedPhotoItem();

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

	album.mTitle = ui.lineEdit_Title->text().toStdString();
	album.mCategory = "Unknown";
	album.mCaption = ui.lineEdit_Caption->text().toStdString();
	album.mWhere = ui.lineEdit_Where->text().toStdString();
	album.mWhen = ui.lineEdit_When->text().toStdString();

	if (rsPhoto->submitAlbumDetails(album, albumThumb))
	{
		/* now have path and album id */
		int photoCount = ui.scrollAreaWidgetContents->getPhotoCount();

		for(int i = 0; i < photoCount; ++i)
		{
			RsPhotoPhoto photo;
			RsPhotoThumbnail thumbnail;
			PhotoItem *item = ui.scrollAreaWidgetContents->getPhotoIdx(i);
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

	ui.textEdit_Pulse->setPlainText("");
#if 0
	ui.lineEdit_Title->setText(QString("title"));
	ui.lineEdit_Caption->setText(QString("Caption"));
	ui.lineEdit_Where->setText(QString("Where"));
	ui.lineEdit_When->setText(QString("When"));

	ui.scrollAreaWidgetContents->clearPhotos();
#endif
}

	
