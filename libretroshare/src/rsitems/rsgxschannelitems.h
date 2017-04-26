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

#include "rsitems/rsserviceids.h"
#include "rsitems/rsgxscommentitems.h"
#include "rsitems/rsgxsitems.h"

#include "serialiser/rstlvfileitem.h"
#include "serialiser/rstlvimage.h"

#include "retroshare/rsgxschannels.h"

#include "serialization/rsserializer.h"

#include "util/rsdir.h"

const uint8_t RS_PKT_SUBTYPE_GXSCHANNEL_GROUP_ITEM = 0x02;
const uint8_t RS_PKT_SUBTYPE_GXSCHANNEL_POST_ITEM  = 0x03;

class RsGxsChannelGroupItem : public RsGxsGrpItem
{
public:

	RsGxsChannelGroupItem():  RsGxsGrpItem(RS_SERVICE_GXS_TYPE_CHANNELS, RS_PKT_SUBTYPE_GXSCHANNEL_GROUP_ITEM) {}
	virtual ~RsGxsChannelGroupItem() {}

	void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	// use conversion functions to transform:
	bool fromChannelGroup(RsGxsChannelGroup &group, bool moveImage);
	bool toChannelGroup(RsGxsChannelGroup &group, bool moveImage);

	std::string mDescription;
	RsTlvImage mImage;
};

class RsGxsChannelPostItem : public RsGxsMsgItem
{
public:

	RsGxsChannelPostItem(): RsGxsMsgItem(RS_SERVICE_GXS_TYPE_CHANNELS, RS_PKT_SUBTYPE_GXSCHANNEL_POST_ITEM) {}
	virtual ~RsGxsChannelPostItem() {}
	void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	// Slightly unusual structure.
	// use conversion functions to transform:
	bool fromChannelPost(RsGxsChannelPost &post, bool moveImage);
	bool toChannelPost(RsGxsChannelPost &post, bool moveImage);

	std::string mMsg;
	RsTlvFileSet mAttachment;
	RsTlvImage mThumbnail;
};


class RsGxsChannelSerialiser : public RsGxsCommentSerialiser
{
public:

	RsGxsChannelSerialiser() :RsGxsCommentSerialiser(RS_SERVICE_GXS_TYPE_CHANNELS) {}
	virtual     ~RsGxsChannelSerialiser() {}

    virtual RsItem *create_item(uint16_t service_id,uint8_t item_subtype) const ;
};

#endif /* RS_GXS_CHANNEL_ITEMS_H */
