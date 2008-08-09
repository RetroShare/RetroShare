/*
 * RetroShare FileCache Module: cachestrapper.cc
 *   
 * Copyright 2004-2007 by Robert Fernie.
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

#include "dbase/cachestrapper.h"
#include "serialiser/rsconfigitems.h"
#include "pqi/p3connmgr.h"
#include "util/rsdir.h"

#include <iostream>
#include <sstream>
#include <iomanip>

/****
 * #define CS_DEBUG 1
 ***/

bool operator<(const CacheId &a, const CacheId &b)
{
        if (a.type == b.type)
		return (a.subid < b.subid);
	return (a.type < b.type);
}

bool operator<(const CachePair &a, const CachePair &b)
{
	return (a.id < b.id);
}


std::ostream &operator<<(std::ostream &out, const CacheData &d)
{
	out << "[ p: " << d.pid << " id: <" << d.cid.type << "," << d.cid.subid;
	out << "> #" << d.hash << " size: " << d.size;
	out << " \"" << d.name << "\"@\"" << d.path;
	out << "\" ]";
	return out;
}

/********************************* Cache Store / Source **************************
 * This is a generic interface which interacts with the FileTransfer Unit
 * to collect Data to be Cached.
 *
 ********************************* Cache Store / Source *************************/

CacheSource::CacheSource(uint16_t t, bool m, CacheStrapper *cs, std::string cachedir)
	:cacheType(t), multiCache(m), mStrapper(cs), cacheDir(cachedir)
	{ 
		return; 
	}

	/* Mutex Stuff -> to be done... */
void    CacheSource::lockData() const
{
#ifdef CS_DEBUG
	std::cerr << "CacheSource::lockData()" << std::endl;
#endif
	cMutex.lock();
}

void    CacheSource::unlockData() const
{
#ifdef CS_DEBUG
	std::cerr << "CacheSource::unlockData()" << std::endl;
#endif
	cMutex.unlock();
}
	
	/* to be overloaded for inherited Classes */
bool    CacheSource::loadLocalCache(const CacheData &data)
{
	return refreshCache(data);
}
	
        /* control Caches available */
bool    CacheSource::refreshCache(const CacheData &data)
{
	lockData(); /* LOCK MUTEX */

	bool ret = false;
	if (data.cid.type == getCacheType())
	{
		if (isMultiCache())
		{
			caches[data.cid.subid] = data;
		}
		else
		{
			caches[0] = data;
		}
		ret = true;
	}

	unlockData(); /* UNLOCK MUTEX */

	if (mStrapper) /* allow testing without full feedback */
		mStrapper->refreshCache(data);
	return ret;
}
		
bool    CacheSource::clearCache(CacheId id)
{
	lockData(); /* LOCK MUTEX */

	bool ret = false;
	if (id.type == getCacheType())
	{
		CacheSet::iterator it;
		if (caches.end() != (it = caches.find(id.subid)))
		{
			caches.erase(it);
			ret = true;
		}
	}

	unlockData(); /* UNLOCK MUTEX */
	return ret;
}
		
bool    CacheSource::cachesAvailable(RsPeerId pid, std::map<CacheId, CacheData> &ids)
{
	lockData(); /* LOCK MUTEX */

	/* can overwrite for more control! */
	CacheSet::iterator it;
	for(it = caches.begin(); it != caches.end(); it++)
	{
		ids[(it->second).cid] = it->second;
	}
	bool ret = (caches.size() > 0);

	unlockData(); /* UNLOCK MUTEX */
	return ret;

}


bool    CacheSource::findCache(std::string hash, CacheData &data) const
{
	lockData(); /* LOCK MUTEX */

	bool found = false;
	CacheSet::const_iterator it;
	for(it = caches.begin(); it != caches.end(); it++)
	{
		if (hash == (it->second).hash)
		{
			data = it->second;
			found = true;
			break;
		}
	}

	unlockData(); /* UNLOCK MUTEX */

	return found;
}

		
void    CacheSource::listCaches(std::ostream &out)
{
	lockData(); /* LOCK MUTEX */

	/* can overwrite for more control! */
	CacheSet::iterator it;
	out << "CacheSource::listCaches() [" << getCacheType();
	out << "] Total: " << caches.size() << std::endl;
	int i;
	for(i = 0, it = caches.begin(); it != caches.end(); it++, i++)
	{
		out << "\tC[" << i << "] : " << it->second << std::endl;
	}

	unlockData(); /* UNLOCK MUTEX */
	return;
}


CacheStore::CacheStore(uint16_t t, bool m, 	
			CacheStrapper *cs, CacheTransfer *cft, std::string cachedir)
	:cacheType(t), multiCache(m), mStrapper(cs), 
		cacheTransfer(cft), cacheDir(cachedir)
	{ 
		/* not much */
		return; 
	}

	/* Mutex Stuff -> to be done... */
void    CacheStore::lockData() const
{
#ifdef CS_DEBUG
	std::cerr << "CacheStore::lockData()" << std::endl;
#endif
	cMutex.lock();
}

void    CacheStore::unlockData() const
{
#ifdef CS_DEBUG
	std::cerr << "CacheStore::unlockData()" << std::endl;
#endif
	cMutex.unlock();
}
		
void    CacheStore::listCaches(std::ostream &out)
{
	lockData(); /* LOCK MUTEX */

	/* can overwrite for more control! */
	std::map<RsPeerId, CacheSet>::iterator pit;
	out << "CacheStore::listCaches() [" << getCacheType();
	out << "] Total People: " << caches.size();
	out << std::endl;
	for(pit = caches.begin(); pit != caches.end(); pit++)
	{
		CacheSet::iterator it;
		out << "\tTotal for [" << pit->first << "] : " << (pit->second).size();
		out << std::endl;
		for(it = (pit->second).begin(); it != (pit->second).end(); it++)
		{
			out << "\t\t" << it->second;
			out << std::endl;
		}
	}

	unlockData(); /* UNLOCK MUTEX */
	return;
}


	/* look for stored data. using pic/cid in CacheData
	 */
bool	CacheStore::getStoredCache(CacheData &data)
{
	lockData(); /* LOCK MUTEX */

	bool ok = locked_getStoredCache(data);

	unlockData(); /* UNLOCK MUTEX */
	return ok;
}


bool	CacheStore::locked_getStoredCache(CacheData &data)
{
	if (data.cid.type != getCacheType())
	{
		return false;
	}

	std::map<RsPeerId, CacheSet>::iterator pit;
	if (caches.end() == (pit = caches.find(data.pid)))
	{
		return false;
	}

	CacheSet::iterator cit;
	if (isMultiCache())
	{
		/* look for subid */
		if ((pit->second).end() == 
			(cit = (pit->second).find(data.cid.subid)))
		{
			return false;
		}
	}
	else
	{
		if ((pit->second).end() == 
			(cit = (pit->second).find(0)))
		{
			return false;
		}
	}

	/* we found it! (cit) */
	data = cit->second;

	return true;
}



bool	CacheStore::getAllStoredCaches(std::list<CacheData> &data)
{
	lockData(); /* LOCK MUTEX */

	std::map<RsPeerId, CacheSet>::iterator pit;
	for(pit = caches.begin(); pit != caches.end(); pit++)
	{
		CacheSet::iterator cit;
		/* look for subid */
		for(cit = (pit->second).begin();
				cit != (pit->second).end(); cit++)
		{
			data.push_back(cit->second);
		}
	}

	unlockData(); /* UNLOCK MUTEX */

	return true;
}


	/* input from CacheStrapper.
	 * check if we want to download it...
	 * determine the new name/path
	 * then request it.
	 */
void	CacheStore::availableCache(const CacheData &data)
{
#ifdef CS_DEBUG
	std::cerr << "CacheStore::availableCache() :" << data << std::endl;
#endif

	/* basic checks */
	lockData(); /* LOCK MUTEX */

	bool rightCache = (data.cid.type == getCacheType());

	unlockData(); /* UNLOCK MUTEX */

	if (!rightCache)
	{
		return; /* bad id */
	}

	/* These Functions lock the Mutex themselves
	 */

	if (!fetchCache(data))
	{
		return; /* ignore it */
	}

	CacheData rData = data;

	/* get new name */
	if (!nameCache(rData))
	{
		return; /* error naming */
	}

	/* request it */
	cacheTransfer -> RequestCache(rData, this);

	/* will get callback when it is complete */
	return;
}


	/* called when the download is completed ... updates internal data */
void 	CacheStore::downloadedCache(const CacheData &data)
{
#ifdef CS_DEBUG
	std::cerr << "CacheStore::downloadedCache() :" << data << std::endl;
#endif

	/* updates data */
	if (!loadCache(data))
	{
		return;
	}
}
	/* called when the download is completed ... updates internal data */
void 	CacheStore::failedCache(const CacheData &data)
{
#ifdef CS_DEBUG
	std::cerr << "CacheStore::failedCache() :" << data << std::endl;
#endif
	return;
}


	/* virtual function overloaded by cache implementor */
bool    CacheStore::fetchCache(const CacheData &data)
{
	/* should we fetch it? */
#ifdef CS_DEBUG
	std::cerr << "CacheStore::fetchCache() tofetch?:" << data << std::endl;
#endif

	CacheData incache = data;

	lockData(); /* LOCK MUTEX */

	bool haveCache = ((locked_getStoredCache(incache)) && (data.hash == incache.hash));

	unlockData(); /* UNLOCK MUTEX */


	if (haveCache)
	{
#ifdef CS_DEBUG
		std::cerr << "CacheStore::fetchCache() Already have it: false" << std::endl;
#endif
		return false;
	}

#ifdef CS_DEBUG
	std::cerr << "CacheStore::fetchCache() Missing this cache: true" << std::endl;
#endif
	return true;
}


int     CacheStore::nameCache(CacheData &data)
{
	/* name it... */
	lockData(); /* LOCK MUTEX */

#ifdef CS_DEBUG
	std::cerr << "CacheStore::nameCache() for:" << data << std::endl;
#endif

	data.name = data.hash;
	data.path = getCacheDir();

#ifdef CS_DEBUG
	std::cerr << "CacheStore::nameCache() done:" << data << std::endl;
#endif

	unlockData(); /* UNLOCK MUTEX */

	return 1;
}


int     CacheStore::loadCache(const CacheData &data)
{
	/* attempt to load -> dummy function */
#ifdef CS_DEBUG
	std::cerr << "CacheStore::loadCache() Dummy Load for:" << data << std::endl;
#endif

	lockData(); /* LOCK MUTEX */

	locked_storeCacheEntry(data);

	unlockData(); /* UNLOCK MUTEX */
	
	return 1;
}

/* This function is called to store Cache Entry in the CacheStore Table.
 * it must be called from within a Mutex Lock....
 *
 * It doesn't lock itself -> to avoid race conditions 
 */

void 	CacheStore::locked_storeCacheEntry(const CacheData &data)
{
	/* store what we loaded - overwriting if necessary */
	std::map<RsPeerId, CacheSet>::iterator pit;
	if (caches.end() == (pit = caches.find(data.pid)))
	{
		/* add in a new CacheSet */
		CacheSet emptySet;
		caches[data.pid] = emptySet;

		pit = caches.find(data.pid);
	}

	if (isMultiCache())
	{
		(pit->second)[data.cid.subid] = data;
	}
	else
	{
		(pit->second)[0] = data;
	}

	/* tell the strapper we've loaded one */
	if (mStrapper)
	{
		mStrapper->refreshCacheStore(data);
	}
	return;
}


/********************************* CacheStrapper *********************************
 * This is the bit which handles queries
 *
 ********************************* CacheStrapper ********************************/

CacheStrapper::CacheStrapper(p3AuthMgr *am, p3ConnectMgr *cm)
	:p3Config(CONFIG_TYPE_CACHE), mAuthMgr(am), mConnMgr(cm)
{
	return;
}


void	CacheStrapper::addCachePair(CachePair set)
{
	caches[set.id.type] = set;
}


        /**************** from pqimonclient ********************/

void    CacheStrapper::statusChange(const std::list<pqipeer> &plist)
{
	std::list<pqipeer>::const_iterator it;
	for(it = plist.begin(); it != plist.end(); it++)
	{
		if (it->actions & RS_PEER_CONNECTED)
		{
			/* grab all the cache ids and add */

			std::map<CacheId,CacheData> hashs;
			std::map<CacheId,CacheData>::iterator cit;

			handleCacheQuery(it->id, hashs);

			RsStackMutex stack(csMtx); /******* LOCK STACK MUTEX *********/
			for(cit = hashs.begin(); cit != hashs.end(); cit++)
			{
				mCacheUpdates.push_back(std::make_pair(it->id, cit->second));
			}
		}
	}
}

        /**************** from pqimonclient ********************/


void	CacheStrapper::refreshCache(const CacheData &data)
{
	/* we've received an update 
	 * send to all online peers + self
	 */
#ifdef CS_DEBUG 
	std::cerr << "CacheStrapper::refreshCache() : " << data << std::endl;
#endif

	std::list<std::string> ids;
	std::list<std::string>::iterator it;

	mConnMgr->getOnlineList(ids);

	RsStackMutex stack(csMtx); /******* LOCK STACK MUTEX *********/
	for(it = ids.begin(); it != ids.end(); it++)
	{
#ifdef CS_DEBUG 
		std::cerr << "CacheStrapper::refreshCache() Send To: " << *it << std::endl;
#endif

		mCacheUpdates.push_back(std::make_pair(*it, data));
	}

	mCacheUpdates.push_back(std::make_pair(mConnMgr->getOwnId(), data));

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
}


void	CacheStrapper::refreshCacheStore(const CacheData &data)
{

	/* indicate to save data */
	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

}

bool	CacheStrapper::getCacheUpdates(std::list<std::pair<RsPeerId, CacheData> > &updates)
{
	RsStackMutex stack(csMtx); /******* LOCK STACK MUTEX *********/
	updates = mCacheUpdates;
	mCacheUpdates.clear();

	return true;
}



	/* pass to correct CacheSet */
void	CacheStrapper::recvCacheResponse(CacheData &data, time_t ts)
{
	/* find cache store */
	std::map<uint16_t, CachePair>::iterator it2;
	if (caches.end() == (it2 = caches.find(data.cid.type)))
	{
		/* error - don't have this type of cache */
		return;
	}

	/* notify the CacheStore */
	(it2 -> second).store -> availableCache(data);

}


	/* generate periodically or at a change */
#if 0
bool    CacheStrapper::sendCacheQuery(std::list<RsPeerId> &id, time_t ts)
{
	/* iterate through peers, and see who we haven't got an answer from recently */
	std::map<RsPeerId, CacheTS>::iterator it;
	for(it = status.begin(); it != status.end(); it++)
	{
		if ((ts - (it->second).query) > queryPeriod)
		{
			/* query this one */
			id.push_back(it->first);
			(it->second).query = ts;
		}
	}
	return (id.size() > 0);
}
#endif


void    CacheStrapper::handleCacheQuery(RsPeerId id, std::map<CacheId,CacheData> &hashs)
{
	/* basic version just iterates through ....
	 * more complex could decide who gets what!
	 *
	 * or that can be handled on a cache by cache basis.
	 */

	std::map<uint16_t, CachePair>::iterator it;
	for(it = caches.begin(); it != caches.end(); it++)
	{
		(it->second).source -> cachesAvailable(id, hashs);
	}
	return;
}

void    CacheStrapper::listCaches(std::ostream &out)
{
	/* can overwrite for more control! */
	std::map<uint16_t, CachePair>::iterator it;
	out << "CacheStrapper::listCaches() [" << mConnMgr->getOwnId();
	out << "] " << " Total Caches: " << caches.size();
	out << std::endl;
	for(it = caches.begin(); it != caches.end(); it++)
	{
		out << "CacheType: " << it->first;
		out << std::endl;

		(it->second).source->listCaches(out);
		(it->second).store->listCaches(out);
		out << std::endl;
	}
	return;
}

void    CacheStrapper::listPeerStatus(std::ostream &out)
{
#if 0
	std::map<RsPeerId, CacheTS>::iterator it;
	out << "CacheStrapper::listPeerStatus() [" << ownId;
	out << "] Total Peers: " << status.size() << " Total Caches: " << caches.size();
	out << std::endl;
	for(it = status.begin(); it != status.end(); it++)
	{
		out << "Peer: " << it->first;
		out << " Query: " << (it->second).query;
		out << " Answer: " << (it->second).answer;
		out << std::endl;
	}
	return;
#endif
}


bool    CacheStrapper::findCache(std::string hash, CacheData &data) const
{
	/* can overwrite for more control! */
	std::map<uint16_t, CachePair>::const_iterator it;
	for(it = caches.begin(); it != caches.end(); it++)
	{
		if ((it->second).source->findCache(hash, data))
		{
			return true;
		}
	}
	return false;
}
	
	

/***************************************************************************/
/****************************** CONFIGURATION HANDLING *********************/
/***************************************************************************/

/**** OVERLOADED FROM p3Config ****/

RsSerialiser *CacheStrapper::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser();

	/* add in the types we need! */
	rss->addSerialType(new RsCacheConfigSerialiser());

	return rss;
}


std::list<RsItem *> CacheStrapper::saveList(bool &cleanup)
{
	std::list<RsItem *> saveData;

	/* it can delete them! */
	cleanup = true;

#ifdef CS_DEBUG 
	std::cerr << "CacheStrapper::saveList()" << std::endl;
#endif

	/* iterate through the Caches (local first) */

	std::list<CacheData>::iterator cit;
	std::list<CacheData> ownCaches;
	std::list<CacheData> remoteCaches;
	std::string ownId = mConnMgr->getOwnId();

	std::map<uint16_t, CachePair>::iterator it;
	for(it = caches.begin(); it != caches.end(); it++)
	{
		std::map<CacheId, CacheData>::iterator tit;
		std::map<CacheId, CacheData> ownTmp;
		(it->second).source -> cachesAvailable(ownId, ownTmp);
		(it->second).store -> getAllStoredCaches(remoteCaches);

		for(tit = ownTmp.begin(); tit != ownTmp.end(); tit++)
		{
			ownCaches.push_back(tit->second);
		}
	}

	for(cit = ownCaches.begin(); cit != ownCaches.end(); cit++)
	{
		RsCacheConfig *rscc = new RsCacheConfig();

		// Fixup lazy behaviour in clients...
		// This ensures correct loading later.  
		// (used to be: rscc->pid = cit->pid;)
		rscc->pid = ownId; 

		//rscc->pname = cit->pname;
		rscc->cachetypeid = cit->cid.type;
		rscc->cachesubid = cit->cid.subid;
		rscc->path = cit->path;
		rscc->name = cit->name;
		rscc->hash = cit->hash;
		rscc->size = cit->size;
		rscc->recvd = cit->recvd;

		saveData.push_back(rscc);
	}

	for(cit = remoteCaches.begin(); cit != remoteCaches.end(); cit++)
	{
		if (cit->pid == ownId)
		{
#ifdef CS_DEBUG 
			std::cerr << "CacheStrapper::loadList() discarding Own Remote Cache";
			std::cerr << std::endl;
#endif
			continue; /* skip own caches -> will get transferred anyway */
		}

		RsCacheConfig *rscc = new RsCacheConfig();

		rscc->pid = cit->pid;
		//rscc->pname = cit->pname;
		rscc->cachetypeid = cit->cid.type;
		rscc->cachesubid = cit->cid.subid;
		rscc->path = cit->path;
		rscc->name = cit->name;
		rscc->hash = cit->hash;
		rscc->size = cit->size;
		rscc->recvd = cit->recvd;

		saveData.push_back(rscc);
	}

	/* list completed! */
	return saveData;
}


bool CacheStrapper::loadList(std::list<RsItem *> load)
{
	std::list<RsItem *>::iterator it;
	RsCacheConfig *rscc;

#ifdef CS_DEBUG 
	std::cerr << "CacheStrapper::loadList() Item Count: " << load.size();
	std::cerr << std::endl;
#endif
	std::list<CacheData> ownCaches;
	std::list<CacheData> remoteCaches;
	std::string ownId = mConnMgr->getOwnId();

	std::map<std::string, std::list<std::string> > saveFiles;
	std::map<std::string, std::list<std::string> >::iterator sit;

	for(it = load.begin(); it != load.end(); it++)
	{
		/* switch on type */
		if (NULL != (rscc = dynamic_cast<RsCacheConfig *>(*it)))
		{
#ifdef CS_DEBUG 
			std::cerr << "CacheStrapper::loadList() Item: ";
			std::cerr << std::endl;
			rscc->print(std::cerr, 10);
			std::cerr << std::endl;
#endif
			CacheData cd;

			cd.pid = rscc->pid;
			cd.pname = mAuthMgr->getName(cd.pid);
			cd.cid.type = rscc->cachetypeid;
			cd.cid.subid = rscc->cachesubid;
			cd.path = rscc->path;
			cd.name = rscc->name;
			cd.hash = rscc->hash;
			cd.size = rscc->size;
			cd.recvd = rscc->recvd;

			/* store files that we want to keep */
			(saveFiles[cd.path]).push_back(cd.name);

			std::map<uint16_t, CachePair>::iterator it2;
			if (caches.end() == (it2 = caches.find(cd.cid.type)))
			{
				/* error - don't have this type of cache */
#ifdef CS_DEBUG 
				std::cerr << "CacheStrapper::loadList() Can't Find Cache discarding";
				std::cerr << std::endl;
#endif
			}
			else
			{
				if (cd.pid == ownId)
				{
					/* load local */
					(it2 -> second).source -> loadLocalCache(cd);
#ifdef CS_DEBUG 
					std::cerr << "CacheStrapper::loadList() loaded Local";
					std::cerr << std::endl;
#endif
				}
				else
				{
					/* load remote */
					(it2 -> second).store -> loadCache(cd);
#ifdef CS_DEBUG 
					std::cerr << "CacheStrapper::loadList() loaded Remote";
					std::cerr << std::endl;
#endif
				}
			}

			/* cleanup */
			delete (*it);

		}
		else
		{
			/* cleanup */
			delete (*it);
		}
	}

	/* assemble a list of dirs to clean (union of cache dirs) */
	std::list<std::string> cacheDirs;
	std::list<std::string>::iterator dit, fit;
	std::map<uint16_t, CachePair>::iterator cit;
	for(cit = caches.begin(); cit != caches.end(); cit++)
	{
		std::string lcdir = (cit->second).source->getCacheDir();
		std::string rcdir = (cit->second).store->getCacheDir();

		if (cacheDirs.end() == std::find(cacheDirs.begin(), cacheDirs.end(), lcdir))
		{
			cacheDirs.push_back(lcdir);
		}

		if (cacheDirs.end() == std::find(cacheDirs.begin(), cacheDirs.end(), rcdir))
		{
			cacheDirs.push_back(rcdir);
		}
	}

#ifdef CS_DEBUG 
	std::cerr << "CacheStrapper::loadList() Files To Save:"  << std::endl;
#endif

	for(sit = saveFiles.begin(); sit != saveFiles.end(); sit++)
	{
#ifdef CS_DEBUG 
		std::cerr << "CacheStrapper::loadList() Files To Save in dir: <" << sit->first << ">" << std::endl;
#endif
		for(fit = (sit->second).begin(); fit != (sit->second).end(); fit++)
		{
#ifdef CS_DEBUG 
			std::cerr << "\tFile: " << *fit << std::endl;
#endif
		}
	}

	std::list<std::string> emptyList;
	for(dit = cacheDirs.begin(); dit != cacheDirs.end(); dit++)
	{
#ifdef CS_DEBUG 
		std::cerr << "CacheStrapper::loadList() Cleaning cache dir: <" << *dit << ">" << std::endl;
#endif
		sit = saveFiles.find(*dit);
		if (sit != saveFiles.end())
		{
			for(fit = (sit->second).begin(); fit != (sit->second).end(); fit++)
			{
#ifdef CS_DEBUG 
				std::cerr << "CacheStrapper::loadList() Keeping File: " << *fit << std::endl;
#endif
			}
			RsDirUtil::cleanupDirectory(*dit, sit->second);
		}
		else
		{
#ifdef CS_DEBUG 
			std::cerr << "CacheStrapper::loadList() No Files to save here!" << std::endl;
#endif
			RsDirUtil::cleanupDirectory(*dit, emptyList);
		}
	}

	return true;

}


/********************************* CacheStrapper *********************************
 * This is the bit which handles queries
 *
 ********************************* CacheStrapper ********************************/


/* request from CacheStore */
bool CacheTransfer::RequestCache(CacheData &data, CacheStore *cbStore)
{
	/* check for a previous request -> and cancel 
	 *
	 * - if duplicate pid, cid -> cancel old transfer
	 * - if duplicate hash -> Fail Transfer
	 */

	std::map<std::string, CacheData>::iterator dit;
	std::map<std::string, CacheStore *>::iterator sit;

	for(dit = cbData.begin(); dit != cbData.end(); dit++)
	{
		if (((dit->second).pid == data.pid) && 
			((dit->second).cid.type == data.cid.type) &&
			((dit->second).cid.subid == data.cid.subid))
		{
			/* cancel old transfer */
			CancelCacheFile(dit->second.pid, dit->second.path, 
				dit->second.hash, dit->second.size);

			sit = cbStores.find(dit->second.hash);
			cbData.erase(dit);
			cbStores.erase(sit);

			break;
		}
	}

	/* find in store.... */
	sit = cbStores.find(data.hash);
	if (sit != cbStores.end())
	{
		/* Duplicate Current Request */
		cbStore -> failedCache(data);
		return false;
	}


	/* store request */
	cbData[data.hash] = data;
	cbStores[data.hash] = cbStore;

	/* request data */
	RequestCacheFile(data.pid, data.path, data.hash, data.size);

	/* wait for answer */
	return true;
}


/* to be overloaded */
bool CacheTransfer::RequestCacheFile(RsPeerId id, std::string path, std::string hash, uint64_t size)
{
#ifdef CS_DEBUG
	std::cerr << "CacheTransfer::RequestCacheFile() : from:" << id << " #";
	std::cerr << hash << " size: " << size;
	std::cerr << " savepath: " << path << std::endl;
	std::cerr << "CacheTransfer::RequestCacheFile() Dummy... saying completed";
	std::cerr << std::endl;
#endif

	/* just tell them we've completed! */
	CompletedCache(hash);
	return true;
}

/* to be overloaded */
bool CacheTransfer::CancelCacheFile(RsPeerId id, std::string path, std::string hash, uint64_t size)
{
#ifdef CS_DEBUG
	std::cerr << "CacheTransfer::CancelCacheFile() : from:" << id << " #";
	std::cerr << hash << " size: " << size;
	std::cerr << " savepath: " << path << std::endl;
	std::cerr << "CacheTransfer::CancelCacheFile() Dummy fn";
	std::cerr << std::endl;
#endif

	return true;
}


/* internal completion -> does cb */
bool CacheTransfer::CompletedCache(std::string hash)
{
	std::map<std::string, CacheData>::iterator dit;
	std::map<std::string, CacheStore *>::iterator sit;

	/* find in store.... */
	sit = cbStores.find(hash);
	dit = cbData.find(hash);

	if ((sit == cbStores.end()) || (dit == cbData.end()))
	{
		return false;
	}
	
	/* callback */
	(sit -> second) -> downloadedCache(dit->second);

	/* clean up store */
	cbStores.erase(sit);
	cbData.erase(dit);

	return true;
}

/* internal completion -> does cb */
bool CacheTransfer::FailedCache(std::string hash)
{
	std::map<std::string, CacheData>::iterator dit;
	std::map<std::string, CacheStore *>::iterator sit;

	/* find in store.... */
	sit = cbStores.find(hash);
	dit = cbData.find(hash);

	if ((sit == cbStores.end()) || (dit == cbData.end()))
	{
		return false;
	}
	
	/* callback */
	(sit -> second) -> failedCache(dit->second);

	/* clean up store */
	cbStores.erase(sit);
	cbData.erase(dit);

	return true;
}


bool    CacheTransfer::FindCacheFile(std::string hash, std::string &path, uint64_t &size)
{
	CacheData data;
	if (strapper->findCache(hash, data))
	{
		path = data.path + "/" + data.name;
		size = data.size;
		return true;
	}

	return false;
}



