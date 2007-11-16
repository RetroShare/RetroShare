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

const uint8_t RS_PKT_TYPE_PEER_CONFIG    = 0x01;
const uint8_t RS_PKT_TYPE_CACHE_CONFIG   = 0x02;

/**************************************************************************/

class RsPeerConfig: public RsItem
{
	public:
	RsPeerConfig() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG,
		RS_PKT_TYPE_PEER_CONFIG, 0)
	{ return; }
virtual ~RsPeerConfig();
virtual void clear();

        RsTlvPeerId     peerid;                 /* Mandatory */
	RsTlvPeerFingerprint fpr;               /* Mandatory */

	struct sockaddr_in lastaddr;            /* Mandatory */
	struct sockaddr_in localaddr;           /* Mandatory */
	struct sockaddr_in serveraddr;          /* Mandatory */

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
		RS_PKT_TYPE_CACHE_CONFIG, 0)
	{ return; }
virtual ~RsCacheConfig();
virtual void clear();

        RsTlvPeerId     peerid;                 /* Mandatory */
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

#endif /* RS_CONFIG_ITEMS_SERIALISER_H */
