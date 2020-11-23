/*******************************************************************************
 * libretroshare/src/rsitems: rsconfigitems.h                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef RS_CONFIG_ITEMS_SERIALISER_H
#define RS_CONFIG_ITEMS_SERIALISER_H

#include <map>
#include <vector>

#include "rsitems/rsitem.h"

#include "retroshare/rstypes.h"
#include "serialiser/rsserial.h"

#include "serialiser/rstlvidset.h"
#include "serialiser/rstlvfileitem.h"
#include "serialiser/rstlvkeyvalue.h"
#include "serialiser/rstlvaddrs.h"

#include "serialiser/rsserializer.h"

struct RsGroupInfo;

const uint8_t RS_PKT_TYPE_GENERAL_CONFIG = 0x01;
const uint8_t RS_PKT_TYPE_PEER_CONFIG    = 0x02;
const uint8_t RS_PKT_TYPE_CACHE_CONFIG   = 0x03;
const uint8_t RS_PKT_TYPE_FILE_CONFIG    = 0x04;
const uint8_t RS_PKT_TYPE_PLUGIN_CONFIG  = 0x05;
const uint8_t RS_PKT_TYPE_HISTORY_CONFIG = 0x06;

	/* GENERAL CONFIG SUBTYPES */
const uint8_t RS_PKT_SUBTYPE_KEY_VALUE = 0x01;

	/* PEER CONFIG SUBTYPES */
const uint8_t RS_PKT_SUBTYPE_PEER_STUN             = 0x02;
const uint8_t RS_PKT_SUBTYPE_PEER_NET              = 0x03;
const uint8_t RS_PKT_SUBTYPE_PEER_GROUP_deprecated = 0x04;
const uint8_t RS_PKT_SUBTYPE_PEER_PERMISSIONS      = 0x05;
const uint8_t RS_PKT_SUBTYPE_PEER_BANDLIMITS       = 0x06;
const uint8_t RS_PKT_SUBTYPE_NODE_GROUP            = 0x07;

	/* FILE CONFIG SUBTYPES */
const uint8_t RS_PKT_SUBTYPE_FILE_TRANSFER            = 0x01;
const uint8_t RS_PKT_SUBTYPE_FILE_ITEM_deprecated     = 0x02;
const uint8_t RS_PKT_SUBTYPE_FILE_ITEM                = 0x03;

/**************************************************************************/

class RsPeerNetItem: public RsItem
{
public:
	RsPeerNetItem()
	  :RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG,
	          RS_PKT_TYPE_PEER_CONFIG,
	          RS_PKT_SUBTYPE_PEER_NET)
	  , netMode(0), vs_disc(0), vs_dht(0), lastContact(0), domain_port(0)
	{}

    virtual ~RsPeerNetItem(){}
	virtual void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	/* networking information */
	RsPeerId    nodePeerId;                   /* Mandatory */
	RsPgpId     pgpId;                        /* Mandatory */
	std::string location;                     /* Mandatory */
	uint32_t    netMode;                      /* Mandatory */
	uint16_t    vs_disc;                      /* Mandatory */
	uint16_t    vs_dht;                       /* Mandatory */
	uint32_t    lastContact;                  /* Mandatory */

	RsTlvIpAddress localAddrV4;            	/* Mandatory */
	RsTlvIpAddress extAddrV4;           	/* Mandatory */
	RsTlvIpAddress localAddrV6;            	/* Mandatory */
	RsTlvIpAddress extAddrV6;            	/* Mandatory */

	std::string dyndns;

	RsTlvIpAddrSet localAddrList;
	RsTlvIpAddrSet extAddrList;

	// for proxy connection.
	std::string domain_addr;
	uint16_t    domain_port;
};

// This item should be merged with the next item, but that is not backward compatible.
class RsPeerServicePermissionItem : public RsItem
{
	public:
		RsPeerServicePermissionItem() : RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_PEER_CONFIG, RS_PKT_SUBTYPE_PEER_PERMISSIONS) {}
		virtual ~RsPeerServicePermissionItem() {}

		virtual void clear()
		{
			pgp_ids.clear() ;
			service_flags.clear() ;
		}
		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);


		/* Mandatory */
		std::vector<RsPgpId> pgp_ids ;
		std::vector<ServicePermissionFlags> service_flags ;
};
class RsPeerBandwidthLimitsItem : public RsItem
{
	public:
		RsPeerBandwidthLimitsItem() : RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_PEER_CONFIG, RS_PKT_SUBTYPE_PEER_BANDLIMITS) {}
		virtual ~RsPeerBandwidthLimitsItem() {}

		virtual void clear()
		{
			peers.clear() ;
		}
		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		/* Mandatory */
		std::map<RsPgpId,PeerBandwidthLimits> peers ;
};

class RsNodeGroupItem: public RsItem
{
public:
    RsNodeGroupItem(): RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_PEER_CONFIG, RS_PKT_SUBTYPE_NODE_GROUP), flag(0) {}
    virtual ~RsNodeGroupItem() {}

    virtual void clear() { pgpList.TlvClear();}

    explicit RsNodeGroupItem(const RsGroupInfo&) ;

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

   // /* set data from RsGroupInfo to RsPeerGroupItem */
   // void set(RsGroupInfo &groupInfo);
   // /* get data from RsGroupInfo to RsPeerGroupItem */
   // void get(RsGroupInfo &groupInfo);

    /* Mandatory */
    RsNodeGroupId id;
    std::string name;
    uint32_t    flag;

    RsTlvPgpIdSet pgpList;
};

class RsPeerStunItem: public RsItem
{
public:
	RsPeerStunItem()
	    :RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG,
	            RS_PKT_TYPE_PEER_CONFIG,
	            RS_PKT_SUBTYPE_PEER_STUN) {}
    virtual ~RsPeerStunItem(){}
    virtual void clear() { stunList.TlvClear() ;}

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsTlvPeerIdSet stunList;		  /* Mandatory */
};

class RsPeerConfigSerialiser: public RsConfigSerializer
{
	public:
	RsPeerConfigSerialiser() :RsConfigSerializer(RS_PKT_CLASS_CONFIG,RS_PKT_TYPE_PEER_CONFIG) {}

    virtual     ~RsPeerConfigSerialiser(){}

    virtual RsItem *create_item(uint8_t item_type, uint8_t item_subtype) const ;
};

/**************************************************************************/

class RsFileTransfer: public RsItem
{
	public:
		RsFileTransfer() :RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_FILE_CONFIG, RS_PKT_SUBTYPE_FILE_TRANSFER)
		{ 
			state = 0;
			in = 0;
			transferred = 0;
			crate = 0;
			trate = 0;
			lrate = 0;
			ltransfer = 0;
			flags = 0;
			chunk_strategy = 0;
		}
        virtual ~RsFileTransfer(){}
		virtual void clear();

		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		RsTlvFileItem file;
		RsTlvPeerIdSet allPeerIds;

		RsPeerId cPeerId;

		uint16_t state;
		uint16_t in;

		uint64_t transferred;
		uint32_t crate;
		uint32_t trate;

		uint32_t lrate;
		uint32_t ltransfer;

		// chunk information
		uint32_t flags ;
		uint32_t chunk_strategy ;				// strategy flags for chunks
		CompressedChunkMap compressed_chunk_map ;	// chunk availability (bitwise)
};

/**************************************************************************/

const uint32_t RS_FILE_CONFIG_CLEANUP_DELETE = 0x0001;

class RsFileConfigItem: public RsItem
{
public:
    RsFileConfigItem() :RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_FILE_CONFIG, RS_PKT_SUBTYPE_FILE_ITEM), flags(0) {}
    virtual ~RsFileConfigItem() {}
    virtual void clear() { parent_groups.TlvClear(); }

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

    RsTlvFileItem file;
    uint32_t flags;
    RsTlvNodeGroupIdSet parent_groups ;
};
/**************************************************************************/

class RsFileConfigSerialiser: public RsConfigSerializer
{
	public:
	RsFileConfigSerialiser() :RsConfigSerializer(RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_FILE_CONFIG) { }
	virtual     ~RsFileConfigSerialiser() {}
	
    virtual RsItem *create_item(uint8_t item_type, uint8_t item_subtype) const ;
};

/**************************************************************************/

/* Config items that are used generally */

class RsConfigKeyValueSet: public RsItem
{
public:
	RsConfigKeyValueSet()  :RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_GENERAL_CONFIG, RS_PKT_SUBTYPE_KEY_VALUE) {}
    virtual ~RsConfigKeyValueSet(){}
    virtual void clear() { tlvkvs.TlvClear();}

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsTlvKeyValueSet tlvkvs;
};


class RsGeneralConfigSerialiser: public RsConfigSerializer
{
	public:
	RsGeneralConfigSerialiser() :RsConfigSerializer(RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_GENERAL_CONFIG) {}

    virtual RsItem *create_item(uint8_t item_type, uint8_t item_subtype) const ;
};

#endif /* RS_CONFIG_ITEMS_SERIALISER_H */
