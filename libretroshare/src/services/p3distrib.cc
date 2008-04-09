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

#include "services/p3distrib.h"
#include "pqi/pqibin.h"


//#include "pqi/pqiservice.h"
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


p3GroupDistrib::p3GroupDistrib(uint16_t subtype, 
		CacheStrapper *cs, CacheTransfer *cft,
		std::string sourcedir, std::string storedir, 
		uint32_t configId, 
		uint32_t storePeriod, uint32_t pubPeriod)

	:CacheSource(subtype, true, cs, sourcedir), 
	CacheStore(subtype, true, cs, cft, storedir), 
	p3Config(configId),
	mStorePeriod(storePeriod), 
	mPubPeriod(pubPeriod)
{
	/* not much yet */
	time_t now = time(NULL);

	/* set this a little in the future -> so we can 
	 * adjust the publish point if needed 
	 */

	mNextPublishTime = now + mPubPeriod / 4;

	return;
}


void	p3GroupDistrib::tick()
{
	time_t now = time(NULL);
	bool toPublish;

	{
		RsStackMutex stack(distribMtx);  /**** STACK LOCKED MUTEX ****/
		toPublish = (now > mNextPublishTime);
	}

	if (toPublish)
	{
		publishPendingMsgs();

		RsStackMutex stack(distribMtx);  /**** STACK LOCKED MUTEX ****/
		mNextPublishTime = now + mPubPeriod;
	}
}


/***************************************************************************************/
/***************************************************************************************/
	/********************** overloaded functions from Cache Store ******************/
/***************************************************************************************/
/***************************************************************************************/

int     p3GroupDistrib::loadAnyCache(const CacheData &data, bool local)
{
	/* if subtype = 0 -> FileGroup, else -> FileMsgs */

	std::string file = data.path;
	file += "/";
	file += data.name;

	if (data.cid.subid == 0)
	{
		loadFileGroups(file, data.pid, local);
	}
	else
	{
		loadFileMsgs(file, data.cid.subid, data.pid, local);
	}
	return true;
}

int    p3GroupDistrib::loadCache(const CacheData &data)
{
	return loadAnyCache(data, false);
}

bool 	p3GroupDistrib::loadLocalCache(const CacheData &data)
{
	return loadAnyCache(data, true);
}


/***************************************************************************************/
/***************************************************************************************/
	/********************** load Cache Files ***************************************/
/***************************************************************************************/
/***************************************************************************************/


/* No need for special treatment for 'own' groups.
 * configuration should be loaded before cache files.
 */
void	p3GroupDistrib::loadFileGroups(std::string filename, std::string src, bool local)
{
	/* create the serialiser to load info */
	BinInterface *bio = new BinFileInterface(filename.c_str(), BIN_FLAGS_READABLE);
	pqistreamer *streamer = createStreamer(bio, src, 0);

	RsItem *item;
	RsDistribGrp *newGrp;
	while(NULL != (item = streamer->GetItem()))
	{
		if (NULL == (newGrp = dynamic_cast<RsDistribGrp *>(item)))
		{
			/* wrong message type */
			delete item;
			continue;
		}
		loadGroup(newGrp);
	}

	delete streamer;

	return;
}


void	p3GroupDistrib::loadFileMsgs(std::string filename, uint16_t cacheSubId, std::string src, bool local)
{
	time_t now = time(NULL);
	time_t start = now;
	time_t end   = 0;

	/* create the serialiser to load msgs */
	BinInterface *bio = new BinFileInterface(filename.c_str(), BIN_FLAGS_READABLE);
	pqistreamer *streamer = createStreamer(bio, src, 0);

	RsItem *item;
	RsDistribMsg *newMsg;
	while(NULL != (item = streamer->GetItem()))
	{
		if (NULL == (newMsg = dynamic_cast<RsDistribMsg *>(item)))
		{
			/* wrong message type */
			delete item;
			continue;
		}

		if (local)
		{
			/* calc the range */
			if (newMsg->timestamp < start)
				start = newMsg->timestamp;
			if (newMsg->timestamp > end)
				end = newMsg->timestamp;
		}

		loadMsg(newMsg, src, local);
	}

	/* Check the Hash? */

	delete streamer;

	if (local)
	{
		GroupCache newCache;

		newCache.filename = filename;
		newCache.cacheSubId = cacheSubId;
		newCache.start = start;
		newCache.end = end;

		distribMtx.lock();   /********************    LOCKED MUTEX ************/

		mLocalCaches.push_back(newCache);		


/******************** This probably ain't necessary *******************/
#if 0
		/* adjust next Publish Time if needed */
		if ((end < now) && (end + mPubPeriod > mNextPublishTime))
		{
			mNextPublishTime = end + mPubPeriod;
		}
#endif 
/******************** This probably ain't necessary *******************/

		distribMtx.unlock(); /********************  UNLOCKED MUTEX ************/

	}

	return;
}


/*******************************
********************************
********************************
*****  COMPLETED TO HERE  ******
********************************
********************************
********************************/


/***************************************************************************************/
/***************************************************************************************/
	/********************** load Cache Msgs  ***************************************/
/***************************************************************************************/
/***************************************************************************************/


void	p3GroupDistrib::loadGroup(RsDistribGrp *newGrp)
{
	/* load groupInfo */

	/* look for duplicate */


	/* check signature */

	/* add in */

	/* */
}


void	p3GroupDistrib::loadMsg(RsDistribMsg *msg, std::string src, bool local)
{
	/****************** check the msg ******************/
	/* Do the most likely checks to fail first....
	 *
	 * timestamp (too old)
	 * group (non existant)
	 * msg (already have it)
	 *
	 * -> then do the expensive Hash / signature checks.
	 */

	distribMtx.lock();   /********************    LOCKED MUTEX ************/

	/* check timestamp */
	time_t now = time(NULL);
	time_t min = now - mStorePeriod;
	time_t minPub = now - mStorePeriod / 2.0;
	time_t max = now + GROUP_MAX_FWD_OFFSET;

	distribMtx.unlock(); /********************  UNLOCKED MUTEX ************/

	if ((msg->timestamp < min) || (msg->timestamp > max))
	{
		/* if outside range -> remove */
		delete msg;
		return;
	}


	distribMtx.lock();   /********************    LOCKED MUTEX ************/

	/* find group */
	std::map<std::string, GroupInfo>::iterator git;
	if (mGroups.end() == (git = mGroups.find(msg->grpId)))
	{
		/* if not there -> remove */
		distribMtx.unlock(); /********************  UNLOCKED MUTEX ************/

		delete msg;
		return;
	}

	/* check for duplicate message */
	std::map<std::string, RsDistribMsg *>::iterator mit;
	if ((git->second).msgs.end() != (git->second).msgs.find(msg->msgId))
	{
		distribMtx.unlock(); /********************  UNLOCKED MUTEX ************/

		/* if already there -> remove */
		delete msg;
		return;
	}

	distribMtx.unlock(); /********************  UNLOCKED MUTEX ************/

	/* save and reset hash/signatures */
	std::string hash = msg->msgId;
	std::string grpSign = msg->grpSignature;
	std::string srcSign = msg->sourceSignature;

	msg->msgId = "";
	msg->grpSignature = "";
	msg->sourceSignature = "";

	std::string computedHash = HashRsItem(msg);

	/* restore data */
	msg->msgId = hash;
	msg->grpSignature = grpSign;
	msg->sourceSignature = srcSign;

	if (computedHash != hash)
	{
		/* hash is wrong */
		delete msg;
		return;
	}

	/* calc group signature */
		/* if fails -> remove */

	/* check peer signature */
		/* if !allowedAnon & anon -> remove */
		/* if !allowedUnknown & unknown -> remove */

	/****************** check the msg ******************/

	/* accept message */
	(git->second).msgs[msg->msgId] = msg;

	if (local)
	{
		/* if from local -> already published */
		/* All local loads - will also come in as Remote loads.
		 * but should be discarded because of duplicates 
		 * (Local load must happen first!)
		 */
		//delete msg;
		return;
	}

	if (msg->timestamp < minPub)
	{
		/* outside publishing range */
		//delete msg;
		return;
	}
	
	/* else if group = subscribed | listener -> publish */
	toPublishMsg(msg);
}


/***************************************************************************************/
/***************************************************************************************/
	/****************** create/mod Cache Content  **********************************/
/***************************************************************************************/
/***************************************************************************************/

void	p3GroupDistrib::toPublishMsg(RsDistribMsg *msg)
{
	RsStackMutex stack(distribMtx); /****** STACK MUTEX LOCKED *******/

	mPendingPublish.push_back(msg);
}

void 	p3GroupDistrib::locked_publishPendingMsgs()
{
	/* get the next message id */
	CacheData newCache;

	newCache.pid = mOwnId;
	newCache.cid.type = CacheSource::getCacheType();
	newCache.cid.subid = determineCacheSubId(); // NOT fixed - should rotate.

	/* create filename */
	std::string path = CacheSource::getCacheDir();
	std::ostringstream out;
	out << "grpdist-t" << CacheSource::getCacheType() << "-msgs-" << time(NULL) << ".dist"; 

	std::string tmpname = out.str();
	std::string filename = path + "/" + tmpname;

	BinInterface *bio = new BinFileInterface(filename.c_str(), 
				BIN_FLAGS_WRITEABLE | BIN_FLAGS_HASH_DATA);
	pqistreamer *streamer = createStreamer(bio, mOwnId, 
						BIN_FLAGS_NO_DELETE);

	RsStackMutex stack(distribMtx); /****** STACK MUTEX LOCKED *******/


	std::list<RsDistribMsg *>::iterator it;
	for(it = mPendingPublish.begin(); it != mPendingPublish.end(); it++)
	{
		streamer->SendItem(*it); /* doesnt delete it */
	}

	/* cleanup */
	mPendingPublish.clear();
	delete streamer;

	/* Extract File Information from pqistreamer */
	newCache.path = path;
	newCache.name = tmpname;

	newCache.hash = bio->gethash();
	newCache.size = bio->bytecount();
	newCache.recvd = time(NULL);

	/* push file to CacheSource */
	refreshCache(newCache);

	/* flag to store config (saying we've published messages) */
	IndicateConfigChanged(); /**** INDICATE CONFIG CHANGED! *****/
}


void 	p3GroupDistrib::publishDistribGroups()
{
	/* set subid = 0 */	
	CacheData newCache;

	newCache.pid = mOwnId;
	newCache.cid.type = CacheSource::getCacheType();
	newCache.cid.subid = 0;

	/* create filename */
	std::string path = CacheSource::getCacheDir();
	std::ostringstream out;
	out << "grpdist-t" << CacheSource::getCacheType() << "-grps-" << time(NULL) << ".dist"; 

	std::string tmpname = out.str();
	std::string filename = path + "/" + tmpname;

	BinInterface *bio = new BinFileInterface(filename.c_str(), 
				BIN_FLAGS_WRITEABLE | BIN_FLAGS_HASH_DATA);
	pqistreamer *streamer = createStreamer(bio, mOwnId, 0);

	RsStackMutex stack(distribMtx); /****** STACK MUTEX LOCKED *******/


	/* Iterate through all the Groups */
	std::map<std::string, GroupInfo>::iterator it;
	for(it = mGroups.begin(); it != mGroups.end(); it++)
	{
		/* if subscribed or listener -> do stuff */

		/* extract public info to RsDistribGrp */
		RsDistribGrp *grp = new RsDistribGrp();


		/* store in Cache File */
		streamer->SendItem(grp); /* deletes it */
	}

	/* Extract File Information from pqistreamer */
	newCache.path = path;
	newCache.name = tmpname;

	newCache.hash = bio->gethash();
	newCache.size = bio->bytecount();
	newCache.recvd = time(NULL);

	/* cleanup */
	delete streamer;

	/* push file to CacheSource */
	refreshCache(newCache);
}


	/* clearing old data */
void	p3GroupDistrib::clear_local_caches(time_t now)
{
	RsStackMutex stack(distribMtx); /****** STACK MUTEX LOCKED *******/

	time_t cutoff = now - mStorePeriod;
	std::list<GroupCache>::iterator it;
	for(it = mLocalCaches.begin(); it != mLocalCaches.end();)
	{
		if (it->end < cutoff)
		{
			/* Call to CacheSource Function */
			CacheId cid(CacheSource::getCacheType(), it->cacheSubId);
			clearCache(cid);
			it = mLocalCaches.erase(it);
		}
		else
		{
			it++;
		}
	}
}



/***************************************************************************************/
/***************************************************************************************/
	/********************** Access Content   ***************************************/
/***************************************************************************************/
/***************************************************************************************/


/* get Group Lists */
bool p3GroupDistrib::getAllGroupList(std::list<std::string> &grpids)
{
	RsStackMutex stack(distribMtx); /*************  STACK MUTEX ************/
	std::map<std::string, GroupInfo>::iterator git;
	for(git = mGroups.begin(); git != mGroups.end(); git++)
	{
		grpids.push_back(git->first);
	}
	return true;
}

bool p3GroupDistrib::getSubscribedGroupList(std::list<std::string> &grpids)
{
	RsStackMutex stack(distribMtx); /*************  STACK MUTEX ************/
	std::map<std::string, GroupInfo>::iterator git;
	for(git = mGroups.begin(); git != mGroups.end(); git++)
	{
		if (git->second.flags & RS_GRPDISTRIB_SUBSCRIBED)
		{
			grpids.push_back(git->first);
		}
	}
	return true;
}

bool p3GroupDistrib::getPublishGroupList(std::list<std::string> &grpids)
{
	RsStackMutex stack(distribMtx); /*************  STACK MUTEX ************/
	std::map<std::string, GroupInfo>::iterator git;
	for(git = mGroups.begin(); git != mGroups.end(); git++)
	{
		if (git->second.flags & RS_GRPDISTRIB_PUBLISH)
		{
			grpids.push_back(git->first);
		}
	}
	return true;
}

bool p3GroupDistrib::getPopularGroupList(uint32_t popMin, uint32_t popMax, std::list<std::string> &grpids)
{
	RsStackMutex stack(distribMtx); /*************  STACK MUTEX ************/
	std::map<std::string, GroupInfo>::iterator git;
	for(git = mGroups.begin(); git != mGroups.end(); git++)
	{
		if ((git->second.pop >= popMin) &&
			(git->second.pop <= popMax))
		{
			grpids.push_back(git->first);
		}
	}
	return true;
}


/* get Msg Lists */
bool p3GroupDistrib::getAllMsgList(std::string grpId, std::list<std::string> &msgIds)
{
	RsStackMutex stack(distribMtx); /*************  STACK MUTEX ************/
	std::map<std::string, GroupInfo>::iterator git;
	if (mGroups.end() == (git = mGroups.find(grpId)))
	{
		return false;
	}

	std::map<std::string, RsDistribMsg *>::iterator mit;
	std::map<std::string, RsDistribMsg *> msgs;

	for(mit = git->second.msgs.begin(); mit != git->second.msgs.end(); mit++)
	{
		msgIds.push_back(mit->first);
	}
	return true;
}

bool p3GroupDistrib::getTimePeriodMsgList(std::string grpId, uint32_t timeMin, 
						uint32_t timeMax, std::list<std::string> &msgIds)
{
	RsStackMutex stack(distribMtx); /*************  STACK MUTEX ************/
	std::map<std::string, GroupInfo>::iterator git;
	if (mGroups.end() == (git = mGroups.find(grpId)))
	{
		return false;
	}

	std::map<std::string, RsDistribMsg *>::iterator mit;

	for(mit = git->second.msgs.begin(); mit != git->second.msgs.end(); mit++)
	{
		if ((mit->second->timestamp >= timeMin) &&
			(mit->second->timestamp <= timeMax))
		{
			msgIds.push_back(mit->first);
		}
	}
	return true;
}


GroupInfo *p3GroupDistrib::locked_getGroupInfo(std::string grpId)
{
	/************* ALREADY LOCKED ************/
	std::map<std::string, GroupInfo>::iterator git;
	if (mGroups.end() == (git = mGroups.find(grpId)))
	{
		return NULL;
	}
	return &(git->second);
}


RsDistribMsg *p3GroupDistrib::locked_getGroupMsg(std::string grpId, std::string msgId)
{
	/************* ALREADY LOCKED ************/
	std::map<std::string, GroupInfo>::iterator git;
	if (mGroups.end() == (git = mGroups.find(grpId)))
	{
		return NULL;
	}

	std::map<std::string, RsDistribMsg *>::iterator mit;
	if (git->second.msgs.end() == (mit = git->second.msgs.find(msgId)))
	{
		return NULL;
	}

	return mit->second;
}


/**** These must be created in derived classes ****/

#if 0
	
/* get Group Details */
bool p3GroupDistrib::getGroupDetails(std::string grpId, RsExternalDistribGroup &grp)
{
	RsStackMutex stack(distribMtx); /*************  STACK MUTEX ************/
	std::map<std::string, GroupInfo>::iterator git;
	if (mGroups.end() == (git = mGroups.find(grpId)))
	{
		return false;
	}

	/* Fill in details */

	return true;
}

/* get Msg */
bool p3GroupDistrib::getGroupMsgDetails(std::string grpId, std::string msgId, RsExternalDistribMsg &msg)
{
	RsStackMutex stack(distribMtx); /*************  STACK MUTEX ************/
	std::map<std::string, GroupInfo>::iterator git;
	if (mGroups.end() == (git = mGroups.find(grpId)))
	{
		return false;
	}

	std::map<std::string, RsDistribMsg *>::iterator mit;
	if (git->second.msgs.end() == (mit = git->second.msgs.find(msgId)))
	{
		return false;
	}

	/* Fill in the message details */


	return true;
}

#endif


/************************************* p3Config *************************************/

RsSerialiser *p3GroupDistrib::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser();

	rss->addSerialType(new RsSerialDistrib());
	rss->addSerialType(new RsConfigDistrib());

	return rss;
}

std::list<RsItem *> p3GroupDistrib::saveList(bool &cleanup)
{
	std::list<RsItem *> saveData;

	/* store private information for OUR lists */
	/* store information on subscribed lists */
	/* store messages for pending Publication */

	return saveData;
}

bool    p3GroupDistrib::loadList(std::list<RsItem *> load)
{
	std::list<RsItem *>::iterator lit;
	for(lit = load.begin(); lit != load.end(); lit++)
	{
		/* decide what type it is */
	}
	return true;
}

/************************************* p3Config *************************************/

pqistreamer *p3GroupDistrib::createStreamer(BinInterface *bio, std::string src, uint32_t bioflags)
{
	RsSerialiser *rsSerialiser = new RsSerialiser();
	RsSerialType *serialType = new RsSerialDistrib(); /* TODO */

	rsSerialiser->addSerialType(serialType);

	pqistreamer *streamer = new pqistreamer(rsSerialiser, src, bio, bioflags);

	return streamer;
}


std::string HashRsItem(RsItem *item)
{
	/* calc/check hash (of serialised data) */
	RsSerialType *serial = new RsSerialDistrib();
	
	uint32_t size = serial->size(item);
	RsRawItem *ri = new RsRawItem(0, size);
	serial->serialise(item, ri->getRawData(), &size);

	pqihash hash;
	std::string computedHash;

	hash.addData(ri->getRawData(), size);
	hash.Complete(computedHash);

	delete ri;
	delete serial;
	
	return computedHash;
}



/***************************************************************************************/
/***************************************************************************************/
	/********************** Create Content   ***************************************/
/***************************************************************************************/
/***************************************************************************************/

std::string p3GroupDistrib::createGroup(std::string name, uint32_t flags)
{
	/* Create a Group */
	GroupInfo grpInfo;
	std::string grpId;

#ifdef  GROUP_SIGNATURES

	/* Create Key Set (Admin) */
	EVP_PKEY *key_admin = EVP_PKEY_new();
	mAuthMgr->generateKeyPair(key_admin, 0);

	/* extract AdminKey Id -> groupId */
	grpId = mAuthMgr->getKeyId(key_admin);

	/* setup GroupInfo */
	grpInfo.adminKey = key_admin;

#else 
	grpInfo.id = generateRandomId();
#endif

	grpInfo.id = grpId;
	grpInfo.flags = flags;
	grpInfo.name  = name;

	/* generate a set of keys */

	/* generate RsDistribGrp */

	/* sign Grp (with date) */

	/* store new GroupInfo */

	return grpId;
}

std::string p3GroupDistrib::addPublishKey(std::string grpId, uint32_t keyflags, time_t startDate, time_t endDate)
{
	/* Find the Group */
	GroupInfo *grpInfo = locked_getGroupInfo(grpId);
	std::string keyId;

	if (!grpInfo)
	{
		return keyId;
	}

	/* if we don't have the admin key -> then we cannot add a key */
	if (!(grpInfo->grpFlags & RS_GRPDISTRIB_ADMIN_KEY))
	{
		return keyId;
	}

	/* Create Key Set (Publish) */
	EVP_PKEY *key_publish = EVP_PKEY_new();
	mAuthMgr->generateKeyPair(key_publish, 0);

	/* extract Key Id -> keyId */
	keyId = mAuthMgr->getKeyId(key_publish);

	/* setup RsKey */
	RsKey *publishKey = new RsKey();
	publishKey -> key = key_publish;
	publishKey -> keyId = keyId;
	publishKey -> startDate = startDate;
	publishKey -> endDate = endDate;

	/* setup data packet for signing */


	/* sign key with Admin Key */
	publishKey -> adminSignature = ...;


	grpInfo.publishKeys.push_back(publishKey);

	return keyId;
}





int p3channel::signRsKey(RsKey *pubkey, EVP_PKEY *signKey, std::string &signature)
{
	/* 





        // sslroot will generate the pair...
	// we need to split it into an pub/private.
	
	EVP_PKEY *keypair = EVP_PKEY_new();
	EVP_PKEY *pubkey = EVP_PKEY_new();

	mAuthMgr->generateKeyPair(keypair, 0);
	
        RSA *rsa1 = EVP_PKEY_get1_RSA(keypair);
        RSA *rsa2 = RSAPublicKey_dup(rsa1);
	
	{
	std::ostringstream out;
	out << "p3channel::generateRandomKeys()" << std::endl;
	out << "Rsa1: " << (void *) rsa1 << " & Rsa2: ";
	out << (void *)  rsa2 << std::endl;
	
	pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	}
	
	EVP_PKEY_assign_RSA(pubkey, rsa1);
	RSA_free(rsa1); // decrement ref count!
	
	priv -> setKey(keypair);
	pub -> setKey(pubkey);
	
	{
	std::ostringstream out;
	out << "p3channel::generateRandomKey(): ";
	priv -> print(out);
	pqioutput(PQL_DEBUG_BASIC, pqichannelzone, out.str());
	}
	return 1;
	}
	
	
void	p3GroupDistrib::publishMsg(RsDistribMsg *msg, bool personalSign)
{
	/* extract grpId */
	std::string grpId = msg->grpId;

	/* ensure Group exists */

	/* hash message */

	/* sign message */

	/* personal signature? */	

	/* Message now Complete */

	/* Insert in Group */

	/* add to PublishPending */

	/* done */
	return;
}



#if 0

/* Modify Group */
std::string p3GroupDistrib::modGroupDescription(std::string grpId, std::string description)
{
	return;
}

std::string p3GroupDistrib::modGroupIcon(std::string grpId, PIXMAP icon)
{
	return;
}

#endif

