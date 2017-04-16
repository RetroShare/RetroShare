/*
 * libretroshare/src/serialiser: rsbwctrlitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2012 by Robert Fernie.
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
#include "serialiser/rsbwctrlitems.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

RsItem *RsBwCtrlSerialiser::create_item(uint16_t service, uint8_t item_sub_id) const
{
    if(service != RS_SERVICE_TYPE_BWCTRL)
        return NULL ;

    switch(item_sub_id)
	{
	case RS_PKT_SUBTYPE_BWCTRL_ALLOWED_ITEM: return new RsBwCtrlAllowedItem();
	default:
		return NULL;
	}
}

void 	RsBwCtrlAllowedItem::clear()
{
	allowedBw = 0;
}

void RsBwCtrlAllowedItem::serial_process(SerializeJob j,SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,TLV_TYPE_UINT32_BW,allowedBw,"allowedBw") ;
}

#ifdef TO_REMOVE
/* serialise the data to the buffer */
bool     RsBwCtrlSerialiser::serialiseAllowed(RsBwCtrlAllowedItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeAllowed(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsBwCtrlSerialiser::serialiseRoute() Header: " << ok << std::endl;
	std::cerr << "RsBwCtrlSerialiser::serialiseRoute() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= SetTlvUInt32(data, tlvsize, &offset, TLV_TYPE_UINT32_BW, item->allowedBw);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsBwCtrlSerialiser::serialiseRoute() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsBwCtrlAllowedItem *RsBwCtrlSerialiser::deserialiseAllowed(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t tlvsize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_BWCTRL != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_BWCTRL_ALLOWED_ITEM != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < tlvsize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = tlvsize;

	bool ok = true;

	/* ready to load */
	RsBwCtrlAllowedItem *item = new RsBwCtrlAllowedItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= GetTlvUInt32(data, tlvsize, &offset, TLV_TYPE_UINT32_BW, &(item->allowedBw));


	if (offset != tlvsize)
	{
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
		delete item;
		return NULL;
	}

	return item;
}

/*************************************************************************/

uint32_t    RsBwCtrlSerialiser::size(RsItem *i)
{
	RsBwCtrlAllowedItem *dri;

	if (NULL != (dri = dynamic_cast<RsBwCtrlAllowedItem *>(i)))
	{
		return sizeAllowed(dri);
	}
	return 0;
}

bool     RsBwCtrlSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsBwCtrlAllowedItem *dri;

	if (NULL != (dri = dynamic_cast<RsBwCtrlAllowedItem *>(i)))
	{
		return serialiseAllowed(dri, data, pktsize);
	}
	return false;
}

RsItem *RsBwCtrlSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_BWCTRL != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_BWCTRL_ALLOWED_ITEM:
			return deserialiseAllowed(data, pktsize);
			break;
		default:
			return NULL;
			break;
	}
}

/*************************************************************************/
#endif



