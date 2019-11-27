/*******************************************************************************
 * libretroshare/src/grouter: grouteritems.cc                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2013 by Cyril Soler <csoler@users.sourceforge.net>                *
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
#include "util/rsprint.h"
#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvbase.h"
#include "grouteritems.h"

#include "serialiser/rstypeserializer.h"

/**********************************************************************************************/
/*                                          SERIALISER STUFF                                  */
/**********************************************************************************************/

RsItem *RsGRouterSerialiser::create_item(uint16_t service_id,uint8_t subtype) const
{
	if(RS_SERVICE_TYPE_GROUTER != service_id)
		return NULL; /* wrong type */

	switch(subtype)
	{
	case RS_PKT_SUBTYPE_GROUTER_DATA:              return new RsGRouterGenericDataItem     ();
	case RS_PKT_SUBTYPE_GROUTER_TRANSACTION_CHUNK: return new RsGRouterTransactionChunkItem();
	case RS_PKT_SUBTYPE_GROUTER_TRANSACTION_ACKN:  return new RsGRouterTransactionAcknItem ();
	case RS_PKT_SUBTYPE_GROUTER_SIGNED_RECEIPT:    return new RsGRouterSignedReceiptItem   ();
	case RS_PKT_SUBTYPE_GROUTER_MATRIX_CLUES:      return new RsGRouterMatrixCluesItem     ();
	case RS_PKT_SUBTYPE_GROUTER_MATRIX_TRACK:      return new RsGRouterMatrixTrackItem     ();
	case RS_PKT_SUBTYPE_GROUTER_FRIENDS_LIST:      return new RsGRouterMatrixFriendListItem();
	case RS_PKT_SUBTYPE_GROUTER_ROUTING_INFO:      return new RsGRouterRoutingInfoItem     ();

	default:
		return NULL;
	}
}

void RsGRouterTransactionChunkItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint64_t>(j,ctx,propagation_id,"propagation_id") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,chunk_start   ,"chunk_start") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,chunk_size    ,"chunk_size") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,total_size    ,"total_size") ;

    // Hack for backward compatibility (the chunk size is not directly next to the chunk data)

    if(j == RsGenericSerializer::DESERIALIZE)
	{
		if(chunk_size > ctx.mSize || ctx.mOffset > ctx.mSize - chunk_size) // better than if(chunk_size + offset > size)
		{
			std::cerr << __PRETTY_FUNCTION__ << ": Cannot read beyond item size. Serialisation error!" << std::endl;
			ctx.mOk = false ;
			return ;
		}
		if( NULL == (chunk_data = (uint8_t*)rs_malloc(chunk_size)))
		{
			ctx.mOk = false ;
			return ;
		}

		memcpy(chunk_data,&((uint8_t*)ctx.mData)[ctx.mOffset],chunk_size) ;
		ctx.mOffset += chunk_size ;
	}
    else if(j== RsGenericSerializer::SERIALIZE)
    {
		memcpy(&((uint8_t*)ctx.mData)[ctx.mOffset],chunk_data,chunk_size) ;
		ctx.mOffset += chunk_size ;
    }
    else if(j== RsGenericSerializer::SIZE_ESTIMATE)
		ctx.mOffset += chunk_size ;
    else
    	std::cerr << "  [Binary data] " << ", length=" << chunk_size << " data=" << RsUtil::BinToHex((uint8_t*)chunk_data,std::min(50u,chunk_size)) << ((chunk_size>50)?"...":"") << std::endl;

}
void RsGRouterTransactionAcknItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint64_t>(j,ctx,propagation_id,"propagation_id") ;
}

void RsGRouterGenericDataItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint64_t>(j,ctx,routing_id,"routing_id") ;
    RsTypeSerializer::serial_process          (j,ctx,destination_key,"destination_key") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,service_id,"service_id") ;

    RsTypeSerializer::TlvMemBlock_proxy prox(data_bytes,data_size) ;

    RsTypeSerializer::serial_process(j,ctx,prox,"data") ;

    if(ctx.mFlags & RsGenericSerializer::SERIALIZATION_FLAG_SIGNATURE)
        return ;

    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,signature,"signature") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,duplication_factor,"duplication_factor") ;
	RS_SERIAL_PROCESS(flags);

    if(j == RsGenericSerializer::DESERIALIZE) // make sure the duplication factor is not altered by friends. In the worst case, the item will duplicate a bit more.
    {
    	if(duplication_factor < 1)
        {
            duplication_factor = 1 ;
            std::cerr << "(II) correcting GRouter item duplication factor from 0 to 1, to ensure backward compat." << std::endl;
        }
    	if(duplication_factor > GROUTER_MAX_DUPLICATION_FACTOR)
        {
            std::cerr << "(WW) correcting GRouter item duplication factor of " << duplication_factor << ". This is very unexpected." << std::endl;
            duplication_factor = GROUTER_MAX_DUPLICATION_FACTOR ;
        }
    }
}

void RsGRouterSignedReceiptItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint64_t> (j,ctx,routing_id,"routing_id") ;
    RS_SERIAL_PROCESS(flags);
    RsTypeSerializer::serial_process           (j,ctx,destination_key,"destination_key") ;
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,service_id,"service_id") ;
    RsTypeSerializer::serial_process           (j,ctx,data_hash,"data_hash") ;

    if(ctx.mFlags &  RsGenericSerializer::SERIALIZATION_FLAG_SIGNATURE)
        return ;

    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,signature,"signature") ;
}

void RsGRouterRoutingInfoItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process          (j,ctx,peerId,"peerId") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,data_status,"data_status") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,tunnel_status,"tunnel_status") ;
    RsTypeSerializer::serial_process<rstime_t>  (j,ctx,received_time_TS,"received_time_TS") ;
    RsTypeSerializer::serial_process<rstime_t>  (j,ctx,last_sent_TS,"last_sent_TS") ;

    RsTypeSerializer::serial_process<rstime_t>  (j,ctx,last_tunnel_request_TS,"last_tunnel_request_TS") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,sending_attempts,"sending_attempts") ;

    RsTypeSerializer::serial_process<uint32_t>(j,ctx,client_id,"client_id") ;
    RsTypeSerializer::serial_process          (j,ctx,item_hash,"item_hash") ;
    RsTypeSerializer::serial_process          (j,ctx,tunnel_hash,"tunnel_hash") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,routing_flags,"routing_flags") ;

    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,incoming_routes,"incoming_routes") ;

    // Hack for backward compatibility. Normally we should need a single commandline to serialise/deserialise a single item here.
    // But the full item is serialised, so we need the header.

	if(j == RsGenericSerializer::DESERIALIZE)
	{
        data_item = new RsGRouterGenericDataItem() ;

        ctx.mOffset += 8 ;
        data_item->serial_process(j,ctx) ;

        if(ctx.mOffset < ctx.mSize)
		{
			receipt_item = new RsGRouterSignedReceiptItem();

			ctx.mOffset += 8 ;
			receipt_item->serial_process(j,ctx) ;
		}
        else
            receipt_item = NULL ;
    }
	else if(j == RsGenericSerializer::SERIALIZE)
    {
        uint32_t remaining_size = ctx.mSize - ctx.mOffset;
        ctx.mOk = ctx.mOk && RsGRouterSerialiser().serialise(data_item,ctx.mData,&remaining_size) ;
        ctx.mOffset += RsGRouterSerialiser().size(data_item) ;

        if(receipt_item != NULL)
		{
			remaining_size = ctx.mSize - ctx.mOffset;
			ctx.mOk = ctx.mOk && RsGRouterSerialiser().serialise(receipt_item,ctx.mData,&remaining_size);
			ctx.mOffset += RsGRouterSerialiser().size(receipt_item) ;
		}
    }
    else if(j == RsGenericSerializer::PRINT)
    {
        std::cerr << "  [Serialized data] " << std::endl;

        if(receipt_item != NULL)
			std::cerr << "  [Receipt item   ]" << std::endl;
    }
    else if(j == RsGenericSerializer::SIZE_ESTIMATE)
    {
        ctx.mOffset += RsGRouterSerialiser().size(data_item) ;

        if(receipt_item != NULL)
			ctx.mOffset += RsGRouterSerialiser().size(receipt_item) ;
    }
}

void RsGRouterMatrixFriendListItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,reverse_friend_indices,"reverse_friend_indices") ;
}

void RsGRouterMatrixTrackItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,provider_id,"provider_id") ;
    RsTypeSerializer::serial_process(j,ctx,message_id,"message_id") ;
    RsTypeSerializer::serial_process<rstime_t>(j,ctx,time_stamp,"time_stamp") ;
}

void RsGRouterMatrixCluesItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,destination_key,"destination_key") ;
    RsTypeSerializer::serial_process(j,ctx,clues,"clues") ;
}

template<> void RsTypeSerializer::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx,RoutingMatrixHitEntry& s,const std::string& name)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,s.friend_id,name+":friend_id") ;
    RsTypeSerializer::serial_process<float>   (j,ctx,s.weight,name+":weight") ;
    RsTypeSerializer::serial_process<rstime_t>  (j,ctx,s.time_stamp,name+":time_stamp") ;
}

RsGRouterGenericDataItem *RsGRouterGenericDataItem::duplicate() const
{
    RsGRouterGenericDataItem *item = new RsGRouterGenericDataItem ;

    // copy all members

    *item = *this ;

    // then duplicate the memory chunk

    if(data_size > 0)
    {
	    item->data_bytes = (uint8_t*)rs_malloc(data_size) ;

        if(item->data_bytes == NULL)
            {
                delete item ;
                return NULL ;
            }
	    memcpy(item->data_bytes,data_bytes,data_size) ;
    }
    else
	    item->data_bytes = NULL ;

    return item ;
}

RsGRouterSignedReceiptItem *RsGRouterSignedReceiptItem::duplicate() const
{
    RsGRouterSignedReceiptItem *item = new RsGRouterSignedReceiptItem ;

    // copy all members

    *item = *this ;

    return item ;
}

RsGRouterAbstractMsgItem::~RsGRouterAbstractMsgItem() = default;
