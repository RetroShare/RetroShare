
/*
 * "$Id: p3face-file.cc,v 1.6 2007-04-15 18:45:23 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
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

#include "rsserver/p3files.h"

#include "rsserver/p3face.h"
#include "server/filedexserver.h"

#include <iostream>
#include <sstream>

RsFiles *rsFiles = NULL;

void	p3Files::lockRsCore()
{
	mCore->lockRsCore();
}

void	p3Files::unlockRsCore()
{
	mCore->unlockRsCore();
}

int     p3Files::UpdateAllTransfers()
{
        /* lock Mutexes */
        lockRsCore();     /* LOCK */

	std::list<RsFileTransfer *> nTransList = mServer -> getTransfers();
	std::list<RsFileTransfer *>::iterator it;

	mTransfers.clear();

	for(it = nTransList.begin(); it != nTransList.end(); it++)
	{
		FileInfo ti;

		ti.id = (*it)->cPeerId;
		ti.source = mAuthMgr->getName(ti.id);
		ti.peerIds = (*it) -> allPeerIds.ids;

		ti.fname = (*it) -> file.name;
		ti.hash  = (*it) -> file.hash;
		ti.path  = (*it) -> file.path;
		ti.size  = (*it) -> file.filesize;

		ti.transfered = (*it) -> transferred;
		ti.avail = ti.transfered;
		/* other ones!!! */
		ti.tfRate = (*it) -> crate / 1000;
		ti.download = (*it) -> in;
		ti.downloadStatus = (*it) -> state;

		mTransfers[ti.hash] = ti;

		delete (*it); 
	}

        unlockRsCore();     /* UNLOCK */

        return 1;
}


/* 1) Access to downloading / uploading files.  */

bool p3Files::FileDownloads(std::list<std::string> &hashs)
{
	RsStackMutex stack(fMutex); /**** LOCKED ****/

	std::map<std::string, FileInfo>::iterator it;
	for(it = mTransfers.begin(); it != mTransfers.end(); it++)
	{
		if (it->second.download)
		{
			hashs.push_back(it->second.hash);
		}
	}
	return true;
}

bool p3Files::FileUploads(std::list<std::string> &hashs)
{
	RsStackMutex stack(fMutex); /**** LOCKED ****/

	std::map<std::string, FileInfo>::iterator it;
	for(it = mTransfers.begin(); it != mTransfers.end(); it++)
	{
		if (!(it->second.download))
		{
			hashs.push_back(it->second.hash);
		}
	}
	return true;
}

bool p3Files::FileDetails(std::string hash, uint32_t hintflags, FileInfo &info)
{
	RsStackMutex stack(fMutex); /**** LOCKED ****/

	/* for the moment this will only show transferred data */
	std::map<std::string, FileInfo>::iterator it;

	if (mTransfers.end() == (it = mTransfers.find(hash)))
	{
		return false;
	}

	info = it->second;
	return true;
}


/* 2) Control of Downloads. */
bool p3Files::FileControl(std::string hash, uint32_t flags)
{
	/* dummy function for now */
	return false;
}

bool p3Files::FileRequest(std::string fname, std::string hash, uint64_t size, 
	std::string dest, uint32_t flags, std::list<std::string> srcIds)
{
	std::cerr << "p3Files::FileRequest()";
	std::cerr << std::endl;
	std::cerr << "name:" << fname;
	std::cerr << std::endl;
	std::cerr << "size:" << size;
	std::cerr << std::endl;
	std::cerr << "dest:" << dest;
	std::cerr << std::endl;

	lockRsCore(); /* LOCKED */

	std::cerr << "p3Files::FileRequest(" << fname << ", ";
	std::cerr << hash << ", " << size << ", " << dest << ")" << std::endl;

	int ret = mServer -> getFile(fname, hash, size, dest);

	unlockRsCore(); /* UNLOCKED */
	std::cerr << "p3Files::FileRequest() Done";
	std::cerr << std::endl;

	return ret;
}

bool p3Files::FileCancel(std::string hash)
{
	lockRsCore(); /* LOCKED */

	mServer -> cancelTransfer("", hash, 0); // other args not actually used!.

	unlockRsCore(); /* UNLOCKED */

	return 1;
}


bool p3Files::FileClearCompleted()
{
	std::cerr << "p3Files::FileClearCompleted()" << std::endl;

        /* lock Mutexes */
        lockRsCore();     /* LOCK */

	mServer -> clear_old_transfers();

        unlockRsCore();     /* UNLOCK */

	/* update data */
	UpdateAllTransfers();

	return 1;
}



/* 3) Addition of Extra Files... From File System (Dummy at the moment) */

bool p3Files::ExtraFileAdd(std::string fname, std::string hash, uint64_t size,
				uint32_t period, uint32_t flags)
{
	return false;
}

bool p3Files::ExtraFileRemove(std::string hash, uint32_t flags)
{
	return false;
}

bool p3Files::ExtraFileHash(std::string localpath, 
				uint32_t period, uint32_t flags)
{
	return false;
}

bool p3Files::ExtraFileStatus(std::string localpath, FileInfo &info)
{
	return false;
}


/* 4) Search and Listing Interface */

int p3Files::RequestDirDetails(std::string uid, std::string path, 
                                        DirDetails &details)
{
        lockRsCore();     /* LOCK */

	/* call to filedexmServer */
	int val = mServer->RequestDirDetails(uid, path, details);

        unlockRsCore();     /* UNLOCK */

	return val;

}

int p3Files::RequestDirDetails(void *ref, DirDetails &details, uint32_t flags)
{
        lockRsCore();     /* LOCK */

	/* call to filedexmServer */
	int val = mServer->RequestDirDetails(ref, details, flags);

        unlockRsCore();     /* UNLOCK */

	return val;
}


int p3Files::SearchKeywords(std::list<std::string> keywords, std::list<FileDetail> &results)
{
        lockRsCore();     /* LOCK */

	/* call to filedexmServer */
	int val = mServer->SearchKeywords(keywords, results);

        unlockRsCore();     /* UNLOCK */

	return val;
}

int p3Files::SearchBoolExp(Expression *exp, std::list<FileDetail> &results)
{
        lockRsCore();     /* LOCK */

	/* call to filedexmServer */
	int val = mServer->SearchBoolExp(exp, results);

        unlockRsCore();     /* UNLOCK */

	return val;
}


/* 5) Utility Functions.  */


bool p3Files::ConvertSharedFilePath(std::string path, std::string &fullpath)
{
        lockRsCore();     /* LOCK */

	/* call to filedexmServer */
	bool val = mServer->ConvertSharedFilePath(path, fullpath);

	/* done! */
        /* unlock Mutexes */
        unlockRsCore();     /* UNLOCK */

	return val;
}


void p3Files::ForceDirectoryCheck()
{
        lockRsCore();     /* LOCK */

	/* call to filedexmServer */
	mServer->ForceDirectoryCheck();

	/* done! */
        /* unlock Mutexes */
        unlockRsCore();     /* UNLOCK */
}


bool p3Files::InDirectoryCheck()
{
        lockRsCore();     /* LOCK */

	/* call to filedexmServer */
	bool val = mServer->InDirectoryCheck();

	/* done! */
        /* unlock Mutexes */
        unlockRsCore();     /* UNLOCK */

	return val;
}

	

void    p3Files::setDownloadDirectory(std::string path)
{
	/* lock Mutexes */
	lockRsCore();     /* LOCK */

	mServer -> setSaveDir(path);

	unlockRsCore();     /* UNLOCK */
}


std::string p3Files::getDownloadDirectory()
{
	/* lock Mutexes */
	lockRsCore();     /* LOCK */

	std::string path = mServer -> getSaveDir();

	unlockRsCore();     /* UNLOCK */

	return path;
}

void    p3Files::setPartialsDirectory(std::string path)
{
	/* dummy */
	return;
}

std::string p3Files::getPartialsDirectory()
{
	std::string path;
	return path;
}

bool    p3Files::getSharedDirectories(std::list<std::string> &dirs)
{
	/* lock Mutexes */
	lockRsCore();     /* LOCK */

	dirs = mServer -> getSearchDirectories();

	unlockRsCore();     /* UNLOCK */
	return true;
}


bool    p3Files::addSharedDirectory(std::string dir)
{
	/* lock Mutexes */
	lockRsCore();     /* LOCK */

	mServer -> addSearchDirectory(dir);

	unlockRsCore();     /* UNLOCK */
	return true;
}

bool    p3Files::removeSharedDirectory(std::string dir)
{
	lockRsCore();     /* LOCK */

	mServer -> removeSearchDirectory(dir);

	unlockRsCore();     /* UNLOCK */

	return true;
}


