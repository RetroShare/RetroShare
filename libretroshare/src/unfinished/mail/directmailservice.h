/*
 * libretroshare/src/services/mail directmailservice.h
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


#ifndef DIRECT_MAIL_SERVICE_HEADER
#define DIRECT_MAIL_SERVICE_HEADER

#include "services/mail/mailtransport.h"
#include "services/p3service.h"

#include "pqi/pqiservicemonitor.h"

#include "util/rsthreads.h"


namespace Rs 
{
namespace Mail
{

class DirectMailService: public MailTransport, public p3Service, public pqiServiceMonitor
{

public:
	DirectMailService(p3ServiceControl *sc);
	virtual RsServiceInfo getServiceInfo();

	/******* MailTransport Interface ********************************************************/

	virtual bool CanSend(const RsTlvMailAddress &addr);
	virtual bool haveIncomingMail();

	virtual bool sendMail(RsMailChunkItem *msg);
	virtual RsMailChunkItem *recvMail();
	virtual bool sendMailAck(RsMailAckItem *ack);
	virtual RsMailAckItem *recvMailAck();

	/******* MsgTransport Interface ********************************************************/

	int	tick();
	int	status();

	/*** Overloaded from pqiMonitor ***/
	virtual void    statusChange(const std::list<pqiServicePeer> &plist);
	/*** Overloaded from pqiMonitor ***/

private:

	int   incomingMsgs();
	void  checkOutgoingMessages();
	bool  attemptToSend(RsItem *item);

	p3ServiceControl *mServiceCtrl;

	/* Mutex Required for stuff below */
	RsMutex mMsgMtx;
	RsMailTransportSerialiser *mSerialiser ;

	/* outgoing messages */
	std::list<RsItem *> mOutgoingMsgs;

	/* incoming msgs */
	std::list<RsMailChunkItem *> mIncomingMsgs;
	std::list<RsMailAckItem *> mIncomingAckMsgs;
};

} // namspace Msg
} // namspace Rs

#endif // DIRECT_MESSAGE_SERVICE_HEADER
