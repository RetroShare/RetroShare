/*
 * libretroshare/src/serialiser: rschatitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#pragma once

#include <openssl/ssl.h>

#include "rsitems/rsserviceids.h"
#include "rsitems/itempriorities.h"
#include "rsitems/rsitem.h"

#include "retroshare/rstypes.h"
#include "serialiser/rstlvkeys.h"
#include "serialiser/rsserial.h"

#include "serialiser/rstlvidset.h"
#include "serialiser/rstlvfileitem.h"

/* chat Flags */
const uint32_t RS_GXS_TUNNEL_FLAG_CLOSING_DISTANT_CONNECTION = 0x0400;
const uint32_t RS_GXS_TUNNEL_FLAG_ACK_DISTANT_CONNECTION     = 0x0800;
const uint32_t RS_GXS_TUNNEL_FLAG_KEEP_ALIVE                 = 0x1000;

const uint8_t RS_PKT_SUBTYPE_GXS_TUNNEL_DATA           = 0x01 ;	
const uint8_t RS_PKT_SUBTYPE_GXS_TUNNEL_DH_PUBLIC_KEY  = 0x02 ;
const uint8_t RS_PKT_SUBTYPE_GXS_TUNNEL_STATUS         = 0x03 ;
const uint8_t RS_PKT_SUBTYPE_GXS_TUNNEL_DATA_ACK       = 0x04 ;

typedef uint64_t		GxsTunnelDHSessionId ;

class RsGxsTunnelItem: public RsItem
{
	public:
		RsGxsTunnelItem(uint8_t item_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_GXS_TUNNEL,item_subtype) 
		{
			setPriorityLevel(QOS_PRIORITY_RS_CHAT_ITEM) ;
		}

		virtual ~RsGxsTunnelItem() {}
		virtual void clear() {}
};

/*!
 * For sending distant communication data. The item is not encrypted after being serialised, but the data it.
 * The MAC is computed over encrypted data using the PFS key. All other items (except DH keys) are serialised, encrypted, and
 * sent as data in a RsGxsTunnelDataItem.
 * 
 * @see p3GxsTunnelService
 */
class RsGxsTunnelDataItem: public RsGxsTunnelItem
{
public:
    RsGxsTunnelDataItem() :RsGxsTunnelItem(RS_PKT_SUBTYPE_GXS_TUNNEL_DATA) { data=NULL ;data_size=0;service_id=0;unique_item_counter=0; }
    RsGxsTunnelDataItem(uint8_t subtype) :RsGxsTunnelItem(subtype) { data=NULL ;data_size=0; }

    virtual ~RsGxsTunnelDataItem() {}
    virtual void clear() {}

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

    uint64_t unique_item_counter;				// this allows to make the item unique
    uint32_t flags;						// mainly NEEDS_HACK?
    uint32_t service_id ;
    uint32_t data_size ;					// encrypted data size
    unsigned char *data ;					// encrypted data
};

// Used to send status of connection. This can be closing orders, flushing orders, etc.
// These items are always sent encrypted.

class RsGxsTunnelStatusItem: public RsGxsTunnelItem
{
	public:
		RsGxsTunnelStatusItem() :RsGxsTunnelItem(RS_PKT_SUBTYPE_GXS_TUNNEL_STATUS) , status(0) {}
		RsGxsTunnelStatusItem(void *data,uint32_t size) ; // deserialization

		virtual ~RsGxsTunnelStatusItem() {}

		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		uint32_t status ;
};

// Used to confirm reception of an encrypted item. 

class RsGxsTunnelDataAckItem: public RsGxsTunnelItem
{
	public:
		RsGxsTunnelDataAckItem() :RsGxsTunnelItem(RS_PKT_SUBTYPE_GXS_TUNNEL_DATA_ACK) {}
		RsGxsTunnelDataAckItem(void *data,uint32_t size) ; // deserialization

		virtual ~RsGxsTunnelDataAckItem() {}

		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		uint64_t unique_item_counter ;					// unique identifier for that item
};


// This class contains the public Diffie-Hellman parameters to be sent
// when performing a DH agreement over a distant chat tunnel.
//
class RsGxsTunnelDHPublicKeyItem: public RsGxsTunnelItem
{
	public:
		RsGxsTunnelDHPublicKeyItem() :RsGxsTunnelItem(RS_PKT_SUBTYPE_GXS_TUNNEL_DH_PUBLIC_KEY) {}
		RsGxsTunnelDHPublicKeyItem(void *data,uint32_t size) ; // deserialization

		virtual ~RsGxsTunnelDHPublicKeyItem() ;

		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		// Private data to DH public key item
		//
		BIGNUM *public_key ;

		RsTlvKeySignature signature ;	// signs the public key in a row.
		RsTlvPublicRSAKey gxs_key ;	// public key of the signer

	private:
		// make the object non copy-able
		RsGxsTunnelDHPublicKeyItem(const RsGxsTunnelDHPublicKeyItem&) : RsGxsTunnelItem(RS_PKT_SUBTYPE_GXS_TUNNEL_DH_PUBLIC_KEY) {}
		const RsGxsTunnelDHPublicKeyItem& operator=(const RsGxsTunnelDHPublicKeyItem&) { return *this ;}
};

class RsGxsTunnelSerialiser: public RsServiceSerializer
{
public:
	RsGxsTunnelSerialiser() :RsServiceSerializer(RS_SERVICE_TYPE_GXS_TUNNEL) {}

	virtual RsItem *create_item(uint16_t service,uint8_t item_subtype) const ;
};

