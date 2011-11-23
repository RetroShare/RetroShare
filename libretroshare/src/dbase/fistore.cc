/*
 * RetroShare FileCache Module: fistore.cc
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

#include "dbase/fistore.h"
#include "retroshare/rsexpr.h"
#include "serialiser/rsserviceids.h"
#include "pqi/p3peermgr.h"

FileIndexStore::FileIndexStore(CacheStrapper *cs, CacheTransfer *cft,
		NotifyBase *cb_in,p3PeerMgr *cnmgr, RsPeerId ownid, std::string cachedir)
	:CacheStore(RS_SERVICE_TYPE_FILE_INDEX, false, cs, cft, cachedir),
		localId(ownid), localindex(NULL), cb(cb_in),mPeerMgr(cnmgr)
{
	return;
}

FileIndexStore::~FileIndexStore()
{
	/* clean up the Index */
	return;
}

/***
 * #define FIS_DEBUG2 1
 * #define FIS_DEBUG 1
 **/

	  /* actual load, once data available */
int FileIndexStore::loadCache(const CacheData &data)
{

#ifdef FIS_DEBUG2
	std::cerr << "FileIndexStore::loadCache() hash: " << data.hash << std::endl;
	std::cerr << "FileIndexStore::loadCache() path: " << data.path << std::endl;
	std::cerr << "FileIndexStore::loadCache() name: " << data.name << std::endl;
	std::cerr << "FileIndexStore::loadCache() size: " << data.size << std::endl;
#endif

	/* do Callback */
	AboutToModify();

	/* lock it up */
	lockData();

	FileIndex *fiold = NULL;
	bool local = (data.pid == localId);

	std::map<RsPeerId, FileIndex *>::iterator it;
	/* remove old cache */
	if (local)
	{
		fiold = localindex;
		localindex = NULL;
	}
	else if (indices.end() != (it = indices.find(data.pid)))
	{
		fiold = it->second;
		indices.erase(it);
		//delete fi;
	}

	/* load Cache */

	FileIndex *finew = new FileIndex(data.pid);

	if(mPeerMgr->isFriend(data.pid))
	{
		// We discard file lists from non friends. This is the place to remove file lists of deleted friends
		// from the cache. Doing this, the file list still shows in a session where we deleted a friend, but will be removed
		// at next restart.
		//
		peerState ps;
		mPeerMgr->getFriendNetStatus(data.pid, ps);
		std::string peername = ps.name + " (" + ps.location + ")";

		if (finew->loadIndex(data.path + '/' + data.name, data.hash, data.size))
		{
#ifdef FIS_DEBUG2
			std::cerr << "FileIndexStore::loadCache() Succeeded!" << std::endl;
#endif
			/* set the name */
			finew->root->name = peername; // data.pid; // HACK HERE TODO XXX. name;

			if (local)
			{
				localindex = finew;
			}
			else
			{
				indices[data.pid] = finew;
			}
			delete fiold;

			/* store in tale */
			locked_storeCacheEntry(data);
		}
		else
		{
#ifdef FIS_DEBUG2
			std::cerr << "FileIndexStore::loadCache() Failed!" << std::endl;
#endif
			/* reinstall the old one! */
			delete finew;
			if (fiold)
			{
				if (local)
				{
					localindex = fiold;
				}
				else
				{
					indices[data.pid] = fiold;
				}
			}
		}
	}
#ifdef FIS_DEBUG
	else
		std::cerr << "Discarding file list from deleted peer " << data.pid << std::endl ;
#endif

	/* need to correct indices(row) for the roots of the FileIndex */
	int i = 0;
	for(it = indices.begin(); it != indices.end(); it++)
	{
		(it->second)->root->row = i++;
		it->second->FileIndex::updateMaxModTime() ;
	}
	if (localindex)
	{
		localindex->root->row = 0;
		localindex->updateMaxModTime() ;
	}

	unlockData();

	ModCompleted();
	bool ret = false;
	return ret;
}


	/* Search Interface - For Directory Access */
int FileIndexStore::RequestDirDetails(std::string uid, std::string /*path*/, DirDetails& details) const
{
	/* lock it up */
	lockData();

	std::map<RsPeerId, FileIndex *>::const_iterator it;
	it = indices.find(uid);
	bool found = true;

	if (it != indices.end())
	{
		//DirEntry *fdir = (it->second).lookupDirectory(path);
		/* translate it
		 */
		bool b = FileIndex::extractData((it->second)->root,details) ;

		found = found && b ;
	}
	else
		found = false;

	unlockData();
	return found;
}

int FileIndexStore::RequestDirDetails(void *ref, DirDetails &details, uint32_t flags) const
{
	/* remove unused parameter warnings */
	(void) flags;

#ifdef FIS_DEBUG
	std::cerr << "FileIndexStore::RequestDirDetails() ref=" << ref << " flags: " << flags << std::endl;
#endif

	std::map<RsPeerId, FileIndex *>::const_iterator pit;

	lockData();

//	checked by FileIndex::extractData
//	if(ref != NULL && !FileIndex::isValid(ref))
//	{
//		unlockData() ;
//		return false ;
//	}

	/* so cast *ref to a DirEntry */
	/* root case */

#ifdef FIS_DEBUG
	std::cerr << "FileIndexStore::RequestDirDetails() CHKS" << std::endl;
	for(pit = indices.begin(); pit != indices.end(); pit++)
	{
		(pit->second)->root->checkParentPointers();
	}
#endif

	if (ref == NULL)
	{
#ifdef FIS_DEBUG
		std::cerr << "FileIndexStore::RequestDirDetails() ref=NULL (root)" << std::endl;
#endif

		/* get remote root entries */
		for(pit = indices.begin(); pit != indices.end(); pit++)
		{
			/*
			 */
			DirStub stub;
			stub.type = DIR_TYPE_PERSON;
			stub.name = (pit->second)->root->name;
			stub.ref =  (pit->second)->root;

			details.children.push_back(stub);
		}
		details.parent = NULL;
		details.prow = -1;
		details.ref = NULL;
		details.type = DIR_TYPE_ROOT;
		details.name = "";
		details.hash = "";
		details.path = "";
		details.count = indices.size();
		details.age = 0;
		details.flags = 0;
		details.min_age = 0;

		unlockData();
		return true ;
	}

	bool b = FileIndex::extractData(ref,details) ;
	
	unlockData();
	return b;
}
uint32_t FileIndexStore::getType(void *ref) const
{
	lockData() ;
	uint32_t b = FileIndex::getType(ref) ;
	unlockData();

	return b;
}

int FileIndexStore::SearchHash(std::string hash, std::list<FileDetail> &results) const
{
	lockData();
	results.clear() ;
	std::map<RsPeerId, FileIndex *>::const_iterator pit;
	std::list<FileEntry *>::iterator rit;
	std::list<FileEntry *> firesults;

	time_t now = time(NULL);

#ifdef FIS_DEBUG
	std::cerr << "FileIndexStore::SearchHash()" << std::endl;
#endif
	for(pit = indices.begin(); pit != indices.end(); pit++)
	{
#ifdef FIS_DEBUG
		std::cerr << "FileIndexStore::SearchHash() Searching: Peer ";
		std::cerr << pit->first << std::endl;
#endif
		firesults.clear();

		(pit->second)->searchHash(hash, firesults);
		/* translate results */
		for(rit = firesults.begin(); rit != firesults.end(); rit++)
		{
			FileDetail fd;
			fd.id = pit->first;
			fd.name = (*rit)->name;
			fd.hash = (*rit)->hash;
			fd.path = ""; /* TODO */
			fd.size = (*rit)->size;
			fd.age  = now - (*rit)->modtime;
			fd.rank = (*rit)->pop;

			results.push_back(fd);
		}

	}


#ifdef FIS_DEBUG
	std::cerr << "FileIndexStore::SearchHash() Found " << results.size();
	std::cerr << " Results from " << indices.size() << " Peers" << std::endl;
#endif

	unlockData();
	return results.size();
}


int FileIndexStore::SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,uint32_t flags) const
{
	lockData();
	std::map<RsPeerId, FileIndex *>::const_iterator pit;
	std::list<FileEntry *>::iterator rit;
	std::list<FileEntry *> firesults;

	results.clear() ;

#ifdef FIS_DEBUG
	std::cerr << "FileIndexStore::SearchKeywords()" << std::endl;
#endif
	if(flags & DIR_FLAGS_REMOTE)
		for(pit = indices.begin(); pit != indices.end(); pit++)
		{
			firesults.clear();

			(pit->second)->searchTerms(keywords, firesults);
			/* translate results */
			for(rit = firesults.begin(); rit != firesults.end(); rit++)
			{
				DirDetails dd;

				if(!FileIndex::extractData(*rit, dd))
					continue ;
				
				results.push_back(dd);
			}
		}

	if(flags & DIR_FLAGS_LOCAL)
		if (localindex)
		{
			firesults.clear();

			localindex->searchTerms(keywords, firesults);
			/* translate results */
			for(rit = firesults.begin(); rit != firesults.end(); rit++)
			{
				DirDetails dd;

				if(!FileIndex::extractData(*rit, dd))
					continue ;

				dd.id = "Local";
				results.push_back(dd);
			}

		}

	unlockData();
	return results.size();
}


int FileIndexStore::searchBoolExp(Expression * exp, std::list<DirDetails> &results) const
{
	lockData();
	std::map<RsPeerId, FileIndex *>::const_iterator pit;
	std::list<FileEntry *>::iterator rit;
	std::list<FileEntry *> firesults;

#ifdef FIS_DEBUG
	std::cerr << "FileIndexStore::searchBoolExp()" << std::endl;
#endif
	for(pit = indices.begin(); pit != indices.end(); pit++)
	{
		firesults.clear();

		(pit->second)->searchBoolExp(exp, firesults);

		/* translate results */
		for(rit = firesults.begin(); rit != firesults.end(); rit++)
		{
			DirDetails dd;
			FileIndex::extractData(*rit, dd);
			results.push_back(dd);
		}

	}

	/* finally search local files */
	if (localindex)
	{
		firesults.clear();

		localindex->searchBoolExp(exp, firesults);

		/* translate results */
		for(rit = firesults.begin(); rit != firesults.end(); rit++)
		{
			DirDetails dd;
			FileIndex::extractData(*rit, dd);
			dd.id = "Local";
			results.push_back(dd);
		}

	}


	unlockData();
	return results.size();
}

int FileIndexStore::AboutToModify()
{
	if (cb)
		cb->notifyListPreChange(NOTIFY_LIST_DIRLIST_FRIENDS, 0);

	return 1;
}


int FileIndexStore::ModCompleted()
{
	if (cb)
		cb->notifyListChange(NOTIFY_LIST_DIRLIST_FRIENDS, 0);

	return 1;
}


