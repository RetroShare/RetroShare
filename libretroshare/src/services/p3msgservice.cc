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
#include "pqi/pqidebug.h"

#include "services/p3msgservice.h"

#include <sstream>
#include <iomanip>


/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)

#include "pqi/xpgpcert.h"

#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#include "pqi/sslcert.h"

#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/


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

static unsigned int msgUniqueId = 1;
unsigned int getNewUniqueMsgId()
{
	return msgUniqueId++;
}

p3MsgService::p3MsgService()
	:p3Service(RS_SERVICE_TYPE_MSG), 
	msgChanged(1), msgMajorChanged(1)
{
	sslr = getSSLRoot();
}


int	p3MsgService::tick()
{
	pqioutput(PQL_DEBUG_BASIC, msgservicezone, 
		"p3MsgService::tick()");

	/* don't worry about increasing tick rate! 
	 * (handled by p3service)
	 */

	incomingMsgs(); 
	checkOutgoingMessages(); 

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
		std::string mesg;

		if (mi -> PeerId() == sslr->getOwnCert()->PeerId())
		{
			/* from the loopback device */
			mi -> msgFlags = RS_MSG_FLAGS_OUTGOING;
		}
		else
		{
			/* from a peer */
			mi -> msgFlags = 0;
		}

		/* new as well! */
		mi -> msgFlags |= RS_MSG_FLAGS_NEW;

		/* STORE MsgID */
		mi -> msgId = getNewUniqueMsgId();

		imsg.push_back(mi);
		msgChanged.IndicateChanged();
	}
	return 1;
}


std::list<RsMsgItem *> &p3MsgService::getMsgList()
{
	return imsg;
}

std::list<RsMsgItem *> &p3MsgService::getMsgOutList()
{
	return msgOutgoing;
}

/* remove based on the unique mid (stored in sid) */
int     p3MsgService::removeMsgId(uint32_t mid)
{
	std::list<RsMsgItem *>::iterator it;

	for(it = imsg.begin(); it != imsg.end(); it++)
	{
		if ((*it)->msgId == mid)
		{
			RsMsgItem *mi = (*it);
			imsg.erase(it);
			delete mi;
			msgChanged.IndicateChanged();
			msgMajorChanged.IndicateChanged();
			return 1;
		}
	}

	/* try with outgoing messages otherwise */
	for(it = msgOutgoing.begin(); it != msgOutgoing.end(); it++)
	{
		if ((*it)->msgId == mid)
		{
			RsMsgItem *mi = (*it);
			msgOutgoing.erase(it);
			delete mi;
			msgChanged.IndicateChanged();
			msgMajorChanged.IndicateChanged();
			return 1;
		}
	}

	return 0;
}

int     p3MsgService::markMsgIdRead(uint32_t mid)
{
	std::list<RsMsgItem *>::iterator it;

	for(it = imsg.begin(); it != imsg.end(); it++)
	{
		if ((*it)->msgId == mid)
		{
			RsMsgItem *mi = (*it);
			mi -> msgFlags &= ~(RS_MSG_FLAGS_NEW);
			msgChanged.IndicateChanged();
			return 1;
		}
	}
	return 0;
}

int     p3MsgService::sendMessage(RsMsgItem *item)
{
	pqioutput(PQL_DEBUG_BASIC, msgservicezone, 
		"p3MsgService::sendMessage()");

	/* add pending flag */
	item->msgFlags |= 
		(RS_MSG_FLAGS_OUTGOING | 
		 RS_MSG_FLAGS_PENDING);
	/* STORE MsgID */
	item -> msgId = getNewUniqueMsgId();
	msgOutgoing.push_back(item);

	return 1;
}

int     p3MsgService::checkOutgoingMessages()
{
	/* iterate through the outgoing queue 
	 *
	 * if online, send
	 */

	std::list<RsMsgItem *>::iterator it;
	for(it = msgOutgoing.begin(); it != msgOutgoing.end();)
	{

		/* find the certificate */
		certsign sign;
		convert_to_certsign((*it)->PeerId(), sign);
		cert *peer = sslr -> findcertsign(sign);

	 	/* if online, send it */
		if ((peer -> Status() & PERSON_STATUS_CONNECTED) 
		    || (peer == sslr->getOwnCert()))
		{
			/* send msg */
			pqioutput(PQL_ALERT, msgservicezone, 
				"p3MsgService::checkOutGoingMessages() Sending out message");
			/* remove the pending flag */
			(*it)->msgFlags &= ~RS_MSG_FLAGS_PENDING;

			sendItem(*it);
			it = msgOutgoing.erase(it);
		}
		else
		{
			pqioutput(PQL_ALERT, msgservicezone, 
				"p3MsgService::checkOutGoingMessages() Delaying until available...");
			it++;
		}
	}
	return 0;
}


int     p3MsgService::save_config()
{
	std::list<std::string>::iterator it;
	std::string empty("");

	pqioutput(PQL_DEBUG_BASIC, msgservicezone, 
		"p3MsgService::save_config()");

	/* now we create a pqiarchive, and stream all the msgs into it
	 */

	std::string statelog = config_dir + "/msgs.rst";
	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsMsgSerialiser());

	BinFileInterface *out = new BinFileInterface((char *) statelog.c_str(), BIN_FLAGS_WRITEABLE);
        pqiarchive *pa_out = new pqiarchive(rss, out, BIN_FLAGS_WRITEABLE | BIN_FLAGS_NO_DELETE);
	bool written = false;

	std::list<RsMsgItem *>::iterator mit;
	for(mit = imsg.begin(); mit != imsg.end(); mit++)
	{
		//RsMsgItem *mi = (*mit)->clone();
		if (pa_out -> SendItem(*mit))
		{
			written = true;
		}
		
	}

	for(mit = msgOutgoing.begin(); mit != msgOutgoing.end(); mit++)
	{
		//RsMsgItem *mi = (*mit)->clone();
		//mi -> msgFlags |= RS_MSG_FLAGS_PENDING;
		if (pa_out -> SendItem(*mit))
		{
			written = true;
		}
		
	}

	if (!written)
	{
		/* need to push something out to overwrite old data! (For WINDOWS ONLY) */
	}

	delete pa_out;	
	return 1;
}

int     p3MsgService::load_config()
{
	std::list<std::string>::iterator it;

	std::string empty("");
	std::string dir("notempty");
	std::string str_true("true");

	/* load msg/ft */
	std::string statelog = config_dir + "/msgs.rst";

	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsMsgSerialiser());

	BinFileInterface *in = new BinFileInterface((char *) statelog.c_str(), BIN_FLAGS_READABLE);
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
				std::cerr << "MSG_PENDING";
				std::cerr << std::endl;
				mitem->print(std::cerr);
				msgOutgoing.push_back(mitem);
			}
			else
			{
				imsg.push_back(mitem);
			}
		}
		else
		{
			delete item;
		}
	}

	delete pa_in;	

	return 1;
}


void p3MsgService::loadWelcomeMsg()
{
	/* Load Welcome Message */
	RsMsgItem *msg = new RsMsgItem();

	msg -> PeerId(sslr->getOwnCert()->PeerId());

	msg -> sendTime = 0;

	msg -> subject = "Welcome to Retroshare";

	msg -> message    = "Send and receive messages\n"; 
	msg -> message   += "with your friends...\n\n";

	msg -> message   += "These can hold recommendations\n";
	msg -> message   += "from your local shared files\n\n";

	msg -> message   += "Add recommendations through\n";
	msg -> message   += "the Local Files Dialog\n\n";

	msg -> message   += "Enjoy.\n";

	imsg.push_back(msg);	
}



