/*
 * "$Id: ftManager.h,v 1.13 2007-02-19 20:08:30 rmf24 Exp $"
 *
 * Other Bits for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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

#ifndef MRK_FT_MANAGER_HEADER
#define MRK_FT_MANAGER_HEADER

/*
 * ftManager - virtual base class for FileTransfer
 */

#include <list>
#include <iostream>
#include <string>

#include "pqi/pqi.h"
#include "serialiser/rsconfigitems.h"

#include "dbase/cachestrapper.h"
#include "server/hashsearch.h"

class ftFileManager;   /* stores files */


class ftFileRequest
{
        public:
	ftFileRequest(std::string id_in, std::string hash_in,
		        uint64_t size_in, uint64_t offset_in,
		        uint32_t chunk_in)
	:id(id_in), hash(hash_in), size(size_in),
	 offset(offset_in), chunk(chunk_in)
	{
	 	return;
	}

virtual ~ftFileRequest() { return; }

        std::string id;
        std::string hash;
        uint64_t size;
        uint64_t offset;
        uint32_t chunk;
};


class ftFileData: public ftFileRequest
{
        public:
	ftFileData(std::string id_in, std::string hash_in,
		        uint64_t size_in, uint64_t offset_in,
		        uint32_t chunk_in, void *data_in)
	:ftFileRequest(id_in, hash_in, size_in, 
		offset_in, chunk_in), data(data_in)
	{
		return;
	}

virtual ~ftFileData()
	{
		if (data)
		{
			free(data);
		}
	}

	void *data;
};


class ftManager: public CacheTransfer
{
	public:
	ftManager(CacheStrapper *cs)
	:CacheTransfer(cs), fhs(NULL) { return; }
virtual ~ftManager() { return; }

void    setFileHashSearch(FileHashSearch *hs) { fhs = hs; }

/****************** PART to be IMPLEMENTE******************/
	/* Functions to implement */

/*********** overloaded from CacheTransfer ***************/
/* Must callback after this fn - using utility functions */
//virtual bool RequestCacheFile(RsPeerId id, std::string path, 
//			std::string hash, uint64_t size);
/******************* GUI Interface ************************/
virtual int	getFile(std::string name, std::string hash, 
			uint64_t size, std::string destpath) = 0;

virtual int 	cancelFile(std::string hash) = 0;
virtual int 	clearFailedTransfers() = 0;

virtual int             tick() = 0;
virtual std::list<RsFileTransfer *> getStatus() = 0;

/************* Network Interface****************************/

	public:
virtual void    	setSaveBasePath(std::string s) = 0;
virtual void    	setEmergencyBasePath(std::string s) = 0;
virtual int             recvFileInfo(ftFileRequest *in) = 0;
virtual ftFileRequest * sendFileInfo() = 0;

	protected:

	/****************** UTILITY FUNCTIONS ********************/

	/* combines two lookup functions */
bool	lookupLocalHash(std::string hash, std::string &path, uint64_t &size);
bool	lookupRemoteHash(std::string hash, std::list<std::string> &ids);

	/*********** callback   from CacheTransfer ***************/
	//bool CompletedCache(std::string hash);                   /* internal completion -> does cb */
	//bool FailedCache(std::string hash);                      /* internal completion -> does cb */
	/*********** available  from CacheTransfer ***************/
        /* upload side of things .... searches through CacheStrapper(Sources) for a cache. */
	//bool    FindCacheFile(std::string id, std::string hash, std::string &path);
	/*********** available  from CacheTransfer ***************/

	private:
	FileHashSearch *fhs;
};

#endif









