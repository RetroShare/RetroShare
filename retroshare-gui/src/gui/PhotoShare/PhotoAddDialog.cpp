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

#include "gui/PhotoShare/PhotoAddDialog.h"
#include "gui/PhotoShare/PhotoDetailsDialog.h"
#include "gui/PhotoShare/PhotoDrop.h"

#include <iostream>

/** Constructor */
PhotoAddDialog::PhotoAddDialog(QWidget *parent)
: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.scrollAreaWidgetContents, SIGNAL( buttonStatus( uint32_t ) ), this, SLOT( updateMoveButtons( uint32_t ) ) );
	connect(ui.pushButton_ShiftLeft, SIGNAL( clicked( void ) ), ui.scrollAreaWidgetContents, SLOT( moveLeft( void ) ) );
	connect(ui.pushButton_ShiftRight, SIGNAL( clicked( void ) ), ui.scrollAreaWidgetContents, SLOT( moveRight( void ) ) );
	connect(ui.pushButton_EditPhotoDetails, SIGNAL( clicked( void ) ), this, SLOT( showPhotoDetails( void ) ) );
	connect(ui.pushButton_EditAlbumDetails, SIGNAL( clicked( void ) ), this, SLOT( showAlbumDetails( void ) ) );
	connect(ui.pushButton_DeleteAlbum, SIGNAL( clicked( void ) ), this, SLOT( deleteAlbum( void ) ) );
	connect(ui.pushButton_DeletePhoto, SIGNAL( clicked( void ) ), this, SLOT( deletePhoto( void ) ) );

	connect(ui.pushButton_Publish, SIGNAL( clicked( void ) ), this, SLOT( publishAlbum( void ) ) );

	mPhotoDetails = NULL;

        mPhotoQueue = new TokenQueueV2(rsPhotoV2->getTokenService(), this);

	ui.AlbumDrop->setSingleImage();
	connect(ui.AlbumDrop, SIGNAL( photosChanged( void ) ), this, SLOT( albumImageChanged( void ) ) );
	connect(ui.scrollAreaWidgetContents, SIGNAL( photosChanged( void ) ), this, SLOT( photoImageChanged( void ) ) );
}


void PhotoAddDialog::updateMoveButtons(uint32_t status)
{
        std::cerr << "PhotoAddDialog::updateMoveButtons(" << status << ")";
        std::cerr << std::endl;

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
}


bool PhotoAddDialog::updateAlbumDetails(const RsPhotoAlbum &album)
{
        std::cerr << "PhotoAddDialog::updateAlbumDetails()";
	std::cerr << " (Copy data to mAlbumData + Add PhotoItem)";
        std::cerr << std::endl;
	// cleanup old image first.
	mAlbumData.mThumbnail.deleteImage();
	mAlbumData = album;

	// copy photo too.
	mAlbumData.mThumbnail.data = 0;
	mAlbumData.mThumbnail.copyFrom(album.mThumbnail);

	/* show iterate through all the photos and update them too  - except normally they haven't arrived yet */

	ui.lineEdit_Title->setText(QString::fromUtf8(album.mMeta.mGroupName.c_str()));
	ui.lineEdit_Caption->setText(QString::fromUtf8(album.mCaption.c_str()));
	ui.lineEdit_Where->setText(QString::fromUtf8(album.mWhere.c_str()));
	ui.lineEdit_When->setText(QString::fromUtf8(album.mWhen.c_str()));

	PhotoItem *item = new PhotoItem(NULL, mAlbumData);
	ui.AlbumDrop->addPhotoItem(item);

	// called via callback AlbumChanged.
	//setAlbumDataToPhotos();
	return true;
}


bool PhotoAddDialog::setAlbumDataToPhotos()
{
        std::cerr << "PhotoAddDialog::setAlbumDataToPhotos()";
        std::cerr << std::endl;

	int photoCount = ui.scrollAreaWidgetContents->getPhotoCount();

	for(int i = 0; i < photoCount; i++)
	{
		PhotoItem *item = ui.scrollAreaWidgetContents->getPhotoIdx(i);
		item->updateAlbumText(mAlbumData);
	}
	return true;
}


void PhotoAddDialog::showPhotoDetails()
{
        std::cerr << "PhotoAddDialog::showPhotoDetails()";
        std::cerr << std::endl;

	PhotoItem *item = ui.scrollAreaWidgetContents->getSelectedPhotoItem();
	if (item)
	{
		if (!mPhotoDetails)
		{
			mPhotoDetails = new PhotoDetailsDialog(NULL);
			connect(mPhotoDetails, SIGNAL( editingDone( void ) ), this, SLOT( editingStageDone( void ) ) );
		}
		mPhotoDetails->setPhotoItem(item);
		mPhotoDetails->show();
		mEditingModeAlbum = false;
	}
}


void PhotoAddDialog::showAlbumDetails()
{
        std::cerr << "PhotoAddDialog::showAlbumDetails()";
        std::cerr << std::endl;


	/* grab the image from the AlbumDrop */
	PhotoItem *item = NULL;
	if (ui.AlbumDrop->getPhotoCount() > 0)
	{
		item = ui.AlbumDrop->getPhotoIdx(0);
	}

	if (item)
	{
		if (!mPhotoDetails)
		{
			mPhotoDetails = new PhotoDetailsDialog(NULL);
			connect(mPhotoDetails, SIGNAL( editingDone( void ) ), this, SLOT( editingStageDone( void ) ) );
		}
		mPhotoDetails->setPhotoItem(item);
		mPhotoDetails->show();
		mEditingModeAlbum = true;
	}
	else
	{
        	std::cerr << "PhotoAddDialog::showAlbumDetails() PhotoItem Invalid";
        	std::cerr << std::endl;
	}
}

/* Callback when AlbumDrop gets new image */
void PhotoAddDialog::albumImageChanged()
{
        std::cerr << "PhotoAddDialog::albumImageChanged()";
        std::cerr << std::endl;

	/* must update the data from the reference stuff */
	PhotoItem *item = NULL;
	if (ui.AlbumDrop->getPhotoCount() > 0)
	{
		item = ui.AlbumDrop->getPhotoIdx(0);
	}

	if (!item)
	{
        	std::cerr << "PhotoAddDialog::albumImageChanged() ERROR no Album PhotoItem";
        	std::cerr << std::endl;
		return;
	}

        std::cerr << "PhotoAddDialog::albumImageChanged() PRE: AlbumDrop: " << item->mAlbumDetails;
        std::cerr << std::endl;
        std::cerr << "PhotoAddDialog::albumImageChanged() PRE: mAlbumData: " << mAlbumData;
        std::cerr << std::endl;
	

	item->mIsPhoto = false; // Force to Album mode.

	/* now AlbumDrop has the image, but AlbumData has the other stuff */

	item->getPhotoThumbnail(mAlbumData.mThumbnail);
	item->updateAlbumText(mAlbumData);



	/* if we are in editing mode -> update it */
	if ((mEditingModeAlbum) && (mPhotoDetails))
	{
        	std::cerr << "PhotoAddDialog::albumImageChanged() Updating PhotoDetails -> PhotoItem";
        	std::cerr << std::endl;
		mPhotoDetails->setPhotoItem(item);
	}

        std::cerr << "PhotoAddDialog::albumImageChanged() POST: AlbumDrop: " << item->mAlbumDetails;
        std::cerr << std::endl;
        std::cerr << "PhotoAddDialog::albumImageChanged() POST: mAlbumData: " << mAlbumData;
        std::cerr << std::endl;
	
}


/* This is called back once PhotoDetailsDialog Finishes */
void PhotoAddDialog::editingStageDone()
{
        std::cerr << "PhotoAddDialog::editingStageDone()";
        std::cerr << std::endl;

	if (mEditingModeAlbum)
	{
		/* need to resolve Album Data, repopulate entries 
		 */

		/* grab the image from the AlbumDrop (This is where PhotoDetailsDialog stores the data) */
		PhotoItem *item = NULL;
		if (ui.AlbumDrop->getPhotoCount() > 0)
		{
			item = ui.AlbumDrop->getPhotoIdx(0);
		}

		if (!item)
		{
        		std::cerr << "PhotoAddDialog::editingStageDone() ERROR no Album PhotoItem";
        		std::cerr << std::endl;
		}

		/* Total Hack here Copy from AlbumDrop to Reference Data */

		// cleanup old image first.
		mAlbumData.mThumbnail.deleteImage();
		mAlbumData = item->mAlbumDetails;
		item->getPhotoThumbnail(mAlbumData.mThumbnail);

		// Push Back data -> to trigger Text Update.
		item->updateAlbumText(mAlbumData);
		mEditingModeAlbum = false;

		// Update GUI too.
		ui.lineEdit_Title->setText(QString::fromUtf8(mAlbumData.mMeta.mGroupName.c_str()));
		ui.lineEdit_Caption->setText(QString::fromUtf8(mAlbumData.mCaption.c_str()));
		ui.lineEdit_Where->setText(QString::fromUtf8(mAlbumData.mWhere.c_str()));
		ui.lineEdit_When->setText(QString::fromUtf8(mAlbumData.mWhen.c_str()));

	}
	else
	{
        	std::cerr << "PhotoAddDialog::editingStageDone() ERROR not EditingModeAlbum";
        	std::cerr << std::endl;
	}

	// This forces item update -> though the AlbumUpdate is only needed if we edited Album.
	setAlbumDataToPhotos();
}


/* Callback when PhotoDrop gets new image */
void PhotoAddDialog::photoImageChanged()
{
	setAlbumDataToPhotos();
}



void PhotoAddDialog::publishAlbum()
{
        std::cerr << "PhotoAddDialog::publishAlbum()";
        std::cerr << std::endl;

	/* we need to iterate through each photoItem, and extract the details */

	RsPhotoAlbum album = mAlbumData;
	album.mThumbnail.data = 0;

	album.mShareOptions.mShareType = 0;
	album.mShareOptions.mShareGroupId = "unknown";
	album.mShareOptions.mPublishKey = "unknown";
	album.mShareOptions.mCommentMode = 0;
	album.mShareOptions.mResizeMode = 0;

	//album.mMeta.mGroupName = ui.lineEdit_Title->text().toStdString();
	//album.mCategory = "Unknown";
	//album.mCaption = ui.lineEdit_Caption->text().toStdString();
	//album.mWhere = ui.lineEdit_Where->text().toStdString();
	//album.mWhen = ui.lineEdit_When->text().toStdString();

	/* grab the image from the AlbumDrop */
	if (ui.AlbumDrop->getPhotoCount() > 0)
	{
		PhotoItem *item = ui.AlbumDrop->getPhotoIdx(0);
		item->getPhotoThumbnail(album.mThumbnail);
	}

	// For the moment, only submit albums Once.
	if (mAlbumEdit)
	{
        	std::cerr << "PhotoAddDialog::publishAlbum() AlbumEdit Mode";
        	std::cerr << std::endl;

		/* call publishPhotos directly */
		publishPhotos(album.mMeta.mGroupId);
	}


        std::cerr << "PhotoAddDialog::publishAlbum() New Album Mode Submitting.....";
        std::cerr << std::endl;

	uint32_t token;
        rsPhotoV2->submitAlbumDetails(album);

	// tell tokenQueue to expect results from submission.
	mPhotoQueue->queueRequest(token, TOKENREQ_GROUPINFO, RS_TOKREQ_ANSTYPE_SUMMARY, 0);

}




void PhotoAddDialog::publishPhotos(std::string albumId)
{
	/* now have path and album id */
	int photoCount = ui.scrollAreaWidgetContents->getPhotoCount();

	for(int i = 0; i < photoCount; i++)
	{
		RsPhotoPhoto photo;
		PhotoItem *item = ui.scrollAreaWidgetContents->getPhotoIdx(i);

		if (!item->mIsPhoto)
		{
			std::cerr << "PhotoAddDialog::publishAlbum() MAJOR ERROR!";
        			std::cerr << std::endl;
		}

		photo = item->mPhotoDetails;
		photo.mThumbnail.data = 0; // do proper data copy.
		item->getPhotoThumbnail(photo.mThumbnail);

		bool isNewPhoto = false;	
		bool isModifiedPhoto = false;	

		if (mAlbumEdit)
		{
			// can have modFlags and be New... so the order is important.
			if (photo.mMeta.mGroupId.length() < 1)
			{
				/* new photo - flag in mods */
				photo.mModFlags |= RSPHOTO_FLAGS_ATTRIB_PHOTO;
				photo.mMeta.mGroupId = albumId;
				isNewPhoto = true;
			}
			else if (photo.mModFlags)
			{
				isModifiedPhoto = true;
			}
		}
		else
		{
			/* new album - update GroupId, all photos are new */
			photo.mMeta.mGroupId = albumId;
			isNewPhoto = true;
		}

		photo.mOrder = i;

		std::cerr << "PhotoAddDialog::publishAlbum() Photo(" << i << ")";
		std::cerr << " mSetFlags: " << photo.mSetFlags;
		std::cerr << " mModFlags: " << photo.mModFlags;
        		std::cerr << std::endl;

#if 0
		/* scale photo if needed */
		if (album.mShareOptions.mResizeMode)
		{
			/* */

		}
#endif

		/* save image to album path */
		photo.path = "unknown";

		std::cerr << "PhotoAddDialog::publishAlbum() Photo(" << i << ") ";

		uint32_t token;
		if (isNewPhoto)
		{
			std::cerr << "Is a New Photo";
                        rsPhotoV2->submitPhoto(photo);
		}
		else if (isModifiedPhoto)
		{
			std::cerr << "Is Updated";
                        rsPhotoV2->submitPhoto(photo);
		}
		else
		{
			std::cerr << "Is Unchanged";
		}
        	std::cerr << std::endl;
	}

	clearDialog();

	hide();
}


void PhotoAddDialog::deleteAlbum()
{
	std::cerr << "PhotoAddDialog::deleteAlbum() Not Implemented Yet";
	std::cerr << std::endl;
}


void PhotoAddDialog::deletePhoto()
{
	std::cerr << "PhotoAddDialog::deletePhoto() Not Implemented Yet";
	std::cerr << std::endl;
}


void PhotoAddDialog::clearDialog()
{
	ui.lineEdit_Title->setText(QString("title"));
	ui.lineEdit_Caption->setText(QString("Caption"));
	ui.lineEdit_Where->setText(QString("Where"));
	ui.lineEdit_When->setText(QString("When"));

	ui.scrollAreaWidgetContents->clearPhotos();
	ui.AlbumDrop->clearPhotos();

	/* clean up album image */
	mAlbumData.mThumbnail.deleteImage();

	RsPhotoAlbum emptyAlbum;
	mAlbumData = emptyAlbum;

	/* add empty image */
	PhotoItem *item = new PhotoItem(NULL, mAlbumData);
	ui.AlbumDrop->addPhotoItem(item);

	mAlbumEdit = false;
}


void PhotoAddDialog::loadAlbum(const std::string &albumId)
{
	/* much like main load fns */
	clearDialog();
	mAlbumEdit = true;

	RsTokReqOptions opts;
	uint32_t token;
        std::list<RsGxsGroupId> albumIds;
	albumIds.push_back(albumId);

	// We need both Album and Photo Data.

	mPhotoQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, albumIds, 0);

}


bool PhotoAddDialog::loadPhotoData(const uint32_t &token)
{
	std::cerr << "PhotoAddDialog::loadPhotoData()";
	std::cerr << std::endl;
	
	bool moreData = true;
	while(moreData)
	{
            PhotoResult res;
		RsPhotoPhoto photo;
		
                if (rsPhotoV2->getPhoto(token, res))
		{
			std::cerr << "PhotoDialog::addAddPhoto() AlbumId: " << photo.mMeta.mGroupId;
			std::cerr << " PhotoId: " << photo.mMeta.mMsgId;
			std::cerr << std::endl;

			PhotoItem *item = new PhotoItem(NULL, photo, mAlbumData);
			ui.scrollAreaWidgetContents->addPhotoItem(item);
			
		}
		else
		{
			moreData = false;
		}
	}
	return true;
}

bool PhotoAddDialog::loadAlbumData(const uint32_t &token)
{
	std::cerr << "PhotoAddDialog::loadAlbumData()";
	std::cerr << std::endl;
			
	bool moreData = true;
	while(moreData)
	{
            std::vector<RsPhotoAlbum> albums;
		RsPhotoAlbum album;
                if (rsPhotoV2->getAlbum(token, albums))
		{
			std::cerr << " PhotoAddDialog::loadAlbumData() AlbumId: " << album.mMeta.mGroupId << std::endl;
			updateAlbumDetails(album);

			RsTokReqOptions opts;
			opts.mOptions = RS_TOKREQOPT_MSG_LATEST;
			uint32_t token;
			std::list<std::string> albumIds;
			albumIds.push_back(album.mMeta.mGroupId);
                        GxsMsgReq req;
                        req[album.mMeta.mGroupId] = std::vector<RsGxsMessageId>();
                        mPhotoQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, req, 0);
		}
		else
		{
			moreData = false;
		}
	}
	return true;
}

bool PhotoAddDialog::loadCreatedAlbum(const uint32_t &token)
{
	std::cerr << "PhotoAddDialog::loadCreatedAlbum()";
	std::cerr << std::endl;
			
	std::list<RsGroupMetaData> groupInfo;
        if (!rsPhotoV2->getGroupSummary(token, groupInfo))
	{
		std::cerr << "PhotoAddDialog::loadCreatedAlbum() ERROR Getting MetaData";
		std::cerr << std::endl;
		return false;
	}

	if (groupInfo.size() != 1)
	{
		std::cerr << "PhotoAddDialog::loadCreatedAlbum() ERROR Too much Info";
		std::cerr << std::endl;
		return false;
	}

	std::cerr << "PhotoAddDialog::loadCreatedAlbum() publishing Photos";
	std::cerr << std::endl;

	publishPhotos(groupInfo.front().mGroupId);

	return true;
}


void PhotoAddDialog::loadRequest(const TokenQueueV2 *queue, const TokenRequestV2 &req)
{
	std::cerr << "PhotoDialog::loadRequest()";
	std::cerr << std::endl;
		
	if (queue == mPhotoQueue)
	{
		/* now switch on req */
		switch(req.mType)
		{
			case TOKENREQ_GROUPINFO:
				switch(req.mAnsType)
				{
					case RS_TOKREQ_ANSTYPE_DATA:
						loadAlbumData(req.mToken);
						break;
					case RS_TOKREQ_ANSTYPE_SUMMARY:
						loadCreatedAlbum(req.mToken);
						break;
					default:
						std::cerr << "PhotoAddDialog::loadRequest() ERROR: GROUP: INVALID ANS TYPE";
						std::cerr << std::endl;
				}
				break;
			case TOKENREQ_MSGINFO:
				loadPhotoData(req.mToken);
				break;
			default:
				std::cerr << "PhotoAddDialog::loadRequest() ERROR: INVALID TYPE";
				std::cerr << std::endl;
				break; 
		}
	}
}




