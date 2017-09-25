/*
 * libretroshare/src/serialiser: rsgxsforumitems.h
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

#ifndef RS_GXS_FORUM_ITEMS_H
#define RS_GXS_FORUM_ITEMS_H

#include <map>

#include "rsitems/rsserviceids.h"
#include "rsitems/rsgxsitems.h"

#include "serialiser/rsserializer.h"

#include "retroshare/rsgxsforums.h"

const uint8_t RS_PKT_SUBTYPE_GXSFORUM_GROUP_ITEM   = 0x02;
const uint8_t RS_PKT_SUBTYPE_GXSFORUM_MESSAGE_ITEM = 0x03;

class RsGxsForumGroupItem : public RsGxsGrpItem
{

public:

	RsGxsForumGroupItem():  RsGxsGrpItem(RS_SERVICE_GXS_TYPE_FORUMS, RS_PKT_SUBTYPE_GXSFORUM_GROUP_ITEM) {}
	virtual ~RsGxsForumGroupItem() {}

	void clear();
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsGxsForumGroup mGroup;
};

class RsGxsForumMsgItem : public RsGxsMsgItem
{
public:

	RsGxsForumMsgItem(): RsGxsMsgItem(RS_SERVICE_GXS_TYPE_FORUMS, RS_PKT_SUBTYPE_GXSFORUM_MESSAGE_ITEM) {}
	virtual ~RsGxsForumMsgItem() {}
	void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsGxsForumMsg mMsg;
};

class RsGxsForumSerialiser : public RsServiceSerializer
{
public:
	RsGxsForumSerialiser() :RsServiceSerializer(RS_SERVICE_GXS_TYPE_FORUMS) {}
	virtual ~RsGxsForumSerialiser() {}

	virtual RsItem *create_item(uint16_t service_id,uint8_t item_subtype) const ;
};

#endif /* RS_GXS_FORUM_ITEMS_H */
