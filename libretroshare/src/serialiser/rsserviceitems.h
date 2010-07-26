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

#include "serialiser/rsserviceids.h"

/**************************************************************************/

class RsChannelMsg: public RsItem
{
	public:
	RsChannelMsg() 
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_CHANNEL, 
		RS_PKT_SUBTYPE_DEFAULT)
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
        :RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_CHANNEL)
	{ return; }

virtual     ~RsChannelMsgSerialiser();
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

};

/**************************************************************************/

class RsChatItem: public RsItem
{
	public:
	RsChatItem() 
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_CHAT, 
		RS_PKT_SUBTYPE_DEFAULT)
	{ return; }
virtual ~RsChatItem();
virtual void clear();
std::ostream &print(std::ostream &out, uint16_t indent);

	uint32_t chatFlags;
	uint32_t sendTime;

	std::string message;

};

class RsChatItemSerialiser: public RsSerialType
{
	public:
	RsChatItemSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_CHAT)
	{ return; }
virtual     ~RsChatItemSerialiser();
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

};

/**************************************************************************/

class RsMessageItem: public RsItem
{
	public:
	RsMessageItem() 
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_MSG, 
		RS_PKT_SUBTYPE_DEFAULT)
	{ return; }
virtual ~RsMessageItem();
virtual void clear();
std::ostream &print(std::ostream &out, uint16_t indent);

	uint32_t msgFlags;
	uint32_t sendTime;
	uint32_t recvTime;

	std::string subject;
	std::string message;

	RsTlvPeerIdSet msgto;
	RsTlvPeerIdSet msgcc;
	RsTlvPeerIdSet msgbcc;

	RsTlvFileSet attachment;
};

class RsMessageItemSerialiser: public RsSerialType
{
	public:
	RsMessageItemSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_MSG)
	{ return; }
virtual     ~RsMessageItemSerialiser();
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

};

/**************************************************************************/

class RsStatus: public RsItem
{
	public:
	RsStatus() 
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_STATUS, 
		RS_PKT_SUBTYPE_DEFAULT)
	{ return; }
virtual ~RsStatus();
virtual void clear();
std::ostream &print(std::ostream &out, uint16_t indent);

	/* status */
	uint32_t status;
	RsTlvServiceIdSet services;
};

class RsStatusSerialiser: public RsSerialType
{
	public:
	RsStatusSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_STATUS)
	{ return; }
virtual     ~RsStatusSerialiser();
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

};

/**************************************************************************/

/**************************************************************************/

/**************************************************************************/

#endif /* RS_SERVICE_ITEMS_H */


