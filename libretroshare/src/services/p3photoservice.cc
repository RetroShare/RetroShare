/*
 * libretroshare/src/services p3photoservice.cc
 *
 * Phot Service for RetroShare.
 *
 * Copyright 2007-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
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


#if 0
PhotoAlbum::PhotoAlbum()
{
	return;
}
#endif

#define PHOTO_REQUEST_ALBUMLIST		0x0001
#define PHOTO_REQUEST_PHOTOLIST		0x0002
#define PHOTO_REQUEST_ALBUMS		0x0004
#define PHOTO_REQUEST_PHOTOS		0x0008

/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3PhotoService::p3PhotoService(uint16_t type)
	:p3GxsService(type), mPhotoMtx("p3PhotoService"), mUpdated(true)
{
     	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/
	return;
}


int	p3PhotoService::tick()
{
	std::cerr << "p3PhotoService::tick()";
	std::cerr << std::endl;

	fakeprocessrequests();
	
	return 0;
}

bool p3PhotoService::updated()
{
	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	if (mUpdated)
	{
		mUpdated = false;
		return true;
	}
	return false;
}


bool p3PhotoService::requestGroupList(     uint32_t &token, const RsTokReqOptions &opts)
{

	generateToken(token);
	std::cerr << "p3PhotoService::requestGroupList() gets Token: " << token << std::endl;

	std::list<std::string> ids;
	storeRequest(token, GXS_REQUEST_TYPE_LIST | GXS_REQUEST_TYPE_GROUPS | PHOTO_REQUEST_ALBUMLIST, ids);

	return true;
}

bool p3PhotoService::requestMsgList(       uint32_t &token, const RsTokReqOptions &opts, const std::list<std::string> &groupIds)
{

	generateToken(token);
	std::cerr << "p3PhotoService::requestMsgList() gets Token: " << token << std::endl;
	storeRequest(token, GXS_REQUEST_TYPE_LIST | GXS_REQUEST_TYPE_MSGS | PHOTO_REQUEST_PHOTOLIST, groupIds);

	return true;
}

bool p3PhotoService::requestMsgRelatedList(       uint32_t &token, const RsTokReqOptions &opts, const std::list<std::string> &groupIds)
{
	std::cerr << "p3PhotoService::requestMsgRelatedList() gets Token: " << token << std::endl;
	return false;
}

bool p3PhotoService::requestGroupData(uint32_t &token, const std::list<std::string> &albumids)
{
	generateToken(token);
	std::cerr << "p3PhotoService::requestGroupData() gets Token: " << token << std::endl;
	storeRequest(token, GXS_REQUEST_TYPE_DATA | GXS_REQUEST_TYPE_GROUPS | PHOTO_REQUEST_ALBUMS, albumids);

	return true;
}

bool p3PhotoService::requestMsgData(uint32_t &token, const std::list<std::string> &photoids)
{

	generateToken(token);
	std::cerr << "p3PhotoService::requestMsgData() gets Token: " << token << std::endl;
	storeRequest(token, GXS_REQUEST_TYPE_DATA | GXS_REQUEST_TYPE_MSGS | PHOTO_REQUEST_PHOTOS, photoids);

	return true;
}


bool p3PhotoService::getGroupList(const uint32_t &token, std::list<std::string> &groupIds)
{
	uint32_t status;
	uint32_t reqtype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, ts);
	
	if (reqtype != (GXS_REQUEST_TYPE_LIST | GXS_REQUEST_TYPE_GROUPS | PHOTO_REQUEST_ALBUMLIST))
	{
		std::cerr << "p3PhotoService::getGroupList() ERROR Type Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3PhotoService::getGroupList() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	bool ans = InternalgetAlbumList(groupIds);
	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
	
	return ans;
}

bool p3PhotoService::getMsgList(const uint32_t &token, std::list<std::string> &msgIds)
{
	uint32_t status;
	uint32_t reqtype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, ts);
	
	if (reqtype != (GXS_REQUEST_TYPE_LIST | GXS_REQUEST_TYPE_MSGS | PHOTO_REQUEST_PHOTOLIST))
	{
		std::cerr << "p3PhotoService::getMsgList() ERROR Type Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3PhotoService::getMsgList() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	bool ans = InternalgetPhotoList(id, msgIds);

	// Only one Album at a time -> so finish it!	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
	
	return ans;
}

bool p3PhotoService::getAlbum(const uint32_t &token, RsPhotoAlbum &album)
{
	uint32_t status;
	uint32_t reqtype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, ts);
	
	if (reqtype != (GXS_REQUEST_TYPE_DATA | GXS_REQUEST_TYPE_GROUPS | PHOTO_REQUEST_ALBUMS))
	{
		std::cerr << "p3PhotoService::getAlbum() ERROR Type Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3PhotoService::getAlbum() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	bool ans = InternalgetAlbum(id, album);
	return ans;

}

bool p3PhotoService::getPhoto(const uint32_t &token, RsPhotoPhoto &photo)
{
	uint32_t status;
	uint32_t reqtype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, ts);
	
	if (reqtype != (GXS_REQUEST_TYPE_DATA | GXS_REQUEST_TYPE_MSGS | PHOTO_REQUEST_PHOTOS))
	{
		std::cerr << "p3PhotoService::getPhoto() ERROR Type Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3PhotoService::getPhoto() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	bool ans = InternalgetPhoto(id, photo);
	return ans;
}

        /* Poll */
uint32_t p3PhotoService::requestStatus(const uint32_t token)
{
	uint32_t status;
	uint32_t reqtype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, ts);

	return status;
}


#if 0
#define MAX_REQUEST_AGE 60

bool p3PhotoService::fakeprocessrequests()
	{
	std::list<uint32_t>::iterator it;
	std::list<uint32_t> tokens;
	
	tokenList(tokens);
	
	time_t now = time(NULL);
	for(it = tokens.begin(); it != tokens.end(); it++)
	{
		uint32_t status;
		uint32_t reqtype;
		uint32_t token = *it;
		time_t   ts;
		checkRequestStatus(token, status, reqtype, ts);
		
		std::cerr << "p3PhotoService::fakeprocessrequests() Token: " << token << " Status: " << status << " ReqType: " << reqtype << "Age: " << now - ts << std::endl;
		
		if (status == GXS_REQUEST_STATUS_PENDING)
		{
			updateRequestStatus(token, GXS_REQUEST_STATUS_PARTIAL);
		}
		else if (status == GXS_REQUEST_STATUS_PARTIAL)
		{
			updateRequestStatus(token, GXS_REQUEST_STATUS_COMPLETE);
		}
		else if (status == GXS_REQUEST_STATUS_DONE)
		{
			std::cerr << "p3PhotoService::fakeprocessrequests() Clearing Done Request Token: " << token;
			std::cerr << std::endl;
			clearRequest(token);
		}
		else if (now - ts > MAX_REQUEST_AGE)
		{
			std::cerr << "p3PhotoService::fakeprocessrequests() Clearing Old Request Token: " << token;
			std::cerr << std::endl;
			clearRequest(token);
		}
	}
	
	return true;
}
#endif



/******************* INTERNALS UNTIL ITS PROPERLY LINKED IN ************************/

bool p3PhotoService::InternalgetAlbumList(std::list<std::string> &album)
{
	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string, RsPhotoAlbum>::iterator it;
	for(it = mAlbums.begin(); it != mAlbums.end(); it++)
	{
		album.push_back(it->second.mAlbumId);
	}
	
	return false;
}

bool p3PhotoService::InternalgetAlbum(const std::string &albumid, RsPhotoAlbum &album)
{
	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/
	
	std::map<std::string, RsPhotoAlbum>::iterator it;
	it = mAlbums.find(albumid);
	if (it == mAlbums.end())
	{
		return false;
	}
	
	album = it->second;
	return true;
}

bool p3PhotoService::InternalgetPhoto(const std::string &photoid, RsPhotoPhoto &photo)
{
	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/
	
	std::map<std::string, RsPhotoPhoto>::iterator it;
	it = mPhotos.find(photoid);
	if (it == mPhotos.end())
	{
		return false;
	}
	
	photo = it->second;
	return true;
}

bool p3PhotoService::InternalgetPhotoList(const std::string &albumid, std::list<std::string> &photoIds)
{
	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string, std::list<std::string> >::iterator it;
	it = mAlbumToPhotos.find(albumid);
	if (it == mAlbumToPhotos.end())
	{
		return false;
	}
	
	std::list<std::string>::iterator lit;
	for(lit = it->second.begin(); lit != it->second.end(); lit++)
	{
		photoIds.push_back(*lit);
	}	
	return true;
}

/* details are updated in album - to choose Album ID, and storage path */
bool p3PhotoService::submitAlbumDetails(RsPhotoAlbum &album)
{
	/* check if its a modification or a new album */


	/* add to database */

	/* check if its a mod or new photo */
	if (album.mId.empty())
	{
		/* new photo */

		/* generate a temp id */
		album.mAlbumId = genRandomId();
	}

	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	mUpdated = true;

	/* add / modify */
	mAlbums[album.mAlbumId] = album;

	return true;
}


bool p3PhotoService::submitPhoto(RsPhotoPhoto &photo)
{
	if (photo.mAlbumId.empty())
	{
		/* new photo */
		std::cerr << "p3PhotoService::submitPhoto() Missing AlbumID: ERROR";
		std::cerr << std::endl;
		return false;
	}
	
	/* check if its a mod or new photo */
	if (photo.mId.empty())
	{
		/* new photo */

		/* generate a temp id */
		photo.mId = genRandomId();
	}

	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	mUpdated = true;

	std::map<std::string, std::list<std::string> >::iterator it;
	it = mAlbumToPhotos.find(photo.mAlbumId);
	if (it == mAlbumToPhotos.end())
	{
		std::list<std::string> emptyList;
		mAlbumToPhotos[photo.mAlbumId] = emptyList;

		it = mAlbumToPhotos.find(photo.mAlbumId);
	}
	
	it->second.push_back(photo.mId);
		
	/* add / modify */
	mPhotos[photo.mId] = photo;

	return true;
}

#if 0
bool p3PhotoService::getPhotoThumbnail(const std::string &photoid, RsPhotoThumbnail &thumbnail)
{
	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/
	
	std::map<std::string, RsPhotoThumbnail *>::iterator it;
	it = mPhotoThumbnails.find(photoid);
	if (it == mPhotoThumbnails.end())
	{
		return false;
	}
	
	// shallow copy??? dangerous!
	thumbnail = *(it->second);
	return true;
}

bool p3PhotoService::getAlbumThumbnail(const std::string &albumid, RsPhotoThumbnail &thumbnail)
{
	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/
	
	std::map<std::string, RsPhotoThumbnail *>::iterator it;
	it = mAlbumThumbnails.find(albumid);
	if (it == mAlbumThumbnails.end())
	{
		return false;
	}
	
	// shallow copy??? dangerous!
	thumbnail = *(it->second);
	return true;
}

/* details are updated in album - to choose Album ID, and storage path */
bool p3PhotoService::submitAlbumDetails(RsPhotoAlbum &album, const RsPhotoThumbnail &thumbnail)
{
	/* check if its a modification or a new album */


	/* add to database */

	/* check if its a mod or new photo */
	if (album.mId.empty())
	{
		/* new photo */

		/* generate a temp id */
		album.mAlbumId = genRandomId();
	}

	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	mUpdated = true;

	/* add / modify */
	mAlbums[album.mAlbumId] = album;

	/* must fix this up later! */
	RsPhotoThumbnail *thumb = new RsPhotoThumbnail();
	thumb->copyFrom(thumbnail);
	mAlbumThumbnails[album.mAlbumId] = thumb;
	
	return true;
}


bool p3PhotoService::submitPhoto(RsPhotoPhoto &photo, const RsPhotoThumbnail &thumbnail)
{
	if (photo.mAlbumId.empty())
	{
		/* new photo */
		std::cerr << "p3PhotoService::submitPhoto() Missing AlbumID: ERROR";
		std::cerr << std::endl;
		return false;
	}
	
	/* check if its a mod or new photo */
	if (photo.mId.empty())
	{
		/* new photo */

		/* generate a temp id */
		photo.mId = genRandomId();
	}

	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	mUpdated = true;

	std::map<std::string, std::list<std::string> >::iterator it;
	it = mAlbumToPhotos.find(photo.mAlbumId);
	if (it == mAlbumToPhotos.end())
	{
		std::list<std::string> emptyList;
		mAlbumToPhotos[photo.mAlbumId] = emptyList;

		it = mAlbumToPhotos.find(photo.mAlbumId);
	}
	
	it->second.push_back(photo.mId);
		
	/* add / modify */
	mPhotos[photo.mId] = photo;

	/* must fix this up later! */
	RsPhotoThumbnail *thumb = new RsPhotoThumbnail();
	thumb->copyFrom(thumbnail);
	mPhotoThumbnails[photo.mId] = thumb;
	
	return true;
}
#endif
	
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
		free(data);
		size = 0;
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





