#ifndef RETROSHARE_PHOTO_GUI_INTERFACE_H
#define RETROSHARE_PHOTO_GUI_INTERFACE_H

/*
 * libretroshare/src/retroshare: rsphoto.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2008-2012 by Robert Fernie.
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

#include <inttypes.h>
#include <string>
#include <list>
#include <retroshare/rsidentity.h>

/* The Main Interface Class - for information about your Peers */
class RsPhoto;
extern RsPhoto *rsPhoto;

/******************* NEW STUFF FOR NEW CACHE SYSTEM *********/

#define RSPHOTO_MODE_NEW	1
#define RSPHOTO_MODE_OWN	2
#define RSPHOTO_MODE_REMOTE	3

class RsPhotoThumbnail
{
	public:
		RsPhotoThumbnail()
		:data(NULL), size(0), type("N/A") { return; }

	bool copyFrom(const RsPhotoThumbnail &nail);

	// Holds Thumbnail image.
	uint8_t *data;
	int size;
	std::string type;
};

class RsPhotoPhoto
{
	public:

	std::string mAlbumId;
	std::string mId;

	std::string mTitle; // only used by Album.
	std::string mCaption;
	std::string mDescription;
	std::string mPhotographer;
	std::string mWhere;
	std::string mWhen;
	std::string mOther;
	std::string mCategory;

	std::string mHashTags;

	int mOrder;

	RsPhotoThumbnail mThumbnail;

	int mMode;
	std::string path; // if in Mode NEW.
};

class RsPhotoAlbumShare
{
	public:

	uint32_t mShareType;
	std::string mShareGroupId;
	std::string mPublishKey;
	uint32_t mCommentMode;
	uint32_t mResizeMode;
};

class RsPhotoAlbum: public RsPhotoPhoto
{
	public:

	std::string mPhotoPath;
	RsPhotoAlbumShare mShareOptions;
};


class RsPhoto: public RsTokenService
{
	public:

	RsPhoto()  { return; }
virtual ~RsPhoto() { return; }

	/* changed? */
virtual bool updated() = 0;

        /* Data Requests (from RsTokenService) */
//virtual bool requestGroupList(     uint32_t &token, const RsTokReqOptions &opts) = 0;
//virtual bool requestMsgList(       uint32_t &token, const RsTokReqOptions &opts, const std::list<std::string> &groupIds) = 0;
//virtual bool requestMsgRelatedList(uint32_t &token, const RsTokReqOptions &opts, const std::list<std::string> &msgIds) = 0;

//virtual bool requestGroupData(     uint32_t &token, const std::list<std::string> &groupIds) = 0;
//virtual bool requestMsgData(       uint32_t &token, const std::list<std::string> &msgIds) = 0;

        /* Poll */
//virtual uint32_t requestStatus(const uint32_t token) = 0;

	/* Generic List Data */
virtual bool getGroupList(const uint32_t &token, std::list<std::string> &groupIds) = 0;
virtual bool getMsgList(const uint32_t &token, std::list<std::string> &msgIds) = 0;

	/* Specific Service Data */
virtual bool getAlbum(const uint32_t &token, RsPhotoAlbum &album) = 0;
virtual bool getPhoto(const uint32_t &token, RsPhotoPhoto &photo) = 0;

/* details are updated in album - to choose Album ID, and storage path */
virtual bool submitAlbumDetails(RsPhotoAlbum &album) = 0;
virtual bool submitPhoto(RsPhotoPhoto &photo) = 0;


#if 0
virtual bool requestAlbumList(uint32_t &token) = 0;
virtual bool requestPhotoList(uint32_t &token, const std::list<std::string> &albumids) = 0;

virtual bool requestAlbums(uint32_t &token, const std::list<std::string> &albumids) = 0;
virtual bool requestPhotos(uint32_t &token, const std::list<std::string> &photoids) = 0;

virtual bool getAlbumList(const uint32_t &token, std::list<std::string> &albums) = 0;
virtual bool getPhotoList(const uint32_t &token, std::list<std::string> &photos) = 0;



virtual bool requestGroupList(uint32_t &token) { return requestAlbumList(token); }
virtual bool requestGroupData(uint32_t &token, const std::list<std::string> &ids) { return requestAlbums(token, ids); }
virtual bool requestMsgList(uint32_t &token, const std::list<std::string> &ids) { return requestPhotoList(token, ids); }
virtual bool requestMsgData(uint32_t &token, const std::list<std::string> &ids) { return requestPhotos(token, ids); }
#endif



};


#endif
