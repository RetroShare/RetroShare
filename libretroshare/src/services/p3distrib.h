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
#include "serialiser/rsdistribitems.h"

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

		std::string grpId; /// the group id
		RsDistribGrp *distribGroup; /// item which contains further information on group

		std::list<std::string> sources;
		std::map<std::string, RsDistribMsg *> msgs;

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
class p3GroupDistrib: public CacheSource, public CacheStore, public p3Config, public p3Service
{
	public:

		p3GroupDistrib(uint16_t subtype,
			CacheStrapper *cs, CacheTransfer *cft,
			std::string sourcedir, std::string storedir, std::string keyBackUpDir,
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
		bool backUpKeys(const std::list<RsDistribGrpKey* > &keysToBackUp, std::string grpId);
		void locked_sharePubKey();

	protected:

			/* load cache msgs */

		/*!
		 * msg is loaded to its group and republished,
		 * msg decrypted if grp is private
		 * @param msg msg to loaded
		 * @param src src of msg (peer id)
		 * @param local is this a local cache msg (your msg)
		 */
		void	loadMsg(RsDistribSignedMsg *msg, std::string src, bool local);

		/*!
		 * adds newgrp to grp set, GroupInfo type created and stored
		 * @param newGrp grp to be added
		 */
		void	loadGroup(RsDistribGrp *newGrp);

		/*!
		 * Adds new keys dependent on whether it is an admin or publish key
		 * @param newKey key to be added
		 */
		void    loadGroupKey(RsDistribGrpKey *newKey);


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
		 *
		 * @param grpId id of group to subscribe to
		 * @param subscribe true to subscribe and vice versa
		 * @return
		 */
		bool	subscribeToGroup(std::string grpId, bool subscribe);


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
		 */
		virtual void locked_notifyGroupChanged(GroupInfo &info, uint32_t flags);

		/*!
		 * client (inheriting class) should use this to determing behaviour of
		 * their service when a duplicate msg is found
		 * @param group should be called when duplicate message loaded
		 * @param the duplicate message
		 * @param id
		 * @return successfully executed or not
		 */
		virtual bool locked_eventDuplicateMsg(GroupInfo *, RsDistribMsg *, std::string id) = 0;

		/*!
		 * Inheriting class should implement this as a response to a new msg arriving
		 * @param
		 * @param
		 * @param id src of msg (peer id)
		 * @return
		 */
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
		virtual bool restoreGrpKeys(std::string grpId); /// restores a group keys from backup

		/**
		 * Allows user to send keys to a list of peers
		 * @param grpId the group for which to share public keys
		 * @param peers The peers to which public keys should be sent
		 */
		virtual bool sharePubKey(std::string grpId, std::list<std::string>& peers);

		/**
		 * Attempt to receive publication keys
		 */
		virtual void locked_receivePubKeys();

		/**
		 * This loads received pub keys
		 *
		 */
		virtual void locked_loadRecvdPubKeys();

		/**
		 * Allows group admin(s) to change group icon, description and name
		 *@param grpId group id
		 *@param gi the changes to grp name, icon, and description should be reflected here
		 */
		virtual bool locked_editGroup(std::string grpId, GroupInfo& gi);

		/**
		 * Encrypts data using envelope encryption (taken from open ssl's evp_sealinit )
		 * only full publish key holders can encrypt data for given group
		 *@param out
		 *@param outlen
		 *@param in
		 *@param inlen
		 */
		virtual bool encrypt(void *&out, int &outlen, const void *in, int inlen, std::string grpId);


		/**
		 * Decrypts data using evelope decryption (taken from open ssl's evp_sealinit )
		 * only full publish key holders can decrypt data for a group
		 *@param out where decrypted data is written to
		 *@param outlen
		 *@param in
		 *@param inlen
		 */
		virtual bool decrypt(void *&out, int &outlen, const void *in, int inlen, std::string grpId);

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
		virtual pqistore *createStore(BinInterface *bio, std::string src, uint32_t bioflags);

		/*!
		 * checks to see if admin signature is valid
		 * @param newGrp grp to validate
		 * @return true if group's signature is valid
		 */
		virtual bool    validateDistribGrp(RsDistribGrp *newGrp);
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
		 * uses groupinfo public key to verify signature of signed message
		 * @param info groupinfo for which msg is meant for
		 * @param msg
		 * @return false if verfication of signature is not passed
		 */
		virtual bool    locked_validateDistribSignedMsg(GroupInfo &info, RsDistribSignedMsg *msg);

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
		time_t mLastKeyPublishTime, mLastRecvdKeyTime;
	
};


/***************************************************************************************/
/***************************************************************************************/

#endif // P3_GENERIC_DISTRIB_HEADER
