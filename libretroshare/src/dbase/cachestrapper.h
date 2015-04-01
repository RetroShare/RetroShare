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

#include "pqi/p3cfgmgr.h"
#include "pqi/pqiservicemonitor.h"
#include "util/rsthreads.h"

#include <list>
#include <set>
#include <map>
#include <string>
#include <iostream>

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

/******************************** CacheId ********************************/

/*!
 * Use this to identify the type of cache source, strapper,
 */
class CacheId
{
	public:
	CacheId() :type(0), subid(0) { return; }
	CacheId(uint16_t a, uint16_t b) :type(a), subid(b) { return; }
	uint16_t type; /// cache types, this should be set to services type
	uint16_t subid; /// should be initialised when using multicache feature of cachestrapper
};


bool operator<(const CacheId &a, const CacheId &b);

/*!
 * Use for identifying physical files that have been chosen as cache data
 * note: this does not actual store the data but serves to locate on network (via hash attribute,
 * and on file via path)
 */
class RsCacheData
{
	public:

    RsPeerId pid; /// peer id
	/// REMOVED as a WASTE to look it up everywhere! std::string pname; /// peer name (can be used by cachestore)
	CacheId  cid;  /// cache id
	std::string path; /// file system path where physical cache data is located
	std::string name;
    RsFileHash hash;
	uint64_t size;
	time_t recvd; /// received timestamp
};


std::ostream &operator<<(std::ostream &out, const RsCacheData &d);

/***************************** CacheTransfer *****************************/

/**
 *  Interface for FileTransfer Class to support cache
 */
class CacheTransfer
{
	public:

		CacheTransfer(CacheStrapper *cs) :strapper(cs) { return; }
		virtual ~CacheTransfer() {}

		/*!
		 * upload side of things .... searches through CacheStrapper.
		 */
        bool    FindCacheFile(const RsFileHash& hash, std::string &path, uint64_t &size);

		/*!
		 * At the download side RequestCache() => overloaded RequestCacheFile()
		 * the class should then call CompletedCache() or FailedCache()
		 */
		bool RequestCache(RsCacheData &data, CacheStore *cbStore); /* request from CacheStore */

	protected:

		/*!
		 * to be overloaded
		 */
        virtual bool RequestCacheFile(const RsPeerId& id, std::string path, const RsFileHash& hash, uint64_t size);
        virtual bool CancelCacheFile(const RsPeerId& id, std::string path, const RsFileHash& hash, uint64_t size);

        bool CompletedCache(const RsFileHash &hash);                   /* internal completion -> does cb */
        bool FailedCache(const RsFileHash &hash);                      /* internal completion -> does cb */

	private:

	CacheStrapper *strapper;

    std::map<RsFileHash, RsCacheData>    cbData;
    std::map<RsFileHash, CacheStore *> cbStores;
};



/************************ CacheSource/CacheStore *************************/

typedef std::map<uint16_t, RsCacheData> CacheSet;

/*!
 * Implements features needed for a service to act as a cachesource and allow pushing a of cache data from service to strapper
 * Service is able to use this class for refresh its cache  (push cache data)
 * and interface to load and check cache availablility among peers (source of cache data)
 * Architecturally Cachestrapper maintains the cachesource (which is passed as a pointer handle) while the cachesource-inheriting
 * service can update cachesource as to new cache sources  (cache data) created. Equivalently it enquiries through cache source for
 * new cache data from peers
 * @see p3Distrib
 */
class CacheSource
{
	public:

		CacheSource(uint16_t t, bool m, CacheStrapper *cs, std::string cachedir);
		virtual ~CacheSource() {}

		/*!
		 *  called to determine available cache for peer -
		 * default acceptable (returns all)
		 */
        virtual bool 	cachesAvailable(const RsPeerId& pid, std::map<CacheId, RsCacheData> &ids)=0;

		/*!
		 * function called at startup to load from
		 * configuration file....
		 * to be overloaded by inherited class
		 */
		virtual bool    loadLocalCache(const RsCacheData &data);

			/* control Caches available */
        bool  refreshCache(const RsCacheData &data,const std::set<RsPeerId>& destination_peers);
		bool  refreshCache(const RsCacheData &data);
		bool 	clearCache(CacheId id);

		/* controls if peer is an accepted receiver for cache items. Default is yes. To be overloaded. */
        virtual bool isPeerAcceptedAsCacheReceiver(const RsPeerId& /*peer_id*/)  { return true ; }

			/* get private data */
		std::string getCacheDir()    { return cacheDir;   }
		bool        isMultiCache()   { return multiCache; }
		uint16_t    getCacheType()   { return cacheType;  }

			/* display */
		void 	listCaches(std::ostream &out);

			/* search */
        bool    findCache(const RsFileHash& hash, RsCacheData &data) const;

	protected:

		uint16_t cacheType;    /// for checking of cache type (usually of child class of source)
		bool   multiCache;   /// whether multisource is in use or not.
		CacheStrapper *mStrapper;

			/*** MUTEX LOCKING */
		void	lockData() const;
		void	unlockData() const;

		CacheSet caches; /// all local cache data stored here
                std::map<RsFileHash, RsCacheData> mOldCaches; /// replaced/cleared caches are pushed here (in case requested)

		private:

		std::string cacheDir;
		mutable RsMutex cMutex;

};


/*!
 * Base Class for data cache. eg. FileCache/Store.
 * This is best used to deal with external caches from other peers
 * @see p3Distrib. pqiMonitor
 */
class CacheStore
{
	public:

		/*!
		 *
		 * @param t set to particular rs_service id. see rsserviceids.h
		 * @param m whether this is multicache service (true) or not(false)
		 * @param cs  cache strapper instance responsible for maintaining the cache service
		 * @param cft cache transfer instance responsible for rquestiing and tranfering caches
		 * @param cachedir directory used to store cache related info for cachestore client
		 * @return
		 */
		CacheStore(uint16_t t, bool m, CacheStrapper *cs, CacheTransfer *cft, std::string cachedir);
		virtual ~CacheStore() {}

		/* current stored data */

		/*!
		 *
		 * @param data returns cache data for pid/cid set in data itself
		 * @return false is unsuccessful and vice versa
		 */
		bool 	getStoredCache(RsCacheData &data); /* use pid/cid in data */

		/*!
		 *
		 * @param data all cache store by cachestore is store here
		 * @return false not returned, only true at the moment
		 */
		bool 	getAllStoredCaches(std::list<RsCacheData> &data); /* use pid/cid in data */

		/*!
		 *  input from CacheStrapper -> store can then download new data
		 */
		void	availableCache(const RsCacheData &data);

		/*!
		 * should be called when the download is completed ... cache data is loaded
		 */
		void 	downloadedCache(const RsCacheData &data);

		/*!
		 *  called if the download fails, TODO: nothing done yet
		 */
		void 	failedCache(const RsCacheData &data);

			/* virtual functions overloaded by cache implementor */

		/* controls if peer is an accepted provider for cache items. Default is yes. To be overloaded. */
        virtual bool isPeerAcceptedAsCacheProvider(const RsPeerId& /*peer_id*/)  { return true ; }

		/*!
		 * @param data cache data is stored here
		 * @return false is failed (cache does not exist), otherwise true
		 */
		virtual bool fetchCache(const RsCacheData &data);   /* a question? */
		virtual int nameCache(RsCacheData &data);           /* fill in the name/path */
		virtual int loadCache(const RsCacheData &data);	  /* actual load, once data available */

		/* get private data */

		std::string getCacheDir()    { return cacheDir;   }
		bool        isMultiCache()   { return multiCache; }
		uint16_t    getCacheType()   { return cacheType;  }

		/*!
		 *  display, e.g. can pass std::out, cerr, ofstream, etc
		 */
		void 	listCaches(std::ostream &out);

	protected:

		/*!
		 * ** MUTEX LOCKING
		 */
		void	lockData()   const;

		/*!
		 * ** MUTEX LOCKING
		 */
		void	unlockData() const;

		/*! This function is called to store Cache Entry in the CacheStore Table.
		 * it must be called from within a Mutex Lock....
		 *
		 * It doesn't lock itself -> to avoid race conditions
		 */
		void    locked_storeCacheEntry(const RsCacheData &data);

		/*! This function is called to store Cache Entry in the CacheStore Table.
		 * it must be called from within a Mutex Lock....
		 *
		 * It doesn't lock itself -> to avoid race conditions
		 */
		bool    locked_getStoredCache(RsCacheData &data);

		private:

		uint16_t cacheType;    /* for checking */
		bool     multiCache;   /* do we care about subid's */

		CacheStrapper *mStrapper;
		CacheTransfer *cacheTransfer;

		std::string cacheDir;

		mutable RsMutex cMutex;
        std::map<RsPeerId, CacheSet> caches;

};



/***************************** CacheStrapper *****************************/


/*!
 *  a convenient to pass cache handles to cachestrapper to maintain the cache service
 *  Make Sure you get the Ids right! see rsservicesids.h.
 *  When creating a cache service this data structure is
 *  source, usually the child class of store and source also serves as both handles
 *  @see CacheStrapper
 */
class CachePair
{
	public:

		/*!
		 * Default constructor, all variables set to NULL
		 */
		CachePair()
		:source(NULL), store(NULL), id(0, 0) { return; }

		/*!
		 *
		 * @param a cache source for service
		 * @param b cache store for service
		 * @param c the cache service id, c.type should be set to service id service-child class of store and source
		 */
		CachePair(CacheSource *a, CacheStore *b, CacheId c)
		:source(a), store(b), id(c) { return; }

		CacheSource *source;
		CacheStore  *store;
		CacheId     id; /// should be set id type to service types of store and source service-child class, and subid for multicache use
};


bool operator<(const CachePair &a, const CachePair &b);

/*!
 * CacheStrapper: maintains a set of CacheSources, and CacheStores,
 * queries and updates as new information arrives.
 */

class p3ServiceControl;

class CacheStrapper: public pqiServiceMonitor, public p3Config
{
	public:

		/*!
		 * @param cm handle used by strapper for getting peer connection information (online peers, sslids...)
		 * @return
		 */
        CacheStrapper(p3ServiceControl *sc, uint32_t ftServiceId);
virtual ~CacheStrapper() { return; }

	/************* from pqiMonitor *******************/
virtual void statusChange(const std::list<pqiServicePeer> &plist);
	/************* from pqiMonitor *******************/

	/* Feedback from CacheSources */

/*!
 * send data to peers online and self
 * @param data
 *
 */
void 	refreshCache(const RsCacheData &data);
void 	refreshCache(const RsCacheData &data,const std::set<RsPeerId>& destination_peers);	// specify a particular list of destination peers (self not added!)

/*!
 * forces config savelist
 * @param data
 * @see saveList()
 */
void 	refreshCacheStore(const RsCacheData &data);

/*!
 *  list of Caches to send out
 */
bool    getCacheUpdates(std::list<std::pair<RsPeerId, RsCacheData> > &updates);

/*!
 * add to strapper's cachepair set so a related service's store and source can be maintained
 * @param pair the source and store handle for a service
 */
void	addCachePair(CachePair pair);

	/*** I/O (2) ***/
void	recvCacheResponse(RsCacheData &data, time_t ts);  
void    handleCacheQuery(const RsPeerId& id, std::map<CacheId, RsCacheData> &data);


/*!
 *  search through CacheSources.
 *  @return false if cachedate mapping to hash not found
 */
bool    findCache(const RsFileHash &hash, RsCacheData &data) const;

	/* display */
void 	listCaches(std::ostream &out);

/*!
 * does not do anything
 * @param out
 * @deprecated
 */
void 	listPeerStatus(std::ostream &out);

/**
 * Checks if the cache physically exist at path given
 * @param data
 * @return whether it exists or not
 */
bool 	CacheExist(RsCacheData& data);

	/* Config */
        protected:

	        /* Key Functions to be overloaded for Full Configuration */
virtual RsSerialiser *setupSerialiser();
virtual bool saveList(bool &cleanup, std::list<RsItem *>&);
virtual bool    loadList(std::list<RsItem *>& load);

	private:

	/* these are static - so shouldn't need mutex */
	p3ServiceControl *mServiceCtrl;
	uint32_t          mFtServiceId; 

	std::map<uint16_t, CachePair> caches;

	RsMutex csMtx; /* protect below */

    std::list<std::pair<RsPeerId, RsCacheData> > mCacheUpdates;
};


#endif
