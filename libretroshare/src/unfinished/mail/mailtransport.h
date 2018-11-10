#pragma once
#ifndef RS_MAIL_TRANSPORT_HEADER
#define RS_MAIL_TRANSPORT_HEADER
/*
 * libretroshare/src/services/mail mailtransport.h
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


#include "serialiser/rsmailtransportitems.h"


namespace Rs
{
namespace Mail
{

const uint32_t DEFAULT_MAX_MESSAGE_SIZE  = (64 * 1024);

// To decide how this will work!
const uint32_t ADDRESS_TYPE_PEER_ID	= 0x001;
const uint32_t ADDRESS_TYPE_GXS_ID	= 0x002;
const uint32_t ADDRESS_TYPE_EMAIL_ID	= 0x003;


class MailTransport
{
public:
        MailTransport(uint32_t max_size, uint16_t transport_type);
        virtual ~MailTransport();

        virtual bool CanSend(const RsTlvMailAddress &addr) = 0;
        virtual bool haveIncomingMail() = 0;

        virtual bool sendMail(RsMailChunkItem *msg) = 0;
        virtual RsMailChunkItem *recvMail() = 0;
        virtual bool sendMailAck(RsMailAckItem *ack) = 0;
        virtual RsMailAckItem *recvMailAck() = 0;

        virtual bool isEnabled();
        virtual void setEnabled(bool enabled);

	uint32_t getMaxSize();
	uint16_t getTransportType();

private:

	bool mEnabled;
	uint32_t mMaxSize;
	uint16_t mTransportType;
};


} // namespace Mail
} // namespace Rs

#endif
