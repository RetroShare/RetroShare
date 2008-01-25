
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

#if 0

unsigned long getMsgId(RsMsgId &id)
{
	unsigned long mid = 0;
	mid = (id.data[0] << 24);
	mid |= (id.data[1] << 16);
	mid |= (id.data[2] << 8);
	mid |= id.data[3];

	return mid;
}

void setMsgId(RsMsgId &rsmid, unsigned int mid)
{
	/* version that uses the uniqueMsgId stored in sid */
	/* 16 Bytes XXX Must be equal! */
	for(int i = 0; i < CHAN_SIGN_SIZE; i++) 
		rsmid.data[i] = 0;

	rsmid.data[0] = (0xff & (mid >> 24));
	rsmid.data[1] = (0xff & (mid >> 16));
	rsmid.data[2] = (0xff & (mid >> 8));
	rsmid.data[3] = (0xff & (mid >> 0));

	return;
}

#endif


/****************************************/
/****************************************/
	/* Message Items */
int RsServer::MessageSend(MessageInfo &info)
{
	/* so we send this.... */
	lockRsCore();     /* LOCK */

	RsMsgItem *msg = new RsMsgItem();

	/* id who it is to ???? handled lower */
	msg -> PeerId("");

	msg -> msgFlags = 0;
	msg -> msgId = 0;
	msg -> sendTime = time(NULL);
	msg -> recvTime = 0;
	
	msg -> subject = info.title;
	msg -> message = info.msg;

	std::list<PersonInfo>::iterator pit;
	for(pit = info.msgto.begin(); pit != info.msgto.end(); pit++)
	{
		msg -> msgto.ids.push_back(pit->id);
	}

	for(pit = info.msgcc.begin(); pit != info.msgcc.end(); pit++)
	{
		msg -> msgcc.ids.push_back(pit->id);
	}

	for(pit = info.msgbcc.begin(); pit != info.msgbcc.end(); pit++)
	{
		msg -> msgbcc.ids.push_back(pit->id);
	}

	msg -> attachment.title   = info.attach_title;
	msg -> attachment.comment = info.attach_comment;

	std::list<FileInfo>::iterator it;
	for(it = info.files.begin(); it != info.files.end(); it++)
	{
		RsTlvFileItem mfi;
		mfi.hash = it -> hash;
		mfi.name = it -> fname;
		mfi.filesize = it -> size;
		msg -> attachment.items.push_back(mfi);
	}

	std::cerr << "RsServer::MessageSend()" << std::endl;
	msg->print(std::cerr);

	msgSrv -> sendMessage(msg);

	unlockRsCore();     /* UNLOCK */

	UpdateAllMsgs();
	return 1;
}

/****************************************/
/****************************************/
int RsServer::MessageDelete(std::string mid)
{
	lockRsCore();     /* LOCK */

	std::cerr << "RsServer::MessageDelete() ";
	std::cerr << "mid: " << mid << std::endl;

	msgSrv -> removeMsgId(atoi(mid.c_str()));

	unlockRsCore();     /* UNLOCK */

	UpdateAllMsgs();
	return 1;
}

int RsServer::MessageRead(std::string mid)
{
	lockRsCore();     /* LOCK */

	std::cerr << "RsServer::MessageRead() ";
	std::cerr << "mid: " << mid << std::endl;

	msgSrv -> markMsgIdRead(atoi(mid.c_str()));

	unlockRsCore();     /* UNLOCK */

	// (needed?) UpdateAllMsgs();
	return 1;
}

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
int 	RsServer::ChatSend(ChatInfo &ci)
{
	lockRsCore();     /* LOCK */
	/* send a message to all for now */
	if (ci.chatflags & RS_CHAT_PRIVATE)
	{
	  chatSrv -> sendPrivateChat(ci.msg, ci.rsid);
	}
	else
	{
	  /* global */
	  chatSrv -> sendChat(ci.msg);
	}
	unlockRsCore();     /* UNLOCK */

	UpdateAllChat();
	return 1;
}

int 	RsServer::UpdateAllChat()
{
	RsIface &iface = getIface();

	/* lock Mutexes */
	lockRsCore();     /* LOCK */
	iface.lockData(); /* LOCK */

	/* get any messages and push them to iface */

	// get the items from the list.
	std::list<RsChatItem *> clist = chatSrv -> getChatQueue();
	std::list<RsChatItem *>::iterator it;
	for(it = clist.begin(); it != clist.end(); it++)
	{
		ChatInfo ci;
		initRsChatInfo((*it), ci);
		iface.mChatList.push_back(ci);
		delete (*it);
	}

  	iface.setChanged(RsIface::Chat);

	/* unlock Mutexes */
	iface.unlockData(); /* UNLOCK */
	unlockRsCore();     /* UNLOCK */

	return 1;
}

/****************************************/
/****************************************/
int     RsServer::UpdateAllMsgs()
{
        NotifyBase &cb = getNotify();
        cb.notifyListPreChange(NOTIFY_LIST_MESSAGELIST, NOTIFY_TYPE_MOD);

	RsIface &iface = getIface();

	/* lock Mutexes */
	lockRsCore();     /* LOCK */
	iface.lockData(); /* LOCK */

	/* do stuff */
	std::list<RsMsgItem *> &msglist =    msgSrv -> getMsgList();
	std::list<RsMsgItem *> &msgOutlist = msgSrv -> getMsgOutList();
  	std::list<RsMsgItem *>::iterator mit;

	std::list<MessageInfo> &msgs = iface.mMessageList;

	msgs.clear();

	for(mit = msglist.begin(); mit != msglist.end(); mit++)
	{
		MessageInfo mi;
		initRsMI(*mit, mi);
		msgs.push_back(mi);
	}

	for(mit = msgOutlist.begin(); mit != msgOutlist.end(); mit++)
	{
		MessageInfo mi;
		initRsMI(*mit, mi);
		msgs.push_back(mi);
	}
  
  	iface.setChanged(RsIface::Message);

	/* unlock Mutexes */
	iface.unlockData(); /* UNLOCK */
	unlockRsCore();     /* UNLOCK */

        cb.notifyListChange(NOTIFY_LIST_MESSAGELIST, NOTIFY_TYPE_MOD);

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

void RsServer::initRsChatInfo(RsChatItem *c, ChatInfo &i)
{
	i.rsid = c -> PeerId();
	i.name = mAuthMgr->getName(i.rsid);

	i.msg  = c -> message;
        if (c -> chatFlags & RS_CHAT_FLAG_PRIVATE)
	{
		std::cerr << "RsServer::initRsChatInfo() Chat Private!!!";
		i.chatflags = RS_CHAT_PRIVATE;
	}
	else
	{
		i.chatflags = RS_CHAT_PUBLIC;
		std::cerr << "RsServer::initRsChatInfo() Chat Public!!!";
	}
	std::cerr << std::endl;
}

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


void RsServer::initRsMI(RsMsgItem *msg, MessageInfo &mi)
{

	mi.msgflags = 0;

	/* translate flags, if we sent it... outgoing */
	if ((msg->msgFlags & RS_MSG_FLAGS_OUTGOING)
	   || (msg->PeerId() == mAuthMgr->OwnId()))
	{
		mi.msgflags |= RS_MSG_OUTGOING;
	}
	/* if it has a pending flag, then its in the outbox */
	if (msg->msgFlags & RS_MSG_FLAGS_PENDING)
	{
		mi.msgflags |= RS_MSG_PENDING;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_NEW)
	{
		mi.msgflags |= RS_MSG_NEW;
	}

	mi.id = msg->PeerId();
	mi.srcname = mAuthMgr->getName(mi.id);

	std::list<std::string>::iterator pit;

	for(pit = msg->msgto.ids.begin(); 
		pit != msg->msgto.ids.end(); pit++)
	{
		PersonInfo pi;
		pi.id = (*pit);
		pi.name = mAuthMgr->getName(pi.id);
		mi.msgto.push_back(pi);
	}

	for(pit = msg->msgcc.ids.begin(); 
		pit != msg->msgcc.ids.end(); pit++)
	{
		PersonInfo pi;
		pi.id = (*pit);
		pi.name = mAuthMgr->getName(pi.id);
		mi.msgcc.push_back(pi);
	}

	for(pit = msg->msgbcc.ids.begin(); 
		pit != msg->msgbcc.ids.end(); pit++)
	{
		PersonInfo pi;
		pi.id = (*pit);
		pi.name = mAuthMgr->getName(pi.id);
		mi.msgbcc.push_back(pi);
	}

	mi.title = msg->subject;
	mi.msg   = msg->message;

	mi.attach_title = msg->attachment.title;
	mi.attach_comment = msg->attachment.comment;

	mi.count = 0;
	mi.size = 0;

	std::list<RsTlvFileItem>::iterator it;
	for(it = msg->attachment.items.begin(); 
			it != msg->attachment.items.end(); it++)
	{
		FileInfo fi;
	        fi.fname = RsDirUtil::getTopDir(it->name);
		fi.size  = it->filesize;
		fi.hash  = it->hash;
		fi.path  = it->path;
		mi.files.push_back(fi);
		mi.count++;
		mi.size += fi.size;
	}

	mi.ts = msg->sendTime;
	mi.msgId = msg->msgId;
}

        /* Flagging Persons / Channels / Files in or out of a set (CheckLists) */
int     RsServer::ClearInChat()
{
	lockRsCore(); /* LOCK */
#if 0
	std::list<cert *>::iterator it;
	std::list<cert *> &certs = sslr -> getCertList();
	
	for(it = certs.begin(); it != certs.end(); it++)
	{
		(*it)->InChat(false);
	}

	sslr->IndicateCertsChanged();
#endif
	unlockRsCore();   /* UNLOCK */

	return 1;
}


        /* Flagging Persons / Channels / Files in or out of a set (CheckLists) */
int     RsServer::SetInChat(std::string id, bool in)             /* friend : chat msgs */
{
	/* so we send this.... */
	lockRsCore();     /* LOCK */

#if 0
	RsCertId rsid(id);
	cert *c = intFindCert(rsid);

	if (c)
	{
		c->InChat(in);
		sslr->IndicateCertsChanged();
		std::cerr << "Set InChat(" << id << ") to " << (in ? "True" : "False") << std::endl;
	}
	else
	{
		std::cerr << "FAILED TO Set InChat(" << id << ") to " << (in ? "True" : "False") << std::endl;
	}
#endif

	unlockRsCore();   /* UNLOCK */

	//UpdateAllCerts();
	return 1;
}


int     RsServer::ClearInMsg()
{
	lockRsCore(); /* LOCK */

#if 0
	std::list<cert *>::iterator it;
	std::list<cert *> &certs = sslr -> getCertList();
	
	for(it = certs.begin(); it != certs.end(); it++)
	{
		(*it)->InMessage(false);
	}
	sslr->IndicateCertsChanged();
#endif

	unlockRsCore();   /* UNLOCK */

	return 1;
}


int     RsServer::SetInMsg(std::string id, bool in)             /* friend : msgs */
{
	/* so we send this.... */
	lockRsCore();     /* LOCK */

#if 0
	RsCertId rsid(id);
	cert *c = intFindCert(rsid);

	if (c)
	{
		c->InMessage(in);
		sslr->IndicateCertsChanged();
		std::cerr << "Set InMsg(" << id << ") to " << (in ? "True" : "False") << std::endl;
	}
	else
	{
		std::cerr << "FAILED to Set InMsg(" << id << ") to " << (in ? "True" : "False") << std::endl;

	}
#endif


	unlockRsCore();   /* UNLOCK */

	//UpdateAllCerts();
	return 1;
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






