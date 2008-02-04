
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

#include "pqi/pqidebug.h"
const int p3facemsgzone = 11453;

#include <sys/time.h>
#include <time.h>

/****************************************/
/****************************************/
int RsServer::ChannelCreateNew(ChannelInfo &info)
{
	UpdateAllChannels();
	return 1;
}

/****************************************/
/****************************************/
int RsServer::ChannelSendMsg(ChannelInfo &info)
{
	UpdateAllChannels();
	return 1;
}

/****************************************/
/****************************************/
int RsServer::UpdateAllChannels()
{

#ifdef PQI_USE_CHANNELS

  std::list<pqichannel *> chanlist;
  std::list<pqichannel *>::iterator cit;

  NotifyBase &cb = getNotify();
  cb.notifyListPreChange(NOTIFY_LIST_CHANNELLIST, NOTIFY_TYPE_MOD);

  RsIface &iface = getIface();

  /* lock Mutexes */
  lockRsCore();     /* LOCK */
  iface.lockData(); /* LOCK */

  std::map<RsChanId, ChannelInfo> &chs = iface.mChannelMap;

  server->getAvailableChannels(chanlist);

  /* empty the old list */
  chs.clear();

  for(cit = chanlist.begin(); cit != chanlist.end(); ++cit)
  {
    ChannelInfo ci;
    initRsCI(*cit, ci);
    intAddChannel(ci);

    {
      std::ostringstream out;
      out << "fltkserver::UpdateAllChannels() Added: ";
      out << ci;
      pqioutput(PQL_DEBUG_BASIC, p3facemsgzone, out.str());
    }

    /* then add msgs */
    std::list<chanMsgSummary> summarylist;
    std::list<chanMsgSummary>::iterator mit;

    channelSign sign = (*cit)->getSign();
    server->getChannelMsgList(sign, summarylist);

    for(mit = summarylist.begin(); mit != summarylist.end(); mit++)
    {
      channelMsg *cm = NULL;
      cm = server->getChannelMsg(sign, mit->mh);

      MessageInfo msg;

      initRsCMI(*cit, cm, msg);
      // the files....
      PQChanItem::FileList::const_iterator it;
      for(it = cm->msg->files.begin(); it != cm->msg->files.end(); ++it)
      {
        FileInfo file;

	/* add to the message */
        //ChanFileDisItem *cfdi = new ChanFileDisItem(it->name, it->size);
	initRsCMFI(*cit, &(*mit), &(*it), file);
	msg.files.push_back(file);

	msg.size += file.size;
	msg.count++;
      } // files loop.

      ci.size += msg.size;
      ci.count += ci.count;

      /* add the Msg? */
      intAddChannelMsg(ci.chanId, msg);

    } // msg loop

  } // channel loop.

  /* Notify of Changes */
  iface.setChanged(RsIface::Channel);

  /* release Mutexes */
  iface.unlockData(); /* UNLOCK */
  unlockRsCore();     /* UNLOCK */

  cb.notifyListChange(NOTIFY_LIST_CHANNELLIST, NOTIFY_TYPE_MOD);
#endif

  return 1;
}

/**** HELPER FNS For Chat/Msg/Channel Lists ************
 *
 * The iface->Mutex is required to be locked
 * for intAddChannel / intAddChannelMsg.
 */

#ifdef PQI_USE_CHANNELS

int RsServer::intAddChannel(ChannelInfo &info)
{
	RsIface &iface = getIface();

	std::map<RsChanId, ChannelInfo> &chs = iface.mChannelMap;
	chs[info.chanId] = info;
	return 1;
}


int RsServer::intAddChannelMsg(RsChanId id, MessageInfo &msg)
{
	RsIface &iface = getIface();

	std::map<RsChanId, ChannelInfo> &chs = iface.mChannelMap;
	std::map<RsChanId, ChannelInfo>::iterator it = chs.find(id);
	if (it != chs.end())
	{
		/* add the message */
		/*
		std::map<MsgId, MessageInfo> &msgs = 
			it -> second.msglist;
		msgs[MsgId] = msg;
		*/
		std::list<MessageInfo> &msgs = 
			it -> second.msglist;
		msgs.push_back(msg);
	}
	return 1;
}

RsChanId RsServer::signToChanId(const channelSign &cs) const
{
	/* hackish here */
	RsChanId id;
	int i;

	for(i = 0; i < CHAN_SIGN_SIZE; i++) /* 16 Bytes XXX Must be equal! */
		id.data[i] = cs.sign[i];

	return id;
}

void RsServer::initRsCI(pqichannel *in, ChannelInfo &out)
{
	out.chanId = signToChanId(in -> getSign());
	out.mode = in -> getMode();
	out.rank = in -> getRanking();
	out.chanName = in -> getName();
	out.count = in -> getMsgCount();
	/*
	out.size = in -> getMsgSize();
	*/
}

void RsServer::initRsCMI(pqichannel *chan, channelMsg *cm, MessageInfo &msg)
{
	msg.title = cm->msg->title;
	msg.msg = cm->msg->msg;

	int i;
	MsgHash h = getMsgHash(cm->msg); /* call from p3channel.h */
	/* Copy MsgId over */
	for(i = 0; i < CHAN_SIGN_SIZE; i++) /* 16 Bytes XXX Must be equal! */
		msg.msgId.data[i] = h.sign[i];

	/* init size to zero */
	msg.size = 0;
	msg.count = 0;
}

void RsServer::initRsCMFI(pqichannel *chan, chanMsgSummary *msg, 
      const PQChanItem::FileItem *cfdi, 
      FileInfo &file)
{
  file.searchId = 0;
  file.path = "";
  file.fname = cfdi -> name;
  file.hash = cfdi -> hash;
  file.ext = "";
  file.size = cfdi -> size;
  file.inRecommend = false;

  /* check the status */
  file.status = FileInfo::kRsFiStatusNone; 
  /* cfdi -> status; */
  if (file.status > FileInfo::kRsFiStatusNone)
  {
    intCheckFileStatus(file);
  }
  else 
  {
    file.avail = 0;
    file.rank = 0;
  }
}

#endif


void RsServer::intCheckFileStatus(FileInfo &file)
{
  /* see if its being transfered */
  file.avail = file.size / 2;
  file.rank = 0;
}


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

	std::cerr << "Set InChat(" << id << ") to " << (in ? "True" : "False") << std::endl;
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

	std::cerr << "Set InMsg(" << id << ") to " << (in ? "True" : "False") << std::endl;
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
		std::cerr << "Set InRecommend (" << id << ") to " << (in ? "True" : "False") << std::endl;
	  }
	}
	
	iface.unlockData(); /* UNLOCK IFACE */

	return 1;
}






