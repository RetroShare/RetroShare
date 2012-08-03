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

#include "gui/PhotoShare/PhotoSlideShow.h"
#include "gui/PhotoShare/PhotoDrop.h"

#include <iostream>

/** Constructor */
PhotoSlideShow::PhotoSlideShow(QWidget *parent)
: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.pushButton_ShiftLeft, SIGNAL( clicked( void ) ), this, SLOT( moveLeft( void ) ) );
	connect(ui.pushButton_ShiftRight, SIGNAL( clicked( void ) ), this, SLOT( moveRight( void ) ) );
	connect(ui.pushButton_ShowDetails, SIGNAL( clicked( void ) ), this, SLOT( showPhotoDetails( void ) ) );
	connect(ui.pushButton_StartStop, SIGNAL( clicked( void ) ), this, SLOT( StartStop( void ) ) );
	connect(ui.pushButton_Close, SIGNAL( clicked( void ) ), this, SLOT( closeShow( void ) ) );

        mPhotoQueue = new TokenQueueV2(rsPhotoV2->getTokenService(), this);

	mRunning = true;
	mShotActive = true;

	mImageIdx = 0;

	//loadImage();
        //QTimer::singleShot(5000, this, SLOT(timerEvent()));
}

PhotoSlideShow::~PhotoSlideShow(){

}

void PhotoSlideShow::showPhotoDetails()
{

}


void PhotoSlideShow::moveLeft()
{
	if (mRunning)
	{
		return;
	}

	mImageIdx--;
	if (mImageIdx < 0)
	{
		mImageIdx = mPhotos.size() - 1;
	}
	loadImage();	

}


void PhotoSlideShow::moveRight()
{
	if (mRunning)
	{
		return;
	}

	mImageIdx++;
	if (mImageIdx >= mPhotos.size())
	{
		mImageIdx = 0;
	}
	loadImage();	

}


void PhotoSlideShow::StartStop()
{
	if (mRunning)
	{
		mRunning = false;
	}
	else
	{
		mRunning = true;
		if (!mShotActive) // make sure only one timer running
		{
			mShotActive = true;
        		QTimer::singleShot(5000, this, SLOT(timerEvent()));
		}
	}
}

void PhotoSlideShow::timerEvent()
{
	if (!mRunning)
	{
		mShotActive = false;
		return;
	}

	mImageIdx++;
	if (mImageIdx >= mPhotos.size())
	{
		mImageIdx = 0;
	}
	loadImage();	
        QTimer::singleShot(5000, this, SLOT(timerEvent()));
}



void PhotoSlideShow::closeShow()
{
	mRunning = false;
	hide();
}


void PhotoSlideShow::loadImage()
{
	/* get the image */
	int i = 0;
	bool found = false;
	std::string msgId;

	//std::map<std::string, RsPhotoPhoto *>::iterator it;
	std::map<int, std::string>::iterator it;
	for(it = mPhotoOrder.begin(); it != mPhotoOrder.end(); it++, i++)
	{
		if (i == mImageIdx)
		{
			msgId = it->second;
			found = true;
			break;
		}
	}

	RsPhotoPhoto *ptr = NULL;
	if (found)
	{
		ptr = mPhotos[msgId];
	}

	if (ptr)
	{
		/* load into the slot */
        	if (ptr->mThumbnail.data != NULL)
        	{
                	QPixmap qtn;

			// copy the data for Qpixmap to use.
			RsPhotoThumbnail tn;
			tn.copyFrom(ptr->mThumbnail);
                	qtn.loadFromData(tn.data, tn.size, tn.type.c_str());
			tn.data = 0;

                	//ui.imgLabel->setPixmap(qtn);

			QPixmap sqtn = qtn.scaled(ui.albumLabel->width(), ui.imgLabel->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        		ui.imgLabel->setPixmap(sqtn);

        	}
	}
}


void PhotoSlideShow::updateMoveButtons(uint32_t status)
{
        std::cerr << "PhotoSlideShow::updateMoveButtons(" << status << ")";
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




void PhotoSlideShow::clearDialog()
{
#if 0
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
#endif
}


void PhotoSlideShow::loadAlbum(const std::string &albumId)
{
	/* much like main load fns */
	clearDialog();

	RsTokReqOptions opts;
	uint32_t token;
	std::list<std::string> albumIds;
	albumIds.push_back(albumId);

	// We need both Album and Photo Data.

	mPhotoQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, albumIds, 0);

}


bool PhotoSlideShow::loadPhotoData(const uint32_t &token)
{
	std::cerr << "PhotoSlideShow::loadPhotoData()";
	std::cerr << std::endl;
	
	bool moreData = true;
	while(moreData)
	{
		RsPhotoPhoto photo;
                PhotoResult res;
		
                if (rsPhotoV2->getPhoto(token, res))
		{
			RsPhotoPhoto *ptr = new RsPhotoPhoto;
			*ptr = photo;
			ptr->mThumbnail.data = 0;
			ptr->mThumbnail.copyFrom(photo.mThumbnail);

			mPhotos[photo.mMeta.mMsgId] = ptr;
			mPhotoOrder[ptr->mOrder] = photo.mMeta.mMsgId;

			std::cerr << "PhotoSlideShow::addAddPhoto() AlbumId: " << photo.mMeta.mGroupId;
			std::cerr << " PhotoId: " << photo.mMeta.mMsgId;
			std::cerr << std::endl;
		}
		else
		{
			moreData = false;
		}
	}

	// Load and Start.
	loadImage();
        QTimer::singleShot(5000, this, SLOT(timerEvent()));

	return true;
}

bool PhotoSlideShow::loadAlbumData(const uint32_t &token)
{
	std::cerr << "PhotoSlideShow::loadAlbumData()";
	std::cerr << std::endl;
			
	bool moreData = true;
	while(moreData)
	{
		RsPhotoAlbum album;
                std::vector<RsPhotoAlbum> res;
                if (rsPhotoV2->getAlbum(token, res))
		{
			std::cerr << " PhotoSlideShow::loadAlbumData() AlbumId: " << album.mMeta.mGroupId << std::endl;
			//updateAlbumDetails(album);

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

void PhotoSlideShow::loadRequest(const TokenQueueV2 *queue, const TokenRequestV2 &req)
{
	std::cerr << "PhotoSlideShow::loadRequest()";
	std::cerr << std::endl;
		
	if (queue == mPhotoQueue)
	{
		/* now switch on req */
		switch(req.mType)
		{
			case TOKENREQ_GROUPINFO:
				loadAlbumData(req.mToken);
				break;
			case TOKENREQ_MSGINFO:
				loadPhotoData(req.mToken);
				break;
			default:
				std::cerr << "PhotoSlideShow::loadRequest() ERROR: GROUP: INVALID ANS TYPE";
				std::cerr << std::endl;
				break; 
		}
	}
}




