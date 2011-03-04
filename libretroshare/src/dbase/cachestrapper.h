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
#include "pqi/pqimonitor.h"
#include "util/rsthreads.h"
#include "util/pugixml.h"

#include <list>
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

/****
typedef uint32_t RsPeerId;
*****/
typedef std::string RsPeerId;

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
class CacheData
{
	public:

	RsPeerId pid; /// peer id
	std::string pname; /// peer name (can be used by cachestore)
	CacheId  cid;  /// cache id
	std::string path; /// file system path where physical cache data is located
	std::string name;
	std::string hash;
	uint64_t size;
	time_t recvd; /// received timestamp
};


std::ostream &operator<<(std::ostream &out, const CacheData &d);

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
		bool    FindCacheFile(std::string hash, std::string &path, uint64_t &size);

		/*!
		 * At the download side RequestCache() => overloaded RequestCacheFile()
		 * the class should then call CompletedCache() or FailedCache()
		 */
		bool RequestCache(CacheData &data, CacheStore *cbStore); /* request from CacheStore */

	protected:

		/*!
		 * to be overloaded
		 */
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
		virtual bool 	cachesAvailable(RsPeerId pid, std::map<CacheId, CacheData> &ids);

		/*!
		 * function called at startup to load from
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
		bool    findCache(std::string hash, CacheData &data) const;

	protected:

		uint16_t cacheType;    /// for checking of cache type (usually of child class of source)
		bool   multiCache;   /// whether multisource is in use or not.
		CacheStrapper *mStrapper;

			/*** MUTEX LOCKING */
		void	lockData() const;
		void	unlockData() const;

		CacheSet caches; /// all cache data local and remote stored here

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
		bool 	getStoredCache(CacheData &data); /* use pid/cid in data */

		/*!
		 *
		 * @param data all cache store by cachestore is store here
		 * @return false not returned, only true at the moment
		 */
		bool 	getAllStoredCaches(std::list<CacheData> &data); /* use pid/cid in data */

		/*!
		 *  input from CacheStrapper -> store can then download new data
		 */
		void	availableCache(const CacheData &data);

		/*!
		 * should be called when the download is completed ... cache data is loaded
		 */
		void 	downloadedCache(const CacheData &data);

		/*!
		 *  called if the download fails, TODO: nothing done yet
		 */
		void 	failedCache(const CacheData &data);

			/* virtual functions overloaded by cache implementor */

		/*!
		 * @param data cache data is stored here
		 * @return false is failed (cache does not exist), otherwise true
		 */
		virtual bool fetchCache(const CacheData &data);   /* a question? */
		virtual int nameCache(CacheData &data);           /* fill in the name/path */
		virtual int loadCache(const CacheData &data);	  /* actual load, once data available */

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
		void    locked_storeCacheEntry(const CacheData &data);

		/*! This function is called to store Cache Entry in the CacheStore Table.
		 * it must be called from within a Mutex Lock....
		 *
		 * It doesn't lock itself -> to avoid race conditions
		 */
		bool    locked_getStoredCache(CacheData &data);


		//////////////// Cache Optimisation //////////////////

		/**
		 *@param id the key for determing whether this data has been cached or not
		 *@return true if data referenced by key has been cached false otherwise
		 */
		bool cached(const std::string cacheId);

		/**
		 * TODO: will be abstract
		 * The deriving class should return a document which accurately reflects its data
		 * structure
		 * @param cacheDoc document reflecting derving class's cache data structure
		 */
		virtual void updateCacheDocument(pugi::xml_document& cacheDoc);




		//////////////// Cache Optimisation //////////////////
		private:
		/**
		 * This updates the cache table with information from the xml document
		 *
		 */
		void updateCacheTable();

		uint16_t cacheType;    /* for checking */
		bool     multiCache;   /* do we care about subid's */

		CacheStrapper *mStrapper;
		CacheTransfer *cacheTransfer;

		std::string cacheDir;

		mutable RsMutex cMutex;
		std::map<RsPeerId, CacheSet> caches;

		////////////// cache optimisation ////////////////

		/// whether to run in cache optimisation mode
		bool cacheOptMode;

		/// stores whether given instance of cache data has been loaded already or not
		std::map<std::string, bool>  cacheTable;

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
class CacheStrapper: public pqiMonitor, public p3Config
{
	public:

		/*!
		 * @param cm handle used by strapper for getting peer connection information (online peers, sslids...)
		 * @return
		 */
        CacheStrapper(p3ConnectMgr *cm);
virtual ~CacheStrapper() { return; }

	/************* from pqiMonitor *******************/
virtual void statusChange(const std::list<pqipeer> &plist);
	/************* from pqiMonitor *******************/

	/* Feedback from CacheSources */

/*!
 * send data to peers online and selfe
 * @param data
 *
 */
void 	refreshCache(const CacheData &data);

/*!
 * forces config savelist
 * @param data
 * @see saveList()
 */
void 	refreshCacheStore(const CacheData &data);

/*!
 *  list of Caches to send out
 */
bool    getCacheUpdates(std::list<std::pair<RsPeerId, CacheData> > &updates);

/*!
 * add to strapper's cachepair set so a related service's store and source can be maintained
 * @param pair the source and store handle for a service
 */
void	addCachePair(CachePair pair);

	/*** I/O (2) ***/
void	recvCacheResponse(CacheData &data, time_t ts);  
void    handleCacheQuery(RsPeerId id, std::map<CacheId, CacheData> &data); 


/*!
 *  search through CacheSources.
 *  @return false if cachedate mapping to hash not found
 */
bool    findCache(std::string hash, CacheData &data) const;

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
bool 	CacheExist(CacheData& data);

	/* Config */
        protected:

	        /* Key Functions to be overloaded for Full Configuration */
virtual RsSerialiser *setupSerialiser();
virtual bool saveList(bool &cleanup, std::list<RsItem *>&);
virtual bool    loadList(std::list<RsItem *>& load);

	private:

	/* these are static - so shouldn't need mutex */
	p3ConnectMgr *mConnMgr;

	std::map<uint16_t, CachePair> caches;

	RsMutex csMtx; /* protect below */

	std::list<std::pair<RsPeerId, CacheData> > mCacheUpdates;
};


#endif
