#ifndef RS_MAIL_TRANSPORT_ITEMS_H
#define RS_MAIL_TRANSPORT_ITEMS_H

/*
 * libretroshare/src/serialiser: rsmailtransportitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2014-2014 by Robert Fernie.
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

#include <map>

#include "retroshare/rstypes.h"
#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"

#include "serialiser/rstlvmail.h"


/**************************************************************************/

// for defining tags themselves and msg tags
const uint8_t RS_PKT_SUBTYPE_MAIL_TRANSPORT_CHUNK 	= 0x01;
const uint8_t RS_PKT_SUBTYPE_MAIL_TRANSPORT_ACK 	= 0x02;

/**************************************************************************/

// These Types are generic - and usable by all MailTransport services.
// However they must be tweaked to indicate the Service ID before sending.

class RsMailTransportItem: public RsItem
{
	public:
		RsMailTransportItem(uint16_t service_type, uint8_t msg_subtype) : RsItem(RS_PKT_VERSION_SERVICE,service_type,msg_subtype) 
		{
			setPriorityLevel(QOS_PRIORITY_RS_MAIL_ITEM) ;
		}

		RsMailTransportItem(uint8_t msg_subtype) : RsItem(RS_PKT_VERSION_SERVICE,0,msg_subtype) 
		{
			setPriorityLevel(QOS_PRIORITY_RS_MAIL_ITEM) ;
		}

		virtual ~RsMailTransportItem() {}
		virtual void clear() {}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) = 0 ;

		virtual bool serialise(void *data,uint32_t& size) = 0 ;	
		virtual uint32_t serial_size() = 0 ; 						
};


class RsMailChunkItem: public RsMailTransportItem
{
	public:
		RsMailChunkItem() :RsMailTransportItem(RS_PKT_SUBTYPE_MAIL_TRANSPORT_CHUNK) {}
		RsMailChunkItem(uint16_t service_type) :RsMailTransportItem(service_type, RS_PKT_SUBTYPE_MAIL_TRANSPORT_CHUNK) {}

		virtual ~RsMailChunkItem() {}
		virtual void clear();

		virtual bool serialise(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 						

		virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

		// extra functions.
		bool isPartial();

		// Serialised.
		RsTlvMailId mMailId;

		uint16_t    mPartCount;
		uint16_t    mMailIndex;
		RsMessageId mWholeMailId;

		std::string mMessage;
};


class RsMailAckItem : public RsMailTransportItem
{
	public:
		RsMailAckItem() :RsMailTransportItem(RS_PKT_SUBTYPE_MAIL_TRANSPORT_ACK) {}
		RsMailAckItem(uint16_t service_type) :RsMailTransportItem(service_type, RS_PKT_SUBTYPE_MAIL_TRANSPORT_ACK) {}

		virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

		virtual bool serialise(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 						

		virtual ~RsMailAckItem() {}
		virtual void clear();

		// serialised
		RsTlvMailId mMailId;
};


class RsMailTransportSerialiser: public RsSerialType
{
	public:
		RsMailTransportSerialiser(uint16_t service_type)
			:RsSerialType(RS_PKT_VERSION_SERVICE, service_type), 
			 mServiceType(service_type) {}

		virtual     ~RsMailTransportSerialiser() {}

		virtual	uint32_t    size(RsItem *item) 
		{ 
			return dynamic_cast<RsMailTransportItem*>(item)->serial_size() ; 
		}
		virtual	bool serialise(RsItem *i, void *d, uint32_t *s);
		virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:
		virtual	RsMailChunkItem   *deserialiseChunkItem(void *data, uint32_t *size);
		virtual	RsMailAckItem     *deserialiseAckItem(void *data, uint32_t *size);

		uint16_t mServiceType;
};

/**************************************************************************/

#endif /* RS_MAIL_TRANSPORT_ITEMS_H */


