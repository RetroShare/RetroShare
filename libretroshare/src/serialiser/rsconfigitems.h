#ifndef RS_CONFIG_ITEMS_SERIALISER_H
#define RS_CONFIG_ITEMS_SERIALISER_H

/*
 * libretroshare/src/serialiser: rsconfigitems.h
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

#include <map>
#include <vector>

#include <rsiface/rstypes.h>
#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"

const uint8_t RS_PKT_TYPE_GENERAL_CONFIG = 0x01;
const uint8_t RS_PKT_TYPE_PEER_CONFIG    = 0x02;
const uint8_t RS_PKT_TYPE_CACHE_CONFIG   = 0x03;
const uint8_t RS_PKT_TYPE_FILE_CONFIG    = 0x04;

	/* GENERAL CONFIG SUBTYPES */
const uint8_t RS_PKT_SUBTYPE_KEY_VALUE = 0x01;

	/* PEER CONFIG SUBTYPES */
const uint8_t RS_PKT_SUBTYPE_PEER_NET  = 0x01;
const uint8_t RS_PKT_SUBTYPE_PEER_STUN = 0x02;

	/* FILE CONFIG SUBTYPES */
const uint8_t RS_PKT_SUBTYPE_FILE_TRANSFER = 0x01;
const uint8_t RS_PKT_SUBTYPE_FILE_ITEM     = 0x02;

/**************************************************************************/

struct IpAddressTimed {
    struct sockaddr_in ipAddr;
    time_t seenTime;
};

class RsPeerNetItem: public RsItem
{
	public:
	RsPeerNetItem() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG,
		RS_PKT_TYPE_PEER_CONFIG,
		RS_PKT_SUBTYPE_PEER_NET)
	{ return; }
virtual ~RsPeerNetItem();
virtual void clear();
std::ostream &print(std::ostream &out, uint16_t indent = 0);

	/* networking information */
	std::string pid;                          /* Mandatory */
	uint32_t    netMode;                      /* Mandatory */
	uint32_t    visState;                     /* Mandatory */
	uint32_t    lastContact;                  /* Mandatory */

	struct sockaddr_in currentlocaladdr;             /* Mandatory */
	struct sockaddr_in currentremoteaddr;            /* Mandatory */

        std::list<IpAddressTimed> ipAddressList;
};

class RsPeerStunItem: public RsItem
{
	public:
	RsPeerStunItem() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG,
		RS_PKT_TYPE_PEER_CONFIG,
		RS_PKT_SUBTYPE_PEER_STUN)
	{ return; }
virtual ~RsPeerStunItem();
virtual void clear();
std::ostream &print(std::ostream &out, uint16_t indent = 0);

	RsTlvPeerIdSet stunList;		  /* Mandatory */
};

class RsPeerConfigSerialiser: public RsSerialType
{
	public:
	RsPeerConfigSerialiser()
        :RsSerialType(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG,
	                RS_PKT_TYPE_PEER_CONFIG)
	{ return; }

virtual     ~RsPeerConfigSerialiser();
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

virtual	uint32_t    sizeNet(RsPeerNetItem *);
virtual	bool        serialiseNet  (RsPeerNetItem *item, void *data, uint32_t *size);
virtual	RsPeerNetItem *deserialiseNet(void *data, uint32_t *size);

virtual	uint32_t    sizeStun(RsPeerStunItem *);
virtual	bool        serialiseStun  (RsPeerStunItem *item, void *data, uint32_t *size);
virtual	RsPeerStunItem *    deserialiseStun(void *data, uint32_t *size);

};

/**************************************************************************/
/**************************************************************************/

/**************************************************************************/

class RsCacheConfig: public RsItem
{
	public:
	RsCacheConfig() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG,
		RS_PKT_TYPE_CACHE_CONFIG,
		RS_PKT_SUBTYPE_DEFAULT)
	{ return; }
virtual ~RsCacheConfig();
virtual void clear();
std::ostream &print(std::ostream &out, uint16_t indent = 0);

        std::string     pid;                 /* Mandatory */
	uint16_t        cachetypeid;            /* Mandatory */
	uint16_t        cachesubid;             /* Mandatory */

	std::string     path;    	        /* Mandatory */
	std::string     name;    	        /* Mandatory */
	std::string     hash;    	        /* Mandatory */
	uint64_t	size;			/* Mandatory */

	uint32_t	recvd;                  /* Mandatory */
};


class RsCacheConfigSerialiser: public RsSerialType
{
	public:
	RsCacheConfigSerialiser()
        :RsSerialType(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG,
	                RS_PKT_TYPE_CACHE_CONFIG)
	{ return; }

virtual     ~RsCacheConfigSerialiser();
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

};

/**************************************************************************/

#ifndef FT_STATE_FAILED
    #define FT_STATE_FAILED         0
    #define FT_STATE_OKAY           1
    #define FT_STATE_WAITING        2
    #define FT_STATE_DOWNLOADING    3
    #define FT_STATE_COMPLETE       4
#endif

class RsFileTransfer: public RsItem
{
	public:
		RsFileTransfer() :RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_FILE_CONFIG, RS_PKT_SUBTYPE_FILE_TRANSFER)
		{ 
			return; 
		}
		virtual ~RsFileTransfer();
		virtual void clear();
		std::ostream &print(std::ostream &out, uint16_t indent = 0);

		RsTlvFileItem file;
		RsTlvPeerIdSet allPeerIds;

		std::string cPeerId;

		uint16_t state;
		uint16_t in;

		uint64_t transferred;
		uint32_t crate;
		uint32_t trate;

		uint32_t lrate;
		uint32_t ltransfer;

		// chunk information
		uint32_t chunk_strategy ;				// strategy flags for chunks
		CompressedChunkMap compressed_chunk_map ;	// chunk availability (bitwise)
};

/**************************************************************************/

const uint32_t RS_FILE_CONFIG_CLEANUP_DELETE = 0x0001;

/* Used by ft / extralist / configdirs / anyone who wants a basic file */ 
class RsFileConfigItem: public RsItem
{
	public:
	RsFileConfigItem() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, 
		RS_PKT_TYPE_FILE_CONFIG,
		RS_PKT_SUBTYPE_FILE_ITEM)
	{ return; }
virtual ~RsFileConfigItem();
virtual void clear();
std::ostream &print(std::ostream &out, uint16_t indent = 0);

	RsTlvFileItem file;
	uint32_t flags;
};

/**************************************************************************/

class RsFileConfigSerialiser: public RsSerialType
{
	public:
	RsFileConfigSerialiser()
	:RsSerialType(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, 
		RS_PKT_TYPE_FILE_CONFIG)
	{ return; }
virtual     ~RsFileConfigSerialiser() { return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

virtual	uint32_t    sizeTransfer(RsFileTransfer *);
virtual	bool        serialiseTransfer(RsFileTransfer *item, void *data, uint32_t *size);
virtual	RsFileTransfer *  deserialiseTransfer(void *data, uint32_t *size);

virtual	uint32_t    sizeFileItem(RsFileConfigItem *);
virtual	bool        serialiseFileItem(RsFileConfigItem *item, void *data, uint32_t *size);
virtual	RsFileConfigItem *  deserialiseFileItem(void *data, uint32_t *size);

};

/**************************************************************************/

/* Config items that are used generally */

class RsConfigKeyValueSet: public RsItem
{
	public:
	RsConfigKeyValueSet() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG,
		RS_PKT_TYPE_GENERAL_CONFIG,
		RS_PKT_SUBTYPE_KEY_VALUE)
	{ return; }
virtual ~RsConfigKeyValueSet();
virtual void clear();
std::ostream &print(std::ostream &out, uint16_t indent = 0);

	RsTlvKeyValueSet tlvkvs;
};


class RsGeneralConfigSerialiser: public RsSerialType
{
	public:
	RsGeneralConfigSerialiser()
        :RsSerialType(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG,
	                RS_PKT_TYPE_GENERAL_CONFIG)
	{ return; }

virtual     ~RsGeneralConfigSerialiser();
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:
uint32_t    sizeKeyValueSet(RsConfigKeyValueSet *item);
bool     serialiseKeyValueSet(RsConfigKeyValueSet *item, void *data, uint32_t *pktsize);
RsConfigKeyValueSet *deserialiseKeyValueSet(void *data, uint32_t *pktsize);

};

#endif /* RS_CONFIG_ITEMS_SERIALISER_H */
