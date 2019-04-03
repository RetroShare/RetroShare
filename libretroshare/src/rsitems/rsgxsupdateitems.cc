/*******************************************************************************
 * libretroshare/src/rsitems: rsgxsupdateitems.cc                              *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012 Christopher Evi-Parker                                       *
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
#include "serialiser/rstypeserializer.h"
#include "serialiser/rsbaseserial.h"

#include "rsgxsupdateitems.h"

/**********************************************************************************************/
/*                                         SERIALIZER                                         */
/**********************************************************************************************/

RsItem* RsGxsUpdateSerialiser::create_item(uint16_t service,uint8_t item_subtype) const
{
    if(service != SERVICE_TYPE)
        return NULL ;

    switch(item_subtype)
    {
		case RS_PKT_SUBTYPE_GXS_MSG_UPDATE:        return new RsGxsMsgUpdateItem(SERVICE_TYPE);
		case RS_PKT_SUBTYPE_GXS_GRP_UPDATE:        return new RsGxsGrpUpdateItem(SERVICE_TYPE);
		case RS_PKT_SUBTYPE_GXS_SERVER_GRP_UPDATE: return new RsGxsServerGrpUpdateItem(SERVICE_TYPE);
		case RS_PKT_SUBTYPE_GXS_SERVER_MSG_UPDATE: return new RsGxsServerMsgUpdateItem(SERVICE_TYPE);
		case RS_PKT_SUBTYPE_GXS_GRP_CONFIG:        return new RsGxsGrpConfigItem(SERVICE_TYPE);
    default:
        return NULL ;
    }
}

/**********************************************************************************************/
/*                                         CLEAR                                              */
/**********************************************************************************************/

void RsGxsGrpUpdateItem::clear()
{
	grpUpdateTS = 0;
	peerID.clear();
}

void RsGxsMsgUpdateItem::clear()
{
    msgUpdateInfos.clear();
    peerID.clear();
}

void RsGxsServerMsgUpdateItem::clear()
{
    msgUpdateTS = 0;
    grpId.clear();
}

void RsGxsServerGrpUpdateItem::clear()
{
    grpUpdateTS = 0;
}

/**********************************************************************************************/
/*                                          SERIALISER                                        */
/**********************************************************************************************/

void RsGxsGrpUpdateItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process          (j,ctx,peerID,"peerID");
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,grpUpdateTS,"grpUpdateTS");
}

void RsGxsServerGrpUpdateItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,grpUpdateTS,"grpUpdateTS");
}

void RsGxsMsgUpdateItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,peerID,"peerID");
    RsTypeSerializer::serial_process(j,ctx,msgUpdateInfos,"msgUpdateInfos");
}

template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t &offset, const RsGxsMsgUpdateItem::MsgUpdateInfo& info)
{
    bool ok = true ;

    ok = ok && setRawUInt32(data,size,&offset,info.time_stamp);
    ok = ok && setRawUInt32(data,size,&offset,info.message_count);

    return ok;
}
template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, RsGxsMsgUpdateItem::MsgUpdateInfo& info)
{
    bool ok = true ;

    ok = ok && getRawUInt32(data,size,&offset,&info.time_stamp);
    ok = ok && getRawUInt32(data,size,&offset,&info.message_count);

    return ok;
}
template<> uint32_t RsTypeSerializer::serial_size(const RsGxsMsgUpdateItem::MsgUpdateInfo& /* info */) { return 8; }

template<> void RsTypeSerializer::print_data(const std::string& name,const RsGxsMsgUpdateItem::MsgUpdateInfo& info)
{
    std::cerr << "[MsgUpdateInfo]: " << name << ": " << info.time_stamp << ", " << info.message_count << std::endl;
}

void RsGxsServerMsgUpdateItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process          (j,ctx,grpId,"grpId");
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,msgUpdateTS,"msgUpdateTS");
}
void RsGxsGrpConfigItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process          (j,ctx,grpId,"grpId") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,msg_keep_delay,"msg_keep_delay") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,msg_send_delay,"msg_send_delay") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,msg_req_delay,"msg_req_delay") ;
}

