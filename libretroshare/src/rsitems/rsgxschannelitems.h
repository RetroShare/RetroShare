/*******************************************************************************
 * libretroshare/src/rsitems: rsgxschannelitems.h                              *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012  Robert Fernie <retroshare@lunamutt.com>                 *
 * Copyright (C) 2021  Gioacchino Mazzurco <gio@eigenlab.org>                  *
 * Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>       *
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
#include "rsitems/rsgxscommentitems.h"
#include "rsitems/rsgxsitems.h"

#include "serialiser/rstlvfileitem.h"
#include "serialiser/rstlvimage.h"
#include "serialiser/rsserializable.h"
#include "retroshare/rsgxschannels.h"
#include "serialiser/rsserializer.h"
#include "util/rsdir.h"

enum class RsGxsChannelItems : uint8_t
{
	GROUP_ITEM = 0x02,
	POST_ITEM  = 0x03,
	SEARCH_REQUEST = 0x04,
	SEARCH_REPLY = 0x05,
};

RS_DEPRECATED_FOR(RsGxsChannelItems)
const uint8_t RS_PKT_SUBTYPE_GXSCHANNEL_GROUP_ITEM = 0x02;

RS_DEPRECATED_FOR(RsGxsChannelItems)
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

struct RsGxsChannelsSearchRequest : RsSerializable
{
	RsGxsChannelsSearchRequest() : mType(RsGxsChannelItems::SEARCH_REQUEST) {}

	/// Just for easier back and forward compatibility
	RsGxsChannelItems mType;

	/// Store search match string
	std::string mQuery;

	/// @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx ) override
	{
		RS_SERIAL_PROCESS(mType);
		RS_SERIAL_PROCESS(mQuery);
	}

	~RsGxsChannelsSearchRequest() override = default;
};

struct RsGxsChannelsSearchReply : RsSerializable
{
	RsGxsChannelsSearchReply() : mType(RsGxsChannelItems::SEARCH_REPLY) {}

	/// Just for easier back and forward compatibility
	RsGxsChannelItems mType;

	/// Results storage
	std::vector<RsGxsSearchResult> mResults;

	/// @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx ) override
	{
		RS_SERIAL_PROCESS(mType);
		RS_SERIAL_PROCESS(mResults);
	}

	~RsGxsChannelsSearchReply() override = default;
};

class RsGxsChannelSerialiser : public RsGxsCommentSerialiser
{
public:

	RsGxsChannelSerialiser() :RsGxsCommentSerialiser(RS_SERVICE_GXS_TYPE_CHANNELS) {}
	virtual     ~RsGxsChannelSerialiser() {}

    virtual RsItem *create_item(uint16_t service_id,uint8_t item_subtype) const ;
};
