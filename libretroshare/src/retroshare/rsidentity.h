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

// FLAGS WILL BE REUSED FROM RSDISTRIB -> FOR NOW.
#include <retroshare/rsdistrib.h>

/********** Generic Token Request Interface ***********************
 * This is packaged here, as most TokenServices will require ID Services too.
 * The requests can be generic, but the reponses are service specific (dependent on data types).
 */

// This bit will be filled out over time.
#define RS_TOKREQOPT_MSG_VERSIONS	0x0001		// MSGRELATED: Returns All MsgIds with OrigMsgId = MsgId.
#define RS_TOKREQOPT_MSG_ORIGMSG	0x0002		// MSGLIST: All Unique OrigMsgIds in a Group.
#define RS_TOKREQOPT_MSG_LATEST		0x0004		// MSGLIST: All Latest MsgIds in Group. MSGRELATED: Latest MsgIds for Input Msgs.

#define RS_TOKREQOPT_MSG_THREAD		0x0010		// MSGRELATED: All Msgs in Thread. MSGLIST: All Unique Thread Ids in Group. 
#define RS_TOKREQOPT_MSG_PARENT		0x0020		// MSGRELATED: All Children Msgs.

#define RS_TOKREQOPT_MSG_AUTHOR		0x0040		// MSGLIST: Messages from this AuthorId 


// Status Filtering... should it be a different Option Field.
#define RS_TOKREQOPT_GROUP_UPDATED	0x0100		// GROUPLIST: Groups that have been updated.
#define RS_TOKREQOPT_MSG_UPDATED	0x0200		// MSGLIST: Msg that have been updated from specified groups.
#define RS_TOKREQOPT_MSG_UPDATED	0x0200		// MSGLIST: Msg that have been updated from specified groups.



// Read Status.
#define RS_TOKREQOPT_READ		0x0001
#define RS_TOKREQOPT_UNREAD		0x0002

#define RS_TOKREQ_ANSTYPE_LIST		0x0001		
#define RS_TOKREQ_ANSTYPE_SUMMARY	0x0002		
#define RS_TOKREQ_ANSTYPE_DATA		0x0003		




class RsTokReqOptions
{
	public:
	RsTokReqOptions() 
	{ 
		mOptions = 0; 
		mStatusFilter = 0; mStatusMask = 0; mSubscribeFilter = 0; 
		mBefore = 0; mAfter = 0; 
	}

	uint32_t mOptions;

	// Request specific matches with Group / Message Status.
	// Should be usable with any Options... applied afterwards.
	uint32_t mStatusFilter;
	uint32_t mStatusMask;

	uint32_t mSubscribeFilter; // Only for Groups.

	// Time range... again applied after Options.
	time_t   mBefore;
	time_t   mAfter;
};


/*********************************************************
 * Documentation for Groups Definitions.
 * 
 * A Group is defined by:
 *	- TWO RSA Keys. (Admin Key & Publish Key)
 *	- Publish TS: Used to select the latest definition.
 *
 *	- Operating Mode:
 *		- Circle (Public, External, Private).
 *		- Publish Mode: Encrypted / All-Signed / Only ThreadHead / None Required.
 *		- AuthorId: GPG Required / Any Required / Only if no Publish Signature.
 *
 *	- Description:
 *		- Name & Description.
 *		- Optional AuthorId.
 * 
 * Most of this information is contained inside the GroupMetaData.
 * except for Actual Admin / Publish Keys, which are maintained internally.
 *
 *******
 *      - Group Definition must be signed by Admin Key, otherwise invalid.
 *	- Circle Definition controls distribution of Group and Messages, see section on this for more details. 
 *	- Public parts of Keys are distributed with Definition.
 *	- Private parts can be distributed to select people via alternative channels.
 *      - A Message Requires at least one signature: publish or Author. This signature will be used as MsgId.
 *
 *	Groups will operate in the following modes:
 *	1) Public Forum: PublishMode = None Required, AuthorId: Required.  
 *	2) Closed Forum: PublishMode = All-Signed, AuthorId: Required.  
 *	3) Private Forum: PublishMode = Encrypted, AuthorId: Required.  
 *
 *      4) Anon Channel: PublishMode = All-Signed, AuthorId: None.
 *      5) Anon Channel with Comments: PublishMode = Only ThreadHead, AuthorId: If No Publish Signature.
 *      6) Private Channel: PublishMode = Encrypted. 
 *
 *      7) Personal Photos - with comments: PublishMode = Only ThreadHead, AuthorId: Required.
 *      8) Personal Photos - no comments: PublishMode = All-Signed, AuthorId: Required.
 *
 *      9 ) Public  Wiki: PublishMode = None Required, AuthorId: Required.
 *      10) Closed  Wiki: PublishMode = All-Signed, AuthorId: Required.
 *      11) Private Wiki: PublishMode = Encrypted, AuthorId: Required.
 *
 *      12) Twitter: PublishMode = Only ThreadHead, AuthorId: Required.
 *
 *      13) Posted: PublishMode = None Required, AuthorId: Required.
 *
 *
 ******
 *
 * Additionally to this information. The MetaData also contains several fields which can 
 * be used to store local information for the benefit of the service.
 *
 * In Particular: MsgStatus & GroupStatus inform the service if the user has read the message or if anything has changed.
 *
 ***/


// Control of Publish Signatures.
#define RSGXS_GROUP_SIGN_PUBLISH_MASK  		0x000000ff
#define RSGXS_GROUP_SIGN_PUBLISH_ENCRYPTED 	0x00000001
#define RSGXS_GROUP_SIGN_PUBLISH_ALLSIGNED 	0x00000002
#define RSGXS_GROUP_SIGN_PUBLISH_THREADHEAD	0x00000004
#define RSGXS_GROUP_SIGN_PUBLISH_NONEREQ	0x00000008

// Author Signature.
#define RSGXS_GROUP_SIGN_AUTHOR_MASK  		0x0000ff00
#define RSGXS_GROUP_SIGN_AUTHOR_GPG 		0x00000100
#define RSGXS_GROUP_SIGN_AUTHOR_REQUIRED 	0x00000200
#define RSGXS_GROUP_SIGN_AUTHOR_IFNOPUBSIGN	0x00000400
#define RSGXS_GROUP_SIGN_AUTHOR_NONE		0x00000800

// NB: That one signature is required...
// so some combinations are not possible. e.g.
// SIGN_PUBLISH_NONEREQ	&& SIGN_AUTHOR_NONE is not allowed.
// SIGN_PUBLISH_THREADHEAD && SIGN_AUTHOR_NONE is also invalid.

#define RSGXS_GROUP_SIGN_RESERVED_MASK  	0xffff0000


// STATUS FLAGS: There is space here for Service specific flags - if they so desire.
//
// Msgs: UNREAD_BY_USER & PROCESSED are useful.
// Groups: NEW_MESSAGES & GROUP_UPDATED.

#define RSGXS_MSG_STATUS_MASK           0x0000000f
#define RSGXS_MSG_STATUS_READ 		0x00000001	// New or Not New
#define RSGXS_MSG_STATUS_UNREAD_BY_USER 0x00000002
#define RSGXS_MSG_STATUS_UNPROCESSED	0x00000004	// By the Service.

#define RSGXS_MSG_STATUS_SERVICE_MASK 	0xffff0000

#define RSGXS_GROUP_STATUS_MASK  	0x0000000f
#define RSGXS_GROUP_STATUS_UPDATED 	0x00000001
#define RSGXS_GROUP_STATUS_NEWGROUP	0x00000002	
#define RSGXS_GROUP_STATUS_NEWMSG       0x00000004	

#define RSGXS_GROUP_STATUS_SERVICE_MASK 0xffff0000



// Subscription Flags. (LOCAL)
#define RSGXS_GROUP_SUBSCRIBE_MASK  		0x0000000f
#define RSGXS_GROUP_SUBSCRIBE_ADMIN  		0x00000001
#define RSGXS_GROUP_SUBSCRIBE_PUBLISH  		0x00000002
#define RSGXS_GROUP_SUBSCRIBE_SUBSCRIBED	0x00000004
#define RSGXS_GROUP_SUBSCRIBE_MONITOR		0x00000008


// Some MACROS for EASE OF USE. (USED BY FORUMSV2 At the moment.
#define IS_MSG_UNREAD(status) 			((status & RSGXS_MSG_STATUS_READ) == 0 || (status & RSGXS_MSG_STATUS_UNREAD_BY_USER))
#define IS_GROUP_ADMIN(subscribeFlags) 		(subscribeFlags & RSGXS_GROUP_SUBSCRIBE_ADMIN)
#define IS_GROUP_SUBSCRIBED(subscribeFlags) 	(subscribeFlags & (RSGXS_GROUP_SUBSCRIBE_ADMIN | RSGXS_GROUP_SUBSCRIBE_SUBSCRIBED))



#define RSGXS_MAX_SERVICE_STRING	200 // Sensible limit for dbase usage.


class RsGroupMetaData
{
	public:

	RsGroupMetaData()
	{
		mGroupFlags = 0;
		mSignFlags = 0;
		mSubscribeFlags = 0;

		mPop = 0;
		mMsgCount = 0;
		mLastPost = 0;
		mGroupStatus = 0;
	
		//mPublishTs = 0;
	}
	
	std::string mGroupId;
	std::string mGroupName;
	uint32_t    mGroupFlags;  // Service Specific Options ????
	uint32_t    mSignFlags;   // Combination of RSGXS_GROUP_SIGN_PUBLISH_MASK & RSGXS_GROUP_SIGN_AUTHOR_MASK.

	time_t      mPublishTs; // Mandatory.
	std::string mAuthorId;   // Optional. 

	// BELOW HERE IS LOCAL DATA, THAT IS NOT FROM MSG.

	uint32_t    mSubscribeFlags;

	uint32_t    mPop; // HOW DO WE DO THIS NOW.
	uint32_t    mMsgCount; // ???
	time_t      mLastPost; // ???

	uint32_t    mGroupStatus;

	std::string mServiceString; // Service Specific Free-Form extra storage.
};




class RsMsgMetaData
{
	public:

	RsMsgMetaData()
	{
		mPublishTs = 0;
		mMsgFlags = 0;
		mMsgStatus = 0;
		mChildTs = 0;
	}

	std::string mGroupId;
	std::string mMsgId;

	std::string mThreadId;
	std::string mParentId;
	std::string mOrigMsgId;

	std::string mAuthorId;
	
	std::string mMsgName;
	time_t      mPublishTs;

	uint32_t    mMsgFlags; // Whats this for? (Optional Service Specific - e.g. flag MsgType)

	// BELOW HERE IS LOCAL DATA, THAT IS NOT FROM MSG.
	// normally READ / UNREAD flags. LOCAL Data.
	uint32_t    mMsgStatus;
	time_t      mChildTs;

	std::string mServiceString; // Service Specific Free-Form extra storage.

};

std::ostream &operator<<(std::ostream &out, const RsGroupMetaData &meta);
std::ostream &operator<<(std::ostream &out, const RsMsgMetaData &meta);

class RsTokenService
{
	public:

	RsTokenService()  { return; }
virtual ~RsTokenService() { return; }

        /* changed? */
virtual bool updated() = 0;

        /* Data Requests */
virtual bool requestGroupInfo(     uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds) = 0;
virtual bool requestMsgInfo(       uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds) = 0;
virtual bool requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &msgIds) = 0;

        /* Generic Lists */
virtual bool getGroupList(         const uint32_t &token, std::list<std::string> &groupIds) = 0;
virtual bool getMsgList(           const uint32_t &token, std::list<std::string> &msgIds) = 0;

        /* Generic Summary */
virtual bool getGroupSummary(      const uint32_t &token, std::list<RsGroupMetaData> &groupInfo) = 0;
virtual bool getMsgSummary(        const uint32_t &token, std::list<RsMsgMetaData> &msgInfo) = 0;

	/* Actual Data -> specific to Interface */



        /* Poll */
virtual uint32_t requestStatus(const uint32_t token) = 0;

	/* Cancel Request */
virtual bool cancelRequest(const uint32_t &token) = 0;


	//////////////////////////////////////////////////////////////////////////////
	/* Functions from Forums -> need to be implemented generically */
	// Groups Changed is now part of requestGroupInfo request.
//virtual bool groupsChanged(std::list<std::string> &groupIds) = 0;

        // Message/Group Status - is retrived via requests...
        // These operations could have a token, but for the moment we are going to assume
        // they are async and always succeed - (or fail silently).
virtual bool setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask) = 0;
virtual bool setGroupStatus(const std::string &grpId, const uint32_t status, const uint32_t statusMask) = 0;

virtual bool setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask) = 0;

virtual bool setMessageServiceString(const std::string &msgId, const std::string &str) = 0;
virtual bool setGroupServiceString(const std::string &grpId, const std::string &str) = 0;

	// (FUTURE WORK).	
virtual bool groupRestoreKeys(const std::string &groupId) = 0;
virtual bool groupShareKeys(const std::string &groupId, std::list<std::string>& peers) = 0;




};




/* The Main Interface Class - for information about your Peers */
class RsIdentity;
extern RsIdentity *rsIdentity;

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

class RsIdGroup
{
	public:


	RsGroupMetaData mMeta;

	// In GroupMetaData.
	//std::string mNickname; (mGroupName)
	//std::string mKeyId;    (mGroupId)

	uint32_t mIdType;

	std::string mGpgIdHash; // SHA(KeyId + Gpg Fingerprint) -> can only be IDed if GPG known.

	// NOTE: These cannot be transmitted as part of underlying messages....
	// Must use ServiceString.
	bool mGpgIdKnown; // if GpgIdHash has been identified.
	std::string mGpgId;   	// if known.
	std::string mGpgName; 	// if known.
	std::string mGpgEmail; 	// if known.
};



class RsIdMsg
{
	public:

	RsMsgMetaData mMeta;

	// In MsgMetaData.
	//std::string mKeyId;  (mGroupId)
	//std::string mPeerId; (mAuthorId) ???

	int mOpinion;
	double mReputation;
	//int mRating;
	//int mPeersRating;
	//std::string mComment;
};



std::ostream &operator<<(std::ostream &out, const RsIdGroup &meta);
std::ostream &operator<<(std::ostream &out, const RsIdMsg &meta);



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


class RsIdentity: public RsTokenService
{
	public:

	RsIdentity()  { return; }
virtual ~RsIdentity() { return; }


	/* INCLUDES INTERFACE FROM RS TOKEN SERVICE */
	//////////////////////////////////////////////////////////////////////////////


        /* Specific Service Data */
virtual bool    getGroupData(const uint32_t &token, RsIdGroup &group) = 0;
virtual bool    getMsgData(const uint32_t &token, RsIdMsg &msg) = 0;

virtual bool    createGroup(uint32_t &token, RsIdGroup &group, bool isNew) = 0;
virtual bool    createMsg(uint32_t &token, RsIdMsg &msg, bool isNew) = 0;

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
						uint32_t &ownOpinion, float &reputation);
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



#endif
