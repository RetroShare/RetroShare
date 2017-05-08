#ifndef RS_DSDV_ITEMS_H
#define RS_DSDV_ITEMS_H

/*
 * libretroshare/src/serialiser: rsdsdvitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2011-2011 by Robert Fernie.
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

#include "rsitems/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvbinary.h"
#include "serialiser/rstlvdsdv.h"

/**************************************************************************/

#define	RS_PKT_SUBTYPE_DSDV_ROUTE	0x01
#define	RS_PKT_SUBTYPE_DSDV_DATA	0x02

class RsDsdvRouteItem: public RsItem
{
	public:
	RsDsdvRouteItem() 
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_DSDV, 
		RS_PKT_SUBTYPE_DSDV_ROUTE)
	{ 
		setPriorityLevel(QOS_PRIORITY_RS_DSDV_ROUTE);
		return; 
	}
virtual ~RsDsdvRouteItem();
virtual void clear();  
std::ostream &print(std::ostream &out, uint16_t indent = 0);

	RsTlvDsdvEntrySet routes;
};

class RsDsdvDataItem: public RsItem
{
	public:
	RsDsdvDataItem() 
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_DSDV, 
		RS_PKT_SUBTYPE_DSDV_DATA), data(TLV_TYPE_BIN_GENERIC)
	{ 
		setPriorityLevel(QOS_PRIORITY_RS_DSDV_DATA);
		return; 
	}
virtual ~RsDsdvDataItem();
virtual void clear();  
std::ostream &print(std::ostream &out, uint16_t indent = 0);

	RsTlvDsdvEndPoint src;
	RsTlvDsdvEndPoint dest;
	uint32_t ttl;
	RsTlvBinaryData data;
};

class RsDsdvSerialiser: public RsSerialType
{
	public:
	RsDsdvSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_DSDV)
	{ return; }
virtual     ~RsDsdvSerialiser()
	{ return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

virtual	uint32_t    sizeRoute(RsDsdvRouteItem *);
virtual	bool        serialiseRoute(RsDsdvRouteItem *item, void *data, uint32_t *size);
virtual	RsDsdvRouteItem *deserialiseRoute(void *data, uint32_t *size);

virtual	uint32_t    sizeData(RsDsdvDataItem *);
virtual	bool        serialiseData(RsDsdvDataItem *item, void *data, uint32_t *size);
virtual	RsDsdvDataItem *deserialiseData(void *data, uint32_t *size);


};

/**************************************************************************/

#endif /* RS_DSDV_ITEMS_H */


