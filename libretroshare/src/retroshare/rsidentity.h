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



// Read Status.
#define RS_TOKREQOPT_READ		0x0001
#define RS_TOKREQOPT_UNREAD		0x0002

#define RS_TOKREQ_ANSTYPE_LIST		0x0001		
#define RS_TOKREQ_ANSTYPE_SUMMARY	0x0002		
#define RS_TOKREQ_ANSTYPE_DATA		0x0003		



// TEMP FLAGS... TO FIX.
#define RSGXS_MSG_STATUS_MASK           0x000f
#define RSGXS_MSG_STATUS_READ           0x0001
#define RSGXS_MSG_STATUS_UNREAD_BY_USER 0x0002



class RsTokReqOptions
{
	public:
	RsTokReqOptions() { mOptions = 0; mBefore = 0; mAfter = 0; }

	uint32_t mOptions;
	time_t   mBefore;
	time_t   mAfter;
};





class RsGroupMetaData
{
	public:

	RsGroupMetaData()
	{
		mGroupFlags = 0;
		mSubscribeFlags = 0;

		mPop = 0;
		mMsgCount = 0;
		mLastPost = 0;
		mGroupStatus = 0;
	}
	
	std::string mGroupId;
	std::string mGroupName;
	uint32_t    mGroupFlags;

	// BELOW HERE IS LOCAL DATA, THAT IS NOT FROM MSG.

	uint32_t    mSubscribeFlags;

	uint32_t    mPop; // HOW DO WE DO THIS NOW.
	uint32_t    mMsgCount; // ???
	time_t      mLastPost; // ???

	uint32_t    mGroupStatus;

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

	uint32_t    mMsgFlags; // Whats this for?

	// BELOW HERE IS LOCAL DATA, THAT IS NOT FROM MSG.
	// normally READ / UNREAD flags. LOCAL Data.
	uint32_t    mMsgStatus;
	time_t      mChildTs;

};

std::ostream &operator<<(std::ostream &out, const RsGroupMetaData &meta);
std::ostream &operator<<(std::ostream &out, const RsMsgMetaData &meta);

class RsTokenService
{
	public:

	RsTokenService()  { return; }
virtual ~RsTokenService() { return; }

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
virtual bool groupsChanged(std::list<std::string> &groupIds) = 0;

	// Get Message Status - is retrived via MessageSummary.
virtual bool setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask) = 0;

	// 
virtual bool groupSubscribe(const std::string &groupId, bool subscribe)     = 0;

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

	int mRating;
	int mPeersRating;
	std::string mComment;
};




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


        /* changed? */
virtual bool updated() = 0;


	/* INCLUDES INTERFACE FROM RS TOKEN SERVICE */
	//////////////////////////////////////////////////////////////////////////////


        /* Specific Service Data */
virtual bool    getGroupData(const uint32_t &token, RsIdGroup &group) = 0;
virtual bool    getMsgData(const uint32_t &token, RsIdMsg &msg) = 0;

virtual bool    createGroup(RsIdGroup &group) = 0;
virtual bool    createMsg(RsIdMsg &msg) = 0;



	/* In the Identity System - You don't access the Messages Directly.
	 * as they represent idividuals opinions....
	 * This is reflected in the TokenService calls returning false.
	 *
	 * Below is the additional interface to look at reputation.
	 */

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
