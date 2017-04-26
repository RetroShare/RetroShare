#ifndef RS_STATUS_ITEMS_H
#define RS_STATUS_ITEMS_H

/*
 * libretroshare/src/serialiser: rsstatusitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Vinny Do.
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
#include "rsitems/itempriorities.h"
#include "rsitems/rsitem.h"

/**************************************************************************/

class RsStatusItem: public RsItem
{
	public:
	RsStatusItem() 
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_STATUS, 
		RS_PKT_SUBTYPE_DEFAULT)
	{ 
		setPriorityLevel(QOS_PRIORITY_RS_STATUS_ITEM); 
	}
virtual ~RsStatusItem();
virtual void clear();
std::ostream &print(std::ostream &out, uint16_t indent = 0);

	uint32_t sendTime;

	uint32_t status;

	/* not serialised */
	uint32_t recvTime; 
};

class RsStatusSerialiser: public RsSerialType
{
	public:
	RsStatusSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_STATUS)
	{ return; }
virtual     ~RsStatusSerialiser()
	{ return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

virtual	uint32_t    sizeItem(RsStatusItem *);
virtual	bool        serialiseItem  (RsStatusItem *item, void *data, uint32_t *size);
virtual	RsStatusItem *deserialiseItem(void *data, uint32_t *size);


};

/**************************************************************************/

#endif /* RS_STATUS_ITEMS_H */


