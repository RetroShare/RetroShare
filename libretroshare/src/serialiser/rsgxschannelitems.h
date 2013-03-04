/*
 * libretroshare/src/serialiser: rsgxschannelitems.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Robert Fernie
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

#ifndef RS_GXS_CHANNEL_ITEMS_H
#define RS_GXS_CHANNEL_ITEMS_H

#include <map>

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvtypes.h"

#include "serialiser/rsgxscommentitems.h"

#include "rsgxsitems.h"
#include "retroshare/rsgxschannels.h"

const uint8_t RS_PKT_SUBTYPE_GXSCHANNEL_GROUP_ITEM = 0x02;
const uint8_t RS_PKT_SUBTYPE_GXSCHANNEL_POST_ITEM = 0x03;

class RsGxsChannelGroupItem : public RsGxsGrpItem
{

public:

	RsGxsChannelGroupItem():  RsGxsGrpItem(RS_SERVICE_GXSV1_TYPE_CHANNELS,
			RS_PKT_SUBTYPE_GXSCHANNEL_GROUP_ITEM) { return;}
        virtual ~RsGxsChannelGroupItem() { return;}

        void clear();
	std::ostream &print(std::ostream &out, uint16_t indent = 0);


	RsGxsChannelGroup mGroup;
};

class RsGxsChannelPostItem : public RsGxsMsgItem
{
public:

	RsGxsChannelPostItem(): RsGxsMsgItem(RS_SERVICE_GXSV1_TYPE_CHANNELS,
			RS_PKT_SUBTYPE_GXSCHANNEL_POST_ITEM) {return; }
        virtual ~RsGxsChannelPostItem() { return;}
        void clear();
	std::ostream &print(std::ostream &out, uint16_t indent = 0);

	RsGxsChannelPost mMsg;
};


class RsGxsChannelSerialiser : public RsGxsCommentSerialiser
{
public:

	RsGxsChannelSerialiser()
	:RsGxsCommentSerialiser(RS_SERVICE_GXSV1_TYPE_CHANNELS)
	{ return; }
	virtual     ~RsGxsChannelSerialiser() { return; }

	uint32_t    size(RsItem *item);
	bool        serialise  (RsItem *item, void *data, uint32_t *size);
	RsItem *    deserialise(void *data, uint32_t *size);

	private:

	uint32_t    sizeGxsChannelGroupItem(RsGxsChannelGroupItem *item);
	bool        serialiseGxsChannelGroupItem  (RsGxsChannelGroupItem *item, void *data, uint32_t *size);
	RsGxsChannelGroupItem *    deserialiseGxsChannelGroupItem(void *data, uint32_t *size);

	uint32_t    sizeGxsChannelPostItem(RsGxsChannelPostItem *item);
	bool        serialiseGxsChannelPostItem  (RsGxsChannelPostItem *item, void *data, uint32_t *size);
	RsGxsChannelPostItem *    deserialiseGxsChannelPostItem(void *data, uint32_t *size);

};

#endif /* RS_GXS_CHANNEL_ITEMS_H */
