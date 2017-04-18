#ifndef RS_HISTORY_ITEMS_H
#define RS_HISTORY_ITEMS_H

/*
 * libretroshare/src/serialiser: rshistoryitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Thunder.
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

#include "rsitems/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "retroshare/rstypes.h"

/**************************************************************************/

class RsHistoryMsgItem: public RsItem
{
public:
	RsHistoryMsgItem();
	virtual ~RsHistoryMsgItem();

	virtual void clear();
	std::ostream& print(std::ostream &out, uint16_t indent = 0);

	RsPeerId    chatPeerId; // empty for global chat
	bool        incoming;
	RsPeerId    peerId;
	std::string peerName;
	uint32_t    sendTime;
	uint32_t    recvTime;
	std::string message;

	/* not serialised */
	uint32_t     msgId;
	bool         saveToDisc;
};

class RsHistorySerialiser: public RsSerialType
{
public:
	RsHistorySerialiser();
	virtual ~RsHistorySerialiser();
	
	virtual	uint32_t size(RsItem*);
	virtual	bool     serialise(RsItem* item, void* data, uint32_t* size);
	virtual	RsItem*  deserialise(void* data, uint32_t* size);

private:
	virtual	uint32_t          sizeHistoryMsgItem(RsHistoryMsgItem*);
	virtual	bool              serialiseHistoryMsgItem  (RsHistoryMsgItem* item, void* data, uint32_t* size);
	virtual	RsHistoryMsgItem* deserialiseHistoryMsgItem(void* data, uint32_t* size);
};

/**************************************************************************/

#endif /* RS_HISTORY_ITEMS_H */


