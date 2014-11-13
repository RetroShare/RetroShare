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

/* serialise the data to the buffer */
// uint32_t RsGRouterPublishKeyItem::serial_size() const
// {
// 	uint32_t s = 8 ; // header
// 	s += POW_PAYLOAD_SIZE  ; // proof of work bytes
// 	s += 4  ; // diffusion_id
//     s += published_key.serial_size() ; // sha1 for published_key
// 	s += 4  ; // service id
// 	s += 4  ; // randomized distance
// 	s += GetTlvStringSize(description_string) ; // description
//     s += fingerprint.serial_size() ;		// fingerprint
// 
// 	return s ;
// }
//bool RsGRouterPublishKeyItem::serialise(void *data, uint32_t& pktsize) const
//{
//	uint32_t tlvsize,offset=0;
//	bool ok = true;
//	
//	if(!serialise_header(data,pktsize,tlvsize,offset))
//		return false ;
//
//	memcpy(&((uint8_t*)data)[offset],pow_bytes,POW_PAYLOAD_SIZE) ;
//	offset += 8 ;
//
//	/* add mandatory parts first */
//    ok &= setRawUInt32(data, tlvsize, &offset, diffusion_id);
//    ok &= published_key.serialise(data, tlvsize, offset) ;
//	ok &= setRawUInt32(data, tlvsize, &offset, service_id);
//	ok &= setRawUFloat32(data, tlvsize, &offset, randomized_distance);
//	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, description_string);
//    ok &= fingerprint.serialise(data, tlvsize, offset) ;
//
//	if (offset != tlvsize)
//	{
//		ok = false;
//		std::cerr << "RsFileItemSerialiser::serialiseData() Size Error! " << std::endl;
//	}
//
//	return ok;
//}

/**********************************************************************************************/
/*                                        PROOF OF WORK STUFF                                 */
/**********************************************************************************************/

bool RsGRouterProofOfWorkObject::checkProofOfWork()
{
	uint32_t size = serial_size() ;
	unsigned char *mem = (unsigned char *)malloc(size) ;

	if(mem == NULL)
	{
		std::cerr << "RsGRouterProofOfWorkObject: cannot allocate memory for " << size << " bytes." << std::endl;
		return false ;
	}

	serialise(mem,size) ;
	bool res = checkProofOfWork(mem,size) ;

	free(mem) ;
	return res ;
}

bool RsGRouterProofOfWorkObject::updateProofOfWork()
{
	uint32_t size = serial_size() ;
	unsigned char *mem = (unsigned char *)malloc(size) ;

	if(mem == NULL)
	{
		std::cerr << "RsGRouterProofOfWorkObject: cannot allocate memory for " << size << " bytes." << std::endl;
		return false ;
	}

	serialise(mem,size) ;

	memset(mem,0,POW_PAYLOAD_SIZE) ;	// init the payload

	while(true)
	{
		if(checkProofOfWork(mem,size))
			break ;

		int k ;
		for(k=0;k<POW_PAYLOAD_SIZE;++k)
		{
			++mem[k] ;
			if(mem[k]!=0)
				break ;
		}
		if(k == POW_PAYLOAD_SIZE)
			return false ;
	}
	memcpy(pow_bytes,mem,POW_PAYLOAD_SIZE) ;	// copy the good bytes.

	free(mem) ;
	return true ;
}


bool RsGRouterProofOfWorkObject::checkProofOfWork(unsigned char *mem,uint32_t size)
{
	Sha1CheckSum sum = RsDirUtil::sha1sum(mem,size) ;

    for(int i=0;i<PROOF_OF_WORK_REQUESTED_BYTES;++i)
        if(sum.toByteArray()[i] != 0)
            return false ;

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
		//case RS_PKT_SUBTYPE_GROUTER_PUBLISH_KEY:  return deserialise_RsGRouterPublishKeyItem(data, *pktsize);
		case RS_PKT_SUBTYPE_GROUTER_DATA:   		return deserialise_RsGRouterGenericDataItem(data, *pktsize);
		case RS_PKT_SUBTYPE_GROUTER_ACK:    		return deserialise_RsGRouterACKItem(data, *pktsize);
		case RS_PKT_SUBTYPE_GROUTER_MATRIX_CLUES:	return deserialise_RsGRouterMatrixCluesItem(data, *pktsize);
		case RS_PKT_SUBTYPE_GROUTER_FRIENDS_LIST:	return deserialise_RsGRouterMatrixFriendListItem(data, *pktsize);
		case RS_PKT_SUBTYPE_GROUTER_ROUTING_INFO:	return deserialise_RsGRouterRoutingInfoItem(data, *pktsize);
		default:
				std::cerr << "RsGRouterSerialiser::deserialise(): Could not de-serialise item. SubPacket id = " << std::hex << getRsItemSubType(rstype) << " id = " << rstype << std::dec << std::endl;
			return NULL;
	}
	return NULL;
}

//RsGRouterPublishKeyItem *RsGRouterSerialiser::deserialise_RsGRouterPublishKeyItem(void *data, uint32_t pktsize) const
//{
//	uint32_t offset = 8; // skip the header 
//	uint32_t rssize = getRsItemSize(data);
//	bool ok = true ;
//
//	RsGRouterPublishKeyItem *item = new RsGRouterPublishKeyItem() ;
//
//	memcpy(&((uint8_t*)data)[offset],item->pow_bytes,RsGRouterProofOfWorkObject::POW_PAYLOAD_SIZE) ;
//	offset += 8 ;
//
//	ok &= getRawUInt32(data, pktsize, &offset, &item->diffusion_id); 	// file hash
//    ok &= item->published_key.deserialise(data, pktsize, offset) ;
//	ok &= getRawUInt32(data, pktsize, &offset, &item->service_id); 	// file hash
//	ok &= getRawUFloat32(data, pktsize, &offset, item->randomized_distance); 	// file hash
//	ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_VALUE,item->description_string);
//    ok &= item->fingerprint.deserialise(data,pktsize,offset) ;
//
//	if (offset != rssize || !ok)
//	{
//		std::cerr << __PRETTY_FUNCTION__ << ": error while deserialising! Item will be dropped." << std::endl;
//		return NULL ;
//	}
//
//	return item;
//}

RsGRouterGenericDataItem *RsGRouterSerialiser::deserialise_RsGRouterGenericDataItem(void *data, uint32_t pktsize) const
{
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

	RsGRouterGenericDataItem *item = new RsGRouterGenericDataItem() ;

	ok &= getRawUInt64(data, pktsize, &offset, &item->routing_id); 	
	ok &= item->destination_key.deserialise(data, pktsize, offset) ;
	ok &= getRawUInt32(data, pktsize, &offset, &item->randomized_distance);	
	ok &= getRawUInt32(data, pktsize, &offset, &item->data_size); 	

	if( NULL == (item->data_bytes = (uint8_t*)malloc(item->data_size)))
	{
		std::cerr << __PRETTY_FUNCTION__ << ": Cannot allocate memory for chunk " << item->data_size << std::endl;
		return NULL ;
	}

	memcpy(item->data_bytes,&((uint8_t*)data)[offset],item->data_size) ;
	offset += item->data_size ;

	if (offset != rssize || !ok)
	{
		std::cerr << __PRETTY_FUNCTION__ << ": error while deserialising! Item will be dropped." << std::endl;
		return NULL ;
	}

	return item;
}

RsGRouterACKItem *RsGRouterSerialiser::deserialise_RsGRouterACKItem(void *data, uint32_t pktsize) const
{
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

	RsGRouterACKItem *item = new RsGRouterACKItem() ;

	ok &= getRawUInt64(data, pktsize, &offset, &item->mid); 	// file hash
	ok &= getRawUInt32(data, pktsize, &offset, &item->state); 	// file hash

	if (offset != rssize || !ok)
	{
		std::cerr << __PRETTY_FUNCTION__ << ": error while deserialising! Item will be dropped." << std::endl;
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

	ok &= getRawUInt32(data, pktsize, &offset, &item->status_flags); 	
	ok &= item->origin.deserialise(data, pktsize, offset) ;
	ok &= getRawTimeT(data, pktsize, &offset, item->received_time); 	
	ok &= getRawTimeT(data, pktsize, &offset, item->last_sent); 	
	ok &= getRawUInt32(data, pktsize, &offset, &item->client_id); 	

	uint32_t s = 0 ;
	ok &= getRawUInt32(data, pktsize, &offset, &s) ;

	for(uint32_t i=0;i<s;++i)
	{
		FriendTrialRecord ftr ;

		ok &= ftr.friend_id.deserialise(data, pktsize, offset) ;
		ok &= getRawTimeT(data, pktsize, &offset, ftr.time_stamp) ;
		ok &= getRawUFloat32(data, pktsize, &offset, ftr.probability) ;
		ok &= getRawUInt32(data, pktsize, &offset, &ftr.nb_friends) ;

		item->tried_friends.push_back(ftr) ;
	}

	item->data_item = deserialise_RsGRouterGenericDataItem(&((uint8_t*)data)[offset],pktsize - offset) ;
	if(item->data_item != NULL) 
		offset += item->data_item->serial_size() ;
	else
		ok = false ;

	item->destination_key = item->data_item->destination_key ;

	if (offset != rssize || !ok)
	{
		std::cerr << __PRETTY_FUNCTION__ << ": error while deserialising! Item will be dropped." << std::endl;
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
		return NULL ;
	}

	return item;
}


RsGRouterGenericDataItem *RsGRouterGenericDataItem::duplicate() const
{
	RsGRouterGenericDataItem *item = new RsGRouterGenericDataItem ;

	item->routing_id = routing_id ;
	item->destination_key = destination_key ;
	item->data_size = data_size ;
	item->randomized_distance = randomized_distance ;

	// then duplicate the memory chunk

	item->data_bytes = (uint8_t*)malloc(data_size) ;
	memcpy(item->data_bytes,data_bytes,data_size) ;

	return item ;
}

uint32_t RsGRouterGenericDataItem::serial_size() const
{
	uint32_t s = 8 ;	                      // header
	s += sizeof(GRouterMsgPropagationId)  ; // routing id
	s += destination_key.serial_size() ;	 // destination_key
	s += 4 ;                                // randomized distance
	s += 4 ;                       		 	 // data_size
	s += data_size ;                        // data

	return s ;
}
uint32_t RsGRouterACKItem::serial_size() const
{
	uint32_t s = 8 ;	// header
	s += sizeof(GRouterMsgPropagationId)  ; // routing id
	s += 4 ;  			// state

	return s ;
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
	ok &= setRawUInt32(data, tlvsize, &offset, randomized_distance) ;
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
bool RsGRouterACKItem::serialise(void *data,uint32_t& size) const
{
	uint32_t tlvsize,offset=0;
	bool ok = true;
	
	if(!serialise_header(data,size,tlvsize,offset))
		return false ;

	/* add mandatory parts first */
	ok &= setRawUInt64(data, tlvsize, &offset, mid);
	ok &= setRawUInt32(data, tlvsize, &offset, state);

	if (offset != tlvsize)
	{
		ok = false;
        std::cerr << "RsGRouterACKItem::serialisedata() size error! " << std::endl;
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
    uint32_t s = 8 ; 							// header
    s += 4  ; 										// status_flags
    s += origin.serial_size()  ; 			// origin
    s += 8  ; 										// received_time
    s += 8  ; 										// last_sent
    s += 4  ; 										// tried_friends.size() ;
    s += sizeof(GRouterServiceId)  ; 		// service_id
    s += tried_friends.size() * ( RsPeerId::SIZE_IN_BYTES + 8 + 4 + 4 ) ;			// FriendTrialRecord
	 s += data_item->serial_size();			// data_item

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

    ok &= setRawUInt32(data, tlvsize, &offset, status_flags) ;
    ok &= origin.serialise(data, tlvsize, offset) ;
    ok &= setRawTimeT(data, tlvsize, &offset, received_time) ;
    ok &= setRawTimeT(data, tlvsize, &offset, last_sent) ;
    ok &= setRawUInt32(data, tlvsize, &offset, client_id) ;
    ok &= setRawUInt32(data, tlvsize, &offset, tried_friends.size()) ;

    for(std::list<FriendTrialRecord>::const_iterator it(tried_friends.begin());it!=tried_friends.end();++it)
		 ok &= (*it).serialise(data,offset,size - offset) ;

	 uint32_t ns = size - offset ;
	 ok &= data_item->serialise( &((uint8_t*)data)[offset], ns) ;
	 offset += ns ;

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
//std::ostream& RsGRouterPublishKeyItem::print(std::ostream& o, uint16_t)
//{
//	o << "GRouterPublishKeyItem:" << std::endl ;
//	o << "  POW bytes    : \""<< RsPgpId(pow_bytes).toStdString() << "\"" << std::endl ;
//	o << "  direct origin: \""<< PeerId() << "\"" << std::endl ;
//	o << "  Key:            " << published_key.toStdString() << std::endl ;
//	o << "  Req. Id:        " << std::hex << diffusion_id << std::dec << std::endl ;
//	o << "  Srv. Id:        " << std::hex << service_id << std::dec << std::endl ;
//	o << "  Distance:       " << randomized_distance << std::endl ;
//	o << "  Description:    " << description_string << std::endl ;
//	o << "  Fingerprint:    " << fingerprint.toStdString() << std::endl ;
//
//	return o ;
//}
std::ostream& RsGRouterACKItem::print(std::ostream& o, uint16_t)
{
	o << "RsGRouterACKItem:" << std::endl ;
	o << "  direct origin: \""<< PeerId() << "\"" << std::endl ;
	o << "  Mid:            " << mid << std::endl ;
	o << "  State:          " << state << std::endl ;

	return o ;
}
std::ostream& RsGRouterGenericDataItem::print(std::ostream& o, uint16_t)
{
	o << "RsGRouterGenericDataItem:" << std::endl ;
	o << "  direct origin: \""<< PeerId() << "\"" << std::endl ;
	o << "  Key:            " << destination_key.toStdString() << std::endl ;
	o << "  Data size:      " << data_size << std::endl ;

	return o ;
}

std::ostream& RsGRouterRoutingInfoItem::print(std::ostream& o, uint16_t)
{
	o << "RsGRouterRoutingInfoItem:" << std::endl ;
	o << "  direct origin: \""<< PeerId() << "\"" << std::endl ;
	o << "  origin:          "<< origin.toStdString() << std::endl ;
	o << "  recv time:       "<< received_time << std::endl ;
	o << "  Last sent:       "<< last_sent << std::endl ;
	o << "  flags:           "<< std::hex << status_flags << std::dec << std::endl ;
	o << "  Key:             "<< data_item->destination_key.toStdString() << std::endl ;
	o << "  Data size:       "<< data_item->data_size << std::endl ;
	o << "  Client id:       "<< client_id << std::endl ;
	o << "  Tried friends:   "<< tried_friends.size() << std::endl;

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
std::ostream& RsGRouterMatrixFriendListItem::print(std::ostream& o, uint16_t)
{
	o << "RsGRouterMatrixCluesItem:" << std::endl ;
	o << "  friends:  " << reverse_friend_indices.size() << std::endl;

	return o ;
}
