#ifndef RSPHOTOV2_H
#define RSPHOTOV2_H

/*
 * libretroshare/src/retroshare: rsphoto.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2008-2012 by Robert Fernie, Christopher Evi-Parker
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
#include "rsgxsservice.h"
#include "rsphoto.h"

/* The Main Interface Class - for information about your Peers */
class RsPhotoV2;
extern RsPhotoV2 *rsPhotoV2;

/******************* NEW STUFF FOR NEW CACHE SYSTEM *********/

#define RSPHOTO_MODE_NEW	1
#define RSPHOTO_MODE_OWN	2
#define RSPHOTO_MODE_REMOTE	3

//class RsPhotoThumbnail
//{
//        public:
//                RsPhotoThumbnail()
//                :data(NULL), size(0), type("N/A") { return; }
//
//        bool deleteImage();
//        bool copyFrom(const RsPhotoThumbnail &nail);
//
//        // Holds Thumbnail image.
//        uint8_t *data;
//        int size;
//        std::string type;
//};


/* If these flags are no set - the Photo inherits values from the Album
 */

#define RSPHOTO_FLAGS_ATTRIB_TITLE		0x0001
#define RSPHOTO_FLAGS_ATTRIB_CAPTION		0x0002
#define RSPHOTO_FLAGS_ATTRIB_DESC		0x0004
#define RSPHOTO_FLAGS_ATTRIB_PHOTOGRAPHER	0x0008
#define RSPHOTO_FLAGS_ATTRIB_WHERE		0x0010
#define RSPHOTO_FLAGS_ATTRIB_WHEN		0x0020
#define RSPHOTO_FLAGS_ATTRIB_OTHER		0x0040
#define RSPHOTO_FLAGS_ATTRIB_CATEGORY		0x0080
#define RSPHOTO_FLAGS_ATTRIB_HASHTAGS		0x0100
#define RSPHOTO_FLAGS_ATTRIB_ORDER		0x0200
#define RSPHOTO_FLAGS_ATTRIB_THUMBNAIL		0x0400
#define RSPHOTO_FLAGS_ATTRIB_MODE		0x0800
#define RSPHOTO_FLAGS_ATTRIB_AUTHOR		0x1000 // PUSH UP ORDER
#define RSPHOTO_FLAGS_ATTRIB_PHOTO		0x2000 // PUSH UP ORDER.


//class RsPhotoPhoto
//{
//        public:
//
//        RsMsgMetaData mMeta;
//
//        RsPhotoPhoto();
//
//        // THESE ARE IN THE META DATA.
//        //std::string mAlbumId;
//        //std::string mId;
//        //std::string mTitle; // only used by Album.
//        std::string mCaption;
//        std::string mDescription;
//        std::string mPhotographer;
//        std::string mWhere;
//        std::string mWhen;
//        std::string mOther;
//        std::string mCategory;
//
//        std::string mHashTags;
//
//        uint32_t mSetFlags;
//
//        int mOrder;
//
//        RsPhotoThumbnail mThumbnail;
//
//        int mMode;
//
//        // These are not saved.
//        std::string path; // if in Mode NEW.
//        uint32_t mModFlags;
//};

//class RsPhotoAlbumShare
//{
//        public:
//
//        uint32_t mShareType;
//        std::string mShareGroupId;
//        std::string mPublishKey;
//        uint32_t mCommentMode;
//        uint32_t mResizeMode;
//};
//
//class RsPhotoAlbum
//{
//        public:
//        RsPhotoAlbum();
//
//        RsGroupMetaData mMeta;
//
//        // THESE ARE IN THE META DATA.
//        //std::string mAlbumId;
//        //std::string mTitle; // only used by Album.
//
//        std::string mCaption;
//        std::string mDescription;
//        std::string mPhotographer;
//        std::string mWhere;
//        std::string mWhen;
//        std::string mOther;
//        std::string mCategory;
//
//        std::string mHashTags;
//
//        RsPhotoThumbnail mThumbnail;
//
//        int mMode;
//
//        std::string mPhotoPath;
//        RsPhotoAlbumShare mShareOptions;
//
//        // These aren't saved.
//        uint32_t mSetFlags;
//        uint32_t mModFlags;
//};

std::ostream &operator<<(std::ostream &out, const RsPhotoPhoto &photo);
std::ostream &operator<<(std::ostream &out, const RsPhotoAlbum &album);

typedef std::map<RsGxsGroupId, std::vector<RsPhotoPhoto> > PhotoResult;
typedef std::map<RsGxsGroupId, std::vector<RsMsgMetaData> > MsgMetaResult;

class RsPhotoV2
{

public:


	RsPhotoV2()  { return; }

	virtual ~RsPhotoV2() { return; }

	/*!
	 * Use to enquire if groups or msgs have changed
	 * Poll regularly, particularly after a photo submission
	 * @return true if msgs or groups have changed
	 */
	virtual bool updated() = 0;

	/*!
	 *
	 * @param grpIds
	 */
	virtual void groupsChanged(std::list<RsGxsGroupId>& grpIds) = 0;

	/*!
	 *
	 * @param msgs
	 */
	virtual void msgsChanged(GxsMsgIdResult& msgs) = 0;

	/*!
	 * To acquire a handle to token service handler
	 * needed to make requests to the service
	 * @return handle to token service for this gxs service
	 */
	virtual RsTokenServiceV2* getTokenService() = 0;

	/* Generic Lists */

	/*!
	 *
	 * @param token token to be redeemed for this request
	 * @param groupIds the ids return for given request token
	 * @return false if request token is invalid, check token status for error report
	 */
	virtual bool getGroupList(const uint32_t &token,
							  std::list<RsGxsGroupId> &groupIds) = 0;

	/*!
	 * @param token token to be redeemed for this request
	 * @param msgIds the ids return for given request token
	 * @return false if request token is invalid, check token status for error report
	 */
	virtual bool getMsgList(const uint32_t &token,
							GxsMsgIdResult &msgIds) = 0;

	/* Generic Summary */

	/*!
	 * @param token token to be redeemed for group summary request
	 * @param groupInfo the ids returned for given request token
	 * @return false if request token is invalid, check token status for error report
	 */
	virtual bool getGroupSummary(const uint32_t &token,
								 std::list<RsGroupMetaData> &groupInfo) = 0;

	/*!
	 * @param token token to be redeemed for message summary request
	 * @param msgInfo the message metadata returned for given request token
	 * @return false if request token is invalid, check token status for error report
	 */
	virtual bool getMsgSummary(const uint32_t &token,
							   MsgMetaResult &msgInfo) = 0;

			/* Specific Service Data */

	/*!
	 * @param token token to be redeemed for album request
	 * @param album the album returned for given request token
	 * @return false if request token is invalid, check token status for error report
	 */
	virtual bool getAlbum(const uint32_t &token, std::vector<RsPhotoAlbum> &album) = 0;

	/*!
	 * @param token token to be redeemed for photo request
	 * @param photo the photo returned for given request token
	 * @return false if request token is invalid, check token status for error report
	 */
	virtual bool getPhoto(const uint32_t &token,
						  PhotoResult &photo) = 0;

	/* details are updated in album - to choose Album ID, and storage path */

	/*!
	 * This RsGenExchange service will be alerted to this album as \n
	 * a new album. Do not keep the submitted album as representative, wait for
	 * notification telling of successful submission
	 * @param album The album to be submitted
	 * @return false if submission failed
	 */
	virtual bool submitAlbumDetails(uint32_t& token, RsPhotoAlbum &album) = 0;

	/*!
	 * This RsGenExchange service will be alerted to this photo as \n
	 * a new photo. Do not keep the submitted photo as representative, wait for new photo
	 * returned
	 * @param photo photo to submit
	 * @return
	 */
	virtual bool submitPhoto(uint32_t& token, RsPhotoPhoto &photo) = 0;


	/*!
	 * This allows the client service to acknowledge that their msgs has
	 * been created/modified and retrieve the create/modified msg ids
	 * @param token the token related to modification/create request
	 * @param msgIds map of grpid->msgIds of message created/modified
	 * @return true if token exists false otherwise
	 */
	virtual bool acknowledgeMsg(const uint32_t& token, std::pair<RsGxsGroupId, RsGxsMessageId>& msgId) = 0;

	/*!
	 * This allows the client service to acknowledge that their grps has
	 * been created/modified and retrieve the create/modified grp ids
	 * @param token the token related to modification/create request
	 * @param msgIds vector of ids of groups created/modified
	 * @return true if token exists false otherwise
	 */
	virtual bool acknowledgeGrp(const uint32_t& token, RsGxsGroupId& grpId) = 0;


};


#endif // RSPHOTOV2_H
