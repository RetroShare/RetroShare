/*******************************************************************************
 * libretroshare/src/rsitems: rsgxsforumitems.h                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef RS_GXS_FORUM_ITEMS_H
#define RS_GXS_FORUM_ITEMS_H

#include <map>

#include "rsitems/rsserviceids.h"
#include "rsitems/rsgxsitems.h"

#include "serialiser/rsserializer.h"

#include "retroshare/rsgxsforums.h"

const uint8_t RS_PKT_SUBTYPE_GXSFORUM_GROUP_ITEM_deprecated   = 0x02;
const uint8_t RS_PKT_SUBTYPE_GXSFORUM_MESSAGE_ITEM            = 0x03;
const uint8_t RS_PKT_SUBTYPE_GXSFORUM_GROUP_ITEM              = 0x04;

// This class was created on 11/2018 in order to get rid of the old group system that uses Tlv Sets for ids, that is not json-friendly.

class RsGxsForumGroupItem_deprecated : public RsGxsGrpItem
{

public:

	RsGxsForumGroupItem_deprecated():  RsGxsGrpItem(RS_SERVICE_GXS_TYPE_FORUMS, RS_PKT_SUBTYPE_GXSFORUM_GROUP_ITEM_deprecated) {}
	virtual ~RsGxsForumGroupItem_deprecated() {}

	void clear();
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsGxsForumGroup_deprecated mGroup;
};

class RsGxsForumGroupItem: public RsGxsGrpItem
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
