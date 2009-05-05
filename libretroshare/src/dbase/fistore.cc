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
#include "rsiface/rsexpr.h"
#include "serialiser/rsserviceids.h"

FileIndexStore::FileIndexStore(CacheStrapper *cs, CacheTransfer *cft, 
		NotifyBase *cb_in, RsPeerId ownid, std::string cachedir)
	:CacheStore(RS_SERVICE_TYPE_FILE_INDEX, false, cs, cft, cachedir), 
		localId(ownid), localindex(NULL), cb(cb_in)
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

	if (finew->loadIndex(data.path + '/' + data.name, data.hash, data.size))
	{
#ifdef FIS_DEBUG2
		std::cerr << "FileIndexStore::loadCache() Succeeded!" << std::endl;
#endif
		/* set the name */
		finew->root->name = data.pname;
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

	/* need to correct indices(row) for the roots of the FileIndex */
	int i = 0;
	for(it = indices.begin(); it != indices.end(); it++)
	{
		(it->second)->root->row = i++;
	}
	if (localindex)
	{
		localindex->root->row = 0;
	}

	unlockData();

	ModCompleted();
	bool ret = false;
	return ret;
}

	/* Search Interface - For Directory Access */
int FileIndexStore::RequestDirDetails(std::string uid, std::string path, DirDetails &details) const
{
	/* lock it up */
	lockData();

	std::map<RsPeerId, FileIndex *>::const_iterator it;
	it = indices.find(uid);
	bool found = true;
	if (it == indices.end())
	{
		//DirEntry *fdir = (it->second).lookupDirectory(path);
		/* translate it 
		 */
		found = false;
	}
	else
	{
		found = false;
	}

	unlockData();
	return found;
}

int FileIndexStore::RequestDirDetails(void *ref, DirDetails &details, uint32_t flags) const
{
	lockData();

#ifdef FIS_DEBUG
	std::cerr << "FileIndexStore::RequestDirDetails() ref=" << ref << " flags: " << flags << std::endl;
#endif

	bool found = true;
	std::map<RsPeerId, FileIndex *>::const_iterator pit;

	/* so cast *ref to a DirEntry */
	FileEntry *file = (FileEntry *) ref;
	DirEntry *dir = dynamic_cast<DirEntry *>(file);
	PersonEntry *person;
	/* root case */

#ifdef FIS_DEBUG
	std::cerr << "FileIndexStore::RequestDirDetails() CHKS" << std::endl;
	for(pit = indices.begin(); pit != indices.end(); pit++)
	{
		(pit->second)->root->checkParentPointers();
	}
#endif

	if (!ref)
	{
#ifdef FIS_DEBUG
		std::cerr << "FileIndexStore::RequestDirDetails() ref=NULL (root)" << std::endl;
#endif
		if (flags & DIR_FLAGS_LOCAL)
		{
			/* local only */
			if (localindex)
			{
				DirStub stub;
				stub.type = DIR_TYPE_PERSON;
				stub.name = localindex->root->name;
				stub.ref  = localindex->root;
				details.children.push_back(stub);
				details.count = 1;
			}
			else
			{
				details.count = 0;
			}

			details.parent = NULL;
			details.prow = 0;
			details.ref = NULL;
			details.type = DIR_TYPE_ROOT;
			details.name = "";
			details.hash = "";
			details.path = "";
			details.age = 0;
			details.rank = 0;
		}
		else
		{
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
			details.prow = 0;
			details.ref = NULL;
			details.type = DIR_TYPE_ROOT;
			details.name = "";
			details.hash = "";
			details.path = "";
			details.count = indices.size();
			details.age = 0;
			details.rank = 0;
		}
	}
	else 
	{
		if (dir) /* has children --- fill */
		{
#ifdef FIS_DEBUG
			std::cerr << "FileIndexStore::RequestDirDetails() ref=dir" << std::endl;
#endif
			std::map<std::string, FileEntry *>::iterator fit;
			std::map<std::string, DirEntry *>::iterator dit;
			/* extract all the entries */
			for(dit = dir->subdirs.begin(); 
				dit != dir->subdirs.end(); dit++)
			{
				DirStub stub;
				stub.type = DIR_TYPE_DIR;
				stub.name = (dit->second) -> name;
				stub.ref  = (dit->second);
	
				details.children.push_back(stub);
			}
	
			for(fit = dir->files.begin(); 
				fit != dir->files.end(); fit++)
			{
				DirStub stub;
				stub.type = DIR_TYPE_FILE;
				stub.name = (fit->second) -> name;
				stub.ref  = (fit->second);
	
				details.children.push_back(stub);
			}

			details.type = DIR_TYPE_DIR;
			details.hash = "";
			details.count = dir->subdirs.size() + 
					dir->files.size();
		}
		else
		{
#ifdef FIS_DEBUG
			std::cerr << "FileIndexStore::RequestDirDetails() ref=file" << std::endl;
#endif
			details.type = DIR_TYPE_FILE;
			details.count = file->size;
		}
	
#ifdef FIS_DEBUG
		std::cerr << "FileIndexStore::RequestDirDetails() name: " << file->name << std::endl;
#endif
		details.ref = file;
		details.name = file->name;
		details.hash = file->hash;
		details.age = time(NULL) - file->modtime;
		details.rank = file->pop;

		/* find parent pointer, and row */
		DirEntry *parent = file->parent;
		if (!parent) /* then must be root */
		{
			details.parent = NULL;
			details.prow = 0;
		}
		else
		{
			details.parent = parent;
			details.prow = parent->row;
		}

		/* find peer id */
		parent = dir;
		if (!dir)
		{
			/* cannot be null -> no files at root level */
			parent=file->parent;
		}
		// Well, yes, it can be null, beleive me. In such a case it may be that
		// file is a person entry.

		if(parent==NULL)
		{
			if(NULL == (person = dynamic_cast<PersonEntry *>(file))) 
			{
				std::cerr << "Major Error- Not PersonEntry!";
				exit(1);
			}
		}
		else
		{
			/* NEW add path (to dir - if dir, or parent dir - if file? */
			if (NULL != (person = dynamic_cast<PersonEntry *>(parent))) {
			    //if parent if root, then the path is inside the name; we should parse the name for the last / char
			    int i;
			    for(i = details.name.length(); (i > 0) && (details.name[i] != '/'); i--);
			    if (i != 0) {
				//the file is in a subdir, let's remove the / char and set correct path
				details.path = details.name.substr(0,i);
				details.name = details.name.substr(i + 1);
			    }

			} else {
			    details.path = parent->path;
			}

			while(parent->parent)
				parent = parent->parent;

			/* we should end up on the PersonEntry */
			if (NULL == (person = dynamic_cast<PersonEntry *>(parent)))
			{
				std::cerr << "Major Error- Not PersonEntry!";
				exit(1);
			}
		}
		details.id = person->id;
	}

	unlockData();
	return found;
}


int FileIndexStore::SearchHash(std::string hash, std::list<FileDetail> &results) const
{
	lockData();
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


int FileIndexStore::SearchKeywords(std::list<std::string> keywords, std::list<FileDetail> &results,uint32_t flags) const
{
	lockData();
	std::map<RsPeerId, FileIndex *>::const_iterator pit;
	std::list<FileEntry *>::iterator rit; 
	std::list<FileEntry *> firesults;

	time_t now = time(NULL);

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

	if(flags & DIR_FLAGS_LOCAL)
		if (localindex)
		{
			firesults.clear();

			localindex->searchTerms(keywords, firesults);
			/* translate results */
			for(rit = firesults.begin(); rit != firesults.end(); rit++)
			{
				FileDetail fd;
				fd.id = "Local"; //localId;
				fd.name = (*rit)->name;
				fd.hash = (*rit)->hash;
				fd.path = ""; /* TODO */
				fd.size = (*rit)->size;
				fd.age  = now - (*rit)->modtime;
				fd.rank = (*rit)->pop;

				results.push_back(fd);
			}

		}

	unlockData();
	return results.size();
}


int FileIndexStore::searchBoolExp(Expression * exp, std::list<FileDetail> &results) const
{
	lockData();
	std::map<RsPeerId, FileIndex *>::const_iterator pit;
	std::list<FileEntry *>::iterator rit; 
	std::list<FileEntry *> firesults;

	time_t now = time(NULL);

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
			FileDetail fd;
			fd.id = pit->first;
			fd.name = (*rit)->name;
			fd.hash = (*rit)->hash;
			fd.path = ""; /* TODO */
			fd.size = (*rit)->size;
			fd.age  = now - (*rit)->modtime;
			fd.rank  = (*rit)->pop;

			results.push_back(fd);
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
			FileDetail fd;
			fd.id = "Local"; //localId;
			fd.name = (*rit)->name;
			fd.hash = (*rit)->hash;
			fd.path = ""; /* TODO */
			fd.size = (*rit)->size;
			fd.age  = now - (*rit)->modtime;
			fd.rank  = (*rit)->pop;

			results.push_back(fd);
		}

	}


	unlockData();
	return results.size();
}

int FileIndexStore::AboutToModify()
{
	if (cb)
		cb->notifyListPreChange(NOTIFY_LIST_DIRLIST, 0);

	return 1;
}


int FileIndexStore::ModCompleted()
{
	if (cb)
		cb->notifyListChange(NOTIFY_LIST_DIRLIST, 0);

	return 1;
}


