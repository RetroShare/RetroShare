/*
 * RetroShare FileCache Module: cachestrapper.h
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

#ifndef MRK_CACHE_STRAPPER_H
#define MRK_CACHE_STRAPPER_H

#include <list>
#include <map>
#include <string>
#include <iostream>
#include "util/rsthreads.h"

/******************* CacheStrapper and Related Classes *******************
 * A generic Cache Update system.
 *
 * CacheStrapper: maintains a set of CacheSources, and CacheStores, 
 *     queries and updates as new information arrives.
 *
 * CacheTransfer: Interface for FileTransfer Class to support.
 *
 * CacheSource: Base Class for cache data provider. eg. FileIndexMonitor.
 * CacheStore: Base Class for data cache. eg. FileCache/Store.
 *
 * Still TODO:
 * (1) Design and Implement the Upload side of CacheTransfer/CacheStrapper.
 * (2) CacheStrapper:: Save / Load Cache lists....
 * (3) Clean up lists, maps on shutdown etc.
 * (4) Consider Mutexes for multithreaded operations.
 * (5) Test the MultiSource/Store capabilities.
 *
 ******************* CacheStrapper and Related Classes *******************/


class CacheTransfer; /* Interface for File Transfer */
class CacheSource;   /* Interface for local File Index/Monitor */
class CacheStore;    /* Interface for the actual Cache */
class CacheStrapper; /* Controlling Class */

/****
typedef uint32_t RsPeerId;
*****/
typedef std::string RsPeerId;

/******************************** CacheId ********************************/
class CacheId
{
	public:
	CacheId() :type(0), subid(0) { return; }
	CacheId(uint16_t a, uint16_t b) :type(a), subid(b) { return; }
	uint16_t type;
	uint16_t subid;
};


bool operator<(const CacheId &a, const CacheId &b);


class CacheData
{
	public:

	RsPeerId pid;
	std::string pname; /* peer name (can be used by cachestore) */
	CacheId  cid;
	std::string path; 
	std::string name;
	std::string hash;
	uint64_t size;
	time_t recvd;
};


std::ostream &operator<<(std::ostream &out, const CacheData &d);

/***************************** CacheTransfer *****************************/

class CacheTransfer
{
	public:
	CacheTransfer(CacheStrapper *cs) :strapper(cs) { return; }
virtual ~CacheTransfer() {}

	/* upload side of things .... searches through CacheStrapper. */
bool    FindCacheFile(std::string hash, std::string &path, uint64_t &size);


	/* At the download side RequestCache() => overloaded RequestCacheFile()
	 * the class should then call CompletedCache() or FailedCache()
	 */

bool RequestCache(CacheData &data, CacheStore *cbStore); /* request from CacheStore */

	protected:
	/* to be overloaded */
virtual bool RequestCacheFile(RsPeerId id, std::string path, std::string hash, uint64_t size); 
virtual bool CancelCacheFile(RsPeerId id, std::string path, std::string hash, uint64_t size);

bool CompletedCache(std::string hash);                   /* internal completion -> does cb */
bool FailedCache(std::string hash);                      /* internal completion -> does cb */

	private:

	CacheStrapper *strapper;

	std::map<std::string, CacheData>    cbData;
	std::map<std::string, CacheStore *> cbStores;
};



/************************ CacheSource/CacheStore *************************/

typedef std::map<uint16_t, CacheData> CacheSet;

class CacheSource
{
	public:
	CacheSource(uint16_t t, bool m, CacheStrapper *cs, std::string cachedir);
virtual ~CacheSource() {}

	/* called to determine available cache for peer - 
	 * default acceptable (returns all) 
	 */
virtual bool 	cachesAvailable(RsPeerId pid, std::map<CacheId, CacheData> &ids);

	/* function called at startup to load from 
	 * configuration file....
	 * to be overloaded by inherited class 
	 */
virtual bool    loadLocalCache(const CacheData &data);

	/* control Caches available */
bool    refreshCache(const CacheData &data);
bool 	clearCache(CacheId id);

	/* get private data */
std::string getCacheDir()    { return cacheDir;   }
bool        isMultiCache()   { return multiCache; }
uint16_t    getCacheType()   { return cacheType;  }

	/* display */
void 	listCaches(std::ostream &out);

	/* search */
bool    findCache(std::string hash, CacheData &data);

	protected:

	uint16_t cacheType;    /* for checking */
	bool   multiCache;   /* do we care about subid's */
	CacheStrapper *mStrapper;

	/*** MUTEX LOCKING */
void	lockData();
void	unlockData();

	CacheSet caches;

	private:

	std::string cacheDir;
	RsMutex cMutex;
};


class CacheStore
{
	public:

	CacheStore(uint16_t t, bool m, CacheStrapper *cs, CacheTransfer *cft, std::string cachedir);
virtual ~CacheStore() {} 

	/* current stored data */
bool 	getStoredCache(CacheData &data); /* use pid/cid in data */
bool 	getAllStoredCaches(std::list<CacheData> &data); /* use pid/cid in data */

	/* input from CacheStrapper -> store can then download new data */
void	availableCache(const CacheData &data);

	/* called when the download is completed ... updates internal data */
void 	downloadedCache(const CacheData &data);

	/* called if the download fails */
void 	failedCache(const CacheData &data);

	/* virtual functions overloaded by cache implementor */
virtual bool fetchCache(const CacheData &data);   /* a question? */
virtual int nameCache(CacheData &data);           /* fill in the name/path */
virtual int loadCache(const CacheData &data);	  /* actual load, once data available */

	/* get private data */
std::string getCacheDir()    { return cacheDir;   }
bool        isMultiCache()   { return multiCache; }
uint16_t    getCacheType()   { return cacheType;  }

	/* display */
void 	listCaches(std::ostream &out);

	protected:
	/*** MUTEX LOCKING */
void	lockData();
void	unlockData();

	/* This function is called to store Cache Entry in the CacheStore Table.
	 * it must be called from within a Mutex Lock....
	 *
	 * It doesn't lock itself -> to avoid race conditions
	 */
void    locked_storeCacheEntry(const CacheData &data);
bool    locked_getStoredCache(CacheData &data);

	private:

	uint16_t cacheType;    /* for checking */
	bool     multiCache;   /* do we care about subid's */

	CacheStrapper *mStrapper;
	CacheTransfer *cacheTransfer;

	std::string cacheDir;

	RsMutex cMutex;

	std::map<RsPeerId, CacheSet> caches;

};



/***************************** CacheStrapper *****************************/


/* Make Sure you get the Ids right! */
class CachePair
{
	public:
	CachePair()
	:source(NULL), store(NULL), id(0, 0) { return; }

	CachePair(CacheSource *a, CacheStore *b, CacheId c)
	:source(a), store(b), id(c) { return; }

	CacheSource *source;
	CacheStore  *store;
	CacheId     id;
};


bool operator<(const CachePair &a, const CachePair &b);

#include "pqi/pqimonitor.h"
#include "pqi/p3cfgmgr.h"

class p3AuthMgr;

class CacheStrapper: public pqiMonitor, public p3Config
{
	public:
	CacheStrapper(p3AuthMgr *am, p3ConnectMgr *cm);
virtual ~CacheStrapper() { return; }

	/************* from pqiMonitor *******************/
virtual void statusChange(const std::list<pqipeer> &plist);
	/************* from pqiMonitor *******************/

	/* Feedback from CacheSources */
void 	refreshCache(const CacheData &data);
void 	refreshCacheStore(const CacheData &data);

	/* list of Caches to send out */
bool    getCacheUpdates(std::list<std::pair<RsPeerId, CacheData> > &updates);

void	addCachePair(CachePair pair);

	/*** I/O (2) ***/
void	recvCacheResponse(CacheData &data, time_t ts);  
void    handleCacheQuery(RsPeerId id, std::map<CacheId, CacheData> &data); 


	/* search through CacheSources. */
bool    findCache(std::string hash, CacheData &data);

	/* display */
void 	listCaches(std::ostream &out);
void 	listPeerStatus(std::ostream &out);


	/* Config */
        protected:

	        /* Key Functions to be overloaded for Full Configuration */
virtual RsSerialiser *setupSerialiser();
virtual std::list<RsItem *> saveList(bool &cleanup);
virtual bool    loadList(std::list<RsItem *> load);

	private:

	/* these are static - so shouldn't need mutex */
	p3AuthMgr *mAuthMgr;
	p3ConnectMgr *mConnMgr;

	std::map<uint16_t, CachePair> caches;

	RsMutex csMtx; /* protect below */

	std::list<std::pair<RsPeerId, CacheData> > mCacheUpdates;
};


#endif
