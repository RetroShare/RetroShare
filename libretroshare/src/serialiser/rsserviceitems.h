#ifndef RS_SERVICE_ITEMS_H
#define RS_SERVICE_ITEMS_H

/*
 * libretroshare/src/serialiser: rsserviceitems.h
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

const uint8_t RS_PKT_TYPE_DISC_MSG    = 0x01;
const uint8_t RS_PKT_TYPE_CHANNEL_MSG = 0x02;
const uint8_t RS_PKT_TYPE_PROXY_MSG   = 0x03;

/**************************************************************************/

class RsDiscMsg: public RsItem
{
	public:
	RsDiscMsg() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_SERVICE,
		RS_PKT_TYPE_DISC_MSG, 0)
	{ return; }
virtual ~RsDiscMsg();
virtual void clear();

	uint32_t discType;
	uint32_t discFlags;

	uint32_t connect_tr;
	uint32_t receive_tr;

	struct sockaddr_in laddr;
	struct sockaddr_in saddr;

	RsTlvBinaryData cert;
};

class RsDiscMsgSerialiser: public RsSerialType
{
	public:
	RsDiscMsgSerialiser()
        :RsSerialType(RS_PKT_VERSION1, RS_PKT_CLASS_SERVICE,
	                RS_PKT_TYPE_DISC_MSG)
	{ return; }

virtual     ~RsDiscMsgSerialiser();
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

};


/**************************************************************************/

class RsChannelMsg: public RsItem
{
	public:
	RsChannelMsg() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_SERVICE,
		RS_PKT_TYPE_CHANNEL_MSG, 0)
	{ return; }
virtual ~RsChannelMsg();
virtual void clear();

	RsTlvBinaryData cert;   /* Mandatory */
	RsTlvFileSet  files;  /* Mandatory */
	RsTlvBinaryData sign;   /* Mandatory */
};

class RsChannelMsgSerialiser: public RsSerialType
{
	public:
	RsChannelMsgSerialiser()
        :RsSerialType(RS_PKT_VERSION1, RS_PKT_CLASS_SERVICE,
	                RS_PKT_TYPE_CHANNEL_MSG)
	{ return; }

virtual     ~RsChannelMsgSerialiser();
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

};


/**************************************************************************/


#endif /* RS_SERVICE_ITEMS_H */
