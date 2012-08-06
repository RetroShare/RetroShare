/*
 * libretroshare/src/services p3photoservice.cc
 *
 * Photo Service for RetroShare.
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

#include "services/p3photoservice.h"
#include "util/rsrandom.h"

/****
 * #define PHOTO_DEBUG 1
 ****/

RsPhoto *rsPhoto = NULL;


/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3PhotoService::p3PhotoService(uint16_t type)
	:p3GxsDataService(type, new PhotoDataProxy()), mPhotoMtx("p3PhotoService"), mUpdated(true)
{
     	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	mPhotoProxy = (PhotoDataProxy *) mProxy;
	return;
}


int	p3PhotoService::tick()
{
	//std::cerr << "p3PhotoService::tick()";
	//std::cerr << std::endl;

	fakeprocessrequests();
	
	return 0;
}

bool 	p3PhotoService::updated()
{
	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/
	
	if (mUpdated)
	{
		mUpdated = false;
		return true;
	}
	return false;
}


       /* Data Requests */
bool p3PhotoService::requestGroupInfo(     uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds)
{
	generateToken(token);
	std::cerr << "p3PhotoService::requestGroupInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_GROUPS, groupIds);

	return true;
}

bool p3PhotoService::requestMsgInfo(       uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds)
{
	generateToken(token);
	std::cerr << "p3PhotoService::requestMsgInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGS, groupIds);

	return true;
}

bool p3PhotoService::requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &msgIds)
{
	generateToken(token);
	std::cerr << "p3PhotoService::requestMsgRelatedInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGRELATED, msgIds);

	return true;
}

        /* Generic Lists */
bool p3PhotoService::getGroupList(         const uint32_t &token, std::list<std::string> &groupIds)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_LIST)
	{
		std::cerr << "p3PhotoService::getGroupList() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3PhotoService::getGroupList() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3PhotoService::getGroupList() ERROR Status Incomplete" << std::endl;
		return false;
	}

	bool ans = loadRequestOutList(token, groupIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	return ans;
}




bool p3PhotoService::getMsgList(           const uint32_t &token, std::list<std::string> &msgIds)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_LIST)
	{
		std::cerr << "p3PhotoService::getMsgList() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3PhotoService::getMsgList() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3PhotoService::getMsgList() ERROR Status Incomplete" << std::endl;
		return false;
	}

	bool ans = loadRequestOutList(token, msgIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	return ans;
}


        /* Generic Summary */
bool p3PhotoService::getGroupSummary(      const uint32_t &token, std::list<RsGroupMetaData> &groupInfo)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_SUMMARY)
	{
		std::cerr << "p3PhotoService::getGroupSummary() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3PhotoService::getGroupSummary() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3PhotoService::getGroupSummary() ERROR Status Incomplete" << std::endl;
		return false;
	}

	std::list<std::string> groupIds;
	bool ans = loadRequestOutList(token, groupIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	/* convert to RsGroupMetaData */
	mProxy->getGroupSummary(groupIds, groupInfo);

	return ans;
}

bool p3PhotoService::getMsgSummary(        const uint32_t &token, std::list<RsMsgMetaData> &msgInfo)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_SUMMARY)
	{
		std::cerr << "p3PhotoService::getMsgSummary() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3PhotoService::getMsgSummary() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3PhotoService::getMsgSummary() ERROR Status Incomplete" << std::endl;
		return false;
	}

	std::list<std::string> msgIds;
	bool ans = loadRequestOutList(token, msgIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	/* convert to RsMsgMetaData */
	mProxy->getMsgSummary(msgIds, msgInfo);

	return ans;
}


        /* Specific Service Data */
bool p3PhotoService::getAlbum(const uint32_t &token, RsPhotoAlbum &album)
{
	std::cerr << "p3PhotoService::getAlbum() Token: " << token;
	std::cerr << std::endl;

	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);
	

	if (anstype != RS_TOKREQ_ANSTYPE_DATA)
	{
		std::cerr << "p3PhotoService::getAlbum() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3PhotoService::getAlbum() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3PhotoService::getAlbum() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestOutList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	/* convert to RsPhotoAlbum */
	bool ans = mPhotoProxy->getAlbum(id, album);
	return ans;
}


bool p3PhotoService::getPhoto(const uint32_t &token, RsPhotoPhoto &photo)
{
	std::cerr << "p3PhotoService::getPhoto() Token: " << token;
	std::cerr << std::endl;

	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);
	

	if (anstype != RS_TOKREQ_ANSTYPE_DATA)
	{
		std::cerr << "p3PhotoService::getPhoto() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3PhotoService::getPhoto() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3PhotoService::getPhoto() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestOutList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	/* convert to RsPhotoAlbum */
	bool ans = mPhotoProxy->getPhoto(id, photo);
	return ans;
}



        /* Poll */
uint32_t p3PhotoService::requestStatus(const uint32_t token)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	return status;
}


        /* Cancel Request */
bool p3PhotoService::cancelRequest(const uint32_t &token)
{
	return clearRequest(token);
}


bool p3PhotoService::setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask)
{
	return mPhotoProxy->setMessageStatus(msgId, status, statusMask);
}

bool p3PhotoService::setGroupStatus(const std::string &groupId, const uint32_t status, const uint32_t statusMask)
{
	return mPhotoProxy->setGroupStatus(groupId, status, statusMask);
}

bool p3PhotoService::setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask)
{
        return mPhotoProxy->setGroupSubscribeFlags(groupId, subscribeFlags, subscribeMask);
}

bool p3PhotoService::setMessageServiceString(const std::string &msgId, const std::string &str)
{
	return mPhotoProxy->setMessageServiceString(msgId, str);
}

bool p3PhotoService::setGroupServiceString(const std::string &grpId, const std::string &str)
{
	return mPhotoProxy->setGroupServiceString(grpId, str);
}


bool p3PhotoService::groupRestoreKeys(const std::string &groupId)
{
	return false;
}

bool p3PhotoService::groupShareKeys(const std::string &groupId, std::list<std::string>& peers)
{
	return false;
}


/* details are updated in album - to choose Album ID, and storage path */

bool p3PhotoService::submitAlbumDetails(uint32_t &token, RsPhotoAlbum &album, bool isNew)
{
	/* check if its a modification or a new album */

	/* add to database */

	/* check if its a mod or new photo */
	if (album.mMeta.mGroupId.empty())
	{
		/* new photo */

		/* generate a temp id */
		album.mMeta.mGroupId = genRandomId();
		// TODO.
		//album.mMeta.mPublishTs = time(NULL);

		std::cerr << "p3PhotoService::submitAlbumDetails() Generated New GroupID: " << album.mMeta.mGroupId;
		std::cerr << std::endl;
	}

	album.mModFlags = 0; // These are always cleared.

	{
		RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

		mUpdated = true;

		/* add / modify */
		mPhotoProxy->addAlbum(album);
	}

	// Fake a request to return the GroupMetaData.
	generateToken(token);
	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptions opts; // NULL is good.
	std::list<std::string> groupIds;  
	groupIds.push_back(album.mMeta.mGroupId); // It will just return this one.

	std::cerr << "p3PhotoService::submitAlbumDetails() Generating Request Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_GROUPS, groupIds);

	return true;
}

bool p3PhotoService::submitPhoto(uint32_t &token, RsPhotoPhoto &photo, bool isNew)
{
	if (photo.mMeta.mGroupId.empty())
	{
		/* new photo */
		std::cerr << "p3PhotoService::submitPhoto() Missing GroupID: ERROR";
		std::cerr << std::endl;
		return false;
	}
	
	/* generate a new id */
	photo.mMeta.mMsgId = genRandomId();
	photo.mMeta.mPublishTs = time(NULL);

	if (isNew)
	{
		/* new (Original Msg) photo */
		photo.mMeta.mOrigMsgId = photo.mMeta.mMsgId; 
		std::cerr << "p3PhotoService::submitPhoto() New Msg";
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "p3PhotoService::submitPhoto() Updated Msg";
		std::cerr << std::endl;
	}

	photo.mModFlags = 0; // These are always cleared.

	std::cerr << "p3PhotoService::submitPhoto() OrigMsgId: " << photo.mMeta.mOrigMsgId;
	std::cerr << " MsgId: " << photo.mMeta.mMsgId;
	std::cerr << std::endl;

	{
		RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

		mUpdated = true;
		mPhotoProxy->addPhoto(photo);
	}

	// Fake a request to return the MsgMetaData.
	generateToken(token);
	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptions opts; // NULL is good.
	std::list<std::string> msgIds;  
	msgIds.push_back(photo.mMeta.mMsgId); // It will just return this one.

	std::cerr << "p3PhotoService::submitPhoto() Generating Request Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGRELATED, msgIds);

	return true;
}



/********************************************************************************************/

bool PhotoDataProxy::getAlbum(const std::string &id, RsPhotoAlbum &album)
{
	void *groupData = NULL;
	RsGroupMetaData meta;
	if (getGroupData(id, groupData) && getGroupSummary(id, meta))
	{
		RsPhotoAlbum *pA = (RsPhotoAlbum *) groupData;
		// Shallow copy of thumbnail.
		album = *pA;

		// update definitive version of the metadata.
		album.mMeta = meta;

		std::cerr << "PhotoDataProxy::getAlbum() Id: " << id;
		std::cerr << " MetaData: " << meta << " DataPointer: " << groupData;
		std::cerr << std::endl;
		return true;
	}

	std::cerr << "PhotoDataProxy::getAlbum() FAILED Id: " << id;
	std::cerr << std::endl;

	return false;
}

bool PhotoDataProxy::getPhoto(const std::string &id, RsPhotoPhoto &photo)
{
	void *msgData = NULL;
	RsMsgMetaData meta;
	if (getMsgData(id, msgData) && getMsgSummary(id, meta))
	{
		RsPhotoPhoto *pP = (RsPhotoPhoto *) msgData;
		// Shallow copy of thumbnail.
		photo = *pP;
	
		// update definitive version of the metadata.
		photo.mMeta = meta;

		std::cerr << "PhotoDataProxy::getPhoto() Id: " << id;
		std::cerr << " MetaData: " << meta << " DataPointer: " << msgData;
		std::cerr << std::endl;
		return true;
	}

	std::cerr << "PhotoDataProxy::getPhoto() FAILED Id: " << id;
	std::cerr << std::endl;

	return false;
}



bool PhotoDataProxy::addAlbum(const RsPhotoAlbum &album)
{
	// Make duplicate.
	RsPhotoAlbum *pA = new RsPhotoAlbum();
	*pA = album;

	std::cerr << "PhotoDataProxy::addAlbum()";
	std::cerr << " MetaData: " << pA->mMeta << " DataPointer: " << pA;
	std::cerr << std::endl;

	// deep copy thumbnail.
	pA->mThumbnail.data = NULL;
	pA->mThumbnail.copyFrom(album.mThumbnail);

	return createGroup(pA);
}


bool PhotoDataProxy::addPhoto(const RsPhotoPhoto &photo)
{
	// Make duplicate.
	RsPhotoPhoto *pP = new RsPhotoPhoto();
	*pP = photo;

	std::cerr << "PhotoDataProxy::addPhoto()";
	std::cerr << " MetaData: " << pP->mMeta << " DataPointer: " << pP;
	std::cerr << std::endl;

	// deep copy thumbnail.
	pP->mThumbnail.data = NULL;
	pP->mThumbnail.copyFrom(photo.mThumbnail);

	return createMsg(pP);
}



        /* These Functions must be overloaded to complete the service */
bool PhotoDataProxy::convertGroupToMetaData(void *groupData, RsGroupMetaData &meta)
{
	RsPhotoAlbum *album = (RsPhotoAlbum *) groupData;
	meta = album->mMeta;

	return true;
}

bool PhotoDataProxy::convertMsgToMetaData(void *msgData, RsMsgMetaData &meta)
{
	RsPhotoPhoto *photo = (RsPhotoPhoto *) msgData;
	meta = photo->mMeta;

	return true;
}


/********************************************************************************************/

std::string p3PhotoService::genRandomId()
{
	std::string randomId;
	for(int i = 0; i < 20; i++)
	{
		randomId += (char) ('a' + (RSRandom::random_u32() % 26));
	}
	
	return randomId;
}
	
	
bool RsPhotoThumbnail::copyFrom(const RsPhotoThumbnail &nail)
{
	if (data)
	{
		deleteImage();
	}

	if ((!nail.data) || (nail.size == 0))
	{
		return false;
	}

	size = nail.size;
	type = nail.type;
	data = (uint8_t *) malloc(size);
	memcpy(data, nail.data, size);

	return true;
}

bool RsPhotoThumbnail::deleteImage()
{
	if (data)
	{
		free(data);
		data = NULL;
		size = 0;
                type.clear();
	}
	return true;
}



/********************************************************************************************/

RsPhotoPhoto::RsPhotoPhoto()
	:mSetFlags(0), mOrder(0), mMode(0), mModFlags(0)
{
	return;
}

RsPhotoAlbum::RsPhotoAlbum()
	:mMode(0), mSetFlags(0), mModFlags(0)
{
	return;
}

std::ostream &operator<<(std::ostream &out, const RsPhotoPhoto &photo)
{
	out << "RsPhotoPhoto [ ";
	out << "Title: " << photo.mMeta.mMsgName;
	out << "]";
	return out;
}


std::ostream &operator<<(std::ostream &out, const RsPhotoAlbum &album)
{
	out << "RsPhotoAlbum [ ";
	out << "Title: " << album.mMeta.mGroupName;
	out << "]";
	return out;
}


