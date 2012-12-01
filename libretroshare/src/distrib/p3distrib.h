/*
 * libretroshare/src/distrib: p3distrib.h
 *
 *
 * Copyright 2004-2011 by Robert Fernie.
 *           2010-2011 Christopher Evi-Parker
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
#include "serialiser/rsdistribitems.h"

#include <openssl/ssl.h>
#include <openssl/evp.h>

#include <set>
#include <vector>

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


/*
 * A data structure to store dummy (missing) msgs.
 * They are added to the GroupInfo if there is a missing parent Msg of thread Msg
 * Basic Logic is:
 *
 */

class RsDistribDummyMsg
{
	public:
	RsDistribDummyMsg( std::string tId, std::string pId, std::string mId, uint32_t ts);
	RsDistribDummyMsg() { return; }
	std::string threadId;
	std::string parentId;
	std::string msgId;

	uint32_t    timestamp;
        time_t childTS; /* timestamp of most recent child */
};


//! for storing group keys to members of a group
/*!
 * This key but be of many types, including private/public publish key, or admin prite key for group
 * @see p3GroupDistrib
 */
class GroupKey
{
	public:

		GroupKey()
		:type(0), startTS(0), endTS(0), key(NULL) { return; }

		uint32_t type; /// whether key is full or public
		std::string keyId;
		time_t   startTS, endTS;
		EVP_PKEY *key; /// actual group key in evp format
};

//! used to store group picture
/*!
 * ensures use of png image format
 * @see p3GroupDistrib
 */
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

	unsigned char* pngImageData; /// pointer to image data in png format
	int imageSize;
};

//! used by p3groupDistrib to store mirror info found in rsDistribGroup (i.e. messages, posts, etc)
/*!
 * used by p3Groudistrib to store group info, also used to communicate group information
 * to p3groupdistrib inherited classes. contain
 * @see rsDistribGroup
 */
class GroupInfo
{
	public:

		GroupInfo()
		:distribGroup(NULL), grpFlags(0), pop(0), lastPost(0), flags(0), grpChanged(false)
		{
			return;
		}
		virtual ~GroupInfo() ;

		std::string grpId; /// the group id
		RsDistribGrp *distribGroup; /// item which contains further information on group

		std::list<std::string> sources;
		std::map<std::string, RsDistribMsg *> msgs;
		std::map<std::string, RsDistribDummyMsg> dummyMsgs; // dummyMsgs.

	/***********************************/

		/* Copied from DistribGrp */
		std::wstring grpName;
		std::wstring grpDesc; /// group description
		std::wstring grpCategory;
		uint32_t     grpFlags;     /// PRIVACY & AUTHENTICATION


		uint32_t pop; 	    /// popularity sources.size()
		time_t   lastPost;  /// modded as msgs added

	/***********************************/

		uint32_t flags;     /// PUBLISH, SUBSCRIBE, ADMIN


		std::string publishKeyId; /// current active Publish Key
		std::map<std::string, GroupKey> publishKeys;

		GroupKey adminKey;


		GroupIcon grpIcon;
		/* NOT USED YET */

		std::map<std::string, RsDistribMsg* > decrypted_msg_cache; /// stores a cache of messages that have been decrypted

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


typedef std::map<std::string, std::list<CacheData> > CacheOptData;


//! Cache based service to implement group messaging
/*!
 *
 * Group Description:
 *
 * Master Public/Private Key: (Admin Key) used to control
 *	Group Name/Description/Icon.
 * 	Filter Lists.
 *	Publish Keys.
 *
 * Publish Keys.
 * TimeStore Length determined by inheriting class
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
 * Group id is the public admin keys id
 *
 */

/* 
 * To Handle Cache Data Loading.... we want to be able to seperate Historical
 * from new data (primarily for the gui's benefit).
 * to do this we have a mHistoricalCaches flag, which is automatically raised at startup, 
 * and a function is called to cancel it (HistoricalCachesDone()).
 */

class CacheDataPending
{
	public:

	CacheDataPending(const CacheData &data, bool local, bool historical);
	CacheData mData;
	bool	  mLocal;
	bool      mHistorical;
};

class p3GroupDistrib: public CacheSource, public CacheStore, public p3Config, public p3ThreadedService
{
	public:

		p3GroupDistrib(uint16_t subtype,
			CacheStrapper *cs, CacheTransfer *cft,
			std::string sourcedir, std::string storedir, std::string keyBackUpDir,
			uint32_t configId,
					uint32_t storePeriod, uint32_t pubPeriod);

		virtual ~p3GroupDistrib() ;

/***************************************************************************************/
/******************************* CACHE SOURCE / STORE Interface ************************/
/***************************************************************************************/
/* TO FINISH */

	public:

		virtual bool   loadLocalCache(const CacheData &data); /// overloaded from Cache Source
		virtual int    loadCache(const CacheData &data); /// overloaded from Cache Store


		/* From RsThread */
		virtual void run(); /* called once the thread is started */

		void 	HistoricalCachesDone(); // called when Stored Caches have been added to Pending List.


	private:

		// derived from CacheSource
		virtual bool isPeerAcceptedAsCacheReceiver(const std::string& ssl_id) ;
		// derived from CacheStore
		virtual bool isPeerAcceptedAsCacheProvider(const std::string& ssl_id) ;

		/* these lists are filled by the overloaded fns... then cleared by the thread */
		bool mHistoricalCaches; // initially true.... falsified by HistoricalCachesDone() 
		std::list<CacheDataPending> mPendingCaches;

		/* top level load */
		int  	loadAnyCache(const CacheData &data, bool local, bool historical);

			/* load cache files */
		void	loadFileGroups(const std::string &filename, const std::string &src, bool local, bool historical);
		void	loadFileMsgs(const std::string &filename, const CacheData& , bool local, bool historical);
		bool backUpKeys(const std::list<RsDistribGrpKey* > &keysToBackUp, std::string grpId);
		void locked_sharePubKey();

		/*!
		 * Attempt to load public key from recvd list if it exists for grpId
		 * @param grpId the id for the group for which private publish key is wanted
		 */
         bool	attemptPublishKeysRecvd();




         /*!
          * Simply load cache opt messages
          * @param data
          */
         void loadCacheOptMsgs(const CacheData& data, const std::string& grpId);

	protected:

			/* load cache msgs */

         /*!
          * processes cache opt request by loading data for group
          * @param grpId the group to process request for
          * @return false if group does not exist
          */
         bool processCacheOptReq(const std::string &grpId);

		/*!
		 * msg is loaded to its group and republished,
		 * msg decrypted if grp is private
		 * @param msg msg to loaded
		 * @param src src of msg (peer id)
		 * @param local is this a local cache msg (your msg)
		 */
		bool	loadMsg(RsDistribSignedMsg *msg, const std::string &src, bool local, bool historical);

		/*!
		 * msg is loaded to its group and republished,
		 * msg decrypted if grp is private
		 * @param msg msg to loaded
		 * @param src src of msg (peer id)
		 * @param local is this a local cache msg (your msg)
		 */
		bool locked_loadMsg(RsDistribSignedMsg *newMsg, const std::string &src, bool local, bool historical);

		/*!
		 * adds newgrp to grp set, GroupInfo type created and stored
		 * @param newGrp grp to be added
		 */
		bool	loadGroup(RsDistribGrp *newGrp, bool historical);

		/*!
		 * Adds new keys dependent on whether it is an admin or publish key
                 * on return resource pointed to by newKey should be considered invalid
		 * @param newKey key to be added
                 * @return if key is loaded to group or stored return true
		 */
                bool    loadGroupKey(RsDistribGrpKey *newKey, bool historical);



/***************************************************************************************/
/***************************************************************************************/

/***************************************************************************************/
/**************************** Create Content *******************************************/
/***************************************************************************************/
/* TO FINISH */

	public:

		/*!
		 * This create a distributed grp which is sent via cache system to connected peers
		 * @param name name of the group created
		 * @param desc description of the group
		 * @param flags privacy flag
		 * @param pngImageData pointer to image data, data is copied
		 * @param imageSize size of the image passed
		 * @return id of the group
		 */
		std::string createGroup(std::wstring name, std::wstring desc, uint32_t flags, unsigned char *pngImageData, uint32_t imageSize);

		/*!
		 * msg is packed into a signed message (and encrypted msg grp is private) and then sent via cache system to connnected peers
		 * @param msg
		 * @param personalSign whether to personal to sign image (this is done using gpg cert)
		 * @return the msg id
		 */
		std::string publishMsg(RsDistribMsg *msg, bool personalSign);

		/*!
		 * note: call back to locked_eventDuplicateMSg is made on execution
		 * @param grpId id of group to subscribe to
		 * @param subscribe true to subscribe and vice versa
		 * @return
		 */
		bool	subscribeToGroup(const std::string &grpId, bool subscribe);



		/***************************************************************************************/
		/***************************************************************************************/

		/***************************************************************************************/
		/****************************** Access Content   ***************************************/
		/***************************************************************************************/

	public:

		/*!
		 *  get Group Lists
		 */
		bool 	getAllGroupList(std::list<std::string> &grpids);
		bool 	getSubscribedGroupList(std::list<std::string> &grpids);
		bool 	getPublishGroupList(std::list<std::string> &grpids);

		/*!
		 *
		 * @param popMin lower limit for a grp's populairty in grpids
		 * @param popMax upper limit  for a grp's popularity in grpids
		 * @param grpids grpids of grps which adhere to upper and lower limit of popularity
		 * @return nothing returned
		 */
		void 	getPopularGroupList(uint32_t popMin, uint32_t popMax, std::list<std::string> &grpids);


			/* get Msg Lists */
		bool 	getAllMsgList(const std::string& grpId, std::list<std::string> &msgIds);
		bool 	getParentMsgList(const std::string& grpId, const std::string& pId, std::list<std::string> &msgIds);
		bool 	getTimePeriodMsgList(const std::string& grpId, uint32_t timeMin,
							uint32_t timeMax, std::list<std::string> &msgIds);


		GroupInfo *locked_getGroupInfo(const std::string& grpId);
		RsDistribMsg *locked_getGroupMsg(const std::string& grpId, const std::string& msgId);

		/*!
		 * for retrieving the grpList for which public keys are available
		 */
		void getGrpListPubKeyAvailable(std::list<std::string>& grpList);

		/* Filter Messages */

/***************************************************************************************/
/***************************** Event Feedback ******************************************/
/***************************************************************************************/

	protected:
		/*!
		 *  root version (p3Distrib::) of this function must be called
		 */
		virtual void locked_notifyGroupChanged(GroupInfo &info, uint32_t flags, bool historical);

		/*!
		 * client (inheriting class) should use this to determing behaviour of
		 * their service when a duplicate msg is found
		 * @param group should be called when duplicate message loaded
		 * @param the duplicate message
		 * @param id
		 * @param historical: is this msg from an historical cache
		 * @return successfully executed or not
		 */
		virtual bool locked_eventDuplicateMsg(GroupInfo *, RsDistribMsg *, const std::string& id, bool historical) = 0;

		/*!
		 * Inheriting class should implement this as a response to a new msg arriving
		 * @param
		 * @param
		 * @param id src of msg (peer id)
		 * @param historical: is this msg from an historical cache
		 * @return
		 */
		virtual bool locked_eventNewMsg(GroupInfo *, RsDistribMsg *, const std::string& id, bool historical) = 0;

/***************************************************************************************/
/********************************* p3Config ********************************************/
/***************************************************************************************/
/* TO FINISH */

	protected:

		virtual RsSerialiser *setupSerialiser();
		virtual bool saveList(bool &cleanup, std::list<RsItem *>& saveList);
		virtual void 	saveDone();
		virtual bool    loadList(std::list<RsItem *>& load);

		/*!
		 * called by top class, child can use to save configs
		 */
		virtual std::list<RsItem *> childSaveList() = 0;

		/*!
		 * called by top class, child can use to load configs
		 */
		virtual bool childLoadList(std::list<RsItem *>& configSaves) = 0;

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

		/*!
		 *	adds msg to pending msg map
		 *  @param msg a signed message by peer
		 */
		void	locked_toPublishMsg(RsDistribSignedMsg *msg);

		/*!
		 *  adds pending msg
		 */
		void 	publishPendingMsgs();

		/*!
		 * sends created groups to cache, to be passed to cache listeners
		 */
		void 	publishDistribGroups();

		/*!
		 * removes old caches based on store period (anything that has been in local cache longer
		 * than the store period is deleted
		 * @param now the current time when method is called
		 */
		void	clear_local_caches(time_t now);

		/*!
		 * assumes RsDistribMtx is locked when call is made
		 */
		void    locked_publishPendingMsgs();

		/*!
		 * @return cache sub id
		 */
		uint16_t locked_determineCacheSubId();

		/**
		 * grp keys are backed up when a grp is created this allows user to retrieve lost keys in case config saving fails
		 * @param grpId the grpId id for which backup keys should be restored
		 * @return false if failed and vice versa
		 */
		virtual bool restoreGrpKeys(const std::string& grpId); /// restores a group keys from backup

		/**
		 * Allows user to send keys to a list of peers
		 * @param grpId the group for which to share public keys
		 * @param peers The peers to which public keys should be sent
		 */
		virtual bool sharePubKey(std::string grpId, std::list<std::string>& peers);

		/**
		 * Attempt to receive publication keys
		 */
                virtual void receivePubKeys();

		/**
		 * Allows group admin(s) to change group icon, description and name
		 *@param grpId group id
		 *@param gi the changes to grp name, icon, and description should be reflected here
		 */
		virtual bool locked_editGroup(std::string grpId, GroupInfo& gi);


	/***************************************************************************************/
	/***************************************************************************************/

	/***************************************************************************************/
	/*************************** Overloaded Functions **************************************/
	/***************************************************************************************/

		/*!
		 * Overloaded by inherited classes to Pack/UnPack their messages
		 * @return inherited class's serialiser
		 */
		virtual RsSerialType *createSerialiser() = 0;

		/*! Used to Create/Load Cache Files only
		 * @param bio binary i/o
		 * @param src peer id from which write/read content originates
		 * @param bioflags read write permision for bio
		 * @return pointer to pqistore instance
		 */
		virtual pqistore *createStore(BinInterface *bio, const std::string &src, uint32_t bioflags);

		virtual bool    locked_checkGroupInfo(GroupInfo  &info, RsDistribGrp *newGrp);
		virtual bool    locked_updateGroupInfo(GroupInfo &info, RsDistribGrp *newGrp);
		virtual bool    locked_checkGroupKeys(GroupInfo &info);

		/*!
		 * @param info group for which admin key will be added to
		 * @param newKey admin key
		 * @return true if key successfully added
		 */
		virtual bool    locked_updateGroupAdminKey(GroupInfo &info, RsDistribGrpKey *newKey);


		/*!
		 * @param info group for which publish key will be added to
		 * @param newKey publish key
		 * @return true if publish key successfully added
		 */
		virtual bool    locked_updateGroupPublishKey(GroupInfo &info, RsDistribGrpKey *newKey);

		/*!
		 * Use this to retrieve packed message from a signed message
		 * @param newMsg signed message
		 * @return pointer to unpacked msg
		 */
		virtual RsDistribMsg* unpackDistribSignedMsg(RsDistribSignedMsg *newMsg);


		/*!
		 * message is checked to see if it is in a valid time range
		 * @param info
		 * @param msg message to be checked
		 * @return false if msg is outside correct time range
		 */
		virtual bool    locked_checkDistribMsg(GroupInfo &info, RsDistribMsg *msg);

		/*!
		 * chooses the best publish key based on it being full and latest
		 * @param info group to choose publish key
		 * @return true if a publish key could be found
		 */
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


/***************************************************************************************/
/**************************** DummyMsgs Functions **************************************/
/***************************************************************************************/
	public:

bool 	locked_CheckNewMsgDummies(GroupInfo &info, RsDistribMsg *msg, std::string id, bool historical);
bool 	locked_addDummyMsg(GroupInfo &info, std::string threadId, std::string parentId, std::string msgId, uint32_t ts);
bool 	locked_clearDummyMsg(GroupInfo &info, std::string msgId);
bool    locked_updateDummyChildTS(GroupInfo &gi, std::string parentId, time_t updateTS); // NOTE MUST BE MERGED WITH nromal version.

bool 	locked_printAllDummyMsgs();
bool 	locked_printDummyMsgs(GroupInfo &info);

	/* access the dummy msgs */
bool    getDummyParentMsgList(const std::string& grpId, const std::string& pId, std::list<std::string> &msgIds);
RsDistribDummyMsg *locked_getGroupDummyMsg(const std::string& grpId, const std::string& msgId);


	/* key cache functions - we use .... (not overloaded)
 	 */

	/* storage */
	protected:

		RsMutex distribMtx; /// Protects all class atrributes
		std::string mOwnId; /// rs peer id

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
		std::string mKeyBackUpDir;
		const std::string BACKUP_KEY_FILE;

		std::map<std::string, RsDistribGrpKey* > mRecvdPubKeys; /// full publishing keys received from users
		std::map<std::string, std::list<std::string> > mPendingPubKeyRecipients; /// peers to receive publics key for a given grp
                std::set<std::string> mPubKeyAvailableGrpId; // groups id for which public keys are available
		time_t mLastKeyPublishTime, mLastRecvdKeyTime;


		/**** cache opt ****/

		/*
		 * 1. when rs starts it loads only subscribed groups
		 * 2. and for unsubscribed groups these are store with their grp to cache mappings
		 * 3. when user clicks on a group this activates process cache which loads cache for only that group
		 *
		 */

		/// stores map of grp to cache mapping
		CacheOptData mGrpCacheMap;

		/// group subscribed to at start of rs
		std::set<std::string> mSubscribedGrp;

		/// unsubscribed groups that are already loaded
		std::set<std::string> mCacheOptLoaded;

		/// current exception group
		std::string mCurrGrpException;




};


/***************************************************************************************/
/***************************************************************************************/

#endif // P3_GENERIC_DISTRIB_HEADER
