
/*
 * "$Id: p3face-msgs.cc,v 1.7 2007-05-05 16:10:06 rmf24 Exp $"
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


#include "rsserver/p3face.h"
#include "util/rsdir.h"

#include <iostream>
#include <sstream>

#include "util/rsdebug.h"
const int p3facemsgzone = 11453;

#include <sys/time.h>
#include <time.h>


        /* Flagging Persons / Channels / Files in or out of a set (CheckLists) */
int     RsServer::ClearInChat()
{
	lockRsCore(); /* LOCK */

	mInChatList.clear();

	unlockRsCore();   /* UNLOCK */

	return 1;
}


        /* Flagging Persons / Channels / Files in or out of a set (CheckLists) */
int     RsServer::SetInChat(std::string id, bool in)             /* friend : chat msgs */
{
	/* so we send this.... */
	lockRsCore();     /* LOCK */

	//std::cerr << "Set InChat(" << id << ") to " << (in ? "True" : "False") << std::endl;
	std::list<std::string>::iterator it;
	it = std::find(mInChatList.begin(), mInChatList.end(), id);
	if (it == mInChatList.end())
	{
		if (in)
		{
			mInChatList.push_back(id);
		}
	}
	else
	{
		if (!in)
		{
			mInChatList.erase(it);
		}
	}

	unlockRsCore();   /* UNLOCK */

	return 1;
}


int     RsServer::ClearInMsg()
{
	lockRsCore(); /* LOCK */

	mInMsgList.clear();

	unlockRsCore();   /* UNLOCK */

	return 1;
}


int     RsServer::SetInMsg(std::string id, bool in)             /* friend : msgs */
{
	/* so we send this.... */
	lockRsCore();     /* LOCK */

	//std::cerr << "Set InMsg(" << id << ") to " << (in ? "True" : "False") << std::endl;
	std::list<std::string>::iterator it;
	it = std::find(mInMsgList.begin(), mInMsgList.end(), id);
	if (it == mInMsgList.end())
	{
		if (in)
		{
			mInMsgList.push_back(id);
		}
	}
	else
	{
		if (!in)
		{
			mInMsgList.erase(it);
		}
	}

	unlockRsCore();   /* UNLOCK */
	return 1;
}

bool    RsServer::IsInChat(std::string id)  /* friend : chat msgs */
{
	/* so we send this.... */
	lockRsCore();     /* LOCK */

	std::list<std::string>::iterator it;
	it = std::find(mInChatList.begin(), mInChatList.end(), id);
	bool inChat = (it != mInChatList.end());

	unlockRsCore();   /* UNLOCK */

	return inChat;
}

	
bool    RsServer::IsInMsg(std::string id)          /* friend : msg recpts*/
{
	/* so we send this.... */
	lockRsCore();     /* LOCK */

	std::list<std::string>::iterator it;
	it = std::find(mInMsgList.begin(), mInMsgList.end(), id);
	bool inMsg = (it != mInMsgList.end());

	unlockRsCore();   /* UNLOCK */

	return inMsg;
}




int     RsServer::ClearInBroadcast()
{
	return 1;
}

int     RsServer::ClearInSubscribe()
{
	return 1;
}

int     RsServer::SetInBroadcast(std::string id, bool in)        /* channel : channel broadcast */
{
	return 1;
}

int     RsServer::SetInSubscribe(std::string id, bool in)        /* channel : subscribed channels */
{
	return 1;
}

int     RsServer::ClearInRecommend()
{
	/* find in people ... set chat flag */
	RsIface &iface = getIface();
	iface.lockData(); /* LOCK IFACE */

	std::list<FileInfo> &recs = iface.mRecommendList;
	std::list<FileInfo>::iterator it;

	for(it = recs.begin(); it != recs.end(); it++)
	{
	  it -> inRecommend = false;
	}
	
	iface.unlockData(); /* UNLOCK IFACE */

	return 1;
}


int     RsServer::SetInRecommend(std::string id, bool in)        /* file : recommended file */
{
	/* find in people ... set chat flag */
	RsIface &iface = getIface();
	iface.lockData(); /* LOCK IFACE */

	std::list<FileInfo> &recs = iface.mRecommendList;
	std::list<FileInfo>::iterator it;

	for(it = recs.begin(); it != recs.end(); it++)
	{
	  if (it -> fname == id)
	  {
		/* set flag */
		it -> inRecommend = in;
		//std::cerr << "Set InRecommend (" << id << ") to " << (in ? "True" : "False") << std::endl;
	  }
	}
	
	iface.unlockData(); /* UNLOCK IFACE */

	return 1;
}






