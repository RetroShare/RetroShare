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

class RsPhotoPhoto
{
public:

	RsMsgMetaData mMeta;

	RsPhotoPhoto();

	// V2 PhotoMsg - keep it simple.
	// mMeta.mTitle used for Photo Caption.
	// mDescription optional field for addtional notes.
	// mLowResImage - < 50k jpg of image.
	// mPhotoFile   - transfer details for original photo.
	std::string mDescription;
	uint32_t    mOrder;
	RsGxsImage mLowResImage;
	RsGxsFile  mPhotoFile;

	// These are not saved.
	std::string mPath; // if New photo
};

#define RSPHOTO_SHAREMODE_LOWRESONLY    (1)
#define RSPHOTO_SHAREMODE_ORIGINAL      (2)
#define RSPHOTO_SHAREMODE_DUP_ORIGINAL  (3)
#define RSPHOTO_SHAREMODE_DUP_200K      (4)
#define RSPHOTO_SHAREMODE_DUP_1M        (5)

class RsPhotoAlbum
{
public:
	RsPhotoAlbum();

	RsGroupMetaData mMeta;

	// V2 Album - keep it simple.
	// mMeta.mTitle.
	uint32_t mShareMode;

	std::string mCaption;
	std::string mDescription;
	std::string mPhotographer;
	std::string mWhere;
	std::string mWhen;

	RsGxsImage mThumbnail;

	// Below is not saved.
	bool mAutoDownload;
};

std::ostream &operator<<(std::ostream &out, const RsPhotoPhoto &photo);
std::ostream &operator<<(std::ostream &out, const RsPhotoAlbum &album);

typedef std::map<RsGxsGroupId, std::vector<RsPhotoPhoto> > PhotoResult;

class RsPhoto: public RsGxsIfaceHelper, public RsGxsCommentService
{
public:
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
