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

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>

#include "rsiface/rsdistrib.h"
#include "services/p3distrib.h"
#include "serialiser/rsdistribitems.h"

#include "pqi/pqibin.h"

#define DISTRIB_DEBUG 1

RSA *extractPublicKey(RsTlvSecurityKey &key);
RSA *extractPrivateKey(RsTlvSecurityKey &key);
void 	setRSAPublicKey(RsTlvSecurityKey &key, RSA *rsa_pub);
void 	setRSAPrivateKey(RsTlvSecurityKey &key, RSA *rsa_priv);


p3GroupDistrib::p3GroupDistrib(uint16_t subtype, 
		CacheStrapper *cs, CacheTransfer *cft,
		std::string sourcedir, std::string storedir, 
		uint32_t configId, 
		uint32_t storePeriod, uint32_t pubPeriod)

	:CacheSource(subtype, true, cs, sourcedir), 
	CacheStore(subtype, true, cs, cft, storedir), 
	p3Config(configId), nullService(subtype),
	mStorePeriod(storePeriod), 
	mPubPeriod(pubPeriod), 
	mLastPublishTime(0),
	mMaxCacheSubId(1)
{
	/* not much yet */
	time_t now = time(NULL);

	/* force publication of groups (cleared if local cache file found) */
	mGroupsRepublish = true;

	return;
}


int	p3GroupDistrib::tick()
{

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::tick()";
	std::cerr << std::endl;
#endif

	time_t now = time(NULL);
	bool toPublish;

	{
		RsStackMutex stack(distribMtx);  /**** STACK LOCKED MUTEX ****/
		toPublish = (mPendingPublish.size() > 0) && (now > mPubPeriod + mLastPublishTime);
	}

	if (toPublish)
	{
		RsStackMutex stack(distribMtx);  /**** STACK LOCKED MUTEX ****/

		locked_publishPendingMsgs(); /* flags taken care of in here */
	}

	bool toPublishGroups;
	{
		RsStackMutex stack(distribMtx);  /**** STACK LOCKED MUTEX ****/
		toPublishGroups = mGroupsRepublish;
	}

	if (toPublishGroups)
	{
		publishDistribGroups();

		IndicateConfigChanged(); /**** INDICATE CONFIG CHANGED! *****/

		RsStackMutex stack(distribMtx);  /**** STACK LOCKED MUTEX ****/
		mGroupsRepublish = false;
	}

	return 0;
}


/***************************************************************************************/
/***************************************************************************************/
	/********************** overloaded functions from Cache Store ******************/
/***************************************************************************************/
/***************************************************************************************/

int     p3GroupDistrib::loadAnyCache(const CacheData &data, bool local)
{
	/* if subtype = 1 -> FileGroup, else -> FileMsgs */

	std::string file = data.path;
	file += "/";
	file += data.name;

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::loadAnyCache() file: " << file << std::endl;
	std::cerr << "PeerId: " << data.pid << std::endl;
	std::cerr << "Cid: " << data.cid.type << ":" << data.cid.subid << std::endl;
#endif

	if (data.cid.subid == 1)
	{
		loadFileGroups(file, data.pid, local);
	}
	else
	{
		loadFileMsgs(file, data.cid.subid, data.pid, data.recvd, local);
	}
	return true;
}

int    p3GroupDistrib::loadCache(const CacheData &data)
{
#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::loadCache()";
	std::cerr << std::endl;
#endif

	loadAnyCache(data, false);

	if (data.size > 0)
	{
        	CacheStore::lockData();   /*****   LOCK ****/
        	locked_storeCacheEntry(data);
        	CacheStore::unlockData(); /***** UNLOCK ****/
	}

	return 1;
}

bool 	p3GroupDistrib::loadLocalCache(const CacheData &data)
{
#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::loadLocalCache()";
	std::cerr << std::endl;
#endif

	loadAnyCache(data, true);

	if (data.size > 0)
	{
		refreshCache(data);
	}

	return true;
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
#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::loadFileGroups()";
	std::cerr << std::endl;
#endif

	/* create the serialiser to load info */
	BinInterface *bio = new BinFileInterface(filename.c_str(), BIN_FLAGS_READABLE);
	pqistreamer *streamer = createStreamer(bio, src, 0);

	RsItem *item;
	RsDistribGrp *newGrp;
	RsDistribGrpKey *newKey;

	streamer->tick();
	while(NULL != (item = streamer->GetItem()))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadFileGroups() Got Item:";
		std::cerr << std::endl;
		item->print(std::cerr, 10);
		std::cerr << std::endl;
#endif

		newKey = dynamic_cast<RsDistribGrpKey *>(item);
		if ((newGrp = dynamic_cast<RsDistribGrp *>(item)))
		{
			loadGroup(newGrp);
		}
		else if ((newKey = dynamic_cast<RsDistribGrpKey *>(item)))
		{
			loadGroupKey(newKey);
		}
		else
		{
#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::loadFileGroups() Unexpected Item - deleting";
			std::cerr << std::endl;
#endif
			delete item;
		}
		streamer->tick();
	}

	delete streamer;



	/* clear publication of groups if local cache file found */
	RsStackMutex stack(distribMtx); /******* STACK LOCKED MUTEX ***********/
	if (local)
	{
		mGroupsRepublish = false;
	}

	return;
}


void	p3GroupDistrib::loadFileMsgs(std::string filename, uint16_t cacheSubId, std::string src, uint32_t ts, bool local)
{

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::loadFileMsgs()";
	std::cerr << std::endl;
#endif

	time_t now = time(NULL);
	//time_t start = now;
	//time_t end   = 0;

	/* create the serialiser to load msgs */
	BinInterface *bio = new BinFileInterface(filename.c_str(), BIN_FLAGS_READABLE);
	pqistreamer *streamer = createStreamer(bio, src, 0);

	RsItem *item;
	RsDistribSignedMsg *newMsg;

	streamer->tick();
	while(NULL != (item = streamer->GetItem()))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadFileMsgs() Got Item:";
		std::cerr << std::endl;
		item->print(std::cerr, 10);
		std::cerr << std::endl;
#endif

		if ((newMsg = dynamic_cast<RsDistribSignedMsg *>(item)))
		{
			loadMsg(newMsg, src, local);
		}
		else
		{
#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::loadFileMsgs() Unexpected Item - deleting";
			std::cerr << std::endl;
#endif
			/* wrong message type */
			delete item;
		}
		streamer->tick();
	}


	if (local)
	{
		/* now we create a map of time -> subid 
	 	 * This is used to determine the newest and the oldest items 
	 	 */
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadFileMsgs() Updating Local TimeStamps";
		std::cerr << std::endl;
		std::cerr << "p3GroupDistrib::loadFileMsgs() CacheSubId: " << cacheSubId << " recvd: " << ts;
		std::cerr << std::endl;
#endif

		mLocalCacheTs[ts] = cacheSubId;
		if (cacheSubId > mMaxCacheSubId)
		{
#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::loadFileMsgs() New Max CacheSubId";
			std::cerr << std::endl;
#endif
			mMaxCacheSubId = cacheSubId;
		}

		if ((ts < now) && (ts > mLastPublishTime))
		{
#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::loadFileMsgs() New LastPublishTime";
			std::cerr << std::endl;
#endif
			mLastPublishTime = ts;
		}
	}

	delete streamer;
	return;
}

/***************************************************************************************/
/***************************************************************************************/
	/********************** load Cache Msgs  ***************************************/
/***************************************************************************************/
/***************************************************************************************/


void	p3GroupDistrib::loadGroup(RsDistribGrp *newGrp)
{
	/* load groupInfo */
	std::string gid = newGrp -> grpId;
	std::string pid = newGrp -> PeerId();

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::loadGroup()" << std::endl;
	std::cerr << "groupId: " << gid << std::endl;
	std::cerr << "PeerId: " << gid << std::endl;
	std::cerr << "Group:" << std::endl;
	newGrp -> print(std::cerr, 10);
	std::cerr << "----------------------" << std::endl;
#endif

	RsStackMutex stack(distribMtx); /******* STACK LOCKED MUTEX ***********/

	/* look for duplicate */
	bool checked = false;
	bool isNew = false;
	std::map<std::string, GroupInfo>::iterator it;
	it = mGroups.find(gid);

	if (it == mGroups.end())
	{

#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadGroup() Group Not Found";
		std::cerr << std::endl;
#endif

		if (!validateDistribGrp(newGrp))
		{
#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::loadGroup() Invalid Group ";
			std::cerr << std::endl;
#endif
			/* fails test */
			delete newGrp;
			return;
		}

		checked = true;

		GroupInfo gi;
		gi.grpId = gid;
		mGroups[gid] = gi;

		it = mGroups.find(gid);

		isNew = true;
	}

	/* at this point - always in the map */

	/* add as source ... don't need to validate for this! */
	std::list<std::string>::iterator pit;
	pit = std::find(it->second.sources.begin(), it->second.sources.end(), pid);
	if (pit == it->second.sources.end())
	{
		it->second.sources.push_back(pid);
		it->second.pop = it->second.sources.size();

#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadGroup() New Source, pop = ";
		std::cerr << it->second.pop;
		std::cerr << std::endl;
#endif
	}

	if (!checked)
	{
		if (!locked_checkGroupInfo(it->second, newGrp))
		{
#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::loadGroup() Fails Check";
			std::cerr << std::endl;
#endif
			/* either fails check or old/same data */
			delete newGrp;
			return;
		}
	}

	/* useful info/update */
	if(!locked_updateGroupInfo(it->second, newGrp))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadGroup() Fails Update";
		std::cerr << std::endl;
#endif
		/* cleanup on false */
		delete newGrp;
	}
	else
	{
		/* Callback for any derived classes */
		locked_eventUpdateGroup(&(it->second), isNew);

		locked_notifyGroupChanged(it->second);
	}

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::loadGroup() Done";
	std::cerr << std::endl;
#endif
}


void	p3GroupDistrib::loadGroupKey(RsDistribGrpKey *newKey)
{
	/* load Key */
	std::string pid = newKey -> PeerId();
	std::string gid = newKey -> grpId;

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::loadGroupKey()" << std::endl;
	std::cerr << "PeerId: " << pid << std::endl;
	std::cerr << "groupId: " << gid << std::endl;
	std::cerr << "Key:" << std::endl;
	newKey -> print(std::cerr, 10);
	std::cerr << "----------------------" << std::endl;
#endif

	RsStackMutex stack(distribMtx); /******* STACK LOCKED MUTEX ***********/

	/* Find the Group */
	std::map<std::string, GroupInfo>::iterator it;
	it = mGroups.find(gid);

	if (it == mGroups.end())
	{

#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadGroupKey() Group Not Found - discarding Key";
		std::cerr << std::endl;
#endif
		delete newKey;
		return;
	}

	/* have the group -> add in the key */
	bool updateOk = false;
	if (newKey->key.keyFlags & RSTLV_KEY_DISTRIB_ADMIN)
	{
		if(!locked_updateGroupAdminKey(it->second, newKey))
		{
#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::loadGroupKey() Failed Admin Key Update";
			std::cerr << std::endl;
#endif
		}
		else
		{
			updateOk = true;
		}
	}
	else
	{
		if(!locked_updateGroupPublishKey(it->second, newKey))
		{
#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::loadGroupKey() Failed Publish Key Update";
			std::cerr << std::endl;
#endif
		}
		else
		{
			updateOk = true;
		}
	}

	if (updateOk)
	{
		locked_notifyGroupChanged(it->second);
	}

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::loadGroupKey() Done - Cleaning up.";
	std::cerr << std::endl;
#endif
	delete newKey;

	return;
}


void	p3GroupDistrib::loadMsg(RsDistribSignedMsg *newMsg, std::string src, bool local)
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

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::loadMsg()" << std::endl;
	std::cerr << "Source:" << src << std::endl;
	std::cerr << "Local:" << local << std::endl;
	newMsg -> print(std::cerr, 10);
	std::cerr << "----------------------" << std::endl;
#endif

	RsStackMutex stack(distribMtx); /******* STACK LOCKED MUTEX ***********/

	/* Check if it exists already */

	/* find group */
	std::map<std::string, GroupInfo>::iterator git;
	if (mGroups.end() == (git = mGroups.find(newMsg->grpId)))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadMsg() Group Dont Exist" << std::endl;
		std::cerr << std::endl;
#endif
		/* if not there -> remove */
		delete newMsg;
		return;
	}

	/* check for duplicate message */
	std::map<std::string, RsDistribMsg *>::iterator mit;
	if ((git->second).msgs.end() != (git->second).msgs.find(newMsg->msgId))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadMsg() Msg already exists" << std::endl;
		std::cerr << std::endl;
#endif
		/* if already there -> remove */
		delete newMsg;
		return;
	}

	/****************** check the msg ******************/
	if (!locked_validateDistribSignedMsg(git->second, newMsg))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadMsg() validate failed" << std::endl;
		std::cerr << std::endl;
#endif
		delete newMsg;
		return;
	}

	/* convert Msg */
	RsDistribMsg *msg = unpackDistribSignedMsg(newMsg);
	if (!msg)
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadMsg() unpack failed" << std::endl;
		std::cerr << std::endl;
#endif
		delete newMsg;
		return;
	}

	if (!locked_checkDistribMsg(git->second, msg))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadMsg() check failed" << std::endl;
		std::cerr << std::endl;
#endif
		delete newMsg;
		delete msg;
		return;
	}

	/* accept message */
	(git->second).msgs[msg->msgId] = msg;

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::loadMsg() Msg Loaded Successfully" << std::endl;
	std::cerr << std::endl;
#endif

	/* Callback for any derived classes to play with */
	locked_eventNewMsg(msg);

	/* else if group = subscribed | listener -> publish */
	/* if it has come from us... then it has been published already */
	if ((!local) && (git->second.flags & (RS_DISTRIB_SUBSCRIBED)))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadMsg() To be Published!";
		std::cerr << std::endl;
#endif
		locked_toPublishMsg(newMsg);
	}
	else
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadMsg() Deleted Original Msg (No Publish)";
		std::cerr << std::endl;
#endif
		delete newMsg;
	}

	locked_notifyGroupChanged(git->second);
}


/***************************************************************************************/
/***************************************************************************************/
	/****************** create/mod Cache Content  **********************************/
/***************************************************************************************/
/***************************************************************************************/

void	p3GroupDistrib::locked_toPublishMsg(RsDistribSignedMsg *msg)
{
#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_toPublishMsg() Adding to PendingPublish List";
	std::cerr << std::endl;
#endif
	mPendingPublish.push_back(msg);
	if (msg->PeerId() == mOwnId)
	{

#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_toPublishMsg() Local -> ConfigSave Requested";
		std::cerr << std::endl;
#endif
		/* we need to trigger Configuration save */
		IndicateConfigChanged(); /**** INDICATE CONFIG CHANGED! *****/
	}
}


uint16_t p3GroupDistrib::locked_determineCacheSubId()
{
	/* if oldest cache is previous to StorePeriod - use that */
	time_t now = time(NULL);
	uint16_t id = 1;

	uint32_t oldest = now;
	if (mLocalCacheTs.size() > 0)
	{
		oldest = mLocalCacheTs.begin()->first;
	}

	if (oldest < now - mStorePeriod)
	{
		/* clear it out, return id */
		id = mLocalCacheTs.begin()->second;
		mLocalCacheTs.erase(mLocalCacheTs.begin());

#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_determineCacheSubId() Replacing Old CacheId: " << id;
		std::cerr << std::endl;
#endif
		return id;
	}

	mMaxCacheSubId++;

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_determineCacheSubId() Returning new Id: " << mMaxCacheSubId;
	std::cerr << std::endl;
#endif
	/* else return maximum */
	return mMaxCacheSubId;
}


void 	p3GroupDistrib::locked_publishPendingMsgs()
{
#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_publishPendingMsgs()";
	std::cerr << std::endl;
#endif
	/* get the next message id */
	CacheData newCache;
	time_t now = time(NULL);

	newCache.pid = mOwnId;
	newCache.cid.type = CacheSource::getCacheType();
	newCache.cid.subid = locked_determineCacheSubId(); 

	/* create filename */
	std::string path = CacheSource::getCacheDir();
	std::ostringstream out;
	out << "grpdist-t" << CacheSource::getCacheType() << "-msgs-" << time(NULL) << ".dist"; 

	std::string tmpname = out.str();
	std::string filename = path + "/" + tmpname;

	BinInterface *bio = new BinFileInterface(filename.c_str(), 
				BIN_FLAGS_WRITEABLE | BIN_FLAGS_HASH_DATA);
	pqistreamer *streamer = createStreamer(bio, mOwnId, 0); /* messages are deleted! */ 

	bool resave = false;
	std::list<RsDistribSignedMsg *>::iterator it;
	for(it = mPendingPublish.begin(); it != mPendingPublish.end(); it++)
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_publishPendingMsgs() Publishing:";
		std::cerr << std::endl;
		(*it)->print(std::cerr, 10);
		std::cerr << std::endl;
#endif
		if ((*it)->PeerId() == mOwnId)
		{

#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::locked_publishPendingMsgs() Own Publish";
			std::cerr << std::endl;
#endif
			resave = true;
		}

		streamer->SendItem(*it); /* deletes it */
		streamer->tick();

	}

	streamer->tick(); /* once more for good luck! */

	/* Extract File Information from pqistreamer */
	newCache.path = path;
	newCache.name = tmpname;

	newCache.hash = bio->gethash();
	newCache.size = bio->bytecount();
	newCache.recvd = now;

	/* cleanup */
	mPendingPublish.clear();
	delete streamer;

	/* indicate not to save for a while */
	mLastPublishTime = now;

	/* push file to CacheSource */
	refreshCache(newCache);

	if (resave)
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_publishPendingMsgs() Indicate Save Data Changed";
		std::cerr << std::endl;
#endif
		/* flag to store config (saying we've published messages) */
		IndicateConfigChanged(); /**** INDICATE CONFIG CHANGED! *****/
	}
}


void 	p3GroupDistrib::publishDistribGroups()
{
#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::publishDistribGroups()";
	std::cerr << std::endl;
#endif

	/* set subid = 1 */	
	CacheData newCache;

	newCache.pid = mOwnId;
	newCache.cid.type = CacheSource::getCacheType();
	newCache.cid.subid = 1;

	/* create filename */
	std::string path = CacheSource::getCacheDir();
	std::ostringstream out;
	out << "grpdist-t" << CacheSource::getCacheType() << "-grps-" << time(NULL) << ".dist"; 

	std::string tmpname = out.str();
	std::string filename = path + "/" + tmpname;

	BinInterface *bio = new BinFileInterface(filename.c_str(), 
				BIN_FLAGS_WRITEABLE | BIN_FLAGS_HASH_DATA);
	pqistreamer *streamer = createStreamer(bio, mOwnId, BIN_FLAGS_NO_DELETE);

	RsStackMutex stack(distribMtx); /****** STACK MUTEX LOCKED *******/


	/* Iterate through all the Groups */
	std::map<std::string, GroupInfo>::iterator it;
	for(it = mGroups.begin(); it != mGroups.end(); it++)
	{
		/* if subscribed or listener -> do stuff */
		if (it->second.flags & (RS_DISTRIB_SUBSCRIBED))
		{
#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::publishDistribGroups() Saving Group: " << it->first;
			std::cerr << std::endl;
#endif

			/* extract public info to RsDistribGrp */
			RsDistribGrp *grp = it->second.distribGroup;

			if (grp)
			{
				/* store in Cache File */
				streamer->SendItem(grp); /* no delete */
				streamer->tick();
			}

			/* if they have public keys, publish these too */
			std::map<std::string, GroupKey>::iterator kit;
			for(kit = it->second.publishKeys.begin(); 
						kit != it->second.publishKeys.end(); kit++)
			{
				if ((kit->second.type & RSTLV_KEY_DISTRIB_PUBLIC) &&
					(kit->second.type & RSTLV_KEY_TYPE_FULL))
				{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::publishDistribGroups() Saving Key: " << kit->first;
		std::cerr << std::endl;
#endif
					/* create Key for sharing */

					RsDistribGrpKey *pubKey = new RsDistribGrpKey();
					pubKey->grpId = it->first;

					RSA *rsa_priv = EVP_PKEY_get1_RSA(kit->second.key);
					setRSAPrivateKey(pubKey->key, rsa_priv);
					RSA_free(rsa_priv);

					pubKey->key.keyFlags = RSTLV_KEY_TYPE_FULL;
					pubKey->key.keyFlags |= RSTLV_KEY_DISTRIB_PUBLIC;
					pubKey->key.startTS = kit->second.startTS;
					pubKey->key.endTS   = kit->second.endTS;

					streamer->SendItem(pubKey);
					streamer->tick();
					delete pubKey;
				}
				else
				{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::publishDistribGroups() Ignoring Key: " << kit->first;
		std::cerr << std::endl;
		std::cerr << "p3GroupDistrib::publishDistribGroups() Key Type: " << kit->second.type;
		std::cerr << std::endl;
#endif
				}

			}
		}
		else
		{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::publishDistribGroups() Ignoring Group: " << it->first;
		std::cerr << std::endl;
#endif
		}

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
		if (git->second.flags & RS_DISTRIB_SUBSCRIBED)
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
		if (git->second.flags & (RS_DISTRIB_ADMIN | RS_DISTRIB_PUBLISH))
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

bool p3GroupDistrib::getParentMsgList(std::string grpId, std::string pId, 
						std::list<std::string> &msgIds)
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
		if (mit->second->parentId == pId)
		{
			msgIds.push_back(mit->first);
		}
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

bool    p3GroupDistrib::subscribeToGroup(std::string grpId, bool subscribe)
{
	RsStackMutex stack(distribMtx); /*************  STACK MUTEX ************/
	std::map<std::string, GroupInfo>::iterator git;
	if (mGroups.end() == (git = mGroups.find(grpId)))
	{
		return false;
	}

	if (subscribe)
	{
		if (!(git->second.flags & RS_DISTRIB_SUBSCRIBED)) 
		{
			git->second.flags |= RS_DISTRIB_SUBSCRIBED;
			mGroupsRepublish = true;
		}
	}
	else 
	{
		if (git->second.flags & RS_DISTRIB_SUBSCRIBED)
		{
			git->second.flags &= (~RS_DISTRIB_SUBSCRIBED);
			mGroupsRepublish = true;
		}
	}

	return true;
}

/************************************* p3Config *************************************/

RsSerialiser *p3GroupDistrib::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsDistribSerialiser());

	return rss;
}

std::list<RsItem *> p3GroupDistrib::saveList(bool &cleanup)
{
	std::list<RsItem *> saveData;

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::saveList()";
	std::cerr << std::endl;
#endif

	cleanup = false;

	distribMtx.lock(); /****** MUTEX LOCKED *******/

	/* Iterate through all the Groups */
	std::map<std::string, GroupInfo>::iterator it;
	for(it = mGroups.begin(); it != mGroups.end(); it++)
	{
		/* if subscribed or listener -> do stuff */
		if (it->second.flags & (RS_DISTRIB_SUBSCRIBED))
		{
#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::saveList() Saving Group: " << it->first;
			std::cerr << std::endl;
#endif

			/* extract public info to RsDistribGrp */
			RsDistribGrp *grp = it->second.distribGroup;

			if (grp)
			{
				/* store in Cache File */
				saveData.push_back(grp); /* no delete */
			}

			/* if they have public keys, publish these too */
			std::map<std::string, GroupKey>::iterator kit;
			for(kit = it->second.publishKeys.begin(); 
						kit != it->second.publishKeys.end(); kit++)
			{
				if (kit->second.type & RSTLV_KEY_TYPE_FULL)
				{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::saveList() Saving Key: " << kit->first;
		std::cerr << std::endl;
#endif
					/* create Key for sharing */

					RsDistribGrpKey *pubKey = new RsDistribGrpKey();
					pubKey->grpId = it->first;

					RSA *rsa_priv = EVP_PKEY_get1_RSA(kit->second.key);
					setRSAPrivateKey(pubKey->key, rsa_priv);
					RSA_free(rsa_priv);

					pubKey->key.keyFlags = kit->second.type;
					pubKey->key.startTS = kit->second.startTS;
					pubKey->key.endTS   = kit->second.endTS;

					saveData.push_back(pubKey);
					saveCleanupList.push_back(pubKey);
				}
				else
				{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::saveList() Ignoring Key: " << kit->first;
		std::cerr << std::endl;
		std::cerr << "p3GroupDistrib::saveList() Key Type: " << kit->second.type;
		std::cerr << std::endl;
#endif
				}

			}

			if (it->second.adminKey.type & RSTLV_KEY_TYPE_FULL)
			{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::saveList() Saving Admin Key";
		std::cerr << std::endl;
#endif
				/* create Key for sharing */

				RsDistribGrpKey *pubKey = new RsDistribGrpKey();
				pubKey->grpId = it->first;

				RSA *rsa_priv = EVP_PKEY_get1_RSA(kit->second.key);
				setRSAPrivateKey(pubKey->key, rsa_priv);
				RSA_free(rsa_priv);

				pubKey->key.keyFlags = RSTLV_KEY_TYPE_FULL;
				pubKey->key.keyFlags |= RSTLV_KEY_DISTRIB_ADMIN;
				pubKey->key.startTS = kit->second.startTS;
				pubKey->key.endTS   = kit->second.endTS;

				saveData.push_back(pubKey);
				saveCleanupList.push_back(pubKey);
			}
			else
			{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::saveList() Ignoring Admin Key: " << it->second.adminKey.keyId;
		std::cerr << std::endl;
		std::cerr << "p3GroupDistrib::saveList() Admin Key Type: " << it->second.adminKey.type;
		std::cerr << std::endl;
#endif
			}
		}
		else
		{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::saveList() Ignoring Group: " << it->first;
		std::cerr << std::endl;
#endif
		}

	}

	std::list<RsDistribSignedMsg *>::iterator mit;
	for(mit = mPendingPublish.begin(); mit != mPendingPublish.end(); mit++)
	{
		if ((*mit)->PeerId() == mOwnId)
		{
			saveData.push_back(*mit);
		}
	}

	return saveData;
}

void    p3GroupDistrib::saveDone()
{
	/* clean up the save List */
	std::list<RsItem *>::iterator it;
	for(it = saveCleanupList.begin(); it != saveCleanupList.end(); it++)
	{
		delete (*it);
	}

	saveCleanupList.clear();

	/* unlock mutex */
	distribMtx.unlock(); /****** MUTEX UNLOCKED *******/
}

bool    p3GroupDistrib::loadList(std::list<RsItem *> load)
{
	std::list<RsItem *>::iterator lit;
	for(lit = load.begin(); lit != load.end(); lit++)
	{
		/* decide what type it is */

		RsDistribGrp *newGrp = NULL;
		RsDistribGrpKey *newKey = NULL;
		RsDistribSignedMsg *newMsg = NULL;

		if ((newGrp = dynamic_cast<RsDistribGrp *>(*lit)))
		{
			std::string gid = newGrp -> grpId;
			loadGroup(newGrp);

			/* flag as SUBSCRIBER */
			RsStackMutex stack(distribMtx); /******* STACK LOCKED MUTEX ***********/

			std::map<std::string, GroupInfo>::iterator it;
			it = mGroups.find(gid);

			if (it != mGroups.end())
			{
				it->second.flags |= RS_DISTRIB_SUBSCRIBED;
			}
		}
		else if ((newKey = dynamic_cast<RsDistribGrpKey *>(*lit)))
		{
			loadGroupKey(newKey);
		}
		else if ((newMsg = dynamic_cast<RsDistribSignedMsg *>(*lit)))
		{
			newMsg->PeerId(mOwnId);
			loadMsg(newMsg, mOwnId, false); /* false so it'll pushed to PendingPublish list */
		}
	}

	/* no need to republish until something new comes in */
	RsStackMutex stack(distribMtx); /******* STACK LOCKED MUTEX ***********/
	mGroupsRepublish = false;

	return true;
}

/************************************* p3Config *************************************/

/* This Streamer is used for Reading and Writing Cache Files....
 * As All the child packets are Packed, we should only need RsSerialDistrib() in it.
 */

pqistreamer *p3GroupDistrib::createStreamer(BinInterface *bio, std::string src, uint32_t bioflags)
{
	RsSerialiser *rsSerialiser = new RsSerialiser();
	RsSerialType *serialType = new RsDistribSerialiser();
	rsSerialiser->addSerialType(serialType);

	pqistreamer *streamer = new pqistreamer(rsSerialiser, src, bio, bioflags);

	return streamer;
}


/***************************************************************************************/
/***************************************************************************************/
	/********************** Create Content   ***************************************/
/***************************************************************************************/
/***************************************************************************************/

std::string getRsaKeySign(RSA *pubkey)
{
	int len = BN_num_bytes(pubkey -> n);
	unsigned char tmp[len];
	BN_bn2bin(pubkey -> n, tmp);

	// copy first CERTSIGNLEN bytes...
	if (len > CERTSIGNLEN)
	{
		len = CERTSIGNLEN;
	}

        std::ostringstream id;
        for(uint32_t i = 0; i < CERTSIGNLEN; i++)
        {
		id << std::hex << std::setw(2) << std::setfill('0')
			<< (uint16_t) (((uint8_t *) (tmp))[i]);
	}
        std::string rsaId = id.str();

	return rsaId;
}

std::string getBinDataSign(void *data, int len)
{
	unsigned char *tmp = (unsigned char *) data;

	// copy first CERTSIGNLEN bytes...
	if (len > CERTSIGNLEN)
	{
		len = CERTSIGNLEN;
	}

        std::ostringstream id;
        for(uint32_t i = 0; i < CERTSIGNLEN; i++)
        {
		id << std::hex << std::setw(2) << std::setfill('0')
			<< (uint16_t) (((uint8_t *) (tmp))[i]);
	}
        std::string Id = id.str();

	return Id;
}


void 	setRSAPublicKey(RsTlvSecurityKey &key, RSA *rsa_pub)
{
	unsigned char data[10240]; /* more than enough space */
	unsigned char *ptr = data;
	int reqspace = i2d_RSAPublicKey(rsa_pub, &ptr);

	key.keyData.setBinData(data, reqspace);

	std::string keyId = getRsaKeySign(rsa_pub);
	key.keyId = keyId;
}

void 	setRSAPrivateKey(RsTlvSecurityKey &key, RSA *rsa_priv)
{
	unsigned char data[10240]; /* more than enough space */
	unsigned char *ptr = data;
	int reqspace = i2d_RSAPrivateKey(rsa_priv, &ptr);

	key.keyData.setBinData(data, reqspace);

	std::string keyId = getRsaKeySign(rsa_priv);
	key.keyId = keyId;
}


RSA *extractPublicKey(RsTlvSecurityKey &key)
{
	const unsigned char *keyptr = (const unsigned char *) key.keyData.bin_data;
	long keylen = key.keyData.bin_len;

	/* extract admin key */
	RSA *rsakey = d2i_RSAPublicKey(NULL, &(keyptr), keylen);

	return rsakey;
}


RSA *extractPrivateKey(RsTlvSecurityKey &key)
{
	const unsigned char *keyptr = (const unsigned char *) key.keyData.bin_data;
	long keylen = key.keyData.bin_len;

	/* extract admin key */
	RSA *rsakey = d2i_RSAPrivateKey(NULL, &(keyptr), keylen);

	return rsakey;
}


std::string p3GroupDistrib::createGroup(std::wstring name, std::wstring desc, uint32_t flags)
{
#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::createGroup()" << std::endl;
	std::cerr << std::endl;
#endif
	/* Create a Group */
	GroupInfo grpInfo;
	std::string grpId;
	time_t now = time(NULL);

	/* create Keys */
	RSA *rsa_admin = RSA_generate_key(2048, 65537, NULL, NULL);
	RSA *rsa_admin_pub = RSAPublicKey_dup(rsa_admin);

	RSA *rsa_publish = RSA_generate_key(2048, 65537, NULL, NULL);
	RSA *rsa_publish_pub = RSAPublicKey_dup(rsa_publish);

	/* Create Group Description */

	RsDistribGrp *newGrp = new RsDistribGrp();

	newGrp->grpName = name;
	newGrp->grpDesc = desc;
	newGrp->timestamp = now;
	newGrp->grpFlags = flags & (RS_DISTRIB_PRIVACY_MASK | RS_DISTRIB_AUTHEN_MASK);
	newGrp->grpControlFlags = 0;

	/* set keys */
	setRSAPublicKey(newGrp->adminKey, rsa_admin_pub);
	newGrp->adminKey.keyFlags = RSTLV_KEY_TYPE_PUBLIC_ONLY | RSTLV_KEY_DISTRIB_ADMIN;
	newGrp->adminKey.startTS = now;
	newGrp->adminKey.endTS = 0; /* no end */

	RsTlvSecurityKey publish_key;
	setRSAPublicKey(publish_key, rsa_publish_pub);

	publish_key.keyFlags = RSTLV_KEY_TYPE_PUBLIC_ONLY;
	if (flags & RS_DISTRIB_PUBLIC)
	{
		publish_key.keyFlags |= RSTLV_KEY_DISTRIB_PUBLIC;
	}
	else
	{
		publish_key.keyFlags |= RSTLV_KEY_DISTRIB_PRIVATE;
	}

	publish_key.startTS = now;
	publish_key.endTS = now + 60 * 60 * 24 * 365 * 5; /* approx 5 years */

	newGrp->publishKeys.keys[publish_key.keyId] = publish_key;
	newGrp->publishKeys.groupId = newGrp->adminKey.keyId;


	/************* create Key Messages (to Add Later) *********************/
	RsDistribGrpKey *adKey = new RsDistribGrpKey();
	adKey->grpId = grpId;

	setRSAPrivateKey(adKey->key, rsa_admin);
	adKey->key.keyFlags = RSTLV_KEY_TYPE_FULL | RSTLV_KEY_DISTRIB_ADMIN;
	adKey->key.startTS = newGrp->adminKey.startTS;
	adKey->key.endTS   = newGrp->adminKey.endTS;

	RsDistribGrpKey *pubKey = new RsDistribGrpKey();
	pubKey->grpId = grpId;

	setRSAPrivateKey(pubKey->key, rsa_publish);
	pubKey->key.keyFlags = RSTLV_KEY_TYPE_FULL;
	if (flags & RS_DISTRIB_PUBLIC)
	{
		pubKey->key.keyFlags |= RSTLV_KEY_DISTRIB_PUBLIC;
	}
	else
	{
		pubKey->key.keyFlags |= RSTLV_KEY_DISTRIB_PRIVATE;
	}
	pubKey->key.startTS = publish_key.startTS;
	pubKey->key.endTS   = publish_key.endTS;
	/************* create Key Messages (to Add Later) *********************/


	/* clean up publish_key manually -> else 
	 * the data will get deleted...
	 */

	publish_key.keyData.bin_data = NULL;
	publish_key.keyData.bin_len = 0;

	grpId = newGrp->adminKey.keyId;
	newGrp->grpId = grpId;

	/************** Serialise and sign **************************************/
	EVP_PKEY *key_admin = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(key_admin, rsa_admin);

	newGrp->adminSignature.TlvClear();

	RsSerialType *serialType = new RsDistribSerialiser(); 

	char data[16000];
	uint32_t size = 16000;

	serialType->serialise(newGrp, data, &size);

	/* calc and check signature */
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

	EVP_SignInit(mdctx, EVP_sha1());
	EVP_SignUpdate(mdctx, data, size);

	unsigned int siglen = EVP_PKEY_size(key_admin);
        unsigned char sigbuf[siglen];
	int ans = EVP_SignFinal(mdctx, sigbuf, &siglen, key_admin);

	/* save signature */
	newGrp->adminSignature.signData.setBinData(sigbuf, siglen);
	newGrp->adminSignature.keyId = grpId;

	/* clean up */
	delete serialType;
	EVP_MD_CTX_destroy(mdctx);


	/******************* clean up all Keys *******************/

	RSA_free(rsa_admin_pub);
	RSA_free(rsa_publish_pub);
	RSA_free(rsa_publish);
	EVP_PKEY_free(key_admin);

	/******************* load up new Group *********************/
	loadGroup(newGrp);

	/* add Keys to GroupInfo */
	RsStackMutex stack(distribMtx); /*************  STACK MUTEX ************/
	GroupInfo *gi = locked_getGroupInfo(grpId);
	if (!gi)
	{
		return grpId;
	}

	gi->flags |= RS_DISTRIB_SUBSCRIBED;
	mGroupsRepublish = true;

	/* replace the public keys */
	locked_updateGroupAdminKey(*gi, adKey);
	locked_updateGroupPublishKey(*gi, pubKey);

	delete adKey;
	delete pubKey;


	return grpId;
}


std::string	p3GroupDistrib::publishMsg(RsDistribMsg *msg, bool personalSign)
{

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::publishMsg()" << std::endl;
	msg->print(std::cerr, 10);
	std::cerr << std::endl;
#endif

	/* extract grpId */
	std::string grpId = msg->grpId;
	std::string msgId;

	RsDistribSignedMsg *signedMsg = NULL;

	/* ensure Group exists */
      { /* STACK MUTEX */
	RsStackMutex stack(distribMtx); /*************  STACK MUTEX ************/
	GroupInfo *gi = locked_getGroupInfo(grpId);
	if (!gi)
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::publishMsg() No Group";
		std::cerr << std::endl;
#endif
		return msgId;
	}

	/******************* FIND KEY ******************************/
	if (!locked_choosePublishKey(*gi))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::publishMsg() No Publish Key(1)";
		std::cerr << std::endl;
#endif
		return msgId;
	}

	/* find valid publish_key */
	EVP_PKEY *publishKey = NULL;
	std::map<std::string, GroupKey>::iterator kit;
	kit = gi->publishKeys.find(gi->publishKeyId);
	if (kit != gi->publishKeys.end())
	{
		publishKey = kit->second.key;
	}

	if (!publishKey)
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::publishMsg() No Publish Key";
		std::cerr << std::endl;
#endif
		/* no publish Key */
		return msgId;
	}
	/******************* FIND KEY ******************************/

	signedMsg = new RsDistribSignedMsg();

	RsSerialType *serialType = createSerialiser();
	uint32_t size = serialType->size(msg);
	void *data = malloc(size);

	serialType->serialise(msg, data, &size);
	signedMsg->packet.setBinData(data, size);

	/* sign Packet */

	/* calc and check signature */
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

	EVP_SignInit(mdctx, EVP_sha1());
	EVP_SignUpdate(mdctx, data, size);

	unsigned int siglen = EVP_PKEY_size(publishKey);
        unsigned char sigbuf[siglen];
	int ans = EVP_SignFinal(mdctx, sigbuf, &siglen, publishKey);

	/* save signature */
	signedMsg->publishSignature.signData.setBinData(sigbuf, siglen);
	signedMsg->publishSignature.keyId = gi->publishKeyId;

#if 0
	if (personalSign)
	{
		/* calc and check signature */
		EVP_MD_CTX *mdctx2 = EVP_MD_CTX_create();

		EVP_SignInit(mdctx2, EVP_sha1());
		EVP_SignUpdate(mdctx2, data, size);

		unsigned int siglen = EVP_PKEY_size(personal_admin);
        	unsigned char sigbuf[siglen];
		int ans = EVP_SignFinal(mdctx2, sigbuf, &siglen, personal_admin);

		signedMsg->personalSignature.signData.setBinData(sigbuf, siglen);
		signedMsg->personalSignature.keyId = ownId;
	
		EVP_MD_CTX_destroy(mdctx2);
	}
#endif

	/* clean up */
	delete serialType;
	EVP_MD_CTX_destroy(mdctx);

      } /* END STACK MUTEX */

	/* extract Ids from publishSignature */
	signedMsg->msgId = getBinDataSign(
		signedMsg->publishSignature.signData.bin_data, 
		signedMsg->publishSignature.signData.bin_len);
	signedMsg->grpId = grpId;
	signedMsg->timestamp = msg->timestamp;

	msgId = signedMsg->msgId;

	/* delete original msg */
	delete msg;

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::publishMsg() Created SignedMsg:";
	std::cerr << std::endl;
	signedMsg->print(std::cerr, 10);
	std::cerr << std::endl;
#endif

	/* load proper -
	 * If we pretend it is coming from an alternative source
	 * it'll automatically get published with other msgs
	 */
	signedMsg->PeerId(mOwnId);
	loadMsg(signedMsg, mOwnId, false);

	/* done */
	return msgId;
}




/********************* Overloaded Functions **************************/

bool 	p3GroupDistrib::validateDistribGrp(RsDistribGrp *newGrp)
{
#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::validateDistribGrp()";
	std::cerr << std::endl;
#endif

	/* check signature */
	RsSerialType *serialType = new RsDistribSerialiser(); 

	char data[16000];
	uint32_t size = 16000;

	/* copy out signature (shallow copy) */
	RsTlvKeySignature tmpSign = newGrp->adminSignature;
	unsigned char *sigbuf = (unsigned char *) tmpSign.signData.bin_data;
	unsigned int siglen = tmpSign.signData.bin_len;

	/* clear signature */
	newGrp->adminSignature.ShallowClear();

	serialType->serialise(newGrp, data, &size);


	const unsigned char *keyptr = (const unsigned char *) newGrp->adminKey.keyData.bin_data;
	long keylen = newGrp->adminKey.keyData.bin_len;

	/* extract admin key */
	RSA *rsakey = d2i_RSAPublicKey(NULL, &(keyptr), keylen);

	EVP_PKEY *key = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(key, rsakey);

	/* calc and check signature */
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

	EVP_VerifyInit(mdctx, EVP_sha1());
	EVP_VerifyUpdate(mdctx, data, size);
	int ans = EVP_VerifyFinal(mdctx, sigbuf, siglen, key);


	/* restore signature */
	newGrp->adminSignature = tmpSign;
	tmpSign.ShallowClear();

	/* clean up */
	EVP_PKEY_free(key);
	delete serialType;
	EVP_MD_CTX_destroy(mdctx);


	if (ans == 1)
		return true;

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::validateDistribGrp() Signature invalid";
	std::cerr << std::endl;
#endif
	return false;
}


bool 	p3GroupDistrib::locked_checkGroupInfo(GroupInfo &info, RsDistribGrp *newGrp)
{
#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_checkGroupInfo()";
	std::cerr << std::endl;
#endif
	/* groupInfo */

	/* If adminKey is the same and 
	 * timestamp is <= timestamp, 
	 * then just discard it.
	 */

	if (info.grpId != newGrp->grpId)
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_checkGroupInfo() Failed GrpId Wrong";
		std::cerr << std::endl;
#endif
		return false;
	}

	if ((info.distribGroup) && 
		(info.distribGroup->timestamp <= newGrp->timestamp))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_checkGroupInfo() Group Data Old/Same";
		std::cerr << std::endl;
#endif
		/* old or same info -> drop it */
		return false;
	}

	/* otherwise validate it */
	return validateDistribGrp(newGrp);
}


/* return false - to cleanup (delete group) afterwards, 
 * true - we've kept the data
 */
bool 	p3GroupDistrib::locked_updateGroupInfo(GroupInfo &info, RsDistribGrp *newGrp)
{
	/* new group has been validated already 
	 * update information.
	 */

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_updateGroupInfo()";
	std::cerr << std::endl;
#endif

	if (info.distribGroup)
	{
		delete info.distribGroup;
	}

	info.distribGroup = newGrp;

	/* copy details  */
	info.grpName = newGrp->grpName;
	info.grpDesc = newGrp->grpDesc;
	info.grpCategory = newGrp->grpCategory;
	info.grpFlags   = newGrp->grpFlags; 

	/* pop already calculated */
	/* last post handled seperately */

	locked_checkGroupKeys(info);

	/* if we are subscribed to the group -> then we need to republish */
	if (info.flags & RS_DISTRIB_SUBSCRIBED)
	{
		mGroupsRepublish = true;
	}

	return true;
}


bool 	p3GroupDistrib::locked_checkGroupKeys(GroupInfo &info)
{
#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_checkGroupKeys()";
	std::cerr << "GrpId: " << info.grpId;
	std::cerr << std::endl;
#endif

	/* iterate through publish keys - check that they exist in distribGrp, or delete */
	RsDistribGrp *grp = info.distribGroup;

	std::list<std::string> removeKeys;
	std::map<std::string, GroupKey>::iterator it;

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_checkGroupKeys() Checking if Expanded Keys still Exist";
	std::cerr << std::endl;
#endif

	for(it = info.publishKeys.begin(); it != info.publishKeys.end(); it++)
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_checkGroupKeys() Publish Key: " << it->first;
#endif
		/* check for key in distribGrp */
		if (grp->publishKeys.keys.end() == grp->publishKeys.keys.find(it->first))
		{
			/* remove publishKey */
			removeKeys.push_back(it->first);
#ifdef DISTRIB_DEBUG
			std::cerr << " Old -> to Remove" << std::endl;
#endif

		}

#ifdef DISTRIB_DEBUG
		std::cerr << " Ok" << std::endl;
#endif

	}

	while(removeKeys.size() > 0)
	{
		std::string rkey = removeKeys.front();
		removeKeys.pop_front();
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_checkGroupKeys() Removing Key: " << rkey;
		std::cerr << std::endl;
#endif

		it = info.publishKeys.find(rkey);
		EVP_PKEY_free(it->second.key);
		info.publishKeys.erase(it);
	}

	/* iterate through distribGrp list - expanding any missing keys */
	std::map<std::string, RsTlvSecurityKey>::iterator dit;
	for(dit = grp->publishKeys.keys.begin(); dit != grp->publishKeys.keys.end(); dit++)
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_checkGroupKeys() Checking for New Keys: KeyId: " << dit->first;
		std::cerr << std::endl;
#endif

		it = info.publishKeys.find(dit->first);
		if (it == info.publishKeys.end())
		{
#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::locked_checkGroupKeys() Key Missing - Expand";
			std::cerr << std::endl;
#endif

			/* create a new expanded public key */
			RSA *rsa_pub = extractPublicKey(dit->second);
			if (!rsa_pub)
			{
#ifdef DISTRIB_DEBUG
				std::cerr << "p3GroupDistrib::locked_checkGroupKeys() Failed to Expand Key";
				std::cerr << std::endl;
#endif
				continue;
			}

			GroupKey newKey;
			newKey.keyId = dit->first;
			newKey.type = RSTLV_KEY_TYPE_PUBLIC_ONLY | (dit->second.keyFlags & RSTLV_KEY_DISTRIB_MASK);
			newKey.startTS = dit->second.startTS;
			newKey.endTS = dit->second.endTS;

			newKey.key = EVP_PKEY_new();
			EVP_PKEY_assign_RSA(newKey.key, rsa_pub);
			
			info.publishKeys[newKey.keyId] = newKey;

#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::locked_checkGroupKeys() Expanded Key: " << dit->first;
			std::cerr << "Key Type: " << newKey.type;
			std::cerr << std::endl;
			std::cerr << "Start: " << newKey.startTS;
			std::cerr << std::endl;
			std::cerr << "End: " << newKey.endTS;
			std::cerr << std::endl;
#endif
		}
	}

	/* now check admin key */
	if ((info.adminKey.keyId == "") || (!info.adminKey.key))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_checkGroupKeys() Must Expand AdminKey Too";
		std::cerr << std::endl;
#endif

		/* must expand admin key too */
		RSA *rsa_pub = extractPublicKey(grp->adminKey);
		if (rsa_pub)
		{
			info.adminKey.keyId = grp->adminKey.keyId;
			info.adminKey.type = RSTLV_KEY_TYPE_PUBLIC_ONLY & RSTLV_KEY_DISTRIB_ADMIN;
			info.adminKey.startTS = grp->adminKey.startTS;
			info.adminKey.endTS = grp->adminKey.endTS;

			info.adminKey.key = EVP_PKEY_new();
			EVP_PKEY_assign_RSA(info.adminKey.key, rsa_pub);
#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::locked_checkGroupKeys() AdminKey Expanded";
			std::cerr << std::endl;
#endif
		}
		else
		{
#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::locked_checkGroupKeys() ERROR Expandng AdminKey";
			std::cerr << std::endl;
#endif
		}
	}

	return true;
}



bool 	p3GroupDistrib::locked_updateGroupAdminKey(GroupInfo &info, RsDistribGrpKey *newKey)
{
	/* so firstly - check that the KeyId matches something in the group */
	std::string keyId = newKey->key.keyId;

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_updateGroupAdminKey() grpId: " << keyId;
	std::cerr << std::endl;
#endif


	if (keyId != info.grpId)
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_updateGroupAdminKey() Id mismatch - ERROR";
		std::cerr << std::endl;
#endif

		return false;
	}

	if (!(newKey->key.keyFlags & RSTLV_KEY_TYPE_FULL))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_updateGroupAdminKey() Key not Full - Ignore";
		std::cerr << std::endl;
#endif

		/* not a full key -> ignore */
		return false;
	}

	if (info.adminKey.type & RSTLV_KEY_TYPE_FULL)
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_updateGroupAdminKey() Already have Full Key - Ignore";
		std::cerr << std::endl;
#endif

		/* if we have full key already - ignore */
		return true;
	}

	/* need to update key */
	RSA *rsa_priv = extractPrivateKey(newKey->key);

	if (!rsa_priv)
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_updateGroupAdminKey() Extract Key failed - ERROR";
		std::cerr << std::endl;
#endif

		return false;
	}

	/* validate they are a matching pair */
	std::string realkeyId = getRsaKeySign(rsa_priv);
	if ((1 != RSA_check_key(rsa_priv)) || (realkeyId != keyId))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_updateGroupAdminKey() Validate Key Failed - ERROR";
		std::cerr << std::endl;
#endif

		/* clean up */
		RSA_free(rsa_priv);
		return false;
	}

	/* add it in */
	EVP_PKEY *evp_pkey = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(evp_pkey, rsa_priv);

	EVP_PKEY_free(info.adminKey.key);
	info.adminKey.key = evp_pkey;
	info.adminKey.type = RSTLV_KEY_TYPE_FULL | RSTLV_KEY_DISTRIB_ADMIN;

	info.flags |= RS_DISTRIB_ADMIN;

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_updateGroupAdminKey() Success";
	std::cerr << std::endl;
#endif

	return true;
}


bool 	p3GroupDistrib::locked_updateGroupPublishKey(GroupInfo &info, RsDistribGrpKey *newKey)
{
	/* so firstly - check that the KeyId matches something in the group */
	std::string keyId = newKey->key.keyId;

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_updateGroupPublishKey() grpId: " << info.grpId << " keyId: " << keyId;
	std::cerr << std::endl;
#endif


	std::map<std::string, GroupKey>::iterator it;
	it = info.publishKeys.find(keyId);
	if (it == info.publishKeys.end())
	{

#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_updateGroupPublishKey() key not Found - Ignore";
		std::cerr << std::endl;
#endif

		/* no key -> ignore */
		return false;
	}

	if (!(newKey->key.keyFlags & RSTLV_KEY_TYPE_FULL))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_updateGroupPublishKey() not FullKey - Ignore";
		std::cerr << std::endl;
#endif

		/* not a full key -> ignore */
		return false;
	}

	if (it->second.type & RSTLV_KEY_TYPE_FULL)
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_updateGroupPublishKey() already have FullKey - Ignore";
		std::cerr << std::endl;
#endif

		/* if we have full key already - ignore */
		return true;
	}

	/* need to update key */
	RSA *rsa_priv = extractPrivateKey(newKey->key);

	if (!rsa_priv)
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_updateGroupPublishKey() Private Extract Failed - ERROR";
		std::cerr << std::endl;
#endif

		return false;
	}

	/* validate they are a matching pair */
	std::string realkeyId = getRsaKeySign(rsa_priv);
	if ((1 != RSA_check_key(rsa_priv)) || (realkeyId != keyId))
	{

#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_updateGroupPublishKey() Validate Private Key failed - ERROR";
		std::cerr << std::endl;
#endif

		/* clean up */
		RSA_free(rsa_priv);
		return false;
	}

	/* add it in */
	EVP_PKEY *evp_pkey = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(evp_pkey, rsa_priv);

	EVP_PKEY_free(it->second.key);
	it->second.key = evp_pkey;
	it->second.type &= (~RSTLV_KEY_TYPE_PUBLIC_ONLY);
	it->second.type |= RSTLV_KEY_TYPE_FULL;

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_updateGroupPublishKey() Success";
	std::cerr << std::endl;
	std::cerr << "Key ID: " << it->first;
	std::cerr << std::endl;
	std::cerr << "Key Type: " << it->second.type;
	std::cerr << std::endl;
	std::cerr << "Start: " << it->second.startTS;
	std::cerr << std::endl;
	std::cerr << "End: " << it->second.endTS;
	std::cerr << std::endl;
#endif

	info.flags |= RS_DISTRIB_PUBLISH;

	/* if we have updated, we are subscribed, and it is a public key */
	if ((info.flags & RS_DISTRIB_SUBSCRIBED) &&
		(it->second.type & RSTLV_KEY_DISTRIB_PUBLIC))
	{
		mGroupsRepublish = true;
	}

	return true;
}


bool 	p3GroupDistrib::locked_choosePublishKey(GroupInfo &info)
{

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_choosePublishKey()";
	std::cerr << std::endl;
#endif
	time_t now = time(NULL);

	/******************* CHECK CURRENT KEY ******************************/
	/* if current key is valid -> okay */

	std::map<std::string, GroupKey>::iterator kit;
	kit = info.publishKeys.find(info.publishKeyId);
	if (kit != info.publishKeys.end())
	{
		if ((kit->second.type & RSTLV_KEY_TYPE_FULL) && 
			(now < kit->second.endTS) && (now > kit->second.startTS)) 
		{
			/* key is okay */
#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::locked_choosePublishKey() Current Key is Okay";
			std::cerr << std::endl;
#endif
			return true;
		}
	}

	/******************* FIND KEY ******************************/
	std::string bestKey = "";
	time_t bestEndTime = 0;

	for(kit = info.publishKeys.begin(); kit != info.publishKeys.end(); kit++)
	{

#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_choosePublishKey() Found Key: ";
		std::cerr << kit->first << " type: " << kit->second.type;
		std::cerr << std::endl;
#endif

		if (kit->second.type & RSTLV_KEY_TYPE_FULL)
		{
#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::locked_choosePublishKey() Found FULL Key: ";
			std::cerr << kit->first << " startTS: " << kit->second.startTS;
			std::cerr << " endTS: " << kit->second.startTS;
			std::cerr << " now: " << now;
			std::cerr << std::endl;
#endif
			if ((now < kit->second.endTS) && (now >= kit->second.startTS)) 
			{
				if (kit->second.endTS > bestEndTime)
				{
					bestKey = kit->first;
					bestEndTime = kit->second.endTS;
#ifdef DISTRIB_DEBUG
				std::cerr << "p3GroupDistrib::locked_choosePublishKey() Better Key: ";
				std::cerr << kit->first;
				std::cerr << std::endl;
#endif
				}
			}
		}
	}

	if (bestEndTime == 0)
	{
#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::locked_choosePublishKey() No Valid Key";
			std::cerr << std::endl;
#endif
		return false;
	}

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_choosePublishKey() Best Key is: " << bestKey;
	std::cerr << std::endl;
#endif

	info.publishKeyId = bestKey;
	return true;
}


/********************/

bool 	p3GroupDistrib::locked_validateDistribSignedMsg(
				GroupInfo &info, RsDistribSignedMsg *newMsg)
{
#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_validateDistribSignedMsg()";
	std::cerr << std::endl;
	std::cerr << "GroupInfo -> distribGrp:";
	std::cerr << std::endl;
	info.distribGroup->print(std::cerr, 10);
	std::cerr << std::endl;
	std::cerr << "RsDistribSignedMsg: ";
	std::cerr << std::endl;
	newMsg->print(std::cerr, 10);
	std::cerr << std::endl;
#endif

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_validateDistribSignedMsg() KeyId: ";
	std::cerr << newMsg->publishSignature.keyId;
	std::cerr << std::endl;
#endif

	/********************* check signature *******************/

	/* find the right key */
	RsTlvSecurityKeySet &keyset = info.distribGroup->publishKeys;

	std::map<std::string, RsTlvSecurityKey>::iterator kit;
	kit = keyset.keys.find(newMsg->publishSignature.keyId);

	if (kit == keyset.keys.end())
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_validateDistribSignedMsg() Missing Publish Key";
		std::cerr << std::endl;
#endif
		return false;
	}

	/* check signature timeperiod */
	if ((newMsg->timestamp < kit->second.startTS) ||
		(newMsg->timestamp > kit->second.endTS))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_validateDistribSignedMsg() TS out of range";
		std::cerr << std::endl;
#endif
		return false;
	}


	/* decode key */
	const unsigned char *keyptr = (const unsigned char *) kit->second.keyData.bin_data;
	long keylen = kit->second.keyData.bin_len;
	unsigned int siglen = newMsg->publishSignature.signData.bin_len;
        unsigned char *sigbuf = (unsigned char *) newMsg->publishSignature.signData.bin_data;

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_validateDistribSignedMsg() Decode Key";
	std::cerr << " keylen: " << keylen << " siglen: " << siglen;
	std::cerr << std::endl;
#endif

	/* extract admin key */
	RSA *rsakey = d2i_RSAPublicKey(NULL, &(keyptr), keylen);

	if (!rsakey)
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_validateDistribSignedMsg()";
		std::cerr << " Invalid RSA Key";
		std::cerr << std::endl;

		unsigned long err = ERR_get_error();
		std::cerr << "RSA Load Failed .... CODE(" << err << ")" << std::endl;
		std::cerr << ERR_error_string(err, NULL) << std::endl;

		kit->second.print(std::cerr, 10);
#endif
	}
		

	EVP_PKEY *signKey = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(signKey, rsakey);

	/* calc and check signature */
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

	EVP_VerifyInit(mdctx, EVP_sha1());
	EVP_VerifyUpdate(mdctx, newMsg->packet.bin_data, newMsg->packet.bin_len);
	int signOk = EVP_VerifyFinal(mdctx, sigbuf, siglen, signKey);

	/* clean up */
	EVP_PKEY_free(signKey);
	EVP_MD_CTX_destroy(mdctx);


	/* now verify Personal signature */
#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_validateDistribSignedMsg() Personal Signature TODO";
	std::cerr << std::endl;
#endif

	if (signOk == 1)
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_validateDistribSignedMsg() Signature OK";
		std::cerr << std::endl;
#endif
		return true;
	}

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_validateDistribSignedMsg() Signature invalid";
	std::cerr << std::endl;
#endif

	return false;
}

	/* deserialise RsDistribSignedMsg */
RsDistribMsg *p3GroupDistrib::unpackDistribSignedMsg(RsDistribSignedMsg *newMsg)
{
	RsSerialType *serialType = createSerialiser();
	uint32_t size = newMsg->packet.bin_len;
	RsDistribMsg *distribMsg = (RsDistribMsg *) 
		serialType->deserialise(newMsg->packet.bin_data, &size);

	if (distribMsg)
	{

		/* transfer data that is not in the serialiser */
		distribMsg->msgId = newMsg->msgId;

		/* Full copies required ? */

		distribMsg->publishSignature.keyId = newMsg->publishSignature.keyId;
		distribMsg->publishSignature.signData.setBinData(
				newMsg->publishSignature.signData.bin_data,
				newMsg->publishSignature.signData.bin_len);

		distribMsg->personalSignature.keyId = newMsg->personalSignature.keyId;
		distribMsg->personalSignature.signData.setBinData(
				newMsg->personalSignature.signData.bin_data,
				newMsg->personalSignature.signData.bin_len);
	}

	delete serialType;

	return distribMsg;
}


bool	p3GroupDistrib::locked_checkDistribMsg(
				GroupInfo &gi, RsDistribMsg *msg)
{

	/* check timestamp */
	time_t now = time(NULL);
	time_t min = now - mStorePeriod;
	time_t minPub = now - mStorePeriod / 2.0;
	time_t max = now + GROUP_MAX_FWD_OFFSET;

	if ((msg->timestamp < min) || (msg->timestamp > max))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::locked_checkDistribMsg() TS out of range";
		std::cerr << std::endl;
		std::cerr << "p3GroupDistrib::locked_checkDistribMsg() msg->timestamp: " << msg->timestamp;
		std::cerr << std::endl;
		std::cerr << "p3GroupDistrib::locked_checkDistribMsg() = " << now - msg->timestamp << " secs ago";
		std::cerr << std::endl;
		std::cerr << "p3GroupDistrib::locked_checkDistribMsg() max TS: " << max;
		std::cerr << std::endl;
		std::cerr << "p3GroupDistrib::locked_checkDistribMsg() min TS: " << min;
		std::cerr << std::endl;
#endif
		/* if outside range -> remove */
		return false;
	}

	/* check filters */

	return true;
}


/***** DEBUG *****/

void    p3GroupDistrib::printGroups(std::ostream &out)
{
	/* iterate through all the groups */
	std::map<std::string, GroupInfo>::iterator git;
	for(git = mGroups.begin(); git != mGroups.end(); git++)
	{
		out << "GroupId: " << git->first << std::endl;
		out << "Group Details: " << std::endl;
		out << git->second;
		out << std::endl;

		std::map<std::string, RsDistribMsg *>::iterator mit;
		for(mit = git->second.msgs.begin(); 
			mit != git->second.msgs.end(); mit++)
		{
			out << "MsgId: " << mit->first << std::endl;
			out << "Message Details: " << std::endl;
			mit->second->print(out, 10);
			out << std::endl;
		}
	}
}



std::ostream &operator<<(std::ostream &out, const GroupInfo &info)
{
	/* print Group Info */
	out << "GroupInfo: " << info.grpId << std::endl;
	out << "sources [" << info.sources.size() << "]: ";

	std::list<std::string>::const_iterator sit;
	for(sit = info.sources.begin(); sit != info.sources.end(); sit++)
	{
		out << " " << *sit;
	}
	out << std::endl;

	out << " Message Count: " << info.msgs.size() << std::endl;

	std::string grpName(info.grpName.begin(), info.grpName.end());
	std::string grpDesc(info.grpDesc.begin(), info.grpDesc.end());

	out << "Group Name:  " << grpName << std::endl;
	out << "Group Desc:  " << grpDesc << std::endl;
	out << "Group Flags: " << info.flags << std::endl;
	out << "Group Pop:   " << info.pop << std::endl;
	out << "Last Post:   " << info.lastPost << std::endl;

	out << "PublishKeyId:" << info.publishKeyId << std::endl;

	out << "Published RsDistribGrp: " << std::endl;
	if (info.distribGroup)
	{
		info.distribGroup->print(out, 10);
	}
	else
	{
		out << "No RsDistribGroup Object" << std::endl;
	}

	return out;
}

void 	p3GroupDistrib::locked_notifyGroupChanged(GroupInfo &info)
{
	mGroupsChanged = true;
	info.grpChanged = true;
}
	

bool p3GroupDistrib::groupsChanged(std::list<std::string> &groupIds)
{
	RsStackMutex stack(distribMtx); /*************  STACK MUTEX ************/

	/* iterate through all groups and get changed list */
	if (!mGroupsChanged)
		return false;

	std::map<std::string, GroupInfo>::iterator it;
	for(it = mGroups.begin(); it != mGroups.end(); it++)
	{
		if (it->second.grpChanged)
		{
			groupIds.push_back(it->first);
			it->second.grpChanged = false;
		}
	}

	mGroupsChanged = false;
	return true;
}


