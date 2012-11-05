#ifndef RETROSHARE_IDENTITY_GUI_INTERFACE_H
#define RETROSHARE_IDENTITY_GUI_INTERFACE_H

/*
 * libretroshare/src/retroshare: rsidentity.h
 *
 * RetroShare C++ Interface.
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

#include <inttypes.h>
#include <string>
#include <list>

#include "gxs/rstokenservice.h"
#include "gxs/rsgxsifaceimpl.h"

/* The Main Interface Class - for information about your Peers */
class RsIdentity;
extern RsIdentity *rsIdentity;


// GroupFlags: Only one so far:
#define RSGXSID_GROUPFLAG_REALID  0x0001


// THESE ARE FLAGS FOR INTERFACE.
#define RSID_TYPE_MASK		0xff00
#define RSID_RELATION_MASK	0x00ff

#define RSID_TYPE_REALID	0x0100
#define RSID_TYPE_PSEUDONYM	0x0200

#define RSID_RELATION_YOURSELF  0x0001
#define RSID_RELATION_FRIEND	0x0002
#define RSID_RELATION_FOF	0x0004
#define RSID_RELATION_OTHER   	0x0008
#define RSID_RELATION_UNKNOWN 	0x0010

std::string rsIdTypeToString(uint32_t idtype);

class RsGxsIdGroup
{
	public:


	RsGroupMetaData mMeta;

	// In GroupMetaData.
	//std::string mNickname; (mGroupName)
	//std::string mKeyId;    (mGroupId)
	//uint32_t mIdType;      (mGroupFlags)

	// SHA(KeyId + Gpg Fingerprint) -> can only be IDed if GPG known.
	// The length of the input must be long enough to make brute force search implausible.

	// Easy to do 1e9 SHA-1 hash computations per second on a GPU.
	// We will need a minimum of 256 bits, ideally 1024 bits or 2048 bits.

	// Actually PgpIdHash is SHA1(.mMeta.mGroupId + PGPHandler->GpgFingerprint(ownId))
	//                                 ???                 160 bits.

	std::string mPgpIdHash; 
	std::string mPgpIdSign;   // Need a signature as proof - otherwise anyone could add others Hashes.
};



class RsGxsIdOpinion
{
	public:

	RsMsgMetaData mMeta;

	// In MsgMetaData.
	//std::string mKeyId;  (mGroupId)
	//std::string mPeerId; (mAuthorId) ???

	uint32_t mOpinion;


	// NOT SERIALISED YET!

	double mReputation;
	//int mRating;
	//int mPeersRating;
	//std::string mComment;
};


// This will probably be dropped.
class RsGxsIdComment
{
	public:

	RsMsgMetaData mMeta;
	std::string mComment;
};


std::ostream &operator<<(std::ostream &out, const RsGxsIdGroup &group);
std::ostream &operator<<(std::ostream &out, const RsGxsIdOpinion &msg);

#if 0
class RsIdReputation
{
	public:
	std::string mKeyId;

	int mYourRating;
	int mPeersRating;
	int mFofRating;
	int mTotalRating;

	std::string mComment;
};

class RsIdOpinion
{
	public:

	std::string mKeyId;
	std::string mPeerId;

	int mRating;
	int mPeersRating;
	std::string mComment;
};

#endif


// DATA TYPE FOR EXTERNAL INTERFACE.

typedef std::string RsGxsId; // TMP. => 

class RsIdentityDetails
{
	public:

	RsGxsId id;

	// identity details.


	// reputation details.
};


class RsIdOpinion
{
	public:
	RsGxsId id;
	int rating;
};
	

class RsIdentityParameters
{
	public:

	int IdType;
};


class RsIdentity: public RsGxsIfaceImpl
{

public:

    RsIdentity(RsGenExchange *gxs): RsGxsIfaceImpl(gxs)  { return; }
    virtual ~RsIdentity() { return; }

    /* Specific Service Data */

    /*!
     * @param token token to be redeemed for album request
     * @param album the album returned for given request token
     * @return false if request token is invalid, check token status for error report
     */
//    virtual bool getAlbum(const uint32_t &token, std::vector<RsPhotoAlbum> &album) = 0;

    /*!
     * @param token token to be redeemed for photo request
     * @param photo the photo returned for given request token
     * @return false if request token is invalid, check token status for error report
     */
//    virtual bool getPhoto(const uint32_t &token,
//                                              PhotoResult &photo) = 0;

    /* details are updated in album - to choose Album ID, and storage path */

    /*!
     * @param token token to be redeemed for photo request
     * @param photo the photo returned for given request token
     * @return false if request token is invalid, check token status for error report
     */
//    virtual bool getPhotoComment(const uint32_t &token,
//                                              PhotoCommentResult& comments) = 0;

    /*!
     * submits album, which returns a token that needs
     * to be acknowledge to get album grp id
     * @param token token to redeem for acknowledgement
     * @param album album to be submitted
     */
//    virtual bool submitAlbumDetails(uint32_t& token, RsPhotoAlbum &album) = 0;

    /*!
     * submits photo, which returns a token that needs
     * to be acknowledged to get photo msg-grp id pair
     * @param token token to redeem for acknowledgement
     * @param photo photo to be submitted
     */
//    virtual bool submitPhoto(uint32_t& token, RsPhotoPhoto &photo) = 0;

    /*!
     * submits photo comment, which returns a token that needs
     * to be acknowledged to get photo msg-grp id pair
     * The mParentId needs to be set to an existing msg for which
     * commenting is enabled
     * @param token token to redeem for acknowledgement
     * @param comment comment to be submitted
     */
//    virtual bool submitComment(uint32_t& token, RsPhotoComment &photo) = 0;

/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/

	// For Other Services....
	// It should be impossible for them to get a message which we don't have the identity.
	// Its a major error if we don't have the identity.

	// We cache all identities, and provide alternative (instantaneous) 
	// functions to extract info, rather than the standard Token system.

virtual bool  getNickname(const RsGxsId &id, std::string &nickname) = 0;
virtual bool  getIdDetails(const RsGxsId &id, RsIdentityDetails &details) = 0;
virtual bool  getOwnIds(std::list<RsGxsId> &ownIds) = 0;

	// 
virtual bool submitOpinion(uint32_t& token, RsIdOpinion &opinion) = 0;
virtual bool createIdentity(uint32_t& token, RsIdentityParameters &params) = 0;

	// Specific RsIdentity Functions....
        /* Specific Service Data */
	/* We expose these initially for testing / GUI purposes.
         */
virtual bool    getGroupData(const uint32_t &token, std::vector<RsGxsIdGroup> &groups) = 0;

	/* In the Identity System - You don't access the Messages Directly.
	 * as they represent idividuals opinions....
	 * This is reflected in the TokenService calls returning false.
	 *
	 * Below is the additional interface to look at reputation.
	 */

	/* So we will want to cache much of the identity stuff, so that we have quick access to the results.
	 * The following bits of data will not use the request/response interface, and should be available immediately.
	 *
 	 * ID => Nickname, knownGPG, reputation.
	 *
	 * This will require quite a bit of data... 
	 *       20 Bytes + 50 + 1 + 4 Bytes? (< 100 Bytes).
	 * 		x 10,000 IDs.    => ~1 MB of cache (Good).
	 * 		x 100,000 IDs.   => ~10 MB of cache (Good).
	 * 		x 1,000,000 IDs. => ~100 MB of cache (Too Big).
	 *
	 * We also need to store quick access to your OwnIds.
	 */

//virtual uint32_t getIdDetails(const std::string &id, std::string &nickname, bool &isGpgKnown, 
//						uint32_t &ownOpinion, float &reputation);
//virtual uint32_t getOwnIds(std::list<std::string> &ownIds);
//virtual bool  setOpinion(const std::string &id, uint32_t opinion);


virtual void generateDummyData() = 0;

#if 0

        /* Data Requests */
virtual bool requestIdentityList(uint32_t &token) = 0;
virtual bool requestIdentities(uint32_t &token, const std::list<std::string> &ids) = 0;
virtual bool requestIdReputations(uint32_t &token, const std::list<std::string> &ids) = 0;
virtual bool requestIdPeerOpinion(uint32_t &token, const std::string &aboutId, const std::string &peerId) = 0;
//virtual bool requestIdGpgDetails(uint32_t &token, const std::list<std::string> &ids) = 0;

        /* Poll */
virtual uint32_t requestStatus(const uint32_t token) = 0;

        /* Retrieve Data */
virtual bool getIdentityList(const uint32_t token, std::list<std::string> &ids) = 0;
virtual bool getIdentity(const uint32_t token, RsIdData &data) = 0;
virtual bool getIdReputation(const uint32_t token, RsIdReputation &reputation) = 0;
virtual bool getIdPeerOpinion(const uint32_t token, RsIdOpinion &opinion) = 0;

        /* Updates */
virtual bool updateIdentity(RsIdData &data) = 0;
virtual bool updateOpinion(RsIdOpinion &opinion) = 0;

#endif

};

#endif // RETROSHARE_IDENTITY_GUI_INTERFACE_H
