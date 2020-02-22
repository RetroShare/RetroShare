/*******************************************************************************
 * libretroshare/src/retroshare: rsphoto.h                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008-2020 by Robert Fernie <retroshare@lunamutt.com>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#ifndef RSPHOTOV2_H
#define RSPHOTOV2_H

#include <inttypes.h>
#include <string>
#include <list>
#include "retroshare/rsgxsservice.h"
#include "retroshare/rsgxscommon.h"
#include "retroshare/rsgxsifacehelper.h"


/* The Main Interface Class - for information about your Peers */
class RsPhoto;
extern RsPhoto *rsPhoto;

/******************* NEW STUFF FOR NEW CACHE SYSTEM *********/

#define RSPHOTO_MODE_NEW	1
#define RSPHOTO_MODE_OWN	2
#define RSPHOTO_MODE_REMOTE	3

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

class RsPhotoPhoto
{
        public:

        RsMsgMetaData mMeta;

        RsPhotoPhoto();

        // THESE ARE IN THE META DATA.
        //std::string mAlbumId;
        //std::string mId;
        //std::string mTitle; // only used by Album.
        std::string mCaption;
        std::string mDescription;
        std::string mPhotographer;
        std::string mWhere;
        std::string mWhen;
        std::string mOther;
        std::string mCategory;

        std::string mHashTags;

        uint32_t mSetFlags;

        int mOrder;

        RsGxsImage mThumbnail;

        int mMode;

        // These are not saved.
        std::string path; // if in Mode NEW.
        uint32_t mModFlags;
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

class RsPhotoAlbum
{
        public:
        RsPhotoAlbum();

        RsGroupMetaData mMeta;

        // THESE ARE IN THE META DATA.
        //std::string mAlbumId;
        //std::string mTitle; // only used by Album.

        std::string mCaption;
        std::string mDescription;
        std::string mPhotographer;
        std::string mWhere;
        std::string mWhen;
        std::string mOther;
        std::string mCategory;

        std::string mHashTags;

        RsGxsImage mThumbnail;

        int mMode;

        std::string mPhotoPath;
        RsPhotoAlbumShare mShareOptions;

        // These aren't saved.
        uint32_t mSetFlags;
        uint32_t mModFlags;
};


std::ostream &operator<<(std::ostream &out, const RsPhotoPhoto &photo);
std::ostream &operator<<(std::ostream &out, const RsPhotoAlbum &album);

typedef std::map<RsGxsGroupId, std::vector<RsPhotoPhoto> > PhotoResult;

class RsPhoto: public RsGxsIfaceHelper, public RsGxsCommentService
{

public:

    static const uint32_t FLAG_MSG_TYPE_PHOTO_POST;
    static const uint32_t FLAG_MSG_TYPE_PHOTO_COMMENT;
    static const uint32_t FLAG_MSG_TYPE_MASK;


    explicit RsPhoto(RsGxsIface &gxs) : RsGxsIfaceHelper(gxs)  { return; }

    virtual ~RsPhoto() { return; }

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
    virtual RsTokenService* getTokenService() = 0;

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

    /*!
     * submits album, which returns a token that needs
     * to be acknowledge to get album grp id
     * @param token token to redeem for acknowledgement
     * @param album album to be submitted
     */
    virtual bool submitAlbumDetails(uint32_t& token, RsPhotoAlbum &album) = 0;

    /*!
     * submits photo, which returns a token that needs
     * to be acknowledged to get photo msg-grp id pair
     * @param token token to redeem for acknowledgement
     * @param photo photo to be submitted
     */
    virtual bool submitPhoto(uint32_t& token, RsPhotoPhoto &photo) = 0;

    /*!
     * subscribes to group, and returns token which can be used
     * to be acknowledged to get group Id
     * @param token token to redeem for acknowledgement
     * @param grpId the id of the group to subscribe to
     */
    virtual bool subscribeToAlbum(uint32_t& token, const RsGxsGroupId& grpId, bool subscribe) = 0;

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
