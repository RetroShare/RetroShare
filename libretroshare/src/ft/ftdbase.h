/*
 * libretroshare/src/ft: ftdbase.h
 *
 * File Transfer for RetroShare.
 *
 * Copyright 2008 by Robert Fernie.
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

#ifndef FT_DBASE_INTERFACE_HEADER
#define FT_DBASE_INTERFACE_HEADER

/* 
 * ftdbase.
 *
 * Wrappers for the Cache/FiStore/FiMonitor classes.
 * So they work in the ft world.
 */

class p3ConnectMgr ;

#include "ft/ftsearch.h"
#include "pqi/p3cfgmgr.h"

#include "dbase/fistore.h"
#include "dbase/fimonitor.h"
#include "dbase/cachestrapper.h"


class ftFiStore: public FileIndexStore, public ftSearch
{
	public:
	ftFiStore(CacheStrapper *cs, CacheTransfer *cft, NotifyBase *cb_in,
					p3ConnectMgr *,
                        RsPeerId ownid, std::string cachedir);

	/* overloaded search function */
virtual bool search(const std::string &hash, uint32_t hintflags, FileInfo &info) const;
};

class ftFiMonitor: public FileIndexMonitor, public ftSearch, public p3Config
{
	public:
	ftFiMonitor(CacheStrapper *cs,NotifyBase *cb_in, std::string cachedir, std::string pid);

	/* overloaded search function */
	virtual bool search(const std::string &hash, uint32_t hintflags, FileInfo &info) const;

	/* overloaded set dirs enables config indication */
	virtual void setSharedDirectories(const std::list<SharedDirInfo>& dirList);
	virtual void updateShareFlags(const SharedDirInfo& info) ;

	void	setRememberHashCacheDuration(uint32_t days) ;
	uint32_t rememberHashCacheDuration() const ;
	void	setRememberHashCache(bool) ;
	bool rememberHashCache() ;
	void clearHashCache() ;

	/***
	* Configuration - store shared directories
	*/
	protected:

virtual RsSerialiser *setupSerialiser();
virtual std::list<RsItem *> saveList(bool &cleanup);
virtual bool    loadList(std::list<RsItem *> load);
	

};

class ftCacheStrapper: public CacheStrapper, public ftSearch
{
	public:
        ftCacheStrapper(p3ConnectMgr *cm);

	/* overloaded search function */
virtual bool search(const std::string &hash, uint32_t hintflags, FileInfo &info) const;

};


#endif

