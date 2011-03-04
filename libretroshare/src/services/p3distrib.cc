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

#ifdef WINDOWS_SYS
#include "util/rswin.h"
#endif

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <algorithm>

#include "retroshare/rsdistrib.h"
#include "services/p3distrib.h"
#include "serialiser/rsdistribitems.h"
#include "serialiser/rstlvkeys.h"

#include "util/rsdir.h"
#include "pqi/pqinotify.h"
#include "pqi/pqibin.h"
#include "pqi/sslfns.h"
#include "pqi/authssl.h"
#include "pqi/authgpg.h"

/*****
 * #define DISTRIB_DEBUG 1
 * #define DISTRIB_THREAD_DEBUG 1
 * #define DISTRIB_DUMMYMSG_DEBUG 1
 ****/

//#define DISTRIB_DEBUG 1
//#define DISTRIB_THREAD_DEBUG 1
//#define DISTRIB_DUMMYMSG_DEBUG 1

RSA *extractPublicKey(RsTlvSecurityKey &key);
RSA *extractPrivateKey(RsTlvSecurityKey &key);
void 	setRSAPublicKey(RsTlvSecurityKey &key, RSA *rsa_pub);
void 	setRSAPrivateKey(RsTlvSecurityKey &key, RSA *rsa_priv);



p3GroupDistrib::p3GroupDistrib(uint16_t subtype, 
		CacheStrapper *cs, CacheTransfer *cft,
		std::string sourcedir, std::string storedir, 
		std::string keyBackUpDir, uint32_t configId,
                uint32_t storePeriod, uint32_t pubPeriod)

	:CacheSource(subtype, true, cs, sourcedir), 
	CacheStore(subtype, true, cs, cft, storedir), 
        p3Config(configId), p3ThreadedService(subtype),
	mHistoricalCaches(true),
	mStorePeriod(storePeriod), 
	mPubPeriod(pubPeriod), 
	mLastPublishTime(0),
	mMaxCacheSubId(1),
	mKeyBackUpDir(keyBackUpDir), BACKUP_KEY_FILE("key.log"), mLastKeyPublishTime(0), mLastRecvdKeyTime(0)
{
	/* force publication of groups (cleared if local cache file found) */
	mGroupsRepublish = true;
	mGroupsChanged = true;

        mOwnId = AuthSSL::getAuthSSL()->OwnId();

        addSerialType(new RsDistribSerialiser(getRsItemService(getType())));
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
                toPublish = ((mPendingPublish.size() > 0) || (mPendingPubKeyRecipients.size() > 0)) && (now > (time_t) (mPubPeriod + mLastPublishTime));

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

        bool attemptRecv = false;

	{
		RsStackMutex stack(distribMtx);
                toPublish = (mPendingPubKeyRecipients.size() > 0) && (now > (time_t) (mPubPeriod + mLastKeyPublishTime));
                attemptRecv = (mRecvdPubKeys.size() > 0); // attempt to load stored keys in case user has subscribed
	}

	if(toPublish){
		RsStackMutex stack(distribMtx);
		locked_sharePubKey();
	}


        if(attemptRecv)
        {
            attemptPublishKeysRecvd();
        }

	bool toReceive = receivedItems();

	if(toReceive){

                receivePubKeys();
	}

	// update cache table every minute
	bool updateCacheDoc = false;
	{
		updateCacheDoc = (now > (time_t) (mLastCacheDocUpdate + 60.));
	}

	if(false)
		updateCacheDocument(mCacheDoc);

	return 0;
}


/***************************************************************************************/
/***************************************************************************************/
	/********************** overloaded functions from Cache Store ******************/
/***************************************************************************************/
/***************************************************************************************/

int    p3GroupDistrib::loadCache(const CacheData &data)
{
#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::loadCache()";
	std::cerr << std::endl;
#endif

	{
		RsStackMutex stack(distribMtx);

#ifdef DISTRIB_THREAD_DEBUG
	std::cerr << "p3GroupDistrib::loadCache() Storing PendingRemoteCache";
	std::cerr << std::endl;
#endif
		/* store the cache file for later processing */
        	mPendingCaches.push_back(CacheDataPending(data, false, mHistoricalCaches));
	}

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

	{
		RsStackMutex stack(distribMtx);

#ifdef DISTRIB_THREAD_DEBUG
	std::cerr << "p3GroupDistrib::loadCache() Storing PendingLocalCache";
	std::cerr << std::endl;
#endif

		/* store the cache file for later processing */
        	mPendingCaches.push_back(CacheDataPending(data, true, mHistoricalCaches));
	}

	if (data.size > 0)
	{
		refreshCache(data);
	}

	return true;
}


void p3GroupDistrib::updateCacheDocument(pugi::xml_document& cacheDoc)
{

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::updateCacheDocument() "
			  << std::endl;
#endif

	/*
	 * find group ids put down in mGroupCacheIds to be set into
	 * the cache document then their subsequent messages
	 */

	std::map<std::string, std::string>::iterator cit =
			mGroupCacheIds.begin(), mit = mMsgCacheIds.begin();

	std::map<std::string, GroupInfo>::iterator git;
	std::map<std::string, RsDistribMsg *>::iterator msgIt;


	for(; cit != mGroupCacheIds.end(); cit++){

		// add the group at depth on
		git = mGroups.find(cit->first);

		if(git != mGroups.end()){

			// add another group node
			cacheDoc.append_child("group");

			// then add title
			cacheDoc.last_child().append_child("title").append_child(
					pugi::node_pcdata).set_value(git->second.grpId.c_str());

			// then add the id
			cacheDoc.last_child().append_child("id").append_child(
					pugi::node_pcdata).set_value(cit->second
							.c_str());

		}
	}

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::updateCacheDocument() "
			  << "\nFinished Building xml doc, \n saving to file"
			  << std::endl;
#endif


	cacheDoc.save_file("distrib_group.xml");
	mLastCacheDocUpdate = time(NULL);

	// essentially build by gathering all your group

	// now do the same with messages which may have arrived for a group


}


/* Handle the Cache Pending Setup */
CacheDataPending::CacheDataPending(const CacheData &data, bool local, bool historical)
	:mData(data), mLocal(local), mHistorical(historical)
{
	return;
}

void p3GroupDistrib::HistoricalCachesDone()
{
	RsStackMutex stack(distribMtx);
	mHistoricalCaches = false; // called when Stored Caches have been added to Pending List.
}

                /* From RsThread */
void p3GroupDistrib::run() /* called once the thread is started */
{

#ifdef DISTRIB_THREAD_DEBUG
	std::cerr << "p3GroupDistrib::run()";
	std::cerr << std::endl;
#endif

#ifdef DISTRIB_DUMMYMSG_DEBUG
	int printed = 0;
#endif

	while(1)
	{
		/* */
		CacheData cache;
		bool validCache = false;
		bool isLocal = false;
		bool isHistorical = false;
		{
			RsStackMutex stack(distribMtx);

			if (mPendingCaches.size() > 0)
			{
				CacheDataPending &pendingCache = mPendingCaches.front();
				cache = pendingCache.mData;
				isLocal = pendingCache.mLocal;
				isHistorical = pendingCache.mHistorical;
				
				validCache = true;
				mPendingCaches.pop_front();

#ifdef DISTRIB_THREAD_DEBUG
				std::cerr << "p3GroupDistrib::run() found pendingCache";
				std::cerr << std::endl;
#endif

			}
		}

		if (validCache)
		{
			loadAnyCache(cache, isLocal, isHistorical);

#ifndef WINDOWS_SYS
			usleep(1000);
#else
			Sleep(1);
#endif
		}
		else
		{
#ifndef WINDOWS_SYS
			sleep(1);
#else
			Sleep(1000);
#endif

#ifdef DISTRIB_DUMMYMSG_DEBUG
			/* HACK for debugging */
			if (printed < 10)
			{
				RsStackMutex stack(distribMtx);
				locked_printAllDummyMsgs();

				printed++;
			}
#endif
			
		}
	}
}



int     p3GroupDistrib::loadAnyCache(const CacheData &data, bool local, bool historical)
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
		loadFileGroups(file, data.pid, local, historical);
	}
	else
	{
		loadFileMsgs(file, data.cid.subid, data.pid, data.recvd, local, historical);
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
void	p3GroupDistrib::loadFileGroups(const std::string &filename, const std::string &src, bool local, bool historical)
{
#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::loadFileGroups()";
	std::cerr << std::endl;
#endif

	/* create the serialiser to load info */
	BinInterface *bio = new BinFileInterface(filename.c_str(), BIN_FLAGS_READABLE);
	pqistore *store = createStore(bio, src, BIN_FLAGS_READABLE);

#ifdef DISTRIB_DEBUG
	std::cerr << "loading file " << filename << std::endl ;
#endif

	RsItem *item;
	RsDistribGrp *newGrp;
	RsDistribGrpKey *newKey;

	while(NULL != (item = store->GetItem()))
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

			loadGroup(newGrp, historical);
		}
		else if ((newKey = dynamic_cast<RsDistribGrpKey *>(item)))
		{
			loadGroupKey(newKey, historical);
		}
		else
		{
#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::loadFileGroups() Unexpected Item - deleting";
			std::cerr << std::endl;
#endif
			delete item;
		}
	}

	delete store;


	/* clear publication of groups if local cache file found */
	RsStackMutex stack(distribMtx); /******* STACK LOCKED MUTEX ***********/
	if (local)
	{
		mGroupsRepublish = false;
	}

	return;
}


void	p3GroupDistrib::loadFileMsgs(const std::string &filename, uint16_t cacheSubId, const std::string &src, uint32_t ts, bool local, bool historical)
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
	pqistore *store = createStore(bio, src, BIN_FLAGS_READABLE);

#ifdef DISTRIB_DEBUG
	std::cerr << "loading file " << filename << std::endl ;
#endif

	RsItem *item;
	RsDistribSignedMsg *newMsg;

	while(NULL != (item = store->GetItem()))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadFileMsgs() Got Item:";
		std::cerr << std::endl;
		item->print(std::cerr, 10);
		std::cerr << std::endl;
#endif

		if ((newMsg = dynamic_cast<RsDistribSignedMsg *>(item)))
		{
			loadMsg(newMsg, src, local, historical);
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

		if (((time_t) ts < now) && ((time_t) ts > mLastPublishTime))
		{
#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::loadFileMsgs() New LastPublishTime";
			std::cerr << std::endl;
#endif
			mLastPublishTime = ts;
		}
	}

	delete store;
	return;
}

/***************************************************************************************/
/***************************************************************************************/
	/********************** load Cache Msgs  ***************************************/
/***************************************************************************************/
/***************************************************************************************/


void	p3GroupDistrib::loadGroup(RsDistribGrp *newGrp, bool historical)
{
	/* load groupInfo */
	const std::string &gid = newGrp -> grpId;
	const std::string &pid = newGrp -> PeerId();

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

		if (isNew)
			locked_notifyGroupChanged(it->second, GRP_NEW_UPDATE, historical);
		else
			locked_notifyGroupChanged(it->second, GRP_UPDATE, historical);
	}

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::loadGroup() Done";
	std::cerr << std::endl;
#endif
}


bool	p3GroupDistrib::loadGroupKey(RsDistribGrpKey *newKey, bool historical)
{
	/* load Key */
	const std::string &gid = newKey -> grpId;

#ifdef DISTRIB_DEBUG
	const std::string &pid = newKey -> PeerId();
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

        //
	if (it == mGroups.end())
	{

#ifdef DISTRIB_DEBUG

                std::cerr << "p3GroupDistrib::loadGroupKey() Group for key not found: discarding";
                std::cerr << std::endl;
#endif

                    delete newKey;
                    newKey = NULL;
                    return false;

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
            locked_notifyGroupChanged(it->second, GRP_LOAD_KEY, historical);



#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::loadGroupKey() Done - Cleaning up.";
	std::cerr << std::endl;
#endif

//		  if(!updateOk)
			  delete newKey;

        newKey = NULL;
        return updateOk;
}


void	p3GroupDistrib::loadMsg(RsDistribSignedMsg *newMsg, const std::string &src, bool local, bool historical)
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


	/****************** check the msg ******************/
	/* check for duplicate message, do this first to ensure minimal signature validations. 
	 * therefore, duplicateMsg... could potentially be called on a dodgey msg (not a big problem!)
	 */

	std::map<std::string, RsDistribMsg *>::iterator mit;
	mit = (git->second).msgs.find(newMsg->msgId);
	if (mit != (git->second).msgs.end())
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadMsg() Msg already exists" << std::endl;
		std::cerr << std::endl;
#endif
		/* if already there -> remove */
		locked_eventDuplicateMsg(&(git->second), mit->second, src, historical);
		delete newMsg;
		return;
	}

	/* if unique (new) msg - do validation */
	if (!locked_validateDistribSignedMsg(git->second, newMsg))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadMsg() validate failed" << std::endl;
		std::cerr << std::endl;
#endif
		delete newMsg;
		return;
	}

        void *temp_ptr = newMsg->packet.bin_data;
        int temp_len = newMsg->packet.bin_len;

	if(git->second.grpFlags & RS_DISTRIB_ENCRYPTED){

                void *out_data = NULL;
                int out_len = 0;

                if(decrypt(out_data, out_len, newMsg->packet.bin_data, newMsg->packet.bin_len, newMsg->grpId)){
                        newMsg->packet.TlvShallowClear();
                        newMsg->packet.setBinData(out_data, out_len);
                        delete[] (unsigned char*) out_data;

		}else{

                        if((out_data != NULL) && (out_len != 0))
                                delete[] (unsigned char*) out_data;

			return;
		}
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

	// update the time stamp of group for last post
	if((git->second.lastPost < (time_t)msg->timestamp))
		git->second.lastPost = msg->timestamp;

	// Interface to handle Dummy Msgs.
	locked_CheckNewMsgDummies(git->second, msg, src, historical);
	
	/* now update parents TS */
	locked_updateChildTS(git->second, msg);

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::loadMsg() Msg Loaded Successfully" << std::endl;
	std::cerr << std::endl;
#endif


	/* Callback for any derived classes to play with */
	locked_eventNewMsg(&(git->second), msg, src, historical);

	/* else if group = subscribed | listener -> publish */
	/* if it has come from us... then it has been published already */
	if ((!local) && (git->second.flags & (RS_DISTRIB_SUBSCRIBED)))
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadMsg() To be Published!";
		std::cerr << std::endl;
#endif

                if(git->second.grpFlags & RS_DISTRIB_ENCRYPTED){
                    newMsg->packet.TlvClear();
                    newMsg->packet.setBinData(temp_ptr, temp_len);
                }

		locked_toPublishMsg(newMsg);
	}
	else
	{
		/* Note it makes it very difficult to republish msg - if we have
		 * deleted the signed version... The load of old messages will occur
		 * at next startup. And publication will happen then too.
		 */

#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::loadMsg() Deleted Original Msg (No Publish)";
		std::cerr << std::endl;
#endif
		delete newMsg;
	}
	locked_notifyGroupChanged(git->second, GRP_NEW_MSG, historical);
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

	bool ok = true; // hass msg/cache file been written successfully

	newCache.pid = mOwnId;
	newCache.cid.type = CacheSource::getCacheType();
	newCache.cid.subid = locked_determineCacheSubId(); 

	/* create filename */
	std::string path = CacheSource::getCacheDir();
	std::ostringstream out;
	out << "grpdist-t" << CacheSource::getCacheType() << "-msgs-" << time(NULL) << ".dist"; 

	std::string tmpname = out.str();
	std::string filename = path + "/" + tmpname ;
	std::string filenametmp = path + "/" + tmpname + ".tmp";

	BinInterface *bio = new BinFileInterface(filenametmp.c_str(), BIN_FLAGS_WRITEABLE | BIN_FLAGS_HASH_DATA);
	pqistore *store = createStore(bio, mOwnId, BIN_FLAGS_WRITEABLE); /* messages are deleted! */ 

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

		if(!store->SendItem(*it)) /* deletes it */
		{
			ok &= false;
		}
	}

	/* Extract File Information from pqistore */
	newCache.path = path;
	newCache.name = tmpname;

	newCache.hash = bio->gethash();
	newCache.size = bio->bytecount();
	newCache.recvd = now;

	/* cleanup */
	mPendingPublish.clear();
	delete store;

	if(!RsDirUtil::renameFile(filenametmp,filename))
	{
		std::ostringstream errlog;
		ok &= false;
#ifdef WIN32
		errlog << "Error " << GetLastError() ;
#else
		errlog << "Error " << errno ;
#endif
		getPqiNotify()->AddSysMessage(0, RS_SYS_WARNING, "File rename error", "Error while renaming file " + filename + ": got error "+errlog.str());
	}

	/* indicate not to save for a while */
	mLastPublishTime = now;

	/* push file to CacheSource */

	if(ok)
		refreshCache(newCache);


	if (ok && resave)
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
	std::string filenametmp = path + "/" + tmpname + ".tmp";

	BinInterface *bio = new BinFileInterface(filenametmp.c_str(), BIN_FLAGS_WRITEABLE | BIN_FLAGS_HASH_DATA);
	pqistore *store = createStore(bio, mOwnId, BIN_FLAGS_NO_DELETE | BIN_FLAGS_WRITEABLE);

	RsStackMutex stack(distribMtx); /****** STACK MUTEX LOCKED *******/


	/* Iterate through all the Groups */
	std::map<std::string, GroupInfo>::iterator it;
	for(it = mGroups.begin(); it != mGroups.end(); it++)
	{
		/* if subscribed or listener or admin -> then send it to be published by cache */
		if ((it->second.flags & RS_DISTRIB_SUBSCRIBED) || (it->second.flags & RS_DISTRIB_ADMIN))
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
				store->SendItem(grp); /* no delete */

				grp->grpFlags &= (~RS_DISTRIB_UPDATE); // if this is an update, ensure flag is removed after publication
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

					store->SendItem(pubKey);
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

	/* Extract File Information from pqistore */
	newCache.path = path;
	newCache.name = tmpname;

	newCache.hash = bio->gethash();
	newCache.size = bio->bytecount();
	newCache.recvd = time(NULL);

	/* cleanup */
	delete store;

	if(!RsDirUtil::renameFile(filenametmp,filename))
	{
		std::ostringstream errlog;
#ifdef WIN32
		errlog << "Error " << GetLastError() ;
#else
		errlog << "Error " << errno ;
#endif
		getPqiNotify()->AddSysMessage(0, RS_SYS_WARNING, "File rename error", "Error while renaming file " + filename + ": got error "+errlog.str());
	}

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

void p3GroupDistrib::getPopularGroupList(uint32_t popMin, uint32_t popMax, std::list<std::string> &grpids)
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
	return;
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

bool    p3GroupDistrib::subscribeToGroup(const std::string &grpId, bool subscribe)
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

			locked_notifyGroupChanged(git->second, GRP_SUBSCRIBED, false);
			mGroupsRepublish = true;

			/* reprocess groups messages .... so actions can be taken (by inherited) 
			 * This could be an very expensive operation! .... but they asked for it.
			 * 
			 * Hopefully a LoadList call will have on existing messages!
			 */

			std::map<std::string, RsDistribMsg *>::iterator mit;
			std::list<std::string>::iterator pit;

			/* assume that each peer can provide all of them */
			for(mit = git->second.msgs.begin();
				mit != git->second.msgs.end(); mit++)
			{
				for(pit = git->second.sources.begin();
					pit != git->second.sources.end(); pit++)
                                {
                                        if(*pit != mOwnId)
                                            locked_eventDuplicateMsg(&(git->second), mit->second, *pit, false);
				}
			}
		}
	}
	else 
	{
		if (git->second.flags & RS_DISTRIB_SUBSCRIBED)
		{
			git->second.flags &= (~RS_DISTRIB_SUBSCRIBED);

			locked_notifyGroupChanged(git->second, GRP_UNSUBSCRIBED, false);
			mGroupsRepublish = true;
		}
	}

	return true;
}


bool p3GroupDistrib::attemptPublishKeysRecvd()
{

#ifdef DISTRIB_DEBUG
    std::cerr << "p3GroupDistrib::attemptPublishKeysRecvd() " << std::endl;
#endif

    RsStackMutex stack(distribMtx);

    std::list<std::string> toDelete;
    std::list<std::string>::iterator sit;
    std::map<std::string, GroupInfo>::iterator it;

    std::map<std::string, RsDistribGrpKey*>::iterator mit;
    mit = mRecvdPubKeys.begin();

    // add received keys for groups that are present
    for(; mit != mRecvdPubKeys.end(); mit++){

        it = mGroups.find(mit->first);

        // group has not arrived yet don't attempt to add
        if(it == mGroups.end()){

            continue;

        }else{



            if(locked_updateGroupPublishKey(it->second, mit->second)){

                locked_notifyGroupChanged(it->second, GRP_LOAD_KEY, false);
            }

            if(it->second.flags & RS_DISTRIB_SUBSCRIBED){
                // remove key shared flag so key is not loaded back into mrecvdpubkeys
                mit->second->key.keyFlags &= (~RSTLV_KEY_TYPE_SHARED);

                delete (mit->second);
                toDelete.push_back(mit->first);
            }

        }
    }

    sit = toDelete.begin();

    for(; sit != toDelete.end(); sit++)
        mRecvdPubKeys.erase(*sit);




	return true;
}

/************************************* p3Config *************************************/

RsSerialiser *p3GroupDistrib::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsDistribSerialiser());

	return rss;
}

bool p3GroupDistrib::saveList(bool &cleanup, std::list<RsItem *>& saveData)
{

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

	// also save pending private publish keys you may have
	std::map<std::string, RsDistribGrpKey* >::iterator pendKeyIt;

	for(pendKeyIt = mRecvdPubKeys.begin(); pendKeyIt != mRecvdPubKeys.end(); pendKeyIt++)
	{

		RsDistribGrpKey *pubKey = new RsDistribGrpKey();
		pubKey->grpId = pendKeyIt->first;
		const unsigned char *keyptr = (const unsigned char *)
				pendKeyIt->second->key.keyData.bin_data;
		long keylen = pendKeyIt->second->key.keyData.bin_len;

		RSA *rsa_priv = d2i_RSAPrivateKey(NULL, &(keyptr), keylen);

		setRSAPrivateKey(pubKey->key, rsa_priv);
		RSA_free(rsa_priv);

		pubKey->key.keyFlags = pendKeyIt->second->key.keyFlags;
		pubKey->key.startTS = pendKeyIt->second->key.startTS;
		pubKey->key.endTS   = pendKeyIt->second->key.endTS;

		saveData.push_back(pubKey);
		saveCleanupList.push_back(pubKey);

	}

	std::list<RsItem *> childSaveL = childSaveList();
	std::list<RsItem *>::iterator cit = childSaveL.begin();
	RsSerialType *childSer = createSerialiser();
	uint32_t pktSize = 0;
	unsigned char *data = NULL;

	for(; cit != childSaveL.end() ; cit++)
	{
		RsDistribConfigData* childConfig = new RsDistribConfigData();

		pktSize = childSer->size(*cit);
		data = new unsigned char[pktSize];
		childSer->serialise(*cit, data, &pktSize);
		childConfig->service_data.setBinData(data, pktSize);
		delete[] data;
		saveData.push_back(childConfig);
		saveCleanupList.push_back(childConfig);
	}

	delete childSer;

	return true;
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

bool    p3GroupDistrib::loadList(std::list<RsItem *>& load)
{
	std::list<RsItem *>::iterator lit;

	/* for child config data */
	std::list<RsItem* > childLoadL;
	RsSerialType* childSer = createSerialiser();

	for(lit = load.begin(); lit != load.end(); lit++)
	{
		/* decide what type it is */

		RsDistribGrp *newGrp = NULL;
		RsDistribGrpKey *newKey = NULL;
		RsDistribSignedMsg *newMsg = NULL;
		RsDistribConfigData* newChildConfig = NULL;

		if ((newGrp = dynamic_cast<RsDistribGrp *>(*lit)))
		{
			const std::string &gid = newGrp -> grpId;
			loadGroup(newGrp, false);

			subscribeToGroup(gid, true);
		}
		else if ((newKey = dynamic_cast<RsDistribGrpKey *>(*lit)))
		{

                    // for shared keys keep
                    if(newKey->key.keyFlags & RSTLV_KEY_TYPE_SHARED){

                        mRecvdPubKeys.insert(std::pair<std::string, RsDistribGrpKey*>(
                                newKey->grpId, newKey));
                        mPubKeyAvailableGrpId.insert(newKey->grpId);
                        continue;
                    }

                    loadGroupKey(newKey, false);


		}
		else if ((newMsg = dynamic_cast<RsDistribSignedMsg *>(*lit)))
		{
			newMsg->PeerId(mOwnId);
			loadMsg(newMsg, mOwnId, false, false); /* false so it'll pushed to PendingPublish list */
		}
		else if ((newChildConfig = dynamic_cast<RsDistribConfigData *>(*lit)))
		{
			RsItem* childConfigItem = childSer->deserialise(newChildConfig->service_data.bin_data,
					&newChildConfig->service_data.bin_len);

			childLoadL.push_back(childConfigItem);

		}
	}

	/* no need to republish until something new comes in */
	RsStackMutex stack(distribMtx); /******* STACK LOCKED MUTEX ***********/

	childLoadList(childLoadL); // send configurations down to child

	mGroupsRepublish = false;
	delete childSer;
	return true;
}

/************************************* p3Config *************************************/

/* This Streamer is used for Reading and Writing Cache Files....
 * As All the child packets are Packed, we should only need RsSerialDistrib() in it.
 */

pqistore *p3GroupDistrib::createStore(BinInterface *bio, const std::string &src, uint32_t bioflags)
{
	RsSerialiser *rsSerialiser = new RsSerialiser();
	RsSerialType *serialType = new RsDistribSerialiser();
	rsSerialiser->addSerialType(serialType);

	pqistore *store = new pqistore(rsSerialiser, src, bio, bioflags);

	return store;
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


std::string p3GroupDistrib::createGroup(std::wstring name, std::wstring desc, uint32_t flags,
		unsigned char* pngImageData, uint32_t imageSize)
{
#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::createGroup()" << std::endl;
	std::cerr << std::endl;
#endif
	/* Create a Group */
	std::string grpId;
	time_t now = time(NULL);

	/* for backup */
	std::list<RsDistribGrpKey* > grpKeySet;

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

	// explicit member wise copy for grp image
	if((pngImageData != NULL) && (imageSize > 0)){
		newGrp->grpPixmap.binData.bin_data = new unsigned char[imageSize];

		memcpy(newGrp->grpPixmap.binData.bin_data, pngImageData,
				imageSize*sizeof(unsigned char));
		newGrp->grpPixmap.binData.bin_len = imageSize;
		newGrp->grpPixmap.image_type = RSTLV_IMAGE_TYPE_PNG;

	}else{
		newGrp->grpPixmap.binData.bin_data = NULL;
		newGrp->grpPixmap.binData.bin_len = 0;
		newGrp->grpPixmap.image_type = 0;
	}




	/* set keys */
	setRSAPublicKey(newGrp->adminKey, rsa_admin_pub);
	newGrp->adminKey.keyFlags = RSTLV_KEY_TYPE_PUBLIC_ONLY | RSTLV_KEY_DISTRIB_ADMIN;
	newGrp->adminKey.startTS = now;
	newGrp->adminKey.endTS = 0; /* no end */
	grpId = newGrp->adminKey.keyId;


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

	newGrp->grpId = grpId;

	/************* Back up Keys  *********************/

	grpKeySet.push_back(adKey);
	grpKeySet.push_back(pubKey);

	backUpKeys(grpKeySet, grpId);

	/************** Serialise and sign **************************************/
	EVP_PKEY *key_admin = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(key_admin, rsa_admin);

	newGrp->adminSignature.TlvClear();

	RsSerialType *serialType = new RsDistribSerialiser();

	uint32_t size = serialType->size(newGrp);
	char* data = new char[size];

	serialType->serialise(newGrp, data, &size);

	/* calc and check signature */
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

	EVP_SignInit(mdctx, EVP_sha1());
	EVP_SignUpdate(mdctx, data, size);

	unsigned int siglen = EVP_PKEY_size(key_admin);
        unsigned char sigbuf[siglen];
	EVP_SignFinal(mdctx, sigbuf, &siglen, key_admin);

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
	loadGroup(newGrp, false);

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
	delete[] data;
	return grpId;
}


bool p3GroupDistrib::backUpKeys(const std::list<RsDistribGrpKey* >& keysToBackUp, std::string grpId){

#ifdef DISTRIB_DEBUG
	std::cerr << "P3Distrib::backUpKeys() Backing up keys for grpId: " << grpId << std::endl;
#endif

	std::string filename =  mKeyBackUpDir + "/" + grpId + "_" + BACKUP_KEY_FILE;
	std::string filenametmp = filename  + ".tmp";

	BinInterface *bio = new BinFileInterface(filenametmp.c_str(), BIN_FLAGS_WRITEABLE);
	pqistore *store = createStore(bio, mOwnId, BIN_FLAGS_NO_DELETE | BIN_FLAGS_WRITEABLE);

	std::list<RsDistribGrpKey* >::const_iterator it;
	bool ok = true;

	for(it=keysToBackUp.begin(); it != keysToBackUp.end(); it++){

		ok &= store->SendItem(*it);

	}

    delete store;

	if(!RsDirUtil::renameFile(filenametmp,filename))
	{
		std::ostringstream errlog;
#ifdef WIN32
		errlog << "Error " << GetLastError() ;
#else
		errlog << "Error " << errno ;
#endif
		getPqiNotify()->AddSysMessage(0, RS_SYS_WARNING, "File rename error", "Error while renaming file " + filename + ": got error "+errlog.str());
		return false;
	}

	return ok;
}

bool p3GroupDistrib::restoreGrpKeys(const std::string& grpId){


#ifdef DISTRIB_DEBUG
			std::cerr << "p3Distrib::restoreGrpKeys() Attempting to restore private keys for grp: "
					  << grpId << std::endl;
#endif

	// build key directory name
	std::string filename = mKeyBackUpDir + "/"+ grpId + "_" + BACKUP_KEY_FILE;


	/* create the serialiser to load keys */
	BinInterface *bio = new BinFileInterface(filename.c_str(), BIN_FLAGS_READABLE);
	pqistore *store = createStore(bio, mOwnId, BIN_FLAGS_READABLE);

	RsItem* item;
	bool ok = true;
	bool itemAttempted = false;
	RsDistribGrpKey* key = NULL;

	RsStackMutex stack(distribMtx);

	GroupInfo* gi = locked_getGroupInfo(grpId);

	//retrieve keys from file and load to appropriate grp
	while(NULL != (item = store->GetItem())){

		itemAttempted = true;
		key = dynamic_cast<RsDistribGrpKey* >(item);

		if(key == NULL){
#ifdef DISTRIB_DEBUG
			std::cerr << "p3groupDistrib::restoreGrpKey() Key file / grp key item not Valid, grp: "
					  "\ngrpId: " <<  grpId << std::endl;
#endif
			delete store ;
			return false;
		}

		if(key->key.keyFlags & RSTLV_KEY_DISTRIB_ADMIN)
			ok &= locked_updateGroupAdminKey(*gi, key);
		else if((key->key.keyFlags & RSTLV_KEY_DISTRIB_PRIVATE) ||
				(key->key.keyFlags & RSTLV_KEY_DISTRIB_PUBLIC))
			ok &= locked_updateGroupPublishKey(*gi, key);
		else
			ok &= false;

	}



	ok &= itemAttempted;

	if(ok){
		gi->flags |= RS_DISTRIB_SUBSCRIBED;
		locked_notifyGroupChanged(*gi, GRP_SUBSCRIBED, false);
		IndicateConfigChanged();
		mGroupsRepublish = true;
	}

#ifdef DISTRIB_DEBUG
	if(!ok){
		std::cerr << "p3Distrib::restoreGrpKeys() Failed to restore private keys for grp "
				  << "\ngrpId: " << grpId << std::endl;
	}
#endif

	delete store;

	return ok;
}


bool p3GroupDistrib::sharePubKey(std::string grpId, std::list<std::string>& peers){

	RsStackMutex stack(distribMtx);

	// first check that group actually exists
	if(mGroups.find(grpId) == mGroups.end()){
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::sharePubKey(): Group does not exist" << std::endl;
#endif
		return false;
	}

	// add to pending list to be sent
	mPendingPubKeyRecipients[grpId] = peers;

	return true;
}

void p3GroupDistrib::locked_sharePubKey(){


#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_sharePubKey() " << std::endl;
#endif

	std::map<std::string, std::list<std::string> >::iterator mit;
	std::list<std::string>::iterator lit;

	// get list of peers that are online
	std::list<std::string> peersOnline;
	rsPeers->getOnlineList(peersOnline);
	std::list<std::string> toDelete;

	/* send public key to peers online */

	for(mit = mPendingPubKeyRecipients.begin();  mit != mPendingPubKeyRecipients.end(); mit++){

		GroupInfo *gi = locked_getGroupInfo(mit->first);

		if(gi == NULL){
			toDelete.push_back(mit->first); // grp does not exist, stop attempting to share key for dead group
			continue;
		}

		// find full public key, and send to given peers
		std::map<std::string, GroupKey>::iterator kit;
		for(kit = gi->publishKeys.begin();
					kit != gi->publishKeys.end(); kit++)
		{
			if (kit->second.type & RSTLV_KEY_TYPE_FULL)
			{
#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::locked_sharePubKey() Sharing Key: " << kit->first;
	std::cerr << std::endl;
#endif

				// send keys to peers who are online
				for(lit = mit->second.begin() ; lit != mit->second.end(); lit++){

					if(std::find(peersOnline.begin(), peersOnline.end(), *lit) != peersOnline.end()){

						/* create Key for sharing */
						RsDistribGrpKey* pubKey = new RsDistribGrpKey(getRsItemService(getType()));


						pubKey->clear();
						pubKey->grpId = mit->first;

						RSA *rsa_priv = EVP_PKEY_get1_RSA(kit->second.key);
						setRSAPrivateKey(pubKey->key, rsa_priv);
						RSA_free(rsa_priv);

						pubKey->key.keyFlags = kit->second.type;
						pubKey->key.startTS = kit->second.startTS;
						pubKey->key.endTS   = kit->second.endTS;
						pubKey->PeerId(*lit);
						std::cout << *lit << std::endl;
						sendItem(pubKey);

						// remove peer from list
						lit = mit->second.erase(lit); // no need to send to peer anymore
						lit--;
					}
				}
			}
		}

		// if given peers have all received key(s) then stop sending for group
		if(mit->second.empty())
			toDelete.push_back(mit->first);
	}

	// delete pending peer list which are done with
	for(lit = toDelete.begin(); lit != toDelete.end(); lit++)
		mPendingPubKeyRecipients.erase(*lit);

	mLastKeyPublishTime = time(NULL);

	return;
}


void p3GroupDistrib::receivePubKeys(){


	RsItem* item;
        std::string gid;

        std::map<std::string, GroupInfo>::iterator it;
        std::list<std::string> toDelete;
        std::list<std::string>::iterator sit;


        // load received keys
	while(NULL != (item = recvItem())){

		RsDistribGrpKey* key_item = dynamic_cast<RsDistribGrpKey*>(item);

		if(key_item != NULL){

                    it = mGroups.find(key_item->grpId);

                    // if group does not exist keep to see if it arrives later
                    if(it == mGroups.end()){

                        // make sure key is in date
                        if(((time_t)(key_item->key.startTS + mStorePeriod) > time(NULL)) &&
                           (key_item->key.keyFlags & RSTLV_KEY_TYPE_FULL)){

                            // make sure keys does not exist in recieved list, then delete
                            if(mRecvdPubKeys.find(gid) == mRecvdPubKeys.end()){

                                // id key as shared so on loadlist sends back mRecvdPubKeys
                                key_item->key.keyFlags |= RSTLV_KEY_TYPE_SHARED;
                                mRecvdPubKeys.insert(std::pair<std::string,
                                                     RsDistribGrpKey*>(key_item->grpId, key_item));

                                mPubKeyAvailableGrpId.insert(key_item->grpId);
                            }

                        }else{
                            delete key_item;
                        }

                        continue;
                    }

#ifdef DISTRIB_DEBUG
			std::cerr << "p3GroupDistrib::locked_receiveKeys()" << std::endl;
			std::cerr << "PeerId : " << key_item->PeerId() << std::endl;
			std::cerr << "GrpId: " << key_item->grpId << std::endl;
			std::cerr << "Got key Item" << std::endl;
#endif
                    if(key_item->key.keyFlags & RSTLV_KEY_TYPE_FULL){

                        gid = key_item->grpId;

                        // add key if user is subscribed if not store it until user subscribes

                            if(locked_updateGroupPublishKey(it->second, key_item)){

                                mPubKeyAvailableGrpId.insert(key_item->grpId);
                                locked_notifyGroupChanged(it->second, GRP_LOAD_KEY, false);

                                // keep key if user not subscribed
                                if(it->second.flags & RS_DISTRIB_SUBSCRIBED){

                                    delete key_item;

                                }else{

                                    // make sure keys does not exist in recieved list
                                    if(mRecvdPubKeys.find(gid) == mRecvdPubKeys.end()){

                                        // id key as shared so on loadlist sends back to mRecvdPubKeys
                                        key_item->key.keyFlags |= RSTLV_KEY_TYPE_SHARED;
                                        mRecvdPubKeys.insert(std::pair<std::string,
                                                             RsDistribGrpKey*>(key_item->grpId, key_item));

                                    }
                                }

                            }
                        }
                    else{
				std::cerr << "p3GroupDistrib::locked_receiveKeys():" << "Not full public key"
                                          << "Deleting item"<< std::endl;

                                delete key_item;
                         }
                    }
                else{
                            delete item;
                        }
                   }




        RsStackMutex stack(distribMtx);

        // indicate config changed and also record the groups keys received for
        if(!mRecvdPubKeys.empty())
            IndicateConfigChanged();

	return;


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
		char *data = new char[size];
		serialType->serialise(msg, data, &size);

		char *out_data = NULL;
		uint32_t out_size = 0;

		// encrypt data if group is private

		if(gi->grpFlags & RS_DISTRIB_ENCRYPTED){

			if(encrypt((void*&)out_data, (int&)out_size, (void*&)data, (int)size, grpId)){

				delete[] data;
			}else{
				delete[] data;
				delete signedMsg;
				delete serialType;
				return msgId;
			}

		}else
		{
			out_data = data;
			out_size = size;
		}

		signedMsg->packet.setBinData(out_data, out_size);

		/* sign Packet */

		/* calc and check signature */
		EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

		EVP_SignInit(mdctx, EVP_sha1());
                EVP_SignUpdate(mdctx, out_data, out_size);

		unsigned int siglen = EVP_PKEY_size(publishKey);
			unsigned char sigbuf[siglen];
		EVP_SignFinal(mdctx, sigbuf, &siglen, publishKey);

		/* save signature */
		signedMsg->publishSignature.signData.setBinData(sigbuf, siglen);
		signedMsg->publishSignature.keyId = gi->publishKeyId;

		bool ok = true;

		if (personalSign)
		{
			unsigned int siglen = MAX_GPG_SIGNATURE_SIZE;
			unsigned char sigbuf[siglen];
                        if (AuthGPG::getAuthGPG()->SignDataBin(out_data, out_size, sigbuf, &siglen))
			{
				signedMsg->personalSignature.signData.setBinData(sigbuf, siglen);
				signedMsg->personalSignature.keyId = AuthGPG::getAuthGPG()->getGPGOwnId();
			} else {
				ok = false;
			}
		}

		/* clean up */
		delete serialType;
		EVP_MD_CTX_destroy(mdctx);
		delete[] out_data;

		if (ok == false) {
			delete signedMsg;
			return msgId;
		}

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
	loadMsg(signedMsg, mOwnId, false, false);

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



	/* copy out signature (shallow copy) */
	RsTlvKeySignature tmpSign = newGrp->adminSignature;
	unsigned char *sigbuf = (unsigned char *) tmpSign.signData.bin_data;
	unsigned int siglen = tmpSign.signData.bin_len;

	/* clear signature */
	newGrp->adminSignature.ShallowClear();

	uint32_t size = serialType->size(newGrp);
	char* data = new char[size];

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
	delete[] data;

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
	 * timestamp is <= timestamp, or not an update (info edit)
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
		((info.distribGroup->timestamp <= newGrp->timestamp) && !(newGrp->grpFlags & RS_DISTRIB_UPDATE)))
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

	if (info.grpIcon.pngImageData != NULL){
		delete[] info.grpIcon.pngImageData;
		info.grpIcon.imageSize = 0;
	}

	info.distribGroup = newGrp;

	/* copy details  */
	info.grpName = newGrp->grpName;
	info.grpDesc = newGrp->grpDesc;
	info.grpCategory = newGrp->grpCategory;
	info.grpFlags   = newGrp->grpFlags; 

	if((newGrp->grpPixmap.binData.bin_data != NULL) && (newGrp->grpPixmap.binData.bin_len > 0)){
		info.grpIcon.pngImageData = new unsigned char[newGrp->grpPixmap.binData.bin_len];

		memcpy(info.grpIcon.pngImageData, newGrp->grpPixmap.binData.bin_data,
				newGrp->grpPixmap.binData.bin_len*sizeof(unsigned char));

		info.grpIcon.imageSize = newGrp->grpPixmap.binData.bin_len;
	}else{
		info.grpIcon.pngImageData = NULL;
		info.grpIcon.imageSize = 0;
	}

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


bool p3GroupDistrib::locked_editGroup(std::string grpId, GroupInfo& gi){

#ifdef DISTRIB_DEBUG
    std::cerr << "p3GroupDistrib::locked_editGroup() " << grpId << std::endl;
#endif

    GroupInfo* gi_curr = locked_getGroupInfo(grpId);

    if(gi_curr == NULL){

    std::cerr << "p3GroupDistrib::locked_editGroup() Failed, group does not exist " << grpId
              << std::endl;
        return false;
    }

    if(!(gi_curr->flags & RS_DISTRIB_ADMIN))
        return false;


    gi_curr->grpName = gi.grpName;
    gi_curr->distribGroup->grpName = gi_curr->grpName;
    gi_curr->grpDesc = gi.grpDesc;
    gi_curr->distribGroup->grpDesc = gi_curr->grpDesc;

    if((gi.grpIcon.imageSize != 0) && gi.grpIcon.pngImageData != NULL){

        if((gi_curr->distribGroup->grpPixmap.binData.bin_data != NULL) &&
           (gi_curr->distribGroup->grpPixmap.binData.bin_len != 0))
            gi_curr->distribGroup->grpPixmap.binData.TlvClear();

        gi_curr->distribGroup->grpPixmap.binData.bin_data = gi_curr->grpIcon.pngImageData;
        gi_curr->distribGroup->grpPixmap.binData.bin_len = gi_curr->grpIcon.imageSize;

        gi_curr->grpIcon.imageSize = gi.grpIcon.imageSize;
        gi_curr->grpIcon.pngImageData = gi.grpIcon.pngImageData;


    }

    // set new timestamp for grp
    gi_curr->distribGroup->timestamp = time(NULL);

    // create new signature for group

    EVP_PKEY *key_admin = gi_curr->adminKey.key;
    gi_curr->distribGroup->adminSignature.TlvClear();
    RsSerialType *serialType = new RsDistribSerialiser();
    uint32_t size = serialType->size(gi_curr->distribGroup);
    char* data = new char[size];
    serialType->serialise(gi_curr->distribGroup, data, &size);

    /* calc and check signature */
    EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
    EVP_SignInit(mdctx, EVP_sha1());
    EVP_SignUpdate(mdctx, data, size);

    unsigned int siglen = EVP_PKEY_size(key_admin);
    unsigned char sigbuf[siglen];
    EVP_SignFinal(mdctx, sigbuf, &siglen, key_admin);

    /* save signature */
    gi_curr->distribGroup->adminSignature.signData.setBinData(sigbuf, siglen);
    gi_curr->distribGroup->adminSignature.keyId = grpId;

    mGroupsChanged = true;
    gi_curr->grpChanged = true;
    mGroupsRepublish = true;

    // this is removed afterwards
    gi_curr->distribGroup->grpFlags |= RS_DISTRIB_UPDATE;

    delete[] data;

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
	const std::string &keyId = newKey->key.keyId;

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
	const std::string &keyId = newKey->key.keyId;

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
	std::cerr << "p3GroupDistrib::locked_validateDistribSignedMsg() publish KeyId: " << newMsg->publishSignature.keyId << std::endl;
	std::cerr << "p3GroupDistrib::locked_validateDistribSignedMsg() personal KeyId: " << newMsg->personalSignature.keyId << std::endl;
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
        if ((signOk == 1) && ((info.grpFlags & RS_DISTRIB_AUTHEN_MASK) & RS_DISTRIB_AUTHEN_REQ))
	{
                unsigned int personalsiglen =
                                newMsg->personalSignature.signData.bin_len;
                unsigned char *personalsigbuf = (unsigned char *)
                                newMsg->personalSignature.signData.bin_data;

		RsPeerDetails signerDetails;
		std::string gpg_fpr;
		if (AuthGPG::getAuthGPG()->getGPGDetails(newMsg->personalSignature.keyId, signerDetails))
		{
			gpg_fpr = signerDetails.fpr;
		}

                bool gpgSign = AuthGPG::getAuthGPG()->VerifySignBin(
                        newMsg->packet.bin_data, newMsg->packet.bin_len,
                        personalsigbuf, personalsiglen, gpg_fpr);
                if (gpgSign) {
                    #ifdef DISTRIB_DEBUG
                    std::cerr << "p3GroupDistrib::locked_validateDistribSignedMsg() Success for gpg signature." << std::endl;
                    #endif
                    signOk = 1;
                } else {
                    #ifdef DISTRIB_DEBUG
                    std::cerr << "p3GroupDistrib::locked_validateDistribSignedMsg() Fail for gpg signature." << std::endl;
                    #endif
                    signOk = 0;
                }
	}

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

void p3GroupDistrib::getGrpListPubKeyAvailable(std::list<std::string>& grpList)
{
	RsStackMutex stack(distribMtx);
        std::set<std::string>::const_iterator cit = mPubKeyAvailableGrpId.begin();

        for(; cit != mPubKeyAvailableGrpId.end(); cit++)
            grpList.push_back(*cit);

	return;
}

bool	p3GroupDistrib::locked_checkDistribMsg(
				GroupInfo &gi, RsDistribMsg *msg)
{

	/* check timestamp */
	time_t now = time(NULL);
	uint32_t min = now - mStorePeriod;
	uint32_t max = now + GROUP_MAX_FWD_OFFSET;

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


        /* now update parents TS */
bool	p3GroupDistrib::locked_updateChildTS(GroupInfo &gi, RsDistribMsg *msg)
{
	/* find all parents - update timestamp */
	time_t updateTS = msg->timestamp;
	msg->childTS = updateTS;

	while("" != msg->parentId)
	{
		std::string parentId = msg->parentId;

        	std::map<std::string, RsDistribMsg *>::iterator mit;
        	if (gi.msgs.end() == (mit = gi.msgs.find(parentId)))
		{
			/* not found - abandon (check for dummyMsgs first) */
			return locked_updateDummyChildTS(gi, parentId, updateTS);
			
		}
		RsDistribMsg *parent = mit->second;
		if ((!parent) || (parent->childTS > updateTS))
		{
			/* we're too old - give up! */
			return true;
		}

		/* update timestamp */
		parent->childTS = updateTS;
		msg = parent;
	}
	return false ;
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

bool p3GroupDistrib::encrypt(void *& out, int& outlen, const void *in, int inlen, std::string grpId)
{


#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::decrypt() " << std::endl;
#endif

	GroupInfo* gi = locked_getGroupInfo(grpId);
	RSA *rsa_publish_pub = NULL;
	EVP_PKEY *public_key = NULL, *private_key = NULL;

	if(gi == NULL){
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::decrypt(): Cannot find group, grpId " << grpId
				  << std::endl;
#endif
		return false;

	}

	/*  generate public key */

	std::map<std::string, GroupKey>::iterator kit;

	for(kit = gi->publishKeys.begin(); kit != gi->publishKeys.end(); kit++ ){

		// Does not allow for possibility of different keys

		if((kit->second.type & RSTLV_KEY_TYPE_FULL) && (kit->second.key->type == EVP_PKEY_RSA)){
			private_key = kit->second.key;
			break;
		}

	}

	if(kit == gi->publishKeys.end()){
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::encrypt(): Cannot find full key, grpId " << grpId
				  << std::endl;
#endif
		return false;
	}


	RSA* rsa_publish = EVP_PKEY_get1_RSA(private_key);
	rsa_publish_pub = RSAPublicKey_dup(rsa_publish);


	if(rsa_publish_pub  != NULL){
		public_key = EVP_PKEY_new();
		EVP_PKEY_assign_RSA(public_key, rsa_publish_pub);
	}else{
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::encrypt(): Could not generate publish key " << grpId
				  << std::endl;
#endif
		return false;
	}

    EVP_CIPHER_CTX ctx;
    int eklen, net_ekl;
    unsigned char *ek;
    unsigned char iv[EVP_MAX_IV_LENGTH];
    EVP_CIPHER_CTX_init(&ctx);
    int out_currOffset = 0;
    int out_offset = 0;

    int max_evp_key_size = EVP_PKEY_size(public_key);
    ek = (unsigned char*)malloc(max_evp_key_size);
    const EVP_CIPHER *cipher = EVP_aes_128_cbc();
    int cipher_block_size = EVP_CIPHER_block_size(cipher);
    int size_net_ekl = sizeof(net_ekl);

    int max_outlen = inlen + cipher_block_size + EVP_MAX_IV_LENGTH + max_evp_key_size + size_net_ekl;

    // intialize context and send store encrypted cipher in ek
	if(!EVP_SealInit(&ctx, EVP_aes_128_cbc(), &ek, &eklen, iv, &public_key, 1)) return false;

	// now assign memory to out accounting for data, and cipher block size, key length, and key length val
    out = new unsigned char[inlen + cipher_block_size + size_net_ekl + eklen + EVP_MAX_IV_LENGTH];

	net_ekl = htonl(eklen);
	memcpy((unsigned char*)out + out_offset, &net_ekl, size_net_ekl);
	out_offset += size_net_ekl;

	memcpy((unsigned char*)out + out_offset, ek, eklen);
	out_offset += eklen;

	memcpy((unsigned char*)out + out_offset, iv, EVP_MAX_IV_LENGTH);
	out_offset += EVP_MAX_IV_LENGTH;

	// now encrypt actual data
	if(!EVP_SealUpdate(&ctx, (unsigned char*) out + out_offset, &out_currOffset, (unsigned char*) in, inlen)) return false;

	// move along to partial block space
	out_offset += out_currOffset;

	// add padding
	if(!EVP_SealFinal(&ctx, (unsigned char*) out + out_offset, &out_currOffset)) return false;

	// move to end
	out_offset += out_currOffset;

	// make sure offset has not gone passed valid memory bounds
	if(out_offset > max_outlen) return false;

	// free encrypted key data
	free(ek);

	outlen = out_offset;
	return true;

    delete[] ek;

#ifdef DISTRIB_DEBUG
    std::cerr << "p3GroupDistrib::encrypt() finished with outlen : " << outlen << std::endl;
#endif

    return true;
}

bool p3GroupDistrib::decrypt(void *& out, int& outlen, const void *in, int inlen, std::string grpId)
{

#ifdef DISTRIB_DEBUG
	std::cerr << "p3GroupDistrib::decrypt() " << std::endl;
#endif

	GroupInfo* gi = locked_getGroupInfo(grpId);
	EVP_PKEY *private_key;

	if(gi == NULL){
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::decrypt(): Cannot find group, grpId " << grpId
				  << std::endl;
#endif
		return false;
	}

	std::map<std::string, GroupKey>::iterator kit;

	for(kit = gi->publishKeys.begin(); kit != gi->publishKeys.end(); kit++ ){


		if((kit->second.type & RSTLV_KEY_TYPE_FULL) && (kit->second.key->type == EVP_PKEY_RSA)){
			private_key = kit->second.key;
			break;
		}
	}

	if(kit == gi->publishKeys.end()){
#ifdef DISTRIB_DEBUG
		std::cerr << "p3GroupDistrib::decrypt(): Cannot find full key, grpId " << grpId
				  << std::endl;
#endif
		return false;
	}

    EVP_CIPHER_CTX ctx;
    int eklen = 0, net_ekl = 0;
    unsigned char *ek = NULL;
    unsigned char iv[EVP_MAX_IV_LENGTH];
    ek = (unsigned char*)malloc(EVP_PKEY_size(private_key));
    EVP_CIPHER_CTX_init(&ctx);

    int in_offset = 0, out_currOffset = 0;
    int size_net_ekl = sizeof(net_ekl);

    memcpy(&net_ekl, (unsigned char*)in, size_net_ekl);
    eklen = ntohl(net_ekl);
    in_offset += size_net_ekl;

    memcpy(ek, (unsigned char*)in + in_offset, eklen);
    in_offset += eklen;

    memcpy(iv, (unsigned char*)in + in_offset, EVP_MAX_IV_LENGTH);
    in_offset += EVP_MAX_IV_LENGTH;

    const EVP_CIPHER* cipher = EVP_aes_128_cbc();

    if(!EVP_OpenInit(&ctx, cipher, ek, eklen, iv, private_key)) return false;

    out = new unsigned char[inlen - in_offset];

    if(!EVP_OpenUpdate(&ctx, (unsigned char*) out, &out_currOffset, (unsigned char*)in + in_offset, inlen - in_offset)) return false;

    in_offset += out_currOffset;
    outlen += out_currOffset;

    if(!EVP_OpenFinal(&ctx, (unsigned char*)out + out_currOffset, &out_currOffset)) return false;

    outlen += out_currOffset;

    free(ek);

	return true;
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

void 	p3GroupDistrib::locked_notifyGroupChanged(GroupInfo &info, uint32_t flags, bool historical)
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



/***************************************************************************************/
/***************************************************************************************/
	/******************* Handle Missing Messages ***********************************/
/***************************************************************************************/
/***************************************************************************************/

/* Find missing messages */



/* LOGIC:
 *
 * dummy(grpId, threadId, parentId, msgId);
 *
 * add new msg....
 *    - search for threadId.
 *      - if missing add thread head: dummy(grpId, threadId, NULL, threadId).
 *
 *    - search for parentId 
 *	- if = threadId, we just added it (ok).
 *	- if missing add dummy(grpId, threadId, threadId, parentId).
 *
 *    - check for matching dummy msgId.
 *	- if yes, delete.
 *
 */

RsDistribDummyMsg::RsDistribDummyMsg( std::string tId, std::string pId, std::string mId, uint32_t ts)
:threadId(tId), parentId(pId), msgId(mId), timestamp(ts), childTS(ts)
{
	return;
}

std::ostream &operator<<(std::ostream &out, const RsDistribDummyMsg &msg)
{
	out << "DummyMsg(" << msg.threadId << "," << msg.parentId << "," << msg.msgId << ")";
	return out;
}



bool p3GroupDistrib::locked_CheckNewMsgDummies(GroupInfo &grp, RsDistribMsg *msg, std::string id, bool historical)
{
	std::string threadId = msg->threadId;
	std::string parentId = msg->parentId;
	std::string msgId = msg->msgId;

#ifdef DISTRIB_DUMMYMSG_DEBUG
	std::cerr << "p3GroupDistrib::locked_CheckNewMsgDummies(grpId:" << grp.grpId << ", threadId: " << threadId;
	std::cerr << ", parentId:" << parentId << ", msgId: " << msgId << ")";
	std::cerr << std::endl;
#endif

#ifdef DISTRIB_DUMMYMSG_DEBUG
	std::cerr << "p3GroupDistrib::locked_CheckNewMsgDummies() Pre Printout";
	std::cerr << std::endl;
	locked_printDummyMsgs(grp);
#endif


	/* search for threadId */
	if (threadId != "")
	{
		std::map<std::string, RsDistribMsg *>::iterator tit = grp.msgs.find(threadId);
	
		if (tit == grp.msgs.end()) // not there!
		{ 
#ifdef DISTRIB_DUMMYMSG_DEBUG
			std::cerr << "p3GroupDistrib::locked_CheckNewMsgDummies() No ThreadId Msg, Adding DummyMsg";
			std::cerr << std::endl;
#endif
			locked_addDummyMsg(grp, threadId, "", threadId, msg->timestamp);
		}
		else
		{
#ifdef DISTRIB_DUMMYMSG_DEBUG
			std::cerr << "p3GroupDistrib::locked_CheckNewMsgDummies() Found ThreadId Msg";
			std::cerr << std::endl;
#endif
		}
	}

	if (parentId != "")
	{
		/* search for parentId */
		std::map<std::string, RsDistribMsg *>::iterator pit = grp.msgs.find(parentId);
		
		if (pit == grp.msgs.end()) // not there!
		{ 
#ifdef DISTRIB_DUMMYMSG_DEBUG
			std::cerr << "p3GroupDistrib::locked_CheckNewMsgDummies() No ParentId Msg, Adding DummyMsg";
			std::cerr << std::endl;
#endif
			locked_addDummyMsg(grp, threadId, threadId, parentId, msg->timestamp);
		}
		else
		{
#ifdef DISTRIB_DUMMYMSG_DEBUG
			std::cerr << "p3GroupDistrib::locked_CheckNewMsgDummies() Found ParentId Msg";
			std::cerr << std::endl;
#endif
		}
	}

#ifdef DISTRIB_DUMMYMSG_DEBUG
	std::cerr << "p3GroupDistrib::locked_CheckNewMsgDummies() Checking for DummyMsg";
	std::cerr << std::endl;
#endif

	/* remove existing dummy */
	locked_clearDummyMsg(grp, msgId);


#ifdef DISTRIB_DUMMYMSG_DEBUG
	std::cerr << "p3GroupDistrib::locked_CheckNewMsgDummies() Post Printout";
	std::cerr << std::endl;
	locked_printDummyMsgs(grp);
#endif

	return true;
}

bool p3GroupDistrib::locked_addDummyMsg(GroupInfo &grp, std::string threadId, std::string parentId, std::string msgId, uint32_t ts)
{
#ifdef DISTRIB_DUMMYMSG_DEBUG
	std::cerr << "p3GroupDistrib::locked_addDummyMsg(grpId:" << grp.grpId << ", threadId: " << threadId;
	std::cerr << ", parentId:" << parentId << ", msgId: " << msgId << ")";
	std::cerr << std::endl;
#endif

	if (msgId == "")
	{
#ifdef DISTRIB_DUMMYMSG_DEBUG
		std::cerr << "p3GroupDistrib::locked_addDummyMsg() ERROR not adding empty MsgId";
		std::cerr << std::endl;
#endif
		return false;
	}

	/* search for the msg Id */
	std::map<std::string, RsDistribDummyMsg>::iterator dit = grp.dummyMsgs.find(msgId);
	
	if (dit == grp.dummyMsgs.end()) // not there!
	{ 
		grp.dummyMsgs[msgId] = RsDistribDummyMsg(threadId, parentId, msgId, ts);


#ifdef DISTRIB_DUMMYMSG_DEBUG
		std::cerr << "p3GroupDistrib::locked_addDummyMsg() Adding Dummy Msg";
		std::cerr << std::endl;
#endif
	}
	else
	{
#ifdef DISTRIB_DUMMYMSG_DEBUG
		std::cerr << "p3GroupDistrib::locked_addDummyMsg() Dummy Msg already there: " << dit->second;
		std::cerr << std::endl;
#endif
	}

	locked_updateDummyChildTS(grp, parentId, ts); // NOTE both ChildTS functions should be merged.
	return true;
}

bool p3GroupDistrib::locked_clearDummyMsg(GroupInfo &grp, std::string msgId)
{
#ifdef DISTRIB_DUMMYMSG_DEBUG
	std::cerr << "p3GroupDistrib::locked_clearDummyMsg(grpId:" << grp.grpId << ", msgId: " << msgId << ")";
	std::cerr << std::endl;
#endif

	/* search for the msg Id */
	std::map<std::string, RsDistribDummyMsg>::iterator dit = grp.dummyMsgs.find(msgId);
	if (dit != grp.dummyMsgs.end())
	{ 

#ifdef DISTRIB_DUMMYMSG_DEBUG
		std::cerr << "p3GroupDistrib::locked_clearDummyMsg() Erasing Dummy Msg: " << dit->second;
		std::cerr << std::endl;
#endif

		grp.dummyMsgs.erase(dit);	
	}
	else
	{
#ifdef DISTRIB_DUMMYMSG_DEBUG
		std::cerr << "p3GroupDistrib::locked_clearDummyMsg() Msg not found";
		std::cerr << std::endl;
#endif
	}
	return true;
}



        /* now update parents TS */
/* NB: it is a hack to have seperate updateChildTS functions for msgs and dummyMsgs, 
 * this need to be combined (do when we add a parentId index.)
 */

bool	p3GroupDistrib::locked_updateDummyChildTS(GroupInfo &gi, std::string parentId, time_t updateTS)
{
	while("" != parentId)
	{
        	std::map<std::string, RsDistribDummyMsg>::iterator mit;
        	if (gi.dummyMsgs.end() == (mit = gi.dummyMsgs.find(parentId)))
		{
			/* not found - abandon */
			return true;
		}
		RsDistribDummyMsg *parent = &(mit->second);
		if (parent->childTS > updateTS)
		{
			/* we're too old - give up! */
			return true;
		}

		/* update timestamp */
		parent->childTS = updateTS;
		parentId = parent->parentId;
	}
	return false ;
}


bool p3GroupDistrib::locked_printAllDummyMsgs()
{
#ifdef DISTRIB_DUMMYMSG_DEBUG
	std::cerr << "p3GroupDistrib::locked_printAllDummyMsg()";
	std::cerr << std::endl;
#endif
	std::map<std::string, GroupInfo>::iterator it;
	for(it = mGroups.begin(); it != mGroups.end(); it++)
	{
		locked_printDummyMsgs(it->second);
	}
	return true ;
}



bool p3GroupDistrib::locked_printDummyMsgs(GroupInfo &grp)
{
#ifdef DISTRIB_DUMMYMSG_DEBUG
	std::cerr << "p3GroupDistrib::locked_printDummyMsg(grpId:" << grp.grpId << ")";
	std::cerr << std::endl;
#endif

	/* search for the msg Id */
	std::map<std::string, RsDistribDummyMsg>::iterator dit;
	for(dit = grp.dummyMsgs.begin(); dit != grp.dummyMsgs.end(); dit++)
	{ 
		std::cerr << dit->second;
		std::cerr << std::endl;
	}
	return true;
}


/***** These Functions are used by the children classes to access the dummyData
 ****/

bool p3GroupDistrib::getDummyParentMsgList(std::string grpId, std::string pId, std::list<std::string> &msgIds)
{
#ifdef DISTRIB_DUMMYMSG_DEBUG
	std::cerr << "p3GroupDistrib::getDummyParentMsgList(grpId:" << grpId << "," << pId << ")";
	std::cerr << std::endl;
#endif
	RsStackMutex stack(distribMtx); /*************  STACK MUTEX ************/
	std::map<std::string, GroupInfo>::iterator git;
	if (mGroups.end() == (git = mGroups.find(grpId)))
	{
#ifdef DISTRIB_DUMMYMSG_DEBUG
		std::cerr << "p3GroupDistrib::getDummyParentMsgList() Group Not Found";
		std::cerr << std::endl;
#endif
		return false;
	}

	std::map<std::string, RsDistribDummyMsg>::iterator mit;

	for(mit = git->second.dummyMsgs.begin(); mit != git->second.dummyMsgs.end(); mit++)
	{
		if (mit->second.parentId == pId)
		{
			msgIds.push_back(mit->first);
		}
	}

#ifdef DISTRIB_DUMMYMSG_DEBUG
	std::cerr << "p3GroupDistrib::getDummyParentMsgList() found " << msgIds.size() << " msgs";
	std::cerr << std::endl;
#endif
	return true;
}


RsDistribDummyMsg *p3GroupDistrib::locked_getGroupDummyMsg(std::string grpId, std::string msgId)
{
#ifdef DISTRIB_DUMMYMSG_DEBUG
	std::cerr << "p3GroupDistrib::locked_getGroupDummyMsg(grpId:" << grpId << "," << msgId << ")";
	std::cerr << std::endl;
#endif
	/************* ALREADY LOCKED ************/
	std::map<std::string, GroupInfo>::iterator git;
	if (mGroups.end() == (git = mGroups.find(grpId)))
	{
#ifdef DISTRIB_DUMMYMSG_DEBUG
		std::cerr << "p3GroupDistrib::locked_getGroupDummyMsg() Group not found";
		std::cerr << std::endl;
#endif
		return NULL;
	}

	std::map<std::string, RsDistribDummyMsg>::iterator dit;
	if (git->second.dummyMsgs.end() == (dit = git->second.dummyMsgs.find(msgId)))
	{
#ifdef DISTRIB_DUMMYMSG_DEBUG
		std::cerr << "p3GroupDistrib::locked_getGroupDummyMsg() Msg not found";
		std::cerr << std::endl;
#endif
		return NULL;
	}

	return &(dit->second);
}


	


