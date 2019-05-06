/*******************************************************************************
 * Gossip discovery service items                                              *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2008  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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
#pragma once

#include "serialiser/rsserial.h"
#include "serialiser/rstlvidset.h"
#include "serialiser/rstlvaddrs.h"
#include "rsitems/rsserviceids.h"
#include "rsitems/rsitem.h"
#include "rsitems/itempriorities.h"

#include "serialiser/rsserializer.h"

enum class RsGossipDiscoveryItemType : uint8_t
{
	PGP_LIST           = 0x1,
	PGP_CERT           = 0x2,
	CONTACT            = 0x5,
	IDENTITY_LIST      = 0x6,
	INVITE             = 0x7,
	INVITE_REQUEST     = 0x8
};

class RsDiscItem: public RsItem
{
protected:
	RsDiscItem(RsGossipDiscoveryItemType subtype);

public:
	RsDiscItem() = delete;
	virtual ~RsDiscItem();
};

/**
 * This enum is underlined by uint32_t for historical reasons.
 * We are conscious that uint32_t is an overkill for so few possible values but,
 * changing here it at this point would break binary serialized item
 * retro-compatibility.
 */
enum class RsGossipDiscoveryPgpListMode : uint32_t
{
	NONE    = 0x0,
	FRIENDS = 0x1,
	GETCERT = 0x2
};

class RsDiscPgpListItem: public RsDiscItem
{
public:

	RsDiscPgpListItem() : RsDiscItem(RsGossipDiscoveryItemType::PGP_LIST)
	{ setPriorityLevel(QOS_PRIORITY_RS_DISC_PGP_LIST); }

	void clear() override;
	void serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext& ctx ) override;

	RsGossipDiscoveryPgpListMode mode;
	RsTlvPgpIdSet pgpIdSet;
};

class RsDiscPgpCertItem: public RsDiscItem
{
public:

	RsDiscPgpCertItem() : RsDiscItem(RsGossipDiscoveryItemType::PGP_CERT)
	{ setPriorityLevel(QOS_PRIORITY_RS_DISC_PGP_CERT); }

	void clear() override;
	void serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext& ctx) override;

	RsPgpId pgpId;
	std::string pgpCert;
};

class RsDiscContactItem: public RsDiscItem
{
public:

	RsDiscContactItem() : RsDiscItem(RsGossipDiscoveryItemType::CONTACT)
	{ setPriorityLevel(QOS_PRIORITY_RS_DISC_CONTACT); }

	void clear() override;
	void serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext& ctx ) override;

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

	RsDiscIdentityListItem() :
	    RsDiscItem(RsGossipDiscoveryItemType::IDENTITY_LIST)
	{ setPriorityLevel(QOS_PRIORITY_RS_DISC_CONTACT); }

	void clear() override { ownIdentityList.clear(); }
	void serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext& ctx) override;

	std::list<RsGxsId> ownIdentityList;
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
