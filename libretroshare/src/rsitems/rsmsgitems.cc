/*******************************************************************************
 * libretroshare/src/rsitems: rsmsgitems.cc                                    *
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
#include <stdexcept>
#include "util/rstime.h"
#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvbase.h"

#include "rsitems/rsmsgitems.h"

#include "serialiser/rstypeserializer.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>


RsItem *RsMsgSerialiser::create_item(uint16_t service,uint8_t type) const
{
    if(service != RS_SERVICE_TYPE_MSG)
        return NULL ;

    switch(type)
	{
	case RS_PKT_SUBTYPE_DEFAULT            : return new RsMsgItem() ;                  //= 0x01;
	case RS_PKT_SUBTYPE_MSG_TAG_TYPE 	   : return new RsMsgTagType() ;               //= 0x03;
	case RS_PKT_SUBTYPE_MSG_TAGS 	 	   : return new RsMsgTags() ;                  //= 0x04;
	case RS_PKT_SUBTYPE_MSG_SRC_TAG 	   : return new RsMsgSrcId();                  //= 0x05;
	case RS_PKT_SUBTYPE_MSG_PARENT_TAG 	   : return new RsMsgParentId() ;              //= 0x06;
	case RS_PKT_SUBTYPE_MSG_GROUTER_MAP    : return new RsMsgGRouterMap();             //= 0x08;
	case RS_PKT_SUBTYPE_MSG_DISTANT_MSG_MAP : return new RsMsgDistantMessagesHashMap();//= 0x09;
	default:
		return NULL ;
	}
}

void 	RsMsgItem::clear()
{
	msgId    = 0;
	msgFlags = 0;
	sendTime = 0;
	recvTime = 0;
	subject.clear();
	message.clear();

	rspeerid_msgto.TlvClear();
	rspeerid_msgcc.TlvClear();
	rspeerid_msgbcc.TlvClear();

	rsgxsid_msgto.TlvClear();
	rsgxsid_msgcc.TlvClear();
	rsgxsid_msgbcc.TlvClear();

	attachment.TlvClear();
}

void RsMsgTagType::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_NAME,text,"text") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,rgb_color,"rgb_color") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,tagId,"tagId") ;
}

void RsMsgTags::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,msgId,"msgId") ;

#warning this is not the correct way to serialise here. We should directly call serial_process<std::vector<uint32_t> >() but for backward compatibility, we cannot

    if(j == RsGenericSerializer::DESERIALIZE)
        while(ctx.mOffset < ctx.mSize)
        {
            uint32_t n ;
			RsTypeSerializer::serial_process<uint32_t>(j,ctx,n,"tagIds element") ;
			tagIds.push_back(n) ;
        }
    else
        for(std::list<uint32_t>::iterator it(tagIds.begin());it!=tagIds.end();++it)
			RsTypeSerializer::serial_process<uint32_t>(j,ctx,*it,"tagIds element") ;
}

void RsMsgSrcId::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,msgId,"msgId") ;
    RsTypeSerializer::serial_process          (j,ctx,srcId,"srcId") ;
}

void RsMsgGRouterMap::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,ongoing_msgs,"ongoing_msgs") ;
}

void RsMsgGRouterMap::clear()
{
    ongoing_msgs.clear() ;

    return;
}

void RsMsgDistantMessagesHashMap::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,hash_map,"hash_map") ;
}

void RsMsgParentId::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,msgId,"msgId") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,msgParentId,"msgParentId") ;
}

void RsMsgItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,msgFlags,"msgFlags");
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,sendTime,"sendTime");
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,recvTime,"recvTime");

    RsTypeSerializer::serial_process          (j,ctx,TLV_TYPE_STR_SUBJECT,subject,"subject");
    RsTypeSerializer::serial_process          (j,ctx,TLV_TYPE_STR_MSG,message,"message");

    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,rspeerid_msgto,"rspeerid_msgto");
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,rspeerid_msgcc,"rspeerid_msgcc");
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,rspeerid_msgbcc,"rspeerid_msgbcc");

    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,rsgxsid_msgto,"rsgxsid_msgto");
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,rsgxsid_msgcc,"rsgxsid_msgcc");
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,rsgxsid_msgbcc,"rsgxsid_msgbcc");

    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,attachment,"attachment");

    if(ctx.mFlags & RsServiceSerializer::SERIALIZATION_FLAG_CONFIG)
    	RsTypeSerializer::serial_process<uint32_t>(j,ctx,msgId,"msgId");
}

void RsMsgTagType::clear()
{
	text.clear();
	tagId = 0;
	rgb_color = 0;
}

void RsMsgTags::clear()
{
	msgId = 0;
	tagIds.clear();
}

