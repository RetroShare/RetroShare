/*
 * libretroshare/src/services: p3distrib.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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

#ifndef P3_GENERIC_DISTRIB_HEADER
#define P3_GENERIC_DISTRIB_HEADER

#include "pqi/pqi.h"
#include "pqi/pqistore.h"
#include "pqi/p3cfgmgr.h"
#include "services/p3service.h"
#include "dbase/cachestrapper.h"
#include "serialiser/rsforumitems.h"

#include <openssl/ssl.h>
#include <openssl/evp.h>

/* 
 * Group Messages....
 * 
 * Forums / Channels / Blogs...
 *
 *
 * Plan.
 *
 * (1) First create basic structures .... algorithms.
 *
 * (2) integrate with Cache Source/Store for data transmission.
 * (3) integrate with Serialiser for messages
 * (4) bring over the final key parts from existing p3channel.
 */

const uint32_t GROUP_MAX_FWD_OFFSET = (60 * 60 * 24 * 2); /* 2 Days */

/************* The Messages that are serialised ****************/

#if 0

class RsConfigDistrib: public RsSerialType
{
	public:

	RsConfigDistrib();

};

class RsSerialDistrib: public RsSerialType
{
	public:

	RsSerialDistrib();

};

#endif


/************* The Messages that are serialised ****************/

#if 0
const uint32_t GROUP_KEY_TYPE_MASK		= 0x000f;
const uint32_t GROUP_KEY_DISTRIB_MASK		= 0x00f0;

const uint32_t GROUP_KEY_TYPE_PUBLIC_ONLY	= 0x0001;
const uint32_t GROUP_KEY_TYPE_FULL		= 0x0002;
const uint32_t GROUP_KEY_DISTRIB_PUBLIC		= 0x0010;
const uint32_t GROUP_KEY_DISTRIB_PRIVATE	= 0x0020;
const uint32_t GROUP_KEY_DISTRIB_ADMIN		= 0x0040;
#endif


//! key to be sent member or members of a groups
/*!
 * This key but be of many types, including private/public publish key, or admin prite key for group
 */
class GroupKey
{
	public:

	GroupKey()
	:type(0), startTS(0), endTS(0), key(NULL) { return; }

	uint32_t type;
	std::string keyId;
	time_t   startTS, endTS;
	EVP_PKEY *key; /// public key
};

class GroupIcon{
public:
	GroupIcon(): pngImageData(NULL), imageSize(0) {
		return;
	}

	~GroupIcon(){

		if((pngImageData != NULL) && (imageSize > 0))
			delete[] pngImageData;

			return;
	}

	unsigned char* pngImageData;
	int imageSize;
};

//! aggregates various information on group activities (i.e. messages, posts, etc)
/*!
 * The aim is to use this to keep track of group changes, so client can respond (get messages, post etc)
 */
class GroupInfo
{
	public:
	GroupInfo()
	:distribGroup(NULL), grpFlags(0), pop(0), lastPost(0), flags(0), grpChanged(false)
	{
		return;
	}

	std::string grpId; /// the group id
	RsDistribGrp *distribGroup; /// item which contains further information on group

	std::list<std::string> sources;
	std::map<std::string, RsDistribMsg *> msgs;

/***********************************/

	/* Copied from DistribGrp */
	std::wstring grpName;
	std::wstring grpDesc;
	std::wstring grpCategory;
	uint32_t     grpFlags;     /* PRIVACY & AUTHEN */


	uint32_t pop; 	    /// sources.size()
	time_t   lastPost;  /// modded as msgs added

/***********************************/

	uint32_t flags;     /// PUBLISH, SUBSCRIBE, ADMIN


	std::string publishKeyId; /// current active Publish Key
	std::map<std::string, GroupKey> publishKeys;

	GroupKey adminKey;

	/* NOT USED YET */

	GroupIcon grpIcon;

	bool publisher, allowAnon, allowUnknown;
	bool subscribed, listener;

	uint32_t type;

	/// FLAG for Client - set if changed
	bool grpChanged;
};



std::ostream &operator<<(std::ostream &out, const GroupInfo &info);

//! information on what cache stores group info
/*!
 * This can refer to idividual cache message, data etc
 */
class GroupCache
{
	public:

	std::string filename;
	time_t start, end;
	uint16_t cacheSubId; /// used to resolve complete cache id
};

	/* Flags for locked_notifyGroupChanged() ***/

const uint32_t GRP_NEW_UPDATE   = 0x0001;
const uint32_t GRP_UPDATE       = 0x0002;
const uint32_t GRP_LOAD_KEY     = 0x0003;
const uint32_t GRP_NEW_MSG      = 0x0004;
const uint32_t GRP_SUBSCRIBED   = 0x0005;
const uint32_t GRP_UNSUBSCRIBED = 0x0006;


//! Class service to implement group messages
/*!
 *
 * Group Description:
 *
 * Master Public/Private Key: (Admin Key) used to control
 *	Group Name/Description/Icon.
 * 	Filter Lists.
 *	Publish Keys.
 *
 * Publish Keys. (multiple possible)
 * Filters: blacklist or whitelist.
 * TimeStore Length ??? (could make it a minimum of this and system one)
 *
 * Everyone gets:
 *    Master Public Key.
 *    Publish Public Keys.
 *	blacklist, or whitelist filter. (Only useful for Non-Anonymous groups)
 *	Name, Desc,
 *	etc.
 *
 * Admins get Master Private Key.
 * Publishers get Publish Private Key.
 *	- Channels only some get publish key.
 *	- Forums everyone gets publish private key.
 *
 * Create a Signing structure for Messages in general.
 */
class p3GroupDistrib: public CacheSource, public CacheStore, public p3Config, public nullService
{
	public:

	p3GroupDistrib(uint16_t subtype, 
		CacheStrapper *cs, CacheTransfer *cft,
		std::string sourcedir, std::string storedir, 
		uint32_t configId, 
                uint32_t storePeriod, uint32_t pubPeriod);


/***************************************************************************************/
/******************************* CACHE SOURCE / STORE Interface ************************/
/***************************************************************************************/
/* TO FINISH */

	public:
virtual bool   loadLocalCache(const CacheData &data); /// overloaded from Cache Source
virtual int    loadCache(const CacheData &data); /// overloaded from Cache Store

	private:
	/* top level load */
int  	loadAnyCache(const CacheData &data, bool local);

	/* load cache files */
void	loadFileGroups(std::string filename, std::string src, bool local);
void	loadFileMsgs(std::string filename, uint16_t cacheSubId, std::string src, uint32_t ts, bool local);

	protected:
	/* load cache msgs */	
void	loadMsg(RsDistribSignedMsg *msg, std::string src, bool local);
void	loadGroup(RsDistribGrp *newGrp);
void    loadGroupKey(RsDistribGrpKey *newKey);


/***************************************************************************************/
/***************************************************************************************/

/***************************************************************************************/
/**************************** Create Content *******************************************/
/***************************************************************************************/
/* TO FINISH */

	public:
std::string createGroup(std::wstring name, std::wstring desc, uint32_t flags, unsigned char*, uint32_t imageSize);
//std::string modGroupDescription(std::string grpId, std::string discription);
//std::string modGroupIcon(std::string grpId, PIXMAP *icon);

std::string publishMsg(RsDistribMsg *msg, bool personalSign);

bool	subscribeToGroup(std::string grpId, bool subscribe);


/***************************************************************************************/
/***************************************************************************************/

/***************************************************************************************/
/****************************** Access Content   ***************************************/
/***************************************************************************************/
	public:

	/* get Group Lists */
bool 	getAllGroupList(std::list<std::string> &grpids);
bool 	getSubscribedGroupList(std::list<std::string> &grpids);
bool 	getPublishGroupList(std::list<std::string> &grpids);
bool 	getPopularGroupList(uint32_t popMin, uint32_t popMax, std::list<std::string> &grpids);


	/* get Msg Lists */
bool 	getAllMsgList(std::string grpId, std::list<std::string> &msgIds);
bool 	getParentMsgList(std::string grpId, std::string pId, std::list<std::string> &msgIds);
bool 	getTimePeriodMsgList(std::string grpId, uint32_t timeMin, 
					uint32_t timeMax, std::list<std::string> &msgIds);


GroupInfo *locked_getGroupInfo(std::string grpId);
RsDistribMsg *locked_getGroupMsg(std::string grpId, std::string msgId);

	/* Filter Messages */

/***************************************************************************************/
/***************************** Event Feedback ******************************************/
/***************************************************************************************/

	protected:
/*!
 *  root version (p3Distrib::) of this function must be called
 **/
virtual void locked_notifyGroupChanged(GroupInfo &info, uint32_t flags);
virtual bool locked_eventDuplicateMsg(GroupInfo *, RsDistribMsg *, std::string id) = 0;
virtual bool locked_eventNewMsg(GroupInfo *, RsDistribMsg *, std::string id) = 0;

/***************************************************************************************/
/********************************* p3Config ********************************************/
/***************************************************************************************/
/* TO FINISH */

	protected:

virtual RsSerialiser *setupSerialiser();
virtual std::list<RsItem *> saveList(bool &cleanup);
virtual void 	saveDone();
virtual bool    loadList(std::list<RsItem *> load);

/***************************************************************************************/
/***************************************************************************************/

	public:
virtual int 	tick(); /* overloaded form pqiService */

/***************************************************************************************/
/**************************** Publish Content ******************************************/
/***************************************************************************************/
/* TO FINISH */
	protected:

	/* create/mod cache content */
void	locked_toPublishMsg(RsDistribSignedMsg *msg);
void 	publishPendingMsgs();
void 	publishDistribGroups();
void	clear_local_caches(time_t now);

void    locked_publishPendingMsgs();
uint16_t locked_determineCacheSubId();



/***************************************************************************************/
/***************************************************************************************/

/***************************************************************************************/
/*************************** Overloaded Functions **************************************/
/***************************************************************************************/

	/* Overloaded by inherited classes to Pack/UnPack their messages */
virtual RsSerialType *createSerialiser() = 0;

	/* Used to Create/Load Cache Files only */
virtual pqistore *createStore(BinInterface *bio, std::string src, uint32_t bioflags);

virtual bool    validateDistribGrp(RsDistribGrp *newGrp);
virtual bool    locked_checkGroupInfo(GroupInfo  &info, RsDistribGrp *newGrp);
virtual bool    locked_updateGroupInfo(GroupInfo &info, RsDistribGrp *newGrp);
virtual bool    locked_checkGroupKeys(GroupInfo &info);
virtual bool    locked_updateGroupAdminKey(GroupInfo &info, RsDistribGrpKey *newKey);
virtual bool    locked_updateGroupPublishKey(GroupInfo &info, RsDistribGrpKey *newKey);


virtual bool    locked_validateDistribSignedMsg(GroupInfo &info, RsDistribSignedMsg *msg);
virtual RsDistribMsg* unpackDistribSignedMsg(RsDistribSignedMsg *newMsg);
virtual bool    locked_checkDistribMsg(GroupInfo &info, RsDistribMsg *msg);
virtual bool    locked_choosePublishKey(GroupInfo &info);


//virtual RsDistribGrp *locked_createPublicDistribGrp(GroupInfo &info);
//virtual RsDistribGrp *locked_createPrivateDistribGrp(GroupInfo &info);


/***************************************************************************************/
/***************************** Utility Functions ***************************************/
/***************************************************************************************/
/* TO FINISH */
	/* utilities */
std::string HashRsItem(const RsItem *item);
bool    locked_updateChildTS(GroupInfo &gi, RsDistribMsg *msg);

/***************************************************************************************/
/***************************************************************************************/

/***************************************************************************************/
/***************************** Utility Functions ***************************************/
/***************************************************************************************/
	public:

void	printGroups(std::ostream &out);
/*!
 * returns list of ids for group caches that have changed
 */
bool 	groupsChanged(std::list<std::string> &groupIds);

/***************************************************************************************/
/***************************************************************************************/

	/* key cache functions - we use .... (not overloaded)
 	 */

	/* storage */
	protected:

	RsMutex distribMtx; /// Protects All Data Below
	std::string mOwnId;

	private:	
	
	std::list<GroupCache> mLocalCaches;
	std::map<std::string, GroupInfo> mGroups;
	uint32_t mStorePeriod, mPubPeriod;

	/* Message Publishing */
	std::list<RsDistribSignedMsg *> mPendingPublish; 
	time_t mLastPublishTime;
	std::map<uint32_t, uint16_t> mLocalCacheTs;
	uint16_t mMaxCacheSubId;

	bool mGroupsChanged;
	bool mGroupsRepublish;

        std::list<RsItem *> saveCleanupList; /* TEMPORARY LIST WHEN SAVING */

};


/***************************************************************************************/
/***************************************************************************************/

#endif // P3_GENERIC_DISTRIB_HEADER
