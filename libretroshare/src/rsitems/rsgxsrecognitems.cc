/*
 * libretroshare/src/serialiser: rsgxsrecogitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2013-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#include "rsitems/rsgxsrecognitems.h"
#include "serialiser/rstypeserializer.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

RsItem *RsGxsRecognSerialiser::create_item(uint16_t service, uint8_t item_sub_id) const
{
    if(service != RS_SERVICE_TYPE_GXS_RECOGN)
        return NULL ;

    switch(item_sub_id)
    {
    case RS_PKT_SUBTYPE_RECOGN_REQ: return new RsGxsRecognReqItem();
    case RS_PKT_SUBTYPE_RECOGN_SIGNER: return new RsGxsRecognSignerItem();
    case RS_PKT_SUBTYPE_RECOGN_TAG: return new RsGxsRecognTagItem();
    default:
        return NULL ;
    }
}

void 	RsGxsRecognReqItem::clear()
{
	issued_at = 0;
	period = 0;
	tag_class = 0;
	tag_type = 0;

	identity.clear();
	nickname.clear();
	comment.clear();

	sign.TlvClear();

}
void RsGxsRecognReqItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,issued_at  ,"issued_at") ;
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,period     ,"period") ;
    RsTypeSerializer::serial_process<uint16_t> (j,ctx,tag_class  ,"tag_class") ;
    RsTypeSerializer::serial_process<uint16_t> (j,ctx,tag_type   ,"tag_type") ;
    RsTypeSerializer::serial_process           (j,ctx,identity   ,"identity") ;
    RsTypeSerializer::serial_process           (j,ctx,1,nickname ,"nickname") ;
    RsTypeSerializer::serial_process           (j,ctx,1,comment  ,"comment") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,sign       ,"sign") ;
}

void 	RsGxsRecognTagItem::clear()
{
	valid_from = 0;
	valid_to = 0;

	tag_class = 0;
	tag_type = 0;

	identity.clear();
	nickname.clear();

	sign.TlvClear();
}

void RsGxsRecognTagItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,valid_from ,"valid_from") ;
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,valid_to   ,"valid_to") ;
    RsTypeSerializer::serial_process<uint16_t> (j,ctx,tag_class  ,"tag_class") ;
    RsTypeSerializer::serial_process<uint16_t> (j,ctx,tag_type   ,"tag_type") ;
    RsTypeSerializer::serial_process           (j,ctx,identity   ,"identity");
    RsTypeSerializer::serial_process           (j,ctx,1,nickname ,"nickname") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,sign       ,"sign") ;
}

void 	RsGxsRecognSignerItem::clear()
{
	signing_classes.TlvClear();
	key.TlvClear();
	sign.TlvClear();
}

void RsGxsRecognSignerItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,signing_classes ,"signing_classes") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,key             ,"key");
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,sign            ,"sign") ;
}



