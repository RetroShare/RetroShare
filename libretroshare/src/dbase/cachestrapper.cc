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

#include <iostream>
#include <sstream>
#include <iomanip>

/**
 * #define CS_DEBUG 1
 */

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

CacheSource::CacheSource(uint16_t t, bool m, std::string cachedir)
	:cacheType(t), multiCache(m), cacheDir(cachedir)
	{ 
		return; 
	}

	/* Mutex Stuff -> to be done... */
void    CacheSource::lockData()
{
#ifdef CS_DEBUG
	std::cerr << "CacheSource::lockData()" << std::endl;
#endif
	cMutex.lock();
}

void    CacheSource::unlockData()
{
#ifdef CS_DEBUG
	std::cerr << "CacheSource::unlockData()" << std::endl;
#endif
	cMutex.unlock();
}
	
	/* to be overloaded for inherited Classes */
bool    CacheSource::loadCache(const CacheData &data)
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


bool    CacheSource::findCache(std::string hash, CacheData &data)
{
	lockData(); /* LOCK MUTEX */

	bool found = false;
	CacheSet::iterator it;
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


CacheStore::CacheStore(uint16_t t, bool m, CacheTransfer *cft, std::string cachedir)
	:cacheType(t), multiCache(m), cacheTransfer(cft), cacheDir(cachedir)
	{ 
		/* not much */
		return; 
	}

	/* Mutex Stuff -> to be done... */
void    CacheStore::lockData()
{
#ifdef CS_DEBUG
	std::cerr << "CacheStore::lockData()" << std::endl;
#endif
	cMutex.lock();
}

void    CacheStore::unlockData()
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



	/* input from CacheStrapper.
	 * check if we want to download it...
	 * determine the new name/path
	 * then request it.
	 */
void	CacheStore::availableCache(const CacheData &data)
{
	std::cerr << "CacheStore::availableCache() :" << data << std::endl;
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
	std::cerr << "CacheStore::downloadedCache() :" << data << std::endl;
	/* updates data */
	if (!loadCache(data))
	{
		return;
	}
}
	/* called when the download is completed ... updates internal data */
void 	CacheStore::failedCache(const CacheData &data)
{
	std::cerr << "CacheStore::failedCache() :" << data << std::endl;
	return;
}


	/* virtual function overloaded by cache implementor */
bool    CacheStore::fetchCache(const CacheData &data)
{
	/* should we fetch it? */
	std::cerr << "CacheStore::fetchCache() tofetch?:" << data << std::endl;

	CacheData incache = data;

	lockData(); /* LOCK MUTEX */

	bool haveCache = ((locked_getStoredCache(incache)) && (data.hash == incache.hash));

	unlockData(); /* UNLOCK MUTEX */


	if (haveCache)
	{
		std::cerr << "CacheStore::fetchCache() Already have it: false" << std::endl;
		return false;
	}

	std::cerr << "CacheStore::fetchCache() Missing this cache: true" << std::endl;
	return true;
}


int     CacheStore::nameCache(CacheData &data)
{
	/* name it... */
	lockData(); /* LOCK MUTEX */


	std::cerr << "CacheStore::nameCache() for:" << data << std::endl;
	data.name = data.hash;
	data.path = getCacheDir();
	std::cerr << "CacheStore::nameCache() done:" << data << std::endl;

	unlockData(); /* UNLOCK MUTEX */

	return 1;
}


int     CacheStore::loadCache(const CacheData &data)
{
	/* attempt to load -> dummy function */
	std::cerr << "CacheStore::loadCache() Dummy Load for:" << data << std::endl;

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
	return;
}


/********************************* CacheStrapper *********************************
 * This is the bit which handles queries
 *
 ********************************* CacheStrapper ********************************/

CacheStrapper::CacheStrapper(RsPeerId id, time_t period)
	:ownId(id), queryPeriod(period)
{
	return;
}


void	CacheStrapper::addCachePair(CachePair set)
{
	caches[set.id.type] = set;
}


        /* from pqimonclient */
void    CacheStrapper::monUpdate(const std::list<pqipeer> &plist)
{
	std::list<pqipeer>::const_iterator it;
	std::map<RsPeerId, CacheTS>::iterator mit;
	for(it = plist.begin(); it != plist.end(); it++)
	{
		if (status.end() == (mit = status.find(it->id)))
		{
			addPeerId(it->id);
		}
	}
}


void	CacheStrapper::addPeerId(RsPeerId pid)
{
	std::map<RsPeerId, CacheTS>::iterator it;

	/* just reset it for the moment */
	CacheTS ts;
	ts.query = 0;
	ts.answer = 0;

	status[pid] = ts;
}

bool	CacheStrapper::removePeerId(RsPeerId pid)
{
	std::map<RsPeerId, CacheTS>::iterator it;
	if (status.end() != (it = status.find(pid)))
	{
		status.erase(it);
		return true;
	}
	return false; 
}


	/* pass to correct CacheSet */
void	CacheStrapper::recvCacheResponse(CacheData &data, time_t ts)
{
	/* update internal data first */
	std::map<RsPeerId, CacheTS>::iterator it;
	if (status.end() == status.find(data.pid))
	{
		/* add it in */
		CacheTS d;
		d.query = 0;
		d.answer = 0;

		status[data.pid] = d;
	}

	it = status.find(data.pid); /* will always succeed */

	/* update status */
	(it -> second).answer = ts;

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
	out << "CacheStrapper::listCaches() [" << ownId;
	out << "] Total Peers: " << status.size() << " Total Caches: " << caches.size();
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
}


bool    CacheStrapper::findCache(std::string hash, CacheData &data)
{
	/* can overwrite for more control! */
	std::map<uint16_t, CachePair>::iterator it;
	for(it = caches.begin(); it != caches.end(); it++)
	{
		if ((it->second).source->findCache(hash, data))
		{
			return true;
		}
	}
	return false;
}


/********************************* CacheStrapper *********************************
 * This is the bit which handles queries
 *
 ********************************* CacheStrapper ********************************/


/* request from CacheStore */
bool CacheTransfer::RequestCache(CacheData &data, CacheStore *cbStore)
{
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
	std::cerr << "CacheTransfer::RequestCacheFile() : from:" << id << " #";
	std::cerr << hash << " size: " << size;
	std::cerr << " savepath: " << path << std::endl;
	std::cerr << "CacheTransfer::RequestCacheFile() Dummy... saying completed";
	std::cerr << std::endl;

	/* just tell them we've completed! */
	CompletedCache(hash);
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



