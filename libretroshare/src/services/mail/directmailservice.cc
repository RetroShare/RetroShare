/*
 * libretroshare/src/services directmailservice.cc
 *
 * Services for RetroShare.
 *
 * Copyright 2014 by Robert Fernie.
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


#include "services/mail/directmailservice.h"

#include "util/rsdebug.h"

//#define MSG_DEBUG 1

const int directmailzone = 54319;

using namespace Rs::Mail;


DirectMailService::DirectMailService(p3ServiceControl *sc)
	:MailTransport(DEFAULT_MAX_MESSAGE_SIZE, RS_SERVICE_TYPE_DIRECT_MAIL), 
         p3Service(), mServiceCtrl(sc), mMsgMtx("DirectMailService")
{
	mSerialiser = new RsMailTransportSerialiser(RS_SERVICE_TYPE_DIRECT_MAIL);
	addSerialType(mSerialiser);
}

const std::string MSGDIRECT_APP_NAME = 	"msgdirect";
const uint16_t MSGDIRECT_APP_MAJOR_VERSION	= 	1;
const uint16_t MSGDIRECT_APP_MINOR_VERSION  	= 	0;
const uint16_t MSGDIRECT_MIN_MAJOR_VERSION  	= 	1;
const uint16_t MSGDIRECT_MIN_MINOR_VERSION	=	0;

RsServiceInfo DirectMailService::getServiceInfo()
{
	return RsServiceInfo(RS_SERVICE_TYPE_DIRECT_MAIL, 
		MSGDIRECT_APP_NAME,
		MSGDIRECT_APP_MAJOR_VERSION, 
		MSGDIRECT_APP_MINOR_VERSION, 
		MSGDIRECT_MIN_MAJOR_VERSION, 
		MSGDIRECT_MIN_MINOR_VERSION);
}


bool DirectMailService::CanSend(const RsTlvMailAddress &addr)
{
	/* we can send if MessageAddress is a peerID,
	 * and the peer ID is a friend.
	 */
        if (addr.mAddressType == ADDRESS_TYPE_PEER_ID)
	{
		return true;
	}
	return false;
}

bool DirectMailService::sendMail(RsMailChunkItem *item)
{
	// set the right serviceId.
	item->setPacketService(RS_SERVICE_TYPE_DIRECT_MAIL);
	if (!attemptToSend(item))
	{
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/
		mOutgoingMsgs.push_back(item);
	}
	return true;
}

RsMailChunkItem *DirectMailService::recvMail()
{
	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	if (!mIncomingMsgs.empty())
	{
		RsMailChunkItem *item = mIncomingMsgs.front();
		mIncomingMsgs.pop_front();
		return item;
	}
	return NULL;
}

RsPeerId convertAddressToPeerId(RsMailAckItem *item)
{
	RsPeerId id;
	return id;
}

bool DirectMailService::sendMailAck(RsMailAckItem *item)
{
	// set the right serviceId.
	item->setPacketService(RS_SERVICE_TYPE_DIRECT_MAIL);
	item->PeerId(convertAddressToPeerId(item));
	if (!attemptToSend(item))
	{
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/
		mOutgoingMsgs.push_back(item);
	}
	return true;
}

RsMailAckItem *DirectMailService::recvMailAck()
{
	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	if (!mIncomingAckMsgs.empty())
	{
		RsMailAckItem *item = mIncomingAckMsgs.front();
		mIncomingMsgs.pop_front();

		return item;
	}
	return NULL;
}


int	DirectMailService::tick()
{
	rslog(RSL_DEBUG_BASIC, directmailzone, 
		"DirectMailService::tick()");

	incomingMsgs(); 
	return 0;
}


int	DirectMailService::status()
{
	rslog(RSL_DEBUG_BASIC, directmailzone, 
		"DirectMailService::status()");

	return 1;
}

void    DirectMailService::statusChange(const std::list<pqiServicePeer> &plist)
{
	/* should do it properly! */
	/* only do this when a new peer is connected */
	bool newPeers = false;
	std::list<pqiServicePeer>::const_iterator it;
	for(it = plist.begin(); it != plist.end(); it++)
	{
		if (it->actions & RS_SERVICE_PEER_CONNECTED)
		{
			newPeers = true;
		}
	}

	if (newPeers)
	{
		checkOutgoingMessages();
	}
}


int DirectMailService::incomingMsgs()
{
	RsItem *item;
	int i = 0;

	while((item = recvItem()) != NULL)
	{
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

		RsMailChunkItem *mi = dynamic_cast<RsMailChunkItem *>(item);
		RsMailAckItem *mai = dynamic_cast<RsMailAckItem *>(item);
		if (mi)
		{
			mIncomingMsgs.push_back(mi);
		}
		else if (mai)
		{
			mIncomingAckMsgs.push_back(mai);
		}
		else
		{
			std::cerr << "DirectMailService::incomingMsgs() Unknown incoming Msg";
			std::cerr << std::endl;
			delete item;
		}
		++i;
	}
	return i;
}

// This is a Local copy of an Outgoing message - either SentBox or Draft.
bool DirectMailService::attemptToSend(RsItem *mi)
{
	RsPeerId pid = mi->PeerId();
	if (mServiceCtrl->isPeerConnected(getServiceInfo().mServiceType, pid))
	{
		sendItem(mi);
		return true;
	}

	return false;
}

void DirectMailService::checkOutgoingMessages()
{
	/* iterate through the outgoing queue and attempt to send.
	 */

	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

        std::list<RsItem *>::iterator mit;
	for(mit = mOutgoingMsgs.begin(); mit != mOutgoingMsgs.end(); mit++)
	{
		if (attemptToSend(*mit))
		{
			/* actually sent the message */
			mit = mOutgoingMsgs.erase(mit);
		}
		else
		{
			mit++;
		}
	}
}

bool DirectMailService::haveIncomingMail()
{
	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	if (!mIncomingMsgs.empty())
	{
		return true;
	}

	if (!mIncomingAckMsgs.empty())
	{
		return true;
	}
	return false;
}


