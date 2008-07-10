
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


#include "rsiface/rsiface.h"
#include "rsserver/p3face.h"
#include "util/rsdir.h"

#include "serialiser/rsconfigitems.h"

#include <iostream>
#include <sstream>

#include "util/rsdebug.h"
const int p3facefilezone = 11452;

#include <sys/time.h>
#include <time.h>

static const int FileMaxAge = 1800; /* Reload Data after 30 minutes */

//int ensureExtension(std::string &name, std::string def_ext);

/****************************************/
/****************************************/
int     RsServer::UpdateRemotePeople()
{
        RsIface &iface = getIface();

        /* lock Mutexes */
        lockRsCore();     /* LOCK */
        iface.lockData(); /* LOCK */

        /* unlock Mutexes */
        iface.unlockData(); /* UNLOCK */
        unlockRsCore();     /* UNLOCK */

        return 1;
}

/****************************************/
/****************************************/
int     RsServer::UpdateAllTransfers()
{
        NotifyBase &cb = getNotify();
        cb.notifyListPreChange(NOTIFY_LIST_TRANSFERLIST, NOTIFY_TYPE_MOD);

        RsIface &iface = getIface();

        /* lock Mutexes */
        lockRsCore();     /* LOCK */
        iface.lockData(); /* LOCK */


	/* clear the old transfer list() */
	std::list<FileTransferInfo> &transfers = iface.mTransferList;
	transfers.clear();

	std::list<RsFileTransfer *> nTransList = server -> getTransfers();
	std::list<RsFileTransfer *>::iterator it;

	for(it = nTransList.begin(); it != nTransList.end(); it++)
	{
		FileTransferInfo ti;

		ti.id = (*it)->cPeerId;
		ti.source = mAuthMgr->getName(ti.id);
		ti.peerIds = (*it) -> allPeerIds.ids;

		ti.fname = (*it) -> file.name;
		ti.hash  = (*it) -> file.hash;
		ti.path  = (*it) -> file.path;
		ti.size  = (*it) -> file.filesize;

		ti.transfered = (*it) -> transferred;
		/* other ones!!! */
		ti.tfRate = (*it) -> crate / 1000;
		ti.download = (*it) -> in;
		ti.downloadStatus = (*it) -> state;
		transfers.push_back(ti);

		delete (*it); 
	}

        iface.setChanged(RsIface::Transfer);

        /* unlock Mutexes */
        iface.unlockData(); /* UNLOCK */
        unlockRsCore();     /* UNLOCK */

        /* Now Notify of the Change */
        cb.notifyListChange(NOTIFY_LIST_TRANSFERLIST, NOTIFY_TYPE_MOD);

        return 1;
}


int RsServer::FileRequest(std::string fname, std::string hash, 
			uint32_t size, std::string dest)
{
	lockRsCore(); /* LOCKED */

	std::cerr << "RsServer::FileRequest(" << fname << ", ";
	std::cerr << hash << ", " << size << ", " << dest << ")" << std::endl;

	int ret = server -> getFile(fname, hash, size, dest);

	unlockRsCore(); /* UNLOCKED */

	return ret;
}


        /* Actions For Upload/Download */
int RsServer::FileRecommend(std::string fname, std::string hash, int size)
{
	/* check for new data */
        RsIface &iface = getIface();

        /* lock Mutexes */
        lockRsCore();     /* LOCK */
        iface.lockData(); /* LOCK */

	/* add the entry to FileRecommends.
	 */

	std::list<FileInfo> &recList = iface.mRecommendList;
	std::list<FileInfo>::iterator it;

	FileInfo fi;
	fi.path  = ""; /* this is not needed / wanted anymore */
	fi.hash  = hash;
	fi.fname = fname;
	fi.size = size;
	fi.rank = 1;
	fi.inRecommend = false;

	/* check if it exists already! */
	bool found = false;
	for(it = recList.begin(); (!found) && (it != recList.end()); it++)
	{
		if ((it->hash == fi.hash) && (it -> fname == fi.fname))
		{
			found = true;
		}
	}

	if (!found)
	{
		recList.push_back(fi);
	}

	/* Notify of change */
	iface.setChanged(RsIface::Recommend);

        /* lock Mutexes */
        iface.unlockData(); /* UNLOCK */
        unlockRsCore();     /* UNLOCK */

	return true;
}

int RsServer::FileCancel(std::string fname, std::string hash, uint32_t size)
{
	lockRsCore(); /* LOCKED */

	server -> cancelTransfer(fname, hash, size);

	unlockRsCore(); /* UNLOCKED */

	return 1;
}


int RsServer::FileClearCompleted()
{
	std::cerr << "RsServer::FileClearCompleted()" << std::endl;

	/* check for new data */
        RsIface &iface = getIface();

        /* lock Mutexes */
        lockRsCore();     /* LOCK */
        iface.lockData(); /* LOCK */

	server -> clear_old_transfers();

        /* lock Mutexes */
        iface.unlockData(); /* UNLOCK */
        unlockRsCore();     /* UNLOCK */

	/* update data */
	UpdateAllTransfers();

	return 1;
}


int RsServer::FileSetBandwidthTotals(float outkB, float inkB)
{
	int ret = 0;
	return ret;
}


/**************************************************************************/
#if 0

int RsServer::FileBroadcast(std::string id, std::string src, int size)
{
	lockRsCore(); /* LOCKED */
	RsCertId uId(id);

	int ret = 1;
	cert *c = intFindCert(uId);
	if ((c == NULL) || (c == sslr -> getOwnCert()))
	{
		ret = 0;
	}

	if (ret)
	{
		/* TO DO */
	}

	unlockRsCore(); /* UNLOCKED */

	return ret;
}

int RsServer::FileDelete(std::string id, std::string fname)
{
	lockRsCore(); /* LOCKED */
	RsCertId uId(id);

	int ret = 1;
	cert *c = intFindCert(uId);
	if ((c == NULL) || (c == sslr -> getOwnCert()))
	{
		ret = 0;
	}

	if (ret)
	{
		/* TO DO */
	}

	unlockRsCore(); /* UNLOCKED */

	return ret;
}

#endif
/**************************************************************************/



int RsServer::RequestDirDetails(std::string uid, std::string path, 
                                        DirDetails &details)
{
        /* lock Mutexes */
        RsIface &iface = getIface();

        lockRsCore();     /* LOCK */
        iface.lockData(); /* LOCK */

	/* call to filedexserver */
	int val = server->RequestDirDetails(uid, path, details);

	/* done! */
        /* unlock Mutexes */
        iface.unlockData(); /* UNLOCK */
        unlockRsCore();     /* UNLOCK */

	return val;

}

int RsServer::RequestDirDetails(void *ref, DirDetails &details, uint32_t flags)
{

        /* lock Mutexes */
        RsIface &iface = getIface();

        lockRsCore();     /* LOCK */
        iface.lockData(); /* LOCK */

	/* call to filedexserver */
	int val = server->RequestDirDetails(ref, details, flags);

	/* done! */
        /* unlock Mutexes */
        iface.unlockData(); /* UNLOCK */
        unlockRsCore();     /* UNLOCK */

	return val;
}


int RsServer::SearchKeywords(std::list<std::string> keywords, std::list<FileDetail> &results)
{
        /* lock Mutexes */
        RsIface &iface = getIface();

        lockRsCore();     /* LOCK */
        iface.lockData(); /* LOCK */

	/* call to filedexserver */
	int val = server->SearchKeywords(keywords, results);

	/* done! */
        /* unlock Mutexes */
        iface.unlockData(); /* UNLOCK */
        unlockRsCore();     /* UNLOCK */

	return val;
}

int RsServer::SearchBoolExp(Expression *exp, std::list<FileDetail> &results)
{
        /* lock Mutexes */
        RsIface &iface = getIface();

        lockRsCore();     /* LOCK */
        iface.lockData(); /* LOCK */

	/* call to filedexserver */
	int val = server->SearchBoolExp(exp, results);

	/* done! */
        /* unlock Mutexes */
        iface.unlockData(); /* UNLOCK */
        unlockRsCore();     /* UNLOCK */

	return val;
}

bool RsServer::ConvertSharedFilePath(std::string path, std::string &fullpath)
{
        lockRsCore();     /* LOCK */

	/* call to filedexserver */
	bool val = server->ConvertSharedFilePath(path, fullpath);

	/* done! */
        /* unlock Mutexes */
        unlockRsCore();     /* UNLOCK */

	return val;
}


void RsServer::ForceDirectoryCheck()
{
        lockRsCore();     /* LOCK */

	/* call to filedexserver */
	server->ForceDirectoryCheck();

	/* done! */
        /* unlock Mutexes */
        unlockRsCore();     /* UNLOCK */
}


bool RsServer::InDirectoryCheck()
{
        lockRsCore();     /* LOCK */

	/* call to filedexserver */
	bool val = server->InDirectoryCheck();

	/* done! */
        /* unlock Mutexes */
        unlockRsCore();     /* UNLOCK */

	return val;
}

	

