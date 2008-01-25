
/*
 * "$Id: rsiface.cc,v 1.6 2007-04-15 18:45:23 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
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



#include "rsiface/rsiface.h"
#include "util/rsdir.h"
			
const DirInfo *RsIface::getDirectory(std::string id, std::string path)
{
	const DirInfo *dir = getDirectoryMod(id, path);
	return dir;
}


const PersonInfo *RsIface::getPerson(std::string id)
{
	const PersonInfo *pi = getPersonMod(id);
	return pi;
}

PersonInfo *RsIface::getPersonMod(std::string uid)
{
	RsCertId id(uid);

	/* get the Root of the Directories */
	std::list<PersonInfo>::iterator pit;

	/* check if local */
	for(pit = mLocalDirList.begin();
		(pit != mLocalDirList.end()) && (pit->id != id); pit++);

	if (pit == mLocalDirList.end())
	{
		/* check the remote ones */
		for(pit = mRemoteDirList.begin();
		    (pit != mRemoteDirList.end()) && (pit->id != id); pit++);

		/* bailout ...? */
		if (pit == mRemoteDirList.end())
		{
			return NULL;
		}
	}
	return &(*pit);
}


DirInfo *RsIface::getDirectoryMod(std::string uid, std::string path)
{
	RsCertId id(uid);

	/* get the Root of the Directories */
	std::list<DirInfo>::iterator dit;

	std::list<std::string> subdirs;
	std::list<std::string>::iterator sit;

	PersonInfo *pi = getPersonMod(uid);
	if (!pi)
	{
		return NULL;
	}

	/* have the correct person now */
	RsDirUtil::breakupDirList(path, subdirs);

	DirInfo *node = &(pi -> rootdir);

	for(sit = subdirs.begin(); sit != subdirs.end(); sit++)
	{
		for(dit = node->subdirs.begin();
			(dit != node->subdirs.end()) && 
			(dit->dirname != *sit); dit++);

		if (dit == node->subdirs.end())
		{
			/* Directory don't exist..... */
			return NULL;
		}
		else
		{
			node = &(*dit);
		}
	}
	return node;
}




const MessageInfo *RsIface::getMessage(std::string cid_in, std::string mid_in)
{
	/* check for this message */
	std::list<MessageInfo>::iterator it;

	RsCertId cId(cid_in);
	RsMsgId mId(mid_in);

	std::cerr << "RsIface::getMessage()" << std::endl;
	std::cerr << "cid: " << cid_in << " -> cId " << cId << std::endl;
	std::cerr << "mid: " << mid_in << " -> mId " << mId << std::endl;

	for(it = mMessageList.begin(); it != mMessageList.end(); it++)
	{
	std::cerr << "VS: cid: " << it->id << std::endl;
	std::cerr << "VS: mid: " << it->msgId << std::endl;
		if ((it->id == cId) && (mId == it->msgId))
		{
		std::cerr << "MATCH!" << std::endl;
			return &(*it);
		}
	}

	std::cerr << "NO MATCH :(" << std::endl;
	return NULL;
}


const MessageInfo *RsIface::getChannelMsg(std::string chid_in, std::string mid_in)
{
	RsChanId cId(chid_in);
	RsMsgId  mId(mid_in);

	std::map<RsChanId, ChannelInfo>::iterator it;
	it = mChannelMap.find(cId);

	if (it == mChannelMap.end())
	{
		it = mChannelOwnMap.find(cId);

		if (it == mChannelOwnMap.end())
		{
			return NULL;
		}
	}

	/* else */
	std::list<MessageInfo>::iterator it2;
	std::list<MessageInfo> &msgs = (it->second).msglist;
	for(it2 = msgs.begin(); it2 != msgs.end(); it2++)
	{
		if (mId == it2->msgId)
		{
			return &(*it2);
		}
	}

	return NULL;
}

/* set to true */
bool    RsIface::setChanged(DataFlags set)
{
	if ((int) set < (int) NumOfFlags)
	{
		/* go for it */
		mChanged[(int) set ] = true;
		return true;
	}
	return false;
}

	
/* leaves it */
bool    RsIface::getChanged(DataFlags set)
{
	if ((int) set < (int) NumOfFlags)
	{
		/* go for it */
		return mChanged[(int) set ];
	}
	return false;
}

/* resets it */
bool    RsIface::hasChanged(DataFlags set)
{
	if ((int) set < (int) NumOfFlags)
	{
		/* go for it */
		if (mChanged[(int) set ])
		{
			mChanged[(int) set ] = false;
			return true;
		}
	}
	return false;
}

/*************************** THE REAL RSIFACE (with MUTEXES) *******/

#include "util/rsthreads.h"

class RsIfaceReal: public RsIface
{
public:
        RsIfaceReal(NotifyBase &callback)
	:RsIface(callback)
	{ return; }

	virtual void lockData()
	{
//		std::cerr << "RsIfaceReal::lockData()" << std::endl;
		return rsIfaceMutex.lock();
	}

	virtual void unlockData()
	{
//		std::cerr << "RsIfaceReal::unlockData()" << std::endl;
		return rsIfaceMutex.unlock();
	}

private:
	RsMutex rsIfaceMutex;
};

RsIface *createRsIface(NotifyBase &cb)
{
	return new RsIfaceReal(cb);
}



