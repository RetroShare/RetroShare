/*******************************************************************************
 * libretroshare/src/rsitems: rshistoryitems.h                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Thunder.                                             *
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
#ifndef RS_HISTORY_ITEMS_H
#define RS_HISTORY_ITEMS_H

#include "rsitems/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "rsitems/rsconfigitems.h"
#include "retroshare/rstypes.h"

#include "serialiser/rsserializer.h"

/**************************************************************************/

class RsHistoryMsgItem: public RsItem
{
public:
	RsHistoryMsgItem();
    virtual ~RsHistoryMsgItem() {}
    virtual void clear() {}

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsPeerId    chatPeerId; // empty for global chat
	bool        incoming;
	RsPeerId    msgPeerId;
	std::string peerName;
	uint32_t    sendTime;
	uint32_t    recvTime;
	std::string message;

	/* not serialised */
	uint32_t     msgId;
	bool         saveToDisc;
};

class RsHistorySerialiser: public RsConfigSerializer
{
public:
	RsHistorySerialiser() : RsConfigSerializer(RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_HISTORY_CONFIG) {}
	virtual ~RsHistorySerialiser() {}

    virtual RsItem *create_item(uint8_t item_type,uint8_t item_subtype) const ;
};

/**************************************************************************/

#endif /* RS_HISTORY_ITEMS_H */


