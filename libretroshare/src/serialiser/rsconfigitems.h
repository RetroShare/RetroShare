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

#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"

const uint8_t RS_PKT_TYPE_PEER_CONFIG    = 0x01;
const uint8_t RS_PKT_TYPE_CACHE_CONFIG   = 0x02;
const uint8_t RS_PKT_TYPE_FILE_CONFIG    = 0x03;

/**************************************************************************/

class RsPeerConfig: public RsItem
{
	public:
	RsPeerConfig() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG,
		RS_PKT_TYPE_PEER_CONFIG,
		RS_PKT_SUBTYPE_DEFAULT)
	{ return; }
virtual ~RsPeerConfig();
virtual void clear();
std::ostream &print(std::ostream &out, uint16_t indent = 0);

        //RsTlvPeerId     peerid;                 /* Mandatory */
	//RsTlvPeerFingerprint fpr;               /* Mandatory */

	//struct sockaddr_in lastaddr;            /* Mandatory */
	//struct sockaddr_in localaddr;           /* Mandatory */
	//struct sockaddr_in serveraddr;          /* Mandatory */

	uint32_t status;                        /* Mandatory */

        uint32_t lastconn_ts;                   /* Mandatory */
        uint32_t lastrecv_ts;                   /* Mandatory */
        uint32_t nextconn_ts;                   /* Mandatory */
        uint32_t nextconn_period;               /* Mandatory */
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

};

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

        //RsTlvPeerId     peerid;                 /* Mandatory */
	uint32_t        cacheid;                /* Mandatory */

	std::string     path;    	        /* Mandatory */
	std::string     name;    	        /* Mandatory */
	std::string     hash;    	        /* Mandatory */

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

#define FT_STATE_FAILED         0
#define FT_STATE_OKAY           1
#define FT_STATE_WAITING        2
#define FT_STATE_DOWNLOADING    3
#define FT_STATE_COMPLETE       4

class RsFileTransfer: public RsItem
{
	public:
	RsFileTransfer() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, 
		RS_PKT_TYPE_FILE_CONFIG,
		RS_PKT_SUBTYPE_DEFAULT)
	{ return; }
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

};

/**************************************************************************/

class RsFileTransferSerialiser: public RsSerialType
{
	public:
	RsFileTransferSerialiser()
	:RsSerialType(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, 
		RS_PKT_TYPE_FILE_CONFIG)
	{ return; }
virtual     ~RsFileTransferSerialiser() { return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

virtual	uint32_t    sizeTransfer(RsFileTransfer *);
virtual	bool        serialiseTransfer(RsFileTransfer *item, void *data, uint32_t *size);
virtual	RsFileTransfer *  deserialiseTransfer(void *data, uint32_t *size);

};

/**************************************************************************/

#endif /* RS_CONFIG_ITEMS_SERIALISER_H */
