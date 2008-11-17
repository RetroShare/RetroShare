/*
 * libretroshare/src/services msgservice.cc
 *
 * Services for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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


#include "pqi/pqibin.h"
#include "pqi/pqiarchive.h"
#include "pqi/p3connmgr.h"

#include "services/p3msgservice.h"
#include "pqi/pqinotify.h"

#include "util/rsdebug.h"
#include "util/rsdir.h"

#include <sstream>
#include <iomanip>

const int msgservicezone = 54319;

/* Another little hack ..... unique message Ids
 * will be handled in this class.....
 * These are unique within this run of the server, 
 * and are not stored long term....
 *
 * Only 3 entry points:
 * (1) from network....
 * (2) from local send
 * (3) from storage...
 */


p3MsgService::p3MsgService(p3ConnectMgr *cm)
	:p3Service(RS_SERVICE_TYPE_MSG), pqiConfig(CONFIG_TYPE_MSGS), 
	mConnMgr(cm), msgChanged(1), mMsgUniqueId(1)
{
	addSerialType(new RsMsgSerialiser());
}

uint32_t p3MsgService::getNewUniqueMsgId()
{
	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/
	return mMsgUniqueId++;
}

/****** Mods/Notifications ****/

bool	p3MsgService::MsgsChanged()
{
	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	bool m1 = msgChanged.Changed();

	return (m1);
}

bool	p3MsgService::MsgNotifications()
{
	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/
	return (msgNotifications.size() > 0);
}

bool    p3MsgService::getMessageNotifications(std::list<MsgInfoSummary> &noteList)
{
	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	noteList = msgNotifications;
	msgNotifications.clear();
	
	return (noteList.size() > 0);
}



int	p3MsgService::tick()
{
	pqioutput(PQL_DEBUG_BASIC, msgservicezone, 
		"p3MsgService::tick()");

	/* don't worry about increasing tick rate! 
	 * (handled by p3service)
	 */

	incomingMsgs(); 
	//checkOutgoingMessages(); 

	return 0;
}


int	p3MsgService::status()
{
	pqioutput(PQL_DEBUG_BASIC, msgservicezone, 
		"p3MsgService::status()");

	return 1;
}

int 	p3MsgService::incomingMsgs()
{
	RsMsgItem *mi;
	int i = 0;
	while((mi = (RsMsgItem *) recvItem()) != NULL)
	{
		++i;
		mi -> recvTime = time(NULL);
		mi -> msgFlags = RS_MSG_FLAGS_NEW;
		mi -> msgId = getNewUniqueMsgId();

		std::string mesg;

		RsStackMutex stack(mMsgMtx); /*** STACK LOCKED MTX ***/

		if (mi -> PeerId() == mConnMgr->getOwnId())
		{
			/* from the loopback device */
			mi -> msgFlags |= RS_MSG_FLAGS_OUTGOING;
		}
		else
		{
			/* from a peer */
			MsgInfoSummary mis;
			initRsMIS(mi, mis);
			
			// msgNotifications.push_back(mis);
			pqiNotify *notify = getPqiNotify();
			if (notify)
			{
				notify->AddPopupMessage(RS_POPUP_MSG, mi->PeerId(), 
						"from: ");

				std::ostringstream out;
				out << mi->msgId;
				notify->AddFeedItem(RS_FEED_ITEM_MESSAGE, out.str(), "", "");
			}
		}

		imsg[mi->msgId] = mi;
		msgChanged.IndicateChanged();
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

		/**** STACK UNLOCKED ***/
	}
	return 1;
}

void    p3MsgService::statusChange(const std::list<pqipeer> &plist)
{
	/* should do it properly! */
	checkOutgoingMessages();
}

int     p3MsgService::checkOutgoingMessages()
{
	/* iterate through the outgoing queue 
	 *
	 * if online, send
	 */

	const std::string ownId = mConnMgr->getOwnId();

	std::list<uint32_t>::iterator it;
	std::list<uint32_t> toErase;

	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	std::map<uint32_t, RsMsgItem *>::iterator mit;
	for(mit = msgOutgoing.begin(); mit != msgOutgoing.end(); mit++)
	{

		/* find the certificate */
		std::string pid = mit->second->PeerId();
		peerConnectState pstate;
		bool toSend = false;

		if (mConnMgr->getFriendNetStatus(pid, pstate))
		{
			if (pstate.state & RS_PEER_S_CONNECTED)
			{
				toSend = true;
			}
		}
		else if (pid == ownId) /* FEEDBACK Msg to Ourselves */
		{
			toSend = true;
		}

		if (toSend)
		{
			/* send msg */
			pqioutput(PQL_DEBUG_BASIC, msgservicezone, 
				"p3MsgService::checkOutGoingMessages() Sending out message");
			/* remove the pending flag */
			(mit->second)->msgFlags &= ~RS_MSG_FLAGS_PENDING;

			sendItem(mit->second);
			toErase.push_back(mit->first);
		}
		else
		{
			pqioutput(PQL_DEBUG_BASIC, msgservicezone, 
				"p3MsgService::checkOutGoingMessages() Delaying until available...");
		}
	}

	/* clean up */
	for(it = toErase.begin(); it != toErase.end(); it++)
	{
		mit = msgOutgoing.find(*it);
		if (mit != msgOutgoing.end())
		{
			msgOutgoing.erase(mit);
		}
	}

	if (toErase.size() > 0)
	{
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	}

	return 0;
}




bool    p3MsgService::saveConfiguration()
{
	std::list<std::string>::iterator it;
	std::string empty("");

	pqioutput(PQL_DEBUG_BASIC, msgservicezone, 
		"p3MsgService::save_config()");

	/* now we create a pqiarchive, and stream all the msgs into it
	 */

	std::string msgfile = Filename();

	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsMsgSerialiser());

	BinFileInterface *out = new BinFileInterface(msgfile.c_str(), BIN_FLAGS_WRITEABLE | BIN_FLAGS_HASH_DATA);
        pqiarchive *pa_out = new pqiarchive(rss, out, BIN_FLAGS_WRITEABLE | BIN_FLAGS_NO_DELETE);
	bool written = false;

	std::map<uint32_t, RsMsgItem *>::iterator mit;
	for(mit = imsg.begin(); mit != imsg.end(); mit++)
	{
		if (pa_out -> SendItem(mit->second))
		{
			written = true;
		}
		
	}

	for(mit = msgOutgoing.begin(); mit != msgOutgoing.end(); mit++)
	{
		if (pa_out -> SendItem(mit->second))
		{
			written = true;
		}
		
	}

	setHash(out->gethash());

	delete pa_out;	
	return true;
}

bool    p3MsgService::loadConfiguration(std::string &loadHash)
{
	std::list<std::string>::iterator it;

	std::string msgfile = Filename();

	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsMsgSerialiser());

	BinFileInterface *in = new BinFileInterface(msgfile.c_str(), BIN_FLAGS_READABLE | BIN_FLAGS_HASH_DATA);
        pqiarchive *pa_in = new pqiarchive(rss, in, BIN_FLAGS_READABLE);
	RsItem *item;
	RsMsgItem *mitem;


	while((item = pa_in -> GetItem()))
	{
		if (NULL != (mitem = dynamic_cast<RsMsgItem *>(item)))
		{
			/* switch depending on the PENDING 
			 * flags
			 */
			/* STORE MsgID */
			mitem->msgId = getNewUniqueMsgId();
			if (mitem -> msgFlags & RS_MSG_FLAGS_PENDING)
			{
				RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

				//std::cerr << "MSG_PENDING";
				//std::cerr << std::endl;
				//mitem->print(std::cerr);

				msgOutgoing[mitem->msgId] = mitem;
			}
			else
			{
				RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

				imsg[mitem->msgId] = mitem;
			}
		}
		else
		{
			delete item;
		}
	}

	std::string hashin = in->gethash();

	delete pa_in;	

	if (hashin != loadHash)
	{
		/* big error message! */
		std::cerr << "p3MsgService::loadConfiguration() FAILED! Msgs Tampered" << std::endl;
		std::string msgfileold = msgfile + ".failed";

		rename(msgfile.c_str(), msgfileold.c_str());

		std::cerr << "Moving Old file to: " << msgfileold << std::endl;
		std::cerr << "removing dodgey msgs" << std::endl;

		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/


		std::map<uint32_t, RsMsgItem *>::iterator mit;
		for(mit = imsg.begin(); mit != imsg.end(); mit++)
		{
			delete (mit->second);
		}
		imsg.clear();

		for(mit = msgOutgoing.begin(); mit != msgOutgoing.end(); mit++)
		{
			delete (mit->second);
		}
		msgOutgoing.clear();
		setHash("");
		return false;

	}

	setHash(hashin);

	return true;
}


void p3MsgService::loadWelcomeMsg()
{
	/* Load Welcome Message */
	RsMsgItem *msg = new RsMsgItem();

	msg -> PeerId(mConnMgr->getOwnId());

	msg -> sendTime = time(NULL);

	msg -> subject = L"Welcome to Retroshare";

	msg -> message    = L"Send and receive messages\n"; 
	msg -> message   += L"with your friends...\n\n";

	msg -> message   += L"These can hold recommendations\n";
	msg -> message   += L"from your local shared files\n\n";

	msg -> message   += L"Add recommendations through\n";
	msg -> message   += L"the Local Files Dialog\n\n";

	msg -> message   += L"Enjoy.\n";

	msg -> msgId = getNewUniqueMsgId();

	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	imsg[msg->msgId] = msg;	
}


/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/


/****************************************/
/****************************************/

bool p3MsgService::getMessageSummaries(std::list<MsgInfoSummary> &msgList)
{
	/* do stuff */
	msgList.clear();

	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	std::map<uint32_t, RsMsgItem *>::iterator mit;
	for(mit = imsg.begin(); mit != imsg.end(); mit++)
	{
		MsgInfoSummary mis;
		initRsMIS(mit->second, mis);
		msgList.push_back(mis);
	}

	for(mit = msgOutgoing.begin(); mit != msgOutgoing.end(); mit++)
	{
		MsgInfoSummary mis;
		initRsMIS(mit->second, mis);
		msgList.push_back(mis);
	}
	return 1;
}


bool p3MsgService::getMessage(std::string mId, MessageInfo &msg)
{
  	std::map<uint32_t, RsMsgItem *>::iterator mit;
	uint32_t msgId = atoi(mId.c_str());

	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	mit = imsg.find(msgId);
	if (mit == imsg.end())
	{
		mit = msgOutgoing.find(msgId);
		if (mit == msgOutgoing.end())
		{
			return false;
		}
	}

	/* mit valid */
	initRsMI(mit->second, msg);
	return true;
}


/* remove based on the unique mid (stored in sid) */
bool    p3MsgService::removeMsgId(std::string mid)
{
  	std::map<uint32_t, RsMsgItem *>::iterator mit;
	uint32_t msgId = atoi(mid.c_str());

	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	mit = imsg.find(msgId);
	if (mit != imsg.end())
	{
		RsMsgItem *mi = mit->second;
		imsg.erase(mit);
		delete mi;
		msgChanged.IndicateChanged();

		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

		return true;
	}

	mit = msgOutgoing.find(msgId);
	if (mit != msgOutgoing.end())
	{
		RsMsgItem *mi = mit->second;
		msgOutgoing.erase(mit);
		delete mi;
		msgChanged.IndicateChanged();

		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

		return true;
	}

	return false;
}

bool    p3MsgService::markMsgIdRead(std::string mid)
{
  	std::map<uint32_t, RsMsgItem *>::iterator mit;
	uint32_t msgId = atoi(mid.c_str());

	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	mit = imsg.find(msgId);
	if (mit != imsg.end())
	{
		RsMsgItem *mi = mit->second;
		mi -> msgFlags &= ~(RS_MSG_FLAGS_NEW);
		msgChanged.IndicateChanged();
	
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
		return true;
	}
	return false;
}

/****************************************/
/****************************************/
	/* Message Items */
int     p3MsgService::sendMessage(RsMsgItem *item)
{
	pqioutput(PQL_DEBUG_BASIC, msgservicezone, 
		"p3MsgService::sendMessage()");

	item -> msgId = getNewUniqueMsgId(); /* grabs Mtx as well */

      {
	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	/* add pending flag */
	item->msgFlags |= 
		(RS_MSG_FLAGS_OUTGOING | 
		 RS_MSG_FLAGS_PENDING);
	/* STORE MsgID */
	msgOutgoing[item->msgId] = item;
      }

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	checkOutgoingMessages(); 

	return 1;
}

bool 	p3MsgService::MessageSend(MessageInfo &info)
{
	std::list<std::string>::const_iterator pit;
	for(pit = info.msgto.begin(); pit != info.msgto.end(); pit++)
	{
		RsMsgItem *msg = initMIRsMsg(info, *pit);
		if (msg)
		{
			sendMessage(msg);
		}
	}

	for(pit = info.msgcc.begin(); pit != info.msgcc.end(); pit++)
	{
		RsMsgItem *msg = initMIRsMsg(info, *pit);
		if (msg)
		{
			sendMessage(msg);
		}
	}

	for(pit = info.msgbcc.begin(); pit != info.msgbcc.end(); pit++)
	{
		RsMsgItem *msg = initMIRsMsg(info, *pit);
		if (msg)
		{
			sendMessage(msg);
		}
	}

	/* send to ourselves as well */
	RsMsgItem *msg = initMIRsMsg(info, mConnMgr->getOwnId());
	if (msg)
	{
		sendMessage(msg);
	}

	return true;
}

/****************************************/
/****************************************/


/****************************************/

/**** HELPER FNS For Chat/Msg/Channel Lists ************
 * These aren't required to be locked, unless
 * the data used is from internal stores -> then they should be.
 */

void p3MsgService::initRsMI(RsMsgItem *msg, MessageInfo &mi)
{

	mi.msgflags = 0;

	/* translate flags, if we sent it... outgoing */
	if ((msg->msgFlags & RS_MSG_FLAGS_OUTGOING)
	   || (msg->PeerId() == mConnMgr->getOwnId()))
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

	mi.ts = msg->sendTime;
	mi.srcId = msg->PeerId();
	{
		//msg->msgId;
		std::ostringstream out;
		out << msg->msgId;
		mi.msgId = out.str();
	}

	std::list<std::string>::iterator pit;

	for(pit = msg->msgto.ids.begin(); 
		pit != msg->msgto.ids.end(); pit++)
	{
		mi.msgto.push_back(*pit);
	}

	for(pit = msg->msgcc.ids.begin(); 
		pit != msg->msgcc.ids.end(); pit++)
	{
		mi.msgcc.push_back(*pit);
	}

	for(pit = msg->msgbcc.ids.begin(); 
		pit != msg->msgbcc.ids.end(); pit++)
	{
		mi.msgbcc.push_back(*pit);
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

}


void p3MsgService::initRsMIS(RsMsgItem *msg, MsgInfoSummary &mis)
{
	mis.msgflags = 0;

	/* translate flags, if we sent it... outgoing */
	if ((msg->msgFlags & RS_MSG_FLAGS_OUTGOING)
	   || (msg->PeerId() == mConnMgr->getOwnId()))
	{
		mis.msgflags |= RS_MSG_OUTGOING;
	}
	/* if it has a pending flag, then its in the outbox */
	if (msg->msgFlags & RS_MSG_FLAGS_PENDING)
	{
		mis.msgflags |= RS_MSG_PENDING;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_NEW)
	{
		mis.msgflags |= RS_MSG_NEW;
	}

	mis.srcId = msg->PeerId();
	{
		//msg->msgId;
		std::ostringstream out;
		out << msg->msgId;
		mis.msgId = out.str();
	}

	mis.title = msg->subject;
	mis.count = msg->attachment.items.size();
	mis.ts = msg->sendTime;
}



RsMsgItem *p3MsgService::initMIRsMsg(MessageInfo &info, std::string to)
{
	RsMsgItem *msg = new RsMsgItem();

	msg -> PeerId(to);

	msg -> msgFlags = 0;
	msg -> msgId = 0;
	msg -> sendTime = time(NULL);
	msg -> recvTime = 0;
	
	msg -> subject = info.title;
	msg -> message = info.msg;

	std::list<std::string>::iterator pit;
	for(pit = info.msgto.begin(); pit != info.msgto.end(); pit++)
	{
		msg -> msgto.ids.push_back(*pit);
	}

	for(pit = info.msgcc.begin(); pit != info.msgcc.end(); pit++)
	{
		msg -> msgcc.ids.push_back(*pit);
	}

	/* We don't fill in bcc (unless to ourselves) */
	if (to == mConnMgr->getOwnId())
	{
		for(pit = info.msgbcc.begin(); pit != info.msgbcc.end(); pit++)
		{
			msg -> msgbcc.ids.push_back(*pit);
		}
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

	//std::cerr << "p3MsgService::initMIRsMsg()" << std::endl;
	//msg->print(std::cerr);
	return msg;
}

