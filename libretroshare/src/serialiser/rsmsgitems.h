#ifndef RS_MSG_ITEMS_H
#define RS_MSG_ITEMS_H

/*
 * libretroshare/src/serialiser: rsmsgitems.h
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
#include "serialiser/rsserial.h"
#include "serialiser/rstlvtypes.h"

/**************************************************************************/

/* chat Flags */
const uint32_t RS_CHAT_FLAG_PRIVATE = 0x0001;


class RsChatItem: public RsItem
{
	public:
	RsChatItem() 
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_CHAT, 
		RS_PKT_SUBTYPE_DEFAULT)
	{ return; }
virtual ~RsChatItem();
virtual void clear();
std::ostream &print(std::ostream &out, uint16_t indent = 0);

	uint32_t chatFlags;
	uint32_t sendTime;

	std::string message;

	/* not serialised */
	uint32_t recvTime; 
};

class RsChatSerialiser: public RsSerialType
{
	public:
	RsChatSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_CHAT)
	{ return; }
virtual     ~RsChatSerialiser()
	{ return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

virtual	uint32_t    sizeItem(RsChatItem *);
virtual	bool        serialiseItem  (RsChatItem *item, void *data, uint32_t *size);
virtual	RsChatItem *deserialiseItem(void *data, uint32_t *size);


};

/**************************************************************************/

const uint32_t RS_MSG_FLAGS_OUTGOING = 0x0001;
const uint32_t RS_MSG_FLAGS_PENDING  = 0x0002;
const uint32_t RS_MSG_FLAGS_DRAFT    = 0x0004;
const uint32_t RS_MSG_FLAGS_NEW      = 0x0010;

class RsMsgItem: public RsItem
{
	public:
	RsMsgItem() 
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_MSG, 
		RS_PKT_SUBTYPE_DEFAULT)
	{ return; }
virtual ~RsMsgItem(); 
virtual void clear();
std::ostream &print(std::ostream &out, uint16_t indent = 0);

	uint32_t msgFlags;
	uint32_t msgId;

	uint32_t sendTime;
	uint32_t recvTime;

	std::string subject;
	std::string message;

	RsTlvPeerIdSet msgto;
	RsTlvPeerIdSet msgcc;
	RsTlvPeerIdSet msgbcc;

	RsTlvFileSet attachment;
};

class RsMsgSerialiser: public RsSerialType
{
	public:
	RsMsgSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_MSG)
	{ return; }
virtual     ~RsMsgSerialiser() { return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);


	private:

virtual	uint32_t    sizeItem(RsMsgItem *);
virtual	bool        serialiseItem  (RsMsgItem *item, void *data, uint32_t *size);
virtual	RsMsgItem *deserialiseItem(void *data, uint32_t *size);

};

/**************************************************************************/

#endif /* RS_MSG_ITEMS_H */


