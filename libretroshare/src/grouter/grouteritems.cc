#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvbase.h"
#include "grouteritems.h"

/**********************************************************************************************/
/*                                         SERIALISATION                                      */
/**********************************************************************************************/

bool RsGRouterItem::serialise_header(void *data,uint32_t& pktsize,uint32_t& tlvsize, uint32_t& offset) const
{
	tlvsize = serial_size() ;
	offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	if(!setRsItemHeader(data, tlvsize, PacketId(), tlvsize))
	{
		std::cerr << "RsFileTransferItem::serialise_header(): ERROR. Not enough size!" << std::endl;
		return false ;
	}
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsFileItemSerialiser::serialiseData() Header: " << ok << std::endl;
#endif
	offset += 8;

	return true ;
}

/**********************************************************************************************/
/*                                          SERIALISER STUFF                                  */
/**********************************************************************************************/

RsItem *RsGRouterSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if(RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype) || RS_SERVICE_TYPE_GROUTER != getRsItemService(rstype)) 
	{
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
        case RS_PKT_SUBTYPE_GROUTER_DATA:   		return deserialise_RsGRouterGenericDataItem     (data, *pktsize);
        case RS_PKT_SUBTYPE_GROUTER_TRANSACTION_CHUNK:  return deserialise_RsGRouterTransactionChunkItem(data, *pktsize);
            case RS_PKT_SUBTYPE_GROUTER_TRANSACTION_ACKN:   return deserialise_RsGRouterTransactionAcknItem (data, *pktsize);
            case RS_PKT_SUBTYPE_GROUTER_SIGNED_RECEIPT:    	return deserialise_RsGRouterSignedReceiptItem   (data, *pktsize);
            case RS_PKT_SUBTYPE_GROUTER_MATRIX_CLUES:	return deserialise_RsGRouterMatrixCluesItem     (data, *pktsize);
		case RS_PKT_SUBTYPE_GROUTER_FRIENDS_LIST:	return deserialise_RsGRouterMatrixFriendListItem(data, *pktsize);
        case RS_PKT_SUBTYPE_GROUTER_ROUTING_INFO:	return deserialise_RsGRouterRoutingInfoItem     (data, *pktsize);

		default:
				std::cerr << "RsGRouterSerialiser::deserialise(): Could not de-serialise item. SubPacket id = " << std::hex << getRsItemSubType(rstype) << " id = " << rstype << std::dec << std::endl;
			return NULL;
	}
	return NULL;
}
RsGRouterTransactionChunkItem *RsGRouterSerialiser::deserialise_RsGRouterTransactionChunkItem(void *data, uint32_t tlvsize) const
{
    uint32_t offset = 8; // skip the header
    uint32_t rssize = getRsItemSize(data);
    bool ok = true ;

    RsGRouterTransactionChunkItem *item = new RsGRouterTransactionChunkItem() ;

    /* add mandatory parts first */
    ok &= getRawUInt64(data, tlvsize, &offset, &item->propagation_id);
    ok &= getRawUInt32(data, tlvsize, &offset, &item->chunk_start);
    ok &= getRawUInt32(data, tlvsize, &offset, &item->chunk_size);
    ok &= getRawUInt32(data, tlvsize, &offset, &item->total_size);

    if( NULL == (item->chunk_data = (uint8_t*)malloc(item->chunk_size)))
    {
        std::cerr << __PRETTY_FUNCTION__ << ": Cannot allocate memory for chunk " << item->chunk_size << std::endl;
	delete item;
        return NULL ;
    }
    if(item->chunk_size + offset > rssize)
    {
        std::cerr << __PRETTY_FUNCTION__ << ": Cannot read beyond item size. Serialisation error!" << std::endl;
	delete item;
        return NULL ;
    }

    memcpy(item->chunk_data,&((uint8_t*)data)[offset],item->chunk_size) ;
    offset += item->chunk_size ;

    if (offset != rssize || !ok)
    {
        std::cerr << __PRETTY_FUNCTION__ << ": error while deserialising! Item will be dropped." << std::endl;
	delete item;
        return NULL ;
    }

    return item;
}
RsGRouterTransactionAcknItem *RsGRouterSerialiser::deserialise_RsGRouterTransactionAcknItem(void *data, uint32_t tlvsize) const
{
    uint32_t offset = 8; // skip the header
    uint32_t rssize = getRsItemSize(data);
    bool ok = true ;

    RsGRouterTransactionAcknItem *item = new RsGRouterTransactionAcknItem() ;

    /* add mandatory parts first */
    ok &= getRawUInt64(data, tlvsize, &offset, &item->propagation_id);

    if (offset != rssize || !ok)
    {
        std::cerr << __PRETTY_FUNCTION__ << ": error while deserialising! Item will be dropped." << std::endl;
	delete item;
        return NULL ;
    }

    return item;
}
RsGRouterGenericDataItem *RsGRouterSerialiser::deserialise_RsGRouterGenericDataItem(void *data, uint32_t pktsize) const
{
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

	RsGRouterGenericDataItem *item = new RsGRouterGenericDataItem() ;

	ok &= getRawUInt64(data, pktsize, &offset, &item->routing_id); 	
	ok &= item->destination_key.deserialise(data, pktsize, offset) ;
    ok &= getRawUInt32(data, pktsize, &offset, &item->service_id);
    ok &= getRawUInt32(data, pktsize, &offset, &item->data_size);

    if( NULL == (item->data_bytes = (uint8_t*)malloc(item->data_size)))
	{
		std::cerr << __PRETTY_FUNCTION__ << ": Cannot allocate memory for chunk " << item->data_size << std::endl;
		delete item;
		return NULL ;
	}

    if(item->data_size + offset > rssize)
    {
        std::cerr << __PRETTY_FUNCTION__ << ": Cannot read beyond item size. Serialisation error!" << std::endl;
	delete item;
        return NULL ;
    }

    memcpy(item->data_bytes,&((uint8_t*)data)[offset],item->data_size) ;
	offset += item->data_size ;

    ok &= item->signature.GetTlv(data, pktsize, &offset) ;

    ok &= getRawUInt32(data, pktsize, &offset, &item->randomized_distance);
    ok &= getRawUInt32(data, pktsize, &offset, &item->flags);

    if (offset != rssize || !ok)
	{
		std::cerr << __PRETTY_FUNCTION__ << ": error while deserialising! Item will be dropped." << std::endl;
		delete item;
		return NULL ;
	}

	return item;
}

RsGRouterSignedReceiptItem *RsGRouterSerialiser::deserialise_RsGRouterSignedReceiptItem(void *data, uint32_t pktsize) const
{
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

    RsGRouterSignedReceiptItem *item = new RsGRouterSignedReceiptItem() ;

    ok &= getRawUInt64(data, pktsize, &offset, &item->routing_id);
    ok &= getRawUInt32(data, pktsize, &offset, &item->flags);
    ok &= item->destination_key.deserialise(data, pktsize, offset);
    ok &= getRawUInt32(data, pktsize, &offset, &item->service_id);
    ok &= item->data_hash.deserialise(data, pktsize, offset);
    ok &= item->signature.GetTlv(data, pktsize, &offset); 	// signature

	if (offset != rssize || !ok)
	{
		std::cerr << __PRETTY_FUNCTION__ << ": error while deserialising! Item will be dropped." << std::endl;
		delete item;
		return NULL ;
	}

	return item;
}

RsGRouterRoutingInfoItem *RsGRouterSerialiser::deserialise_RsGRouterRoutingInfoItem(void *data, uint32_t pktsize) const
{
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

	RsGRouterRoutingInfoItem *item = new RsGRouterRoutingInfoItem() ;

    RsPeerId peer_id ;
    ok &= peer_id.deserialise(data, pktsize, offset) ;
    item->PeerId(peer_id) ;

    ok &= getRawUInt32(data, pktsize, &offset, &item->data_status);
    ok &= getRawUInt32(data, pktsize, &offset, &item->tunnel_status);
    ok &= getRawTimeT(data, pktsize, &offset, item->received_time_TS);
    ok &= getRawTimeT(data, pktsize, &offset, item->last_tunnel_sent_TS);
    ok &= getRawTimeT(data, pktsize, &offset, item->last_friend_sent_TS);

    ok &= getRawTimeT(data, pktsize, &offset, item->last_tunnel_request_TS);
    ok &= getRawUInt32(data, pktsize, &offset, &item->sending_attempts);

    ok &= getRawUInt32(data, pktsize, &offset, &item->client_id);
    ok &= item->item_hash.deserialise(data, pktsize, offset) ;
    ok &= item->tunnel_hash.deserialise(data, pktsize, offset) ;
    ok &= getRawUInt32(data, pktsize, &offset, &item->routing_flags) ;

    ok &= item->incoming_routes.GetTlv(data,pktsize,&offset) ;

	item->data_item = deserialise_RsGRouterGenericDataItem(&((uint8_t*)data)[offset],pktsize - offset) ;
	if(item->data_item != NULL) 
		offset += item->data_item->serial_size() ;
	else
		ok = false ;

    // receipt item is optional.

    if(offset < pktsize)
    {
        item->receipt_item = deserialise_RsGRouterSignedReceiptItem(&((uint8_t*)data)[offset],pktsize - offset) ;
        if(item->receipt_item != NULL)
            offset += item->receipt_item->serial_size() ;
        else
            ok = false ;
    }
    else
        item->receipt_item = NULL ;


	if (offset != rssize || !ok)
	{
		std::cerr << __PRETTY_FUNCTION__ << ": error while deserialising! Item will be dropped." << std::endl;
		delete item;
		return NULL ;
	}

	return item;
}
RsGRouterMatrixFriendListItem *RsGRouterSerialiser::deserialise_RsGRouterMatrixFriendListItem(void *data, uint32_t pktsize) const
{
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

	RsGRouterMatrixFriendListItem *item = new RsGRouterMatrixFriendListItem() ;

	uint32_t nb_friends = 0 ;
	ok &= getRawUInt32(data, pktsize, &offset, &nb_friends); 	// file hash

    item->reverse_friend_indices.resize(nb_friends) ;

	for(uint32_t i=0;ok && i<nb_friends;++i)
        ok &= item->reverse_friend_indices[i].deserialise(data, pktsize, offset) ;

	if (offset != rssize || !ok)
	{
		std::cerr << __PRETTY_FUNCTION__ << ": error while deserialising! Item will be dropped." << std::endl;
		delete item;
		return NULL ;
	}

	return item;
}
RsGRouterMatrixCluesItem *RsGRouterSerialiser::deserialise_RsGRouterMatrixCluesItem(void *data, uint32_t pktsize) const
{
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

	RsGRouterMatrixCluesItem *item = new RsGRouterMatrixCluesItem() ;

    ok &= item->destination_key.deserialise(data,pktsize,offset) ;
		
	uint32_t nb_clues = 0 ;
	ok &= getRawUInt32(data, pktsize, &offset, &nb_clues); 	

	item->clues.clear() ;

	for(uint32_t j=0;j<nb_clues;++j)
	{
		RoutingMatrixHitEntry HitE ;

		ok &= getRawUInt32(data, pktsize, &offset, &HitE.friend_id); 	
		ok &= getRawUFloat32(data, pktsize, &offset, HitE.weight); 	
		ok &= getRawTimeT(data, pktsize, &offset, HitE.time_stamp); 	

		item->clues.push_back(HitE) ;
	}

	if (offset != rssize || !ok)
	{
		std::cerr << __PRETTY_FUNCTION__ << ": error while deserialising! Item will be dropped." << std::endl;
		delete item;
		return NULL ;
	}

	return item;
}
RsGRouterGenericDataItem *RsGRouterGenericDataItem::duplicate() const
{
    RsGRouterGenericDataItem *item = new RsGRouterGenericDataItem ;

    // copy all members

    *item = *this ;

    // then duplicate the memory chunk

    item->data_bytes = (uint8_t*)malloc(data_size) ;
    memcpy(item->data_bytes,data_bytes,data_size) ;

    return item ;
}

RsGRouterSignedReceiptItem *RsGRouterSignedReceiptItem::duplicate() const
{
    RsGRouterSignedReceiptItem *item = new RsGRouterSignedReceiptItem ;

    // copy all members

    *item = *this ;

    return item ;
}
uint32_t RsGRouterGenericDataItem::serial_size() const
{
    uint32_t s = 8 ;	                      // header
    s += sizeof(GRouterMsgPropagationId)  ; // routing id
    s += destination_key.serial_size() ;	 // destination_key
    s += 4 ;                       		 	 // data_size
    s += 4 ;                       		 	 // service id
    s += data_size ;                        // data
    s += signature.TlvSize() ;		// signature
    s += 4 ;                                // randomized distance
    s += 4 ; 				// flags

    return s ;
}
uint32_t RsGRouterGenericDataItem::signed_data_size() const
{
    uint32_t s = 0 ;	                      // no header
    s += sizeof(GRouterMsgPropagationId)  ; // routing id
    s += destination_key.serial_size() ;	 // destination_key
    s += 4 ;                       		 	 // data_size
    s += 4 ;                       		 	 // service id
    s += data_size ;                        // data

    return s ;
}
uint32_t RsGRouterSignedReceiptItem::serial_size() const
{
    uint32_t s = 8 ;	// header
    s += sizeof(GRouterMsgPropagationId)  ; // routing id
    s += destination_key.serial_size() ;	// destination_key
    s += data_hash.serial_size() ;
    s += 4 ;  			// state
    s += 4 ;  			// service_id
    s += signature.TlvSize() ;	// signature

    return s ;
}
uint32_t RsGRouterSignedReceiptItem::signed_data_size() const
{
    uint32_t s = 0 ;	// no header
    s += sizeof(GRouterMsgPropagationId)  ; // routing id
    s += destination_key.serial_size() ;	// destination_key
    s += data_hash.serial_size() ;
    s += 4 ;  			// service_id
    s += 4 ;  			// state

    return s ;
}
uint32_t RsGRouterTransactionChunkItem::serial_size() const
{
    uint32_t s = 8 ;	// header
    s += sizeof(GRouterMsgPropagationId)  ; // routing id
    s += 4 ;				// chunk_start
    s += 4 ;				// chunk_size
    s += 4 ;				// total_size
    s += chunk_size ;			// data

    return s;
}
uint32_t RsGRouterTransactionAcknItem::serial_size() const
{
    uint32_t s = 8 ;	// header
    s += sizeof(GRouterMsgPropagationId)  ; // routing id

    return s;
}
bool RsGRouterTransactionChunkItem::serialise(void *data,uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

    /* add mandatory parts first */
    ok &= setRawUInt64(data, tlvsize, &offset, propagation_id);
    ok &= setRawUInt32(data, tlvsize, &offset, chunk_start);
    ok &= setRawUInt32(data, tlvsize, &offset, chunk_size);
    ok &= setRawUInt32(data, tlvsize, &offset, total_size);

    memcpy(&((uint8_t*)data)[offset],chunk_data,chunk_size) ;
    offset += chunk_size ;

    if (offset != tlvsize)
    {
        ok = false;
        std::cerr << "RsGRouterGenericDataItem::serialisedata() size error! " << std::endl;
    }

    return ok;
}
bool RsGRouterGenericDataItem::serialise(void *data,uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

    /* add mandatory parts first */
    ok &= setRawUInt64(data, tlvsize, &offset, routing_id);
    ok &= destination_key.serialise(data, tlvsize, offset) ;
    ok &= setRawUInt32(data, tlvsize, &offset, service_id);
    ok &= setRawUInt32(data, tlvsize, &offset, data_size);

    memcpy(&((uint8_t*)data)[offset],data_bytes,data_size) ;
    offset += data_size ;

    ok &= signature.SetTlv(data, tlvsize, &offset) ;

    ok &= setRawUInt32(data, tlvsize, &offset, randomized_distance) ;
    ok &= setRawUInt32(data, tlvsize, &offset, flags) ;

    if (offset != tlvsize)
    {
        ok = false;
        std::cerr << "RsGRouterGenericDataItem::serialisedata() size error! " << std::endl;
    }

    return ok;
}
bool RsGRouterTransactionAcknItem::serialise(void *data,uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

    /* add mandatory parts first */
    ok &= setRawUInt64(data, tlvsize, &offset, propagation_id);

    if (offset != tlvsize)
    {
        ok = false;
        std::cerr << "RsGRouterGenericDataItem::serialisedata() size error! " << std::endl;
    }

    return ok;
}
bool RsGRouterGenericDataItem::serialise_signed_data(void *data,uint32_t& size) const
{
    bool ok = true;

    uint32_t offset = 0;
    uint32_t tlvsize = signed_data_size() ;

    /* add mandatory parts first */
    ok &= setRawUInt64(data, tlvsize, &offset, routing_id);
    ok &= destination_key.serialise(data, tlvsize, offset) ;
    ok &= setRawUInt32(data, tlvsize, &offset, service_id);
    ok &= setRawUInt32(data, tlvsize, &offset, data_size);

    memcpy(&((uint8_t*)data)[offset],data_bytes,data_size) ;
    offset += data_size ;

    if (offset != tlvsize)
    {
        ok = false;
        std::cerr << "RsGRouterGenericDataItem::serialisedata() size error! " << std::endl;
    }

    return ok;
}
bool RsGRouterSignedReceiptItem::serialise(void *data,uint32_t& size) const
{
	uint32_t tlvsize,offset=0;
	bool ok = true;
	
	if(!serialise_header(data,size,tlvsize,offset))
		return false ;

	/* add mandatory parts first */
    ok &= setRawUInt64(data, tlvsize, &offset, routing_id);
    ok &= setRawUInt32(data, tlvsize, &offset, flags);
    ok &= destination_key.serialise(data,tlvsize,offset) ;
    ok &= setRawUInt32(data, tlvsize, &offset, service_id);
    ok &= data_hash.serialise(data,tlvsize,offset) ;
    ok &= signature.SetTlv(data,tlvsize,&offset) ;

	if (offset != tlvsize)
	{
		ok = false;
        std::cerr << "RsGRouterReceiptItem::serialisedata() size error! " << std::endl;
	}

	return ok;
}
bool RsGRouterSignedReceiptItem::serialise_signed_data(void *data,uint32_t& size) const
{
    bool ok = true;

    uint32_t offset=0;
    uint32_t tlvsize = signed_data_size() ;

    /* add mandatory parts first */
    ok &= setRawUInt64(data, tlvsize, &offset, routing_id);
    ok &= setRawUInt32(data, tlvsize, &offset, flags);
    ok &= destination_key.serialise(data,tlvsize,offset) ;
    ok &= setRawUInt32(data, tlvsize, &offset, service_id);
    ok &= data_hash.serialise(data,tlvsize,offset) ;

    if (offset != tlvsize)
    {
        ok = false;
        std::cerr << "RsGRouterReceiptItem::serialisedata() size error! " << std::endl;
    }

    return ok;
}
/* serialise the data to the buffer */
uint32_t RsGRouterMatrixCluesItem::serial_size() const
{
	uint32_t s = 8 ; 									// header

    s += destination_key.serial_size() ;				// Key size
	s += 4 ; 				// list<RoutingMatrixHitEntry>::size()
	s += (4+4+8) * clues.size() ;

	return s ;
}
uint32_t RsGRouterMatrixFriendListItem::serial_size() const
{
	uint32_t s = 8 ; 									// header
	s += 4  ; 											// reverse_friend_indices.size()
	s += RsPeerId::SIZE_IN_BYTES * reverse_friend_indices.size() ; // sha1 for published_key

	return s ;
}
uint32_t RsGRouterRoutingInfoItem::serial_size() const
{
    uint32_t s = 8 ; 			// header
    s += PeerId().serial_size() ;

    s += 4  ; 				// data status_flags
    s += 4  ; 				// tunnel status_flags
    s += 8  ; 				// received_time
    s += 8  ; 				// last_tunnel_sent_TS
    s += 8  ; 				// last_friend_sent_TS

    s += 8  ; 				// last_TR_TS
    s += 4  ; 				// sending attempts

    s += sizeof(GRouterServiceId)  ; 	// service_id
    s += tunnel_hash.serial_size() ;
    s += item_hash.serial_size() ;

    s += 4 ; 				// routing_flags
    s += incoming_routes.TlvSize() ;	// incoming_routes

    s += data_item->serial_size();	// data_item

    if(receipt_item != NULL)
        s += receipt_item->serial_size();	// receipt_item

    return s ;
}

bool RsGRouterMatrixFriendListItem::serialise(void *data,uint32_t& size) const
{
	uint32_t tlvsize,offset=0;
	bool ok = true;
	
	if(!serialise_header(data,size,tlvsize,offset))
		return false ;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, reverse_friend_indices.size());

	for(uint32_t i=0;ok && i<reverse_friend_indices.size();++i)
        ok &= reverse_friend_indices[i].serialise(data,tlvsize,offset) ;

	if (offset != tlvsize)
	{
		ok = false;
        std::cerr << "RsGRouterMatrixFriendListItem::serialisedata() size error! " << std::endl;
	}

	return ok;
}
bool RsGRouterMatrixCluesItem::serialise(void *data,uint32_t& size) const
{
	uint32_t tlvsize,offset=0;
	bool ok = true;
	
	if(!serialise_header(data,size,tlvsize,offset))
		return false ;

	/* add mandatory parts first */
    ok &= destination_key.serialise(data,tlvsize,offset) ;
	ok &= setRawUInt32(data, tlvsize, &offset, clues.size());

	for(std::list<RoutingMatrixHitEntry>::const_iterator it2(clues.begin());it2!=clues.end();++it2)
	{
		ok &= setRawUInt32(data, tlvsize, &offset,  (*it2).friend_id) ;
		ok &= setRawUFloat32(data, tlvsize, &offset, (*it2).weight) ;
		ok &= setRawTimeT(data, tlvsize, &offset,  (*it2).time_stamp) ;
	}

	if (offset != tlvsize)
	{
		ok = false;
        std::cerr << "RsGRouterMatrixCluesItem::serialisedata() size error! " << std::endl;
	}

	return ok;
}

bool FriendTrialRecord::deserialise(void *data,uint32_t& offset,uint32_t size)
{
	bool ok = true ;
	ok &= friend_id.deserialise(data, size, offset) ;
	ok &= getRawTimeT(data, size, &offset, time_stamp) ;
	ok &= getRawUFloat32(data, size, &offset, probability) ;
	ok &= getRawUInt32(data, size, &offset, &nb_friends) ;
	return ok ;
}
bool FriendTrialRecord::serialise(void *data,uint32_t& offset,uint32_t size) const
{
	bool ok = true ;
	ok &= friend_id.serialise(data, size, offset) ;
	ok &= setRawTimeT(data, size, &offset, time_stamp) ;
	ok &= setRawUFloat32(data, size, &offset, probability) ;
	ok &= setRawUInt32(data, size, &offset, nb_friends) ;
	return ok ;
}
bool RsGRouterRoutingInfoItem::serialise(void *data,uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

    ok &= PeerId().serialise(data, tlvsize, offset) ;	// we keep this.
    ok &= setRawUInt32(data, tlvsize, &offset, data_status) ;
    ok &= setRawUInt32(data, tlvsize, &offset, tunnel_status) ;
    ok &= setRawTimeT(data, tlvsize, &offset, received_time_TS) ;
    ok &= setRawTimeT(data, tlvsize, &offset, last_tunnel_sent_TS) ;
    ok &= setRawTimeT(data, tlvsize, &offset, last_friend_sent_TS) ;
    ok &= setRawTimeT(data, tlvsize, &offset, last_tunnel_request_TS) ;
    ok &= setRawUInt32(data, tlvsize, &offset, sending_attempts) ;

    ok &= setRawUInt32(data, tlvsize, &offset, client_id) ;
    ok &= item_hash.serialise(data, tlvsize, offset) ;
    ok &= tunnel_hash.serialise(data, tlvsize, offset) ;
    ok &= setRawUInt32(data, tlvsize, &offset, routing_flags) ;

    ok &= incoming_routes.SetTlv(data,tlvsize,&offset) ;

     uint32_t ns = size - offset ;
     ok &= data_item->serialise( &((uint8_t*)data)[offset], ns) ;
     offset += data_item->serial_size() ;

     if(receipt_item != NULL)
     {
         uint32_t ns = size - offset ;
         ok &= receipt_item->serialise( &((uint8_t*)data)[offset], ns) ;
         offset += receipt_item->serial_size() ;
     }
    if (offset != tlvsize)
    {
        ok = false;
        std::cerr << "RsGRouterRoutingInfoItem::serialisedata() size error! " << std::endl;
    }

    return ok;
}

// -----------------------------------------------------------------------------------//
// -------------------------------------  IO  --------------------------------------- // 
// -----------------------------------------------------------------------------------//
//
std::ostream& RsGRouterSignedReceiptItem::print(std::ostream& o, uint16_t)
{
    o << "RsGRouterReceiptItem:" << std::endl ;
    o << "  direct origin: \""<< PeerId() << "\"" << std::endl ;
    o << "  Mid:            " << std::hex << routing_id << std::dec << std::endl ;
    o << "  State:          " << flags << std::endl ;
    o << "  Dest:           " << destination_key << std::endl ;
    o << "  Sign:           " << signature.keyId << std::endl ;

    return o ;
}
std::ostream& RsGRouterGenericDataItem::print(std::ostream& o, uint16_t)
{
    o << "RsGRouterGenericDataItem:" << std::endl ;
    o << "  Direct origin: \""<< PeerId() << "\"" << std::endl ;
    o << "  Routing ID:     " << std::hex << routing_id << std::dec << "\"" << std::endl ;
    o << "  Key:            " << destination_key.toStdString() << std::endl ;
    o << "  Data size:      " << data_size << std::endl ;
    o << "  Data hash:      " << RsDirUtil::sha1sum(data_bytes,data_size)  << std::endl ;
    o << "  signature key:  " << signature.keyId << std::endl;
    o << "  randomized dist:" << randomized_distance << std::endl;
    o << "  flags:          " << flags << std::endl;

    return o ;
}

std::ostream& RsGRouterRoutingInfoItem::print(std::ostream& o, uint16_t)
{
    o << "RsGRouterRoutingInfoItem:" << std::endl ;
    o << "  direct origin:   "<< PeerId() << std::endl ;
    o << "  data   status:   "<< std::hex<< data_status << std::dec << std::endl ;
    o << "  tunnel status:   "<< tunnel_status << std::endl ;
        o << "  recv time:       "<< received_time_TS << std::endl 	;
        o << "  Last tunnel sent:       "<< last_tunnel_sent_TS << std::endl ;
        o << "  Last friend sent:       "<< last_friend_sent_TS << std::endl ;
        o << "  Sending attempts:"<< sending_attempts << std::endl ;
        o << "  destination key: "<< data_item->destination_key << std::endl ;
    o << "  Client id:       "<< client_id << std::endl ;
    o << "  item hash:     "<< item_hash << std::endl ;
    o << "  tunnel hash:     "<< tunnel_hash << std::endl ;
        o << "  Data size:       "<< data_item->data_size << std::endl ;
        o << "  Signed receipt:  "<< (void*)receipt_item << std::endl ;

	return o ;
}

std::ostream& RsGRouterMatrixCluesItem::print(std::ostream& o, uint16_t)
{
	o << "RsGRouterMatrixCluesItem:" << std::endl ;
	o << "  destination k:  " << destination_key.toStdString()  << std::endl;
	o << "  routing clues:  " << clues.size() << std::endl;

	for(std::list<RoutingMatrixHitEntry>::const_iterator it(clues.begin());it!=clues.end();++it)
		o << "    " << (*it).friend_id << " " << (*it).time_stamp << " " << (*it).weight << std::endl;

	return o ;
}
std::ostream& RsGRouterTransactionChunkItem::print(std::ostream& o, uint16_t)
{
    o << "RsGRouterTransactionChunkItem:" << std::endl ;
    o << "   total_size:  " << total_size << std::endl;
    o << "   chunk_size:  " << chunk_size << std::endl;
    o << "  chunk_start:  " << chunk_start << std::endl;

    return o ;
}
std::ostream& RsGRouterTransactionAcknItem::print(std::ostream& o, uint16_t)
{
    o << "RsGRouterTransactionAcknItem:" << std::endl ;
    o << "   routing id:  " << propagation_id << std::endl;

    return o ;
}
std::ostream& RsGRouterMatrixFriendListItem::print(std::ostream& o, uint16_t)
{
	o << "RsGRouterMatrixCluesItem:" << std::endl ;
	o << "  friends:  " << reverse_friend_indices.size() << std::endl;

	return o ;
}
