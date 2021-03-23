/*******************************************************************************
 * libretroshare/src/rsitems: rsgxsforumitems.h                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012  Robert Fernie <retroshare@lunamutt.com>                 *
 * Copyright (C) 2018-2021  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2019-2021  Asociación Civil Altermundi <info@altermundi.net>  *
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
#pragma once

#include <map>

#include "rsitems/rsserviceids.h"
#include "rsitems/rsgxsitems.h"

#include "serialiser/rsserializer.h"

#include "retroshare/rsgxsforums.h"

enum class RsGxsForumsItems : uint8_t
{
	GROUP_ITEM     = 0x02,
	MESSAGE_ITEM   = 0x03,
	SEARCH_REQUEST = 0x04,
	SEARCH_REPLY   = 0x05,
};

RS_DEPRECATED_FOR(RsGxsForumsItems)
const uint8_t RS_PKT_SUBTYPE_GXSFORUM_GROUP_ITEM   = 0x02;

RS_DEPRECATED_FOR(RsGxsForumsItems)
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

struct RsGxsForumsSearchRequest : RsSerializable
{
	RsGxsForumsSearchRequest() : mType(RsGxsForumsItems::SEARCH_REQUEST) {}

	/// Just for easier back and forward compatibility
	RsGxsForumsItems mType;

	/// Store search match string
	std::string mQuery;

	/// @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx ) override
	{
		RS_SERIAL_PROCESS(mType);
		RS_SERIAL_PROCESS(mQuery);
	}

	~RsGxsForumsSearchRequest() override = default;
};

struct RsGxsForumsSearchReply : RsSerializable
{
	RsGxsForumsSearchReply() : mType(RsGxsForumsItems::SEARCH_REPLY) {}

	/// Just for easier back and forward compatibility
	RsGxsForumsItems mType;

	/// Results storage
	std::vector<RsGxsSearchResult> mResults;

	/// @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx ) override
	{
		RS_SERIAL_PROCESS(mType);
		RS_SERIAL_PROCESS(mResults);
	}

	~RsGxsForumsSearchReply() override = default;
};

class RsGxsForumSerialiser : public RsServiceSerializer
{
public:
	RsGxsForumSerialiser() :RsServiceSerializer(RS_SERVICE_GXS_TYPE_FORUMS) {}
	virtual ~RsGxsForumSerialiser() {}

	virtual RsItem *create_item(uint16_t service_id,uint8_t item_subtype) const ;
};
