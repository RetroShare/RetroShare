/*******************************************************************************
 * libretroshare/src/rsitems: rsmsgitems.h                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie <retroshare@lunamutt.com>              *
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

#include "retroshare/rstypes.h"
#include "serialiser/rstlvkeys.h"
#include "rsitems/rsserviceids.h"
#include "serialiser/rsserial.h"

#include "serialiser/rstlvidset.h"
#include "serialiser/rstlvfileitem.h"
#include "grouter/grouteritems.h"


/**************************************************************************/

// for defining tags themselves and msg tags
const uint8_t RS_PKT_SUBTYPE_MSG_TAG_TYPE 	   = 0x03;
const uint8_t RS_PKT_SUBTYPE_MSG_TAGS 	 	   = 0x04;
const uint8_t RS_PKT_SUBTYPE_MSG_SRC_TAG 	   = 0x05;
const uint8_t RS_PKT_SUBTYPE_MSG_PARENT_TAG 	   = 0x06;
const uint8_t RS_PKT_SUBTYPE_MSG_INVITE    	   = 0x07;
const uint8_t RS_PKT_SUBTYPE_MSG_GROUTER_MAP  	   = 0x08;
const uint8_t RS_PKT_SUBTYPE_MSG_DISTANT_MSG_MAP  = 0x09;


/**************************************************************************/

const uint32_t RS_MSG_FLAGS_OUTGOING              = 0x00000001;
const uint32_t RS_MSG_FLAGS_PENDING               = 0x00000002;
const uint32_t RS_MSG_FLAGS_DRAFT                 = 0x00000004;
const uint32_t RS_MSG_FLAGS_NEW                   = 0x00000010;
const uint32_t RS_MSG_FLAGS_TRASH                 = 0x00000020;
const uint32_t RS_MSG_FLAGS_UNREAD_BY_USER        = 0x00000040;
const uint32_t RS_MSG_FLAGS_REPLIED               = 0x00000080;
const uint32_t RS_MSG_FLAGS_FORWARDED             = 0x00000100;
const uint32_t RS_MSG_FLAGS_STAR                  = 0x00000200;
const uint32_t RS_MSG_FLAGS_PARTIAL               = 0x00000400;
const uint32_t RS_MSG_FLAGS_USER_REQUEST          = 0x00000800;
const uint32_t RS_MSG_FLAGS_FRIEND_RECOMMENDATION = 0x00001000;
const uint32_t RS_MSG_FLAGS_RETURN_RECEPT         = 0x00002000;
const uint32_t RS_MSG_FLAGS_ENCRYPTED             = 0x00004000;
const uint32_t RS_MSG_FLAGS_DISTANT               = 0x00008000;
const uint32_t RS_MSG_FLAGS_SIGNATURE_CHECKS      = 0x00010000;
const uint32_t RS_MSG_FLAGS_SIGNED                = 0x00020000;
const uint32_t RS_MSG_FLAGS_LOAD_EMBEDDED_IMAGES  = 0x00040000;
const uint32_t RS_MSG_FLAGS_DECRYPTED             = 0x00080000;
const uint32_t RS_MSG_FLAGS_ROUTED                = 0x00100000;
const uint32_t RS_MSG_FLAGS_PUBLISH_KEY           = 0x00200000;

const uint32_t RS_MSG_FLAGS_SYSTEM                = RS_MSG_FLAGS_USER_REQUEST | RS_MSG_FLAGS_FRIEND_RECOMMENDATION | RS_MSG_FLAGS_PUBLISH_KEY;

class RsMessageItem: public RsItem
{
	public:
		RsMessageItem(uint8_t msg_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_MSG,msg_subtype) 
		{
			setPriorityLevel(QOS_PRIORITY_RS_MSG_ITEM) ;
		}

		virtual ~RsMessageItem() {}
		virtual void clear() {}
};


class RsMsgItem: public RsMessageItem
{
	public:
		RsMsgItem() :RsMessageItem(RS_PKT_SUBTYPE_DEFAULT) {}

		virtual ~RsMsgItem() {}
		virtual void clear();

		virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */);

		// ----------- Specific fields ------------- //

		uint32_t msgFlags;
		uint32_t msgId;

		uint32_t sendTime;
		uint32_t recvTime;

		std::string subject;
		std::string message;

		RsTlvPeerIdSet rspeerid_msgto;
		RsTlvPeerIdSet rspeerid_msgcc;
		RsTlvPeerIdSet rspeerid_msgbcc;

		RsTlvGxsIdSet rsgxsid_msgto;
		RsTlvGxsIdSet rsgxsid_msgcc;
		RsTlvGxsIdSet rsgxsid_msgbcc;

		RsTlvFileSet attachment;
};

class RsMsgTagType : public RsMessageItem
{
	public:
		RsMsgTagType() :RsMessageItem(RS_PKT_SUBTYPE_MSG_TAG_TYPE) {}

		virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */);

		virtual ~RsMsgTagType() {}
		virtual void clear();

		// ----------- Specific fields ------------- //
		//
		std::string text;
		uint32_t rgb_color;
		uint32_t tagId;
};

class RsMsgTags : public RsMessageItem
{
public:
	RsMsgTags()
		:RsMessageItem(RS_PKT_SUBTYPE_MSG_TAGS) {}

		virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */);

	virtual ~RsMsgTags() {}
	virtual void clear();

		// ----------- Specific fields ------------- //
		//
	uint32_t msgId;
	std::list<uint32_t> tagIds;
};

class RsMsgSrcId : public RsMessageItem
{
	public:
		RsMsgSrcId() : RsMessageItem(RS_PKT_SUBTYPE_MSG_SRC_TAG) {}

		virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */);

		virtual ~RsMsgSrcId() {}
        virtual void clear(){}

		// ----------- Specific fields ------------- //
		//

		uint32_t msgId;
		RsPeerId srcId;
};

class RsMsgGRouterMap : public RsMessageItem
{
    public:
        RsMsgGRouterMap() : RsMessageItem(RS_PKT_SUBTYPE_MSG_GROUTER_MAP) {}

		virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */);

        virtual ~RsMsgGRouterMap() {}
        virtual void clear();

        // ----------- Specific fields ------------- //
        //
        std::map<GRouterMsgPropagationId,uint32_t> ongoing_msgs ;
};
class RsMsgDistantMessagesHashMap : public RsMessageItem
{
    public:
        RsMsgDistantMessagesHashMap() : RsMessageItem(RS_PKT_SUBTYPE_MSG_DISTANT_MSG_MAP) {}

		virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */);

        virtual ~RsMsgDistantMessagesHashMap() {}
        virtual void clear() { hash_map.clear() ;}

        // ----------- Specific fields ------------- //
        //
        std::map<Sha1CheckSum,uint32_t> hash_map ;
};
class RsMsgParentId : public RsMessageItem
{
	public:
		RsMsgParentId() : RsMessageItem(RS_PKT_SUBTYPE_MSG_PARENT_TAG) {}

		virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */);

		virtual ~RsMsgParentId() {}
        virtual void clear(){}

		// ----------- Specific fields ------------- //
		//
		uint32_t msgId;
		uint32_t msgParentId;
};

class RsMsgSerialiser: public RsServiceSerializer
{
public:
	RsMsgSerialiser(
	        RsSerializationFlags flags = RsSerializationFlags::NONE ):
	    RsServiceSerializer(RS_SERVICE_TYPE_MSG, flags){}

	RsItem* create_item(uint16_t service,uint8_t type) const override;

	~RsMsgSerialiser() override = default;
};
