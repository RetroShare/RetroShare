/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/PhotoSlideShow.h                          *
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

#include "gui/PhotoShare/PhotoSlideShow.h"
#include "gui/PhotoShare/PhotoDrop.h"
#include "gui/gxs/GxsIdDetails.h"

#include <iostream>

/** Constructor */
PhotoSlideShow::PhotoSlideShow(const RsPhotoAlbum& album, QWidget *parent)
: QWidget(parent), mAlbum(album)
{
	ui.setupUi(this);

	connect(ui.pushButton_ShiftLeft, SIGNAL( clicked( void ) ), this, SLOT( moveLeft( void ) ) );
	connect(ui.pushButton_ShiftRight, SIGNAL( clicked( void ) ), this, SLOT( moveRight( void ) ) );
	connect(ui.pushButton_ShowDetails, SIGNAL( clicked( void ) ), this, SLOT( showPhotoDetails( void ) ) );
	connect(ui.pushButton_StartStop, SIGNAL( clicked( void ) ), this, SLOT( StartStop( void ) ) );
	connect(ui.pushButton_Close, SIGNAL( clicked( void ) ), this, SLOT( closeShow( void ) ) );
	connect(ui.fullscreenButton, SIGNAL(clicked()),this, SLOT(setFullScreen()));

        mPhotoQueue = new TokenQueue(rsPhoto->getTokenService(), this);

	mRunning = true;
	mShotActive = true;

	mImageIdx = 0;

        requestPhotos();
        loadImage();
        //QTimer::singleShot(5000, this, SLOT(timerEvent()));
}

PhotoSlideShow::~PhotoSlideShow()
{
    std::map<RsGxsMessageId, RsPhotoPhoto *>::iterator mit = mPhotos.begin();

    for(; mit != mPhotos.end(); ++mit)
    {
        delete mit->second;
    }

    delete(mPhotoQueue);
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

	++mImageIdx;
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
		ui.pushButton_StartStop->setText(tr("Start"));
		ui.pushButton_StartStop->setToolTip(tr("Start Slide Show"));
	}
	else
	{
		mRunning = true;
		ui.pushButton_StartStop->setText(tr("Stop"));
		ui.pushButton_StartStop->setToolTip(tr("Stop Slide Show"));
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

	++mImageIdx;
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
    RsGxsMessageId msgId;

	//std::map<std::string, RsPhotoPhoto *>::iterator it;
    std::map<int, RsGxsMessageId>::iterator it;
	for(it = mPhotoOrder.begin(); it != mPhotoOrder.end(); ++it, ++i)
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
		if (ptr->mThumbnail.mData != NULL)
        	{
                	QPixmap qtn;
                        GxsIdDetails::loadPixmapFromData(ptr->mThumbnail.mData, ptr->mThumbnail.mSize,qtn, GxsIdDetails::ORIGINAL);
                        QPixmap sqtn = qtn.scaled(800, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation);
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

void PhotoSlideShow::requestPhotos()
{
    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
    uint32_t token;
    std::list<RsGxsGroupId> grpIds;
    grpIds.push_back(mAlbum.mMeta.mGroupId);
    mPhotoQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, grpIds, 0);
}

bool PhotoSlideShow::loadPhotoData(const uint32_t &token)
{
	std::cerr << "PhotoSlideShow::loadPhotoData()";
	std::cerr << std::endl;
	
        PhotoResult res;
        rsPhoto->getPhoto(token, res);
        PhotoResult::iterator mit = res.begin();


        for(; mit != res.end(); ++mit)
        {
            std::vector<RsPhotoPhoto>& photoV = mit->second;
            std::vector<RsPhotoPhoto>::iterator vit = photoV.begin();
            int i = 0;
            for(; vit != photoV.end(); ++vit)
            {
                RsPhotoPhoto& photo = *vit;
                RsPhotoPhoto *ptr = new RsPhotoPhoto;
                *ptr = photo;

                ptr->mThumbnail = photo.mThumbnail; // copies data.
                ptr->mOrder = i++;
                mPhotos[photo.mMeta.mMsgId] = ptr;
                mPhotoOrder[ptr->mOrder] = photo.mMeta.mMsgId;

                std::cerr << "PhotoSlideShow::addAddPhoto() AlbumId: " << photo.mMeta.mGroupId;
                std::cerr << " PhotoId: " << photo.mMeta.mMsgId;
                std::cerr << std::endl;
            }
        }

        

	// Load and Start.
	loadImage();
        QTimer::singleShot(5000, this, SLOT(timerEvent()));

	return true;
}

void PhotoSlideShow::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "PhotoSlideShow::loadRequest()";
	std::cerr << std::endl;
		
	if (queue == mPhotoQueue)
	{
		/* now switch on req */
		switch(req.mType)
		{
			case TOKENREQ_MSGINFO:
				loadPhotoData(req.mToken);
				break;
			default:
                                std::cerr << "PhotoSlideShow::loadRequest() ERROR: REQ: INVALID REQ TYPE";
				std::cerr << std::endl;
				break; 
		}
	}
}

void PhotoSlideShow::setFullScreen()
{
  if (!isFullScreen()) {
    // hide menu & toolbars

#ifdef Q_OS_LINUX
    show();
    raise();
    setWindowState( windowState() | Qt::WindowFullScreen );
#else
    setWindowState( windowState() | Qt::WindowFullScreen );
    show();
    raise();
#endif
  } else {

    setWindowState( windowState() ^ Qt::WindowFullScreen );
    show();
  }
}


