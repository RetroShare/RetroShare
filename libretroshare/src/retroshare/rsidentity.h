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

#include "retroshare/rstokenservice.h"
#include "retroshare/rsgxsifacehelper.h"
#include "retroshare/rsreputations.h"
#include "retroshare/rsids.h"
#include "serialiser/rstlvimage.h"
#include "retroshare/rsgxscommon.h"

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

#define RSRECOGN_MAX_TAGINFO	5

// Unicode symbols. NOT utf-8 bytes, because of multi byte characters
#define RSID_MAXIMUM_NICKNAME_SIZE 30

std::string rsIdTypeToString(uint32_t idtype);

static const uint32_t RS_IDENTITY_FLAGS_IS_A_CONTACT = 0x0001;
static const uint32_t RS_IDENTITY_FLAGS_PGP_LINKED   = 0x0002;
static const uint32_t RS_IDENTITY_FLAGS_PGP_KNOWN    = 0x0004;
static const uint32_t RS_IDENTITY_FLAGS_IS_OWN_ID    = 0x0008;

class GxsReputation
{
        public:
        GxsReputation();

        bool updateIdScore(bool pgpLinked, bool pgpKnown);
        bool update();    // checks ranges and calculates overall score.
        int mOverallScore;
        int mIdScore;      // PGP, Known, etc.
        int mOwnOpinion;
        int mPeerOpinion;
};


class RsGxsIdGroup
{
	public:
    RsGxsIdGroup(): mLastUsageTS(0), mPgpKnown(false) { return; }
	~RsGxsIdGroup() { return; }


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

	Sha1CheckSum mPgpIdHash;
	// Need a signature as proof - otherwise anyone could add others Hashes.
	// This is a string, as the length is variable.
	std::string mPgpIdSign;   

	// Recognition Strings. MAX# defined above.
	std::list<std::string> mRecognTags;

    // Avatar
    RsGxsImage mImage ;
    time_t mLastUsageTS ;

    // Not Serialised - for GUI's benefit.
    bool mPgpKnown;
    bool mIsAContact;	// change that into flags one day
    RsPgpId mPgpId;
    GxsReputation mReputation;
};


std::ostream &operator<<(std::ostream &out, const RsGxsIdGroup &group);

// DATA TYPE FOR EXTERNAL INTERFACE.

class RsRecognTag
{
	public:
	RsRecognTag(uint16_t tc, uint16_t tt, bool v)
	:tag_class(tc), tag_type(tt), valid(v) { return; }
	uint16_t tag_class;
	uint16_t tag_type;
	bool valid;
};


class RsRecognTagDetails
{
	public:
	RsRecognTagDetails()
	:valid_from(0), valid_to(0), tag_class(0), tag_type(0), 
	is_valid(false), is_pending(false) { return; }
	
	time_t valid_from;
	time_t valid_to;
	uint16_t tag_class;
	uint16_t tag_type;
	
	std::string signer;
	
	bool is_valid;
	bool is_pending;
};

class RsIdentityDetails
{
public:
    RsIdentityDetails()
            : mFlags(0), mLastUsageTS(0) { return; }

    RsGxsId mId;

    // identity details.
    std::string mNickname;
    
    uint32_t mFlags ;

    // PGP Stuff.
    RsPgpId mPgpId;

    // Recogn details.
    std::list<RsRecognTag> mRecognTags;

    // Cyril: Reputation details. At some point we might want to merge information
    // between the two into a single global score. Since the old reputation system
    // is not finished yet, I leave this in place. We should decide what to do with it.
    
    GxsReputation mReputation_oldSystem;		// this is the old "mReputation" field, which apparently is not used.
    RsReputations::ReputationInfo mReputation;

    // avatar
    RsGxsImage mAvatar ;

    // last usage
    time_t mLastUsageTS ;
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
	RsIdentityParameters(): isPgpLinked(false) { return; }
	bool isPgpLinked;
    std::string nickname;
    RsGxsImage mImage ;
};


class RsIdentity: public RsGxsIfaceHelper
{

public:

    RsIdentity(RsGxsIface *gxs): RsGxsIfaceHelper(gxs)  { return; }
    virtual ~RsIdentity() { return; }

/********************************************************************************************/
/********************************************************************************************/
    
	// For Other Services....
	// It should be impossible for them to get a message which we don't have the identity.
	// Its a major error if we don't have the identity.

	// We cache all identities, and provide alternative (instantaneous) 
	// functions to extract info, rather than the standard Token system.

//virtual bool  getNickname(const RsGxsId &id, std::string &nickname) = 0;
virtual bool  getIdDetails(const RsGxsId &id, RsIdentityDetails &details) = 0;

// Fills up list of all own ids. Returns false if ids are not yet loaded.
virtual bool  getOwnIds(std::list<RsGxsId> &ownIds) = 0;

	// 
virtual bool submitOpinion(uint32_t& token, const RsGxsId &id, 
				bool absOpinion, int score) = 0;
virtual bool createIdentity(uint32_t& token, RsIdentityParameters &params) = 0;

virtual bool updateIdentity(uint32_t& token, RsGxsIdGroup &group) = 0;
virtual bool deleteIdentity(uint32_t& token, RsGxsIdGroup &group) = 0;

virtual bool parseRecognTag(const RsGxsId &id, const std::string &nickname,
                        const std::string &tag, RsRecognTagDetails &details) = 0;
virtual bool getRecognTagRequest(const RsGxsId &id, const std::string &comment,
                        uint16_t tag_class, uint16_t tag_type, std::string &tag) = 0;

    virtual bool setAsRegularContact(const RsGxsId& id,bool is_a_contact) = 0 ;
    virtual bool isARegularContact(const RsGxsId& id) = 0 ;
    
	// Specific RsIdentity Functions....
        /* Specific Service Data */
	/* We expose these initially for testing / GUI purposes.
         */

virtual bool    getGroupData(const uint32_t &token, std::vector<RsGxsIdGroup> &groups) = 0;
//virtual bool 	getMsgData(const uint32_t &token, std::vector<RsGxsIdOpinion> &opinions) = 0;

};

#endif // RETROSHARE_IDENTITY_GUI_INTERFACE_H
