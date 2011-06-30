#ifndef RS_GAME_ITEMS_H
#define RS_GAME_ITEMS_H

/*
 * libretroshare/src/serialiser: rsgameitems.h
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

class RsGameItem: public RsItem
{
	public:
	RsGameItem() 
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_GAME_LAUNCHER, 
		RS_PKT_SUBTYPE_DEFAULT)
	{ return; }
virtual ~RsGameItem();
virtual void clear();  
std::ostream &print(std::ostream &out, uint16_t indent = 0);

        uint32_t serviceId;
        uint32_t numPlayers;
        uint32_t msg; /* RS_GAME_MSG_XXX */

        std::string gameId;
        std::wstring gameComment;

	RsTlvPeerIdSet players;
};

class RsGameSerialiser: public RsSerialType
{
	public:
	RsGameSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_GAME_LAUNCHER)
	{ return; }
virtual     ~RsGameSerialiser()
	{ return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

virtual	uint32_t    sizeItem(RsGameItem *);
virtual	bool        serialiseItem  (RsGameItem *item, void *data, uint32_t *size);
virtual	RsGameItem *deserialiseItem(void *data, uint32_t *size);


};

/**************************************************************************/

#endif /* RS_GAME_ITEMS_H */


