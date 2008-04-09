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
#include "pqi/pqistreamer.h"
#include "pqi/p3cfgmgr.h"
#include "services/p3service.h"
#include "dbase/cachestrapper.h"

//#include "util/rsthreads.h"


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
 *
 *****************************************************************
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
 *
 */

/* Our Mode for the Group */
const uint32_t 	RS_GRPDISTRIB_SUBSCRIBED = 0x0001;
const uint32_t 	RS_GRPDISTRIB_PUBLISH	 = 0x0002;



/* Group Type */
const uint32_t RS_DISTRIB_PRIVATE	= 0x0001;	/* retain Private Key ( Default ) */
const uint32_t RS_DISTRIB_PUBLIC	= 0x0002;	/* share  All Keys */
const uint32_t RS_DISTRIB_ENCRYPTED	= 0x0004;	/* encrypt Msgs */

class RsSignature
{
	public:

	uint32_t type;	/* MASTER, PUBLISH, PERSONAL */
	std::string signerId;
	std::string signature;
};

class RsAuthMsg
{
	public:

	RsItem *msg;
	std::string hash;
	std::map<std::string, RsSignature> signs;
};


class RsKey
{
	public:

	uint32_t type; /* PUBLIC, PRIVATE */
	uint32_t type2; /* MASTER, PUBLISH, PERSONAL (not sent) */

	EVP_PKEY *key;
};


const uint32_t GROUP_MAX_FWD_OFFSET = (60 * 60 * 24 * 2); /* 2 Days */

/************* The Messages that are serialised ****************/
class RsDistribMsg: public RsItem
{
	public:
	RsDistribMsg(); //uint16_t type);

virtual void clear();
virtual std::ostream& print(std::ostream&, uint16_t);

	std::string grpId; 
	std::string headId;    /* head of the thread */
	std::string parentId;  /* parent id */
	time_t      timestamp;

	/* This data is not Hashed (set to zero - before hash calced) */
	std::string msgId; /* SHA1 Hash */
	std::string grpSignature; /* sign of msgId */
	std::string sourceSignature; /* sign of msgId */

};

class RsDistribGrp: public RsItem
{
	public:
	RsDistribGrp(); //uint16_t type);

virtual void clear();
virtual std::ostream& print(std::ostream&, uint16_t);

	RsKey rsKey;
};

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



/************* The Messages that are serialised ****************/

//class PIXMAP; 

class GroupInfo
{
	public:

	std::string grpId;
	std::string grpName;
	std::string grpDesc;

	std::string category; 
	//PIXMAP *GroupIcon;

	//RSA_KEY privateKey;
	//RSA_KEY publicKey;

	bool publisher, allowAnon, allowUnknown;
	bool subscribed, listener;

	uint32_t type;
	uint32_t pop;
	uint32_t flags;

	std::list<std::string> sources;

	std::map<std::string, RsDistribMsg *> msgs;
};

class GroupCache
{
	public:

	std::string filename;
	time_t start, end;
	uint16_t cacheSubId;
};


class p3GroupDistrib: public CacheSource, public CacheStore, public p3Config
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
virtual bool   loadLocalCache(const CacheData &data); /* overloaded from Cache Source */
virtual int    loadCache(const CacheData &data); /* overloaded from Cache Store */

	private:
	/* top level load */
int  	loadAnyCache(const CacheData &data, bool local);

	/* load cache files */
void	loadFileGroups(std::string filename, std::string src, bool local);
void	loadFileMsgs(std::string filename, uint16_t cacheSubId, std::string src, bool local);

	/* load cache msgs */	
void	loadMsg(RsDistribMsg *msg, std::string src, bool local);
void	loadGroup(RsDistribGrp *newGrp);

/***************************************************************************************/
/***************************************************************************************/

/***************************************************************************************/
/**************************** Create Content *******************************************/
/***************************************************************************************/
/* TO FINISH */

	public:
std::string createGroup(std::string name, uint32_t flags);
//std::string modGroupDescription(std::string grpId, std::string discription);
//std::string modGroupIcon(std::string grpId, PIXMAP *icon);

void		publishMsg(RsDistribMsg *msg, bool personalSign);

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
bool 	getTimePeriodMsgList(std::string grpId, uint32_t timeMin, 
					uint32_t timeMax, std::list<std::string> &msgIds);

RsDistribMsg *locked_getGroupMsg(std::string grpId, std::string msgId);

/* TO FINISH DEFINITIONS */

	/* get Details */
//bool 	getGroupDetails(std::string grpId, RsExternalDistribGroup &grp);
//bool 	getGroupMsgDetails(std::string grpId, std::string msgId, RsExternalDistribMsg &msg);

	/* Filter Messages */

/***************************************************************************************/
/***************************************************************************************/

/***************************************************************************************/
/********************************* p3Config ********************************************/
/***************************************************************************************/
/* TO FINISH */

	protected:

virtual RsSerialiser *setupSerialiser();
virtual std::list<RsItem *> saveList(bool &cleanup);
virtual bool    loadList(std::list<RsItem *> load);

/***************************************************************************************/
/***************************************************************************************/
/* TO FINISH */


	public:
void	tick();

/***************************************************************************************/
/**************************** Publish Content ******************************************/
/***************************************************************************************/
/* TO FINISH */
	protected:

	/* create/mod cache content */
void	toPublishMsg(RsDistribMsg *msg);
void 	publishPendingMsgs();
void 	publishDistribGroups();
void	clear_local_caches(time_t now);

void    locked_publishPendingMsgs();
uint16_t determineCacheSubId();


/***************************************************************************************/
/***************************************************************************************/

/***************************************************************************************/
/***************************** Utility Functions ***************************************/
/***************************************************************************************/
/* TO FINISH */
	/* utilities */
pqistreamer *createStreamer(BinInterface *bio, std::string src, uint32_t bioflags);
std::string HashRsItem(const RsItem *item);

/***************************************************************************************/
/***************************************************************************************/

	/* key cache functions - we use .... (not overloaded)
 	 */

	private:	
	
	/* storage */

	RsMutex distribMtx; /* Protects All Data Below */

	std::string mOwnId;
	std::list<GroupCache> mLocalCaches;
	std::map<std::string, GroupInfo> mGroups;
	uint32_t mStorePeriod, mPubPeriod;
	time_t mNextPublishTime;

	std::list<RsDistribMsg *> mPendingPublish;
};


/***************************************************************************************/
/***************************************************************************************/
/*
 * Structure of the Storage:
 *
 * map<std::string(id) -> GroupInfo>
 *
 *
 *
 *
 */
/***************************************************************************************/
/***************************************************************************************/

#endif // P3_GENERIC_DISTRIB_HEADER
