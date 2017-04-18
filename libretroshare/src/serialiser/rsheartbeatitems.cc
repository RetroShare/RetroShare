
/*
 * libretroshare/src/serialiser: rsheartbeatitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2013-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#include "serialiser/rsbaseserial.h"
#include "rsitems/rsserviceids.h"
#include "serialiser/rsheartbeatitems.h"

/***
 * #define HEART_DEBUG 		1
 ***/

#define HEART_DEBUG 		1

#include <iostream>

/*************************************************************************/

uint32_t    RsHeartbeatSerialiser::size(RsItem *i)
{
        RsHeartbeatItem *beat;

	if (NULL != (beat = dynamic_cast<RsHeartbeatItem *>(i)))
	{
		return sizeHeartbeat(beat);
	}
	return 0;
}

/* serialise the data to the buffer */
bool    RsHeartbeatSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
        RsHeartbeatItem *beat;

	if (NULL != (beat = dynamic_cast<RsHeartbeatItem *>(i)))
	{
		return serialiseHeartbeat(beat, data, pktsize);
	}
	return false;
}

RsItem *RsHeartbeatSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_HEARTBEAT != getRsItemService(rstype)))
	{

#ifdef HEART_DEBUG
		std::cerr << "RsHeartbeatSerialiser::deserialise() Wrong Type" << std::endl;
#endif
		return NULL; 
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_HEARTBEAT_PULSE:
                        return deserialiseHeartbeat(data, pktsize);
                        break;
                default:
			return NULL;
			break;
	}
	return NULL;
}

/*************************************************************************/


void RsHeartbeatItem::clear()
{
}

std::ostream &RsHeartbeatItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsHeartbeatItem", indent);
	printRsItemEnd(out, "RsHeartbeatItem", indent);
	return out;
}

uint32_t RsHeartbeatSerialiser::sizeHeartbeat(RsHeartbeatItem */*item*/)
{
	uint32_t s = 8; /* header */
	return s;
}

/* serialise the data to the buffer */
bool RsHeartbeatSerialiser::serialiseHeartbeat(RsHeartbeatItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeHeartbeat(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
	{
#ifdef HEART_DEBUG
		std::cerr << "RsHeartbeatSerialiser::serialiseHeartbeat() Not enough space" << std::endl;
#endif
		return false;   /* not enough space */
	}

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, *pktsize, item->PacketId(), *pktsize);

	/* skip the header */
	offset += 8;

	if (offset != tlvsize)
	{
#ifdef HEART_DEBUG
		std::cerr << "RsHeartbeatSerialiser::serialiseHeartbeat() size error" << std::endl;
#endif
		ok = false;
	}

	return ok;
}

RsHeartbeatItem *RsHeartbeatSerialiser::deserialiseHeartbeat(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_HEARTBEAT != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_HEARTBEAT_PULSE != getRsItemSubType(rstype)))
	{
#ifdef HEART_DEBUG
		std::cerr << "RsHeartbeatSerialiser::deserialiseHeartbeat() Wrong Type" << std::endl;
#endif
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)	/* check size */
	{
#ifdef HEART_DEBUG
		std::cerr << "RsHeartbeatSerialiser::deserialiseHeartbeat() size error" << std::endl;
#endif
		return NULL; /* not enough data */
	}

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsHeartbeatItem *item = new RsHeartbeatItem();
	item->clear();

	/* skip the header */
	offset += 8;

	if (offset != rssize)
	{
#ifdef HEART_DEBUG
		std::cerr << "RsHeartbeatSerialiser::deserialiseHeartbeat() size error2" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
#ifdef HEART_DEBUG
		std::cerr << "RsHeartbeatSerialiser::deserialiseHeartbeat() ok = false" << std::endl;
#endif
		delete item;
		return NULL;
	}

	return item;
}


/*************************************************************************/
