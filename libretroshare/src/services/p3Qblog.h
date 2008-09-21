#ifndef P3QBLOG_H_
#define P3QBLOG_H_

/*
 * libretroshare/src/rsiface: p3Qblog.h
 *
 * RetroShare Blog Service.
 *
 * Copyright 2007-2008 by Chris Evi-Parker, Robert Fernie.
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


#include <iostream>
#include <string>
#include <list>
#include <map>

#include "dbase/cachestrapper.h"
#include "pqi/pqiservice.h"
#include "pqi/pqistreamer.h"
#include "pqi/p3connmgr.h"
#include "pqi/p3cfgmgr.h"

#include "serialiser/rsserial.h"
#include "rsiface/rstypes.h"

class RsQblogMsg; /* to populate maps of blogs */

/*!
 * contains definitions of the interface and blog information to be manipulated
 * See RsQblog class for virtual methods documentation
 */
 class p3Qblog : public CacheSource, public CacheStore
 {
 	    /**
    	 * overloads extractor to make printing wstrings easier
    	 */
    	friend std::ostream &operator<<(std::ostream &out, const std::wstring);

 	public:

		p3Qblog(p3ConnectMgr *connMgr,
		uint16_t type, CacheStrapper *cs, CacheTransfer *cft,
		std::string sourcedir, std::string storedir,
		uint32_t storePeriod);
 		virtual ~p3Qblog ();

 	public:

 		/******************************* CACHE SOURCE / STORE Interface *********************/

		/// overloaded functions from Cache Source
		virtual bool    loadLocalCache(const CacheData &data);

		/// overloaded functions from Cache Store
		virtual int    loadCache(const CacheData &data);

		/******************************* CACHE SOURCE / STORE Interface *********************/

 	public:

    	virtual bool sendBlog(const std::wstring &msg);
    	virtual bool getBlogs(std::map< std::string, std::multimap<long int, std::wstring> > &blogs);
    	virtual bool getPeerLatestBlog(std::string id, uint32_t &ts, std::wstring &post);

    	/**
    	 * to be run by server, update method
    	 */
    	void tick();

  	private:

/********************* begining of private utility methods **************************/

  		/*
    	 * to load and transform cache source to normal attribute format of a blog message
    	 * @param filename
    	 * @param source
    	 */
    	bool loadBlogFile(std::string filename, std::string src);

    	/*
    	 * add a blog item to maps
    	 * @param newBlog a blog item from a peer or yourself
    	 */
    	bool addBlog(RsQblogMsg  *newBlog);

    	/*
    	 * post our blog to our friends, connectivity method
    	 */
    	bool postBlogs(void);

    	/*
    	 * sort usr/blog maps in time order
    	 */
    	bool sort(void);


/************************* end of private methods **************************/

  		/// handles connection to peers
  		p3ConnectMgr *mConnMgr;
		/// for locking files provate members below
		RsMutex mBlogMtx;
		/// the current usr
 		std::string mOwnId;
		/// contain usrs and their blogs
 		std::map< std::string, std::multimap<long int, std::wstring> > mUsrBlogSet;
 		///fills up above sets
 		std::list<RsQblogMsg*> mBlogs;
 		///how long to keep posts
 		uint32_t mStorePeriod;
 		/// to track blog updates
 		bool mPostsUpdated;
 		///  to track profile updates
 		bool mProfileUpdated;

 		/*
 		 * load dummy data
 		 */
 		void loadDummy(void);

 };

#endif /*P3QBLOG_H_*/
