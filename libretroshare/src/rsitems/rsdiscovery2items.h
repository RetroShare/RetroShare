/*******************************************************************************
 * libretroshare/src/rsitems: rsdiscitems.h                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2008 by Robert Fernie <retroshare@lunamutt.com>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#ifndef RS_DISC_ITEMS_H
#define RS_DISC_ITEMS_H

#include "serialiser/rsserial.h"
#include "serialiser/rstlvidset.h"
#include "serialiser/rstlvaddrs.h"
#include "rsitems/rsserviceids.h"
#include "rsitems/rsitem.h"
#include "rsitems/itempriorities.h"

#include "serialiser/rsserializer.h"

// TODO: port to RsGossipDiscoveryItemType
const uint8_t RS_PKT_SUBTYPE_DISC_PGP_LIST           = 0x01;
const uint8_t RS_PKT_SUBTYPE_DISC_PGP_CERT           = 0x02;
const uint8_t RS_PKT_SUBTYPE_DISC_CONTACT_deprecated = 0x03;
const uint8_t RS_PKT_SUBTYPE_DISC_CONTACT            = 0x05;
const uint8_t RS_PKT_SUBTYPE_DISC_IDENTITY_LIST      = 0x06;

enum class RsGossipDiscoveryItemType : uint8_t
{
	INVITE = 7,
	INVITE_REQUEST = 8
};

class RsDiscItem: public RsItem
{
protected:
	RsDiscItem(uint8_t subtype) :
	    RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_DISC, subtype) {}
};


#define DISC_PGP_LIST_MODE_NONE		0x00
#define DISC_PGP_LIST_MODE_FRIENDS	0x01
#define DISC_PGP_LIST_MODE_GETCERT	0x02

class RsDiscPgpListItem: public RsDiscItem
{
public:

	RsDiscPgpListItem()
	    :RsDiscItem(RS_PKT_SUBTYPE_DISC_PGP_LIST)
	{
		setPriorityLevel(QOS_PRIORITY_RS_DISC_PGP_LIST);
	}

    virtual ~RsDiscPgpListItem(){}

	virtual  void clear();
	virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */);

	uint32_t mode;
	RsTlvPgpIdSet pgpIdSet;
};

class RsDiscPgpCertItem: public RsDiscItem
{
public:

	RsDiscPgpCertItem()
	    :RsDiscItem(RS_PKT_SUBTYPE_DISC_PGP_CERT)
	{
		setPriorityLevel(QOS_PRIORITY_RS_DISC_PGP_CERT);
	}

    virtual ~RsDiscPgpCertItem(){}

	virtual  void clear();
	virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */);

	RsPgpId pgpId;
	std::string pgpCert;
};

class RsDiscContactItem: public RsDiscItem
{
public:

	RsDiscContactItem()
	    :RsDiscItem(RS_PKT_SUBTYPE_DISC_CONTACT)
	{
		setPriorityLevel(QOS_PRIORITY_RS_DISC_CONTACT);
	}

    virtual ~RsDiscContactItem() {}

	virtual  void clear();
	virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */);

	RsPgpId pgpId;
	RsPeerId sslId;

	// COMMON
	std::string location;
	std::string version;

	uint32_t    netMode;			/* Mandatory */
	uint16_t    vs_disc;		    	/* Mandatory */
	uint16_t    vs_dht;		    	/* Mandatory */
	uint32_t    lastContact;

	bool   isHidden;			/* not serialised */

	// HIDDEN.
	std::string hiddenAddr;
	uint16_t    hiddenPort;

	// STANDARD.

	RsTlvIpAddress currentConnectAddress ;	// used to check!

	RsTlvIpAddress localAddrV4;		/* Mandatory */
	RsTlvIpAddress extAddrV4;		/* Mandatory */

	RsTlvIpAddress localAddrV6;		/* Mandatory */
	RsTlvIpAddress extAddrV6;		/* Mandatory */

	std::string dyndns;

	RsTlvIpAddrSet localAddrList;
	RsTlvIpAddrSet extAddrList;
};

class RsDiscIdentityListItem: public RsDiscItem
{
public:

	RsDiscIdentityListItem()
	    :RsDiscItem(RS_PKT_SUBTYPE_DISC_IDENTITY_LIST)
	{
		setPriorityLevel(QOS_PRIORITY_RS_DISC_CONTACT);
	}

    virtual ~RsDiscIdentityListItem() {}

    virtual void clear() { ownIdentityList.clear() ; }
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

    std::list<RsGxsId> ownIdentityList ;
};

struct RsGossipDiscoveryInviteItem : RsDiscItem
{
	RsGossipDiscoveryInviteItem();

	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx ) override
	{ RS_SERIAL_PROCESS(mInvite); }
	void clear() override { mInvite.clear(); }

	std::string mInvite;
};

struct RsGossipDiscoveryInviteRequestItem : RsDiscItem
{
	RsGossipDiscoveryInviteRequestItem();

	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx ) override
	{ RS_SERIAL_PROCESS(mInviteId); }
	void clear() override { mInviteId.clear(); }

	RsPeerId mInviteId;
};

class RsDiscSerialiser: public RsServiceSerializer
{
public:
	RsDiscSerialiser() :RsServiceSerializer(RS_SERVICE_TYPE_DISC) {}
	virtual ~RsDiscSerialiser() {}

	RsItem* create_item(uint16_t service, uint8_t item_subtype) const;
};


#endif // RS_DISC_ITEMS_H

