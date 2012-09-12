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

#include "PhotoDialog.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsphotoV2.h>

#include <iostream>
#include <sstream>

#include <QTimer>
#include <QMessageBox>

#include "AlbumCreateDialog.h"
#include "AlbumItem.h"

/******
 * #define PHOTO_DEBUG 1
 *****/


/****************************************************************
 * New Photo Display Widget.
 *
 * This has two 'lists'.
 * Top list shows Albums.
 * Lower list is photos from the selected Album.
 * 
 * Notes:
 *   Each Item will be an AlbumItem, which contains a thumbnail & random details.
 *   We will limit Items to < 100. With a 'Filter to see more message.
 * 
 *   Thumbnails will come from Service.
 *   Option to Share albums / pictures onward (if permissions allow).
 *   Option to Download the albums to a specified directory. (is this required if sharing an album?)
 *
 *   Will introduce a FullScreen SlideShow later... first get basics happening.
 */




/** Constructor */
PhotoDialog::PhotoDialog(QWidget *parent)
: MainPage(parent)
{
	ui.setupUi(this);

        mAlbumSelected = NULL;
        mPhotoSelected = NULL;
        mSlideShow = NULL;

	connect( ui.toolButton_NewAlbum, SIGNAL(clicked()), this, SLOT(OpenOrShowPhotoAddDialog()));
        connect( ui.toolButton_ViewAlbum, SIGNAL(clicked()), this, SLOT(OpenPhotoEditDialog()));
	connect( ui.toolButton_SlideShow, SIGNAL(clicked()), this, SLOT(OpenSlideShow()));

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
	timer->start(1000);


	/* setup TokenQueue */
        mPhotoQueue = new TokenQueueV2(rsPhotoV2->getTokenService(), this);

}



void PhotoDialog::checkUpdate()
{
	/* update */
        if (!rsPhotoV2)
		return;

        if (rsPhotoV2->updated())
	{
		//insertAlbums();
            std::list<std::string> grpIds;
            rsPhotoV2->groupsChanged(grpIds);
            if(!grpIds.empty())
                requestAlbumList(grpIds);

            GxsMsgIdResult res;
            rsPhotoV2->msgsChanged(res);
            if(!res.empty())
                requestPhotoList(res);
	}

	return;
}


/*************** New Photo Dialog ***************/

void PhotoDialog::OpenSlideShow()
{

	// TODO.
	if (!mAlbumSelected)
	{
		// ALERT. 
	 	int ret = QMessageBox::information(this, tr("PhotoShare"),
                                tr("Please select an album before\n"
                                   "requesting to edit it!"), 
                                QMessageBox::Ok);
		return;
	}

        std::string albumId = mAlbumSelected->getAlbum().mMeta.mGroupId;

	if (mSlideShow)
	{
		mSlideShow->show();
	}
	else
	{
		mSlideShow = new PhotoSlideShow(NULL);
		mSlideShow->show();
	}
	mSlideShow->loadAlbum(albumId);

}


/*************** New Photo Dialog ***************/

void PhotoDialog::createAlbum()
{
    AlbumCreateDialog albumCreate(mPhotoQueue, rsPhotoV2, this);
    albumCreate.exec();
}

void PhotoDialog::OpenPhotoEditDialog()
{

}

/*************** Edit Photo Dialog ***************/


bool PhotoDialog::matchesAlbumFilter(const RsPhotoAlbum &album)
{

	return true;
}

double PhotoDialog::AlbumScore(const RsPhotoAlbum &album)
{
	return 1;
}


bool PhotoDialog::matchesPhotoFilter(const RsPhotoPhoto &photo)
{

	return true;
}

double PhotoDialog::PhotoScore(const RsPhotoPhoto &photo)
{
	return 1;
}

void PhotoDialog::insertPhotosForSelectedAlbum()
{
	std::cerr << "PhotoDialog::insertPhotosForSelectedAlbum()";
	std::cerr << std::endl;

	clearPhotos();

	//std::list<std::string> albumIds;
	if (mAlbumSelected)
	{
                std::string albumId = mAlbumSelected->getAlbum().mMeta.mGroupId;
		//albumIds.push_back(albumId);

		std::cerr << "PhotoDialog::insertPhotosForSelectedAlbum() AlbumId: " << albumId;
		std::cerr << std::endl;
		requestPhotoList(albumId);
	}
	//requestPhotoList(albumIds);
}


void PhotoDialog::clearAlbums()
{

    std::cerr << "PhotoDialog::clearAlbums()" << std::endl;


}

void PhotoDialog::clearPhotos()
{
	std::cerr << "PhotoDialog::clearPhotos()" << std::endl;


	mPhotoSelected = NULL;
	
	
}

void PhotoDialog::addAlbum(const RsPhotoAlbum &album)
{
	std::cerr << " PhotoDialog::addAlbum() AlbumId: " << album.mMeta.mGroupId << std::endl;

        AlbumItem *item = new AlbumItem(album, this);
	QLayout *alayout = ui.scrollAreaWidgetContents->layout();
	alayout->addWidget(item);
}


void PhotoDialog::addPhoto(const RsPhotoPhoto &photo)
{
	std::cerr << "PhotoDialog::addPhoto() AlbumId: " << photo.mMeta.mGroupId;
	std::cerr << " PhotoId: " << photo.mMeta.mMsgId;
	std::cerr << std::endl;

}

/**************************** Request / Response Filling of Data ************************/


void PhotoDialog::requestAlbumList(std::list<std::string>& ids)
{
	RsTokReqOptionsV2 opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_IDS;
	uint32_t token;
	mPhotoQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_LIST, opts, ids, 0);
}

void PhotoDialog::requestPhotoList(GxsMsgReq& req)
{
    RsTokReqOptionsV2 opts;
    opts.mReqType = GXS_REQUEST_TYPE_MSG_IDS;
    uint32_t token;
    mPhotoQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_LIST, opts, req, 0);
    return;
}


void PhotoDialog::loadAlbumList(const uint32_t &token)
{
	std::cerr << "PhotoDialog::loadAlbumList()";
	std::cerr << std::endl;

	std::list<std::string> albumIds;
        rsPhotoV2->getGroupList(token, albumIds);

	requestAlbumData(albumIds);

	clearPhotos();

	std::list<std::string>::iterator it;
	for(it = albumIds.begin(); it != albumIds.end(); it++)
	{
		requestPhotoList(*it);
	}
}


void PhotoDialog::requestAlbumData(std::list<std::string> &ids)
{
	RsTokReqOptionsV2 opts;
	uint32_t token;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
        mPhotoQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, ids, 0);
}


bool PhotoDialog::loadAlbumData(const uint32_t &token)
{
    std::cerr << "PhotoDialog::loadAlbumData()";
    std::cerr << std::endl;

    std::vector<RsPhotoAlbum> albums;
    rsPhotoV2->getAlbum(token, albums);

    std::vector<RsPhotoAlbum>::iterator vit = albums.begin();

    for(; vit != albums.end(); vit++)
    {
        RsPhotoAlbum& album = *vit;

        std::cerr << " PhotoDialog::addAlbum() AlbumId: " << album.mMeta.mGroupId << std::endl;

        AlbumItem *item = new AlbumItem(album, this);
        QLayout *alayout = ui.scrollAreaWidgetContents->layout();
        alayout->addWidget(item);
    }
    return true;
}


void PhotoDialog::requestPhotoList(const std::string &albumId)
{

    std::list<RsGxsGroupId> grpIds;
    grpIds.push_back(albumId);
    RsTokReqOptionsV2 opts;
    opts.mReqType = GXS_REQUEST_TYPE_MSG_IDS;
    opts.mOptions = RS_TOKREQOPT_MSG_LATEST;
    uint32_t token;
    mPhotoQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_LIST, opts, grpIds, 0);
}


void PhotoDialog::acknowledgeGroup(const uint32_t &token)
{
    RsGxsGroupId grpId;
    rsPhotoV2->acknowledgeGrp(token, grpId);

    if(!grpId.empty())
    {
        std::list<RsGxsGroupId> grpIds;
        grpIds.push_back(grpId);

        RsTokReqOptionsV2 opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
        uint32_t reqToken;
        mPhotoQueue->requestGroupInfo(reqToken, RS_TOKREQ_ANSTYPE_DATA, opts, grpIds, 0);
    }
}

void PhotoDialog::acknowledgeMessage(const uint32_t &token)
{
    std::pair<RsGxsGroupId, RsGxsMessageId> p;
    rsPhotoV2->acknowledgeMsg(token, p);

    if(!p.first.empty())
    {
        GxsMsgReq req;
        std::vector<RsGxsMessageId> v;
        v.push_back(p.second);
        req[p.first] = v;
        RsTokReqOptionsV2 opts;
        opts.mOptions = RS_TOKREQOPT_MSG_LATEST;
        opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
        uint32_t reqToken;
        mPhotoQueue->requestMsgInfo(reqToken, RS_TOKREQ_ANSTYPE_DATA, opts, req, 0);
    }
}

void PhotoDialog::loadPhotoList(const uint32_t &token)
{
	std::cerr << "PhotoDialog::loadPhotoList()";
	std::cerr << std::endl;

        GxsMsgIdResult res;

        rsPhotoV2->getMsgList(token, res);
        requestPhotoData(res);
}


void PhotoDialog::requestPhotoData(GxsMsgReq &photoIds)
{
	RsTokReqOptionsV2 opts;
        opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
	uint32_t token;
        mPhotoQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, photoIds, 0);
}


void PhotoDialog::loadPhotoData(const uint32_t &token)
{
	std::cerr << "PhotoDialog::loadPhotoData()";
	std::cerr << std::endl;

        PhotoResult res;
        rsPhotoV2->getPhoto(token, res);
        PhotoResult::iterator mit = res.begin();


        for(; mit != res.end(); mit++)
        {
            std::vector<RsPhotoPhoto>& photoV = mit->second;
            std::vector<RsPhotoPhoto>::iterator vit = photoV.begin();

            for(; vit != photoV.end(); vit++)
            {
                RsPhotoPhoto& photo = *vit;
                addPhoto(photo);
                std::cerr << "PhotoDialog::loadPhotoData() AlbumId: " << photo.mMeta.mGroupId;
                std::cerr << " PhotoId: " << photo.mMeta.mMsgId;
                std::cerr << std::endl;
            }
        }
}


/**************************** Request / Response Filling of Data ************************/

void PhotoDialog::loadRequest(const TokenQueueV2 *queue, const TokenRequestV2 &req)
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
					case RS_TOKREQ_ANSTYPE_LIST:
						loadAlbumList(req.mToken);
						break;
					case RS_TOKREQ_ANSTYPE_DATA:
						loadAlbumData(req.mToken);
						break;
                                        case RS_TOKREQ_ANSTYPE_ACK:
                                                acknowledgeGroup(req.mToken);
                                                break;
					default:
						std::cerr << "PhotoDialog::loadRequest() ERROR: GROUP: INVALID ANS TYPE";
						std::cerr << std::endl;
						break;
				}
				break;
			case TOKENREQ_MSGINFO:
				switch(req.mAnsType)
				{
					case RS_TOKREQ_ANSTYPE_LIST:
						loadPhotoList(req.mToken);
						break;
                                        case RS_TOKREQ_ANSTYPE_ACK:
                                                acknowledgeMessage(req.mToken);
                                                break;
                                        case RS_TOKREQ_ANSTYPE_DATA:
                                                loadPhotoData(req.mToken);
                                                break;
					default:
						std::cerr << "PhotoDialog::loadRequest() ERROR: MSG: INVALID ANS TYPE";
						std::cerr << std::endl;
						break;
				}
				break;
			case TOKENREQ_MSGRELATEDINFO:
				switch(req.mAnsType)
				{
					case RS_TOKREQ_ANSTYPE_DATA:
						loadPhotoData(req.mToken);
						break;
					default:
						std::cerr << "PhotoDialog::loadRequest() ERROR: MSG: INVALID ANS TYPE";
						std::cerr << std::endl;
						break;
				}
				break;
			default:
				std::cerr << "PhotoDialog::loadRequest() ERROR: INVALID TYPE";
				std::cerr << std::endl;
				break;
		}
	}
}


/**************************** Request / Response Filling of Data ************************/

