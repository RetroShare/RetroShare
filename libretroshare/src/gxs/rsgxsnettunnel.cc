/*******************************************************************************
 * libretroshare/src/gxs: gxsnettunnel.cc                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2018 by Cyril Soler <retroshare.project@gmail.com>                *
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

#include "util/rsdir.h"
#include "util/rstime.h"
#include "retroshare/rspeers.h"
#include "serialiser/rstypeserializer.h"
#include "gxs/rsnxs.h"
#include "rsgxsnettunnel.h"

//#define DEBUG_RSGXSNETTUNNEL 1

#define GXS_NET_TUNNEL_NOT_IMPLEMENTED() { std::cerr << __PRETTY_FUNCTION__ << ": not yet implemented." << std::endl; }
#define GXS_NET_TUNNEL_DEBUG()             std::cerr << time(NULL) << " : GXS_NET_TUNNEL: " << __FUNCTION__ << " : "
#define GXS_NET_TUNNEL_ERROR()             std::cerr << "(EE) GXS_NET_TUNNEL ERROR : "

static const uint32_t RS_GXS_NET_TUNNEL_MAX_ALLOWED_HITS_GROUP_DATA    =   1;
static const uint32_t RS_GXS_NET_TUNNEL_MAX_ALLOWED_HITS_GROUP_SEARCH  = 100;

RsGxsDistSync *rsGxsDistSync = NULL;

RsGxsNetTunnelService::RsGxsNetTunnelService(): mGxsNetTunnelMtx("GxsNetTunnel")
{
	mRandomBias.clear();

	mLastKeepAlive = time(NULL) + (RSRandom::random_u32()%20);	// adds some variance in order to avoid doing all this tasks at once across services
	mLastAutoWash = time(NULL) + (RSRandom::random_u32()%20);
	mLastDump = time(NULL) + (RSRandom::random_u32()%20);
}

//===========================================================================================================================================//
//                                                               Transport Items                                                             //
//===========================================================================================================================================//

const uint16_t RS_SERVICE_TYPE_GXS_NET_TUNNEL = 0x2233 ;

const uint8_t  RS_PKT_SUBTYPE_GXS_NET_TUNNEL_VIRTUAL_PEER                = 0x01 ;
const uint8_t  RS_PKT_SUBTYPE_GXS_NET_TUNNEL_KEEP_ALIVE                  = 0x02 ;
const uint8_t  RS_PKT_SUBTYPE_GXS_NET_TUNNEL_RANDOM_BIAS                 = 0x03 ;
const uint8_t  RS_PKT_SUBTYPE_GXS_NET_TUNNEL_TURTLE_SEARCH_SUBSTRING     = 0x04 ;
const uint8_t  RS_PKT_SUBTYPE_GXS_NET_TUNNEL_TURTLE_SEARCH_GROUP_REQUEST = 0x05 ;
// const uint8_t  RS_PKT_SUBTYPE_GXS_NET_TUNNEL_TURTLE_SEARCH_GROUP_SUMMARY = 0x06; // DEPRECATED
const uint8_t  RS_PKT_SUBTYPE_GXS_NET_TUNNEL_TURTLE_SEARCH_GROUP_DATA    = 0x07 ;
const uint8_t  RS_PKT_SUBTYPE_GXS_NET_TUNNEL_TURTLE_SEARCH_GROUP_SUMMARY = 0x08;

class RsGxsNetTunnelItem: public RsItem
{
public:
	explicit RsGxsNetTunnelItem(uint8_t item_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_GXS_NET_TUNNEL,item_subtype)
	{
		// no priority. All items are encapsulated into generic Turtle items anyway.
	}

	virtual ~RsGxsNetTunnelItem() {}
	virtual void clear() {}
};

class RsGxsNetTunnelVirtualPeerItem: public RsGxsNetTunnelItem
{
public:
    RsGxsNetTunnelVirtualPeerItem() :RsGxsNetTunnelItem(RS_PKT_SUBTYPE_GXS_NET_TUNNEL_VIRTUAL_PEER) {}

    virtual ~RsGxsNetTunnelVirtualPeerItem() {}

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
	{
		RsTypeSerializer::serial_process(j,ctx,virtual_peer_id,"virtual_peer_id") ;
	}

	RsPeerId virtual_peer_id ;
};

class RsGxsNetTunnelKeepAliveItem: public RsGxsNetTunnelItem
{
public:
    RsGxsNetTunnelKeepAliveItem() :RsGxsNetTunnelItem(RS_PKT_SUBTYPE_GXS_NET_TUNNEL_KEEP_ALIVE) {}

    virtual ~RsGxsNetTunnelKeepAliveItem() {}
	virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */) {}
};

class RsGxsNetTunnelRandomBiasItem: public RsGxsNetTunnelItem
{
public:
	explicit RsGxsNetTunnelRandomBiasItem() : RsGxsNetTunnelItem(RS_PKT_SUBTYPE_GXS_NET_TUNNEL_RANDOM_BIAS) { clear();}
    virtual ~RsGxsNetTunnelRandomBiasItem() {}

	virtual void clear() { mRandomBias.clear() ; }
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
	{
		RsTypeSerializer::serial_process(j,ctx,mRandomBias,"random bias") ;
	}

	Bias20Bytes mRandomBias; // Cannot be a simple char[] because of serialization.
};

class RsGxsNetTunnelTurtleSearchSubstringItem: public RsGxsNetTunnelItem
{
public:
    explicit RsGxsNetTunnelTurtleSearchSubstringItem(): RsGxsNetTunnelItem(RS_PKT_SUBTYPE_GXS_NET_TUNNEL_TURTLE_SEARCH_SUBSTRING) {}
    virtual ~RsGxsNetTunnelTurtleSearchSubstringItem() {}

    uint16_t service ;
    std::string substring_match ;

	virtual void clear() { substring_match.clear() ; }
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
	{
		RsTypeSerializer::serial_process<uint16_t>(j,ctx,service,"service") ;
		RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_KEY,substring_match,"substring_match") ;
	}
};

class RsGxsNetTunnelTurtleSearchGroupRequestItem: public RsGxsNetTunnelItem
{
public:
    explicit RsGxsNetTunnelTurtleSearchGroupRequestItem(): RsGxsNetTunnelItem(RS_PKT_SUBTYPE_GXS_NET_TUNNEL_TURTLE_SEARCH_GROUP_REQUEST) {}
    virtual ~RsGxsNetTunnelTurtleSearchGroupRequestItem() {}

    uint16_t service ;
    Sha1CheckSum hashed_group_id ;

	virtual void clear() { hashed_group_id.clear() ; }
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
	{
		RsTypeSerializer::serial_process<uint16_t>(j,ctx,service,"service") ;
		RsTypeSerializer::serial_process(j,ctx,hashed_group_id,"hashed_group_id") ;
	}
};

class RsGxsNetTunnelTurtleSearchGroupSummaryItem: public RsGxsNetTunnelItem
{
public:
    explicit RsGxsNetTunnelTurtleSearchGroupSummaryItem(): RsGxsNetTunnelItem(RS_PKT_SUBTYPE_GXS_NET_TUNNEL_TURTLE_SEARCH_GROUP_SUMMARY) {}
    virtual ~RsGxsNetTunnelTurtleSearchGroupSummaryItem() {}

    uint16_t service ;
    std::list<RsGxsGroupSummary> group_infos;

	virtual void clear() { group_infos.clear() ; }

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
	{
		RsTypeSerializer::serial_process<uint16_t>(j,ctx,service,"service") ;
		RsTypeSerializer::serial_process(j,ctx,group_infos,"group_infos") ;
	}
};
class RsGxsNetTunnelTurtleSearchGroupDataItem: public RsGxsNetTunnelItem
{
public:
    explicit RsGxsNetTunnelTurtleSearchGroupDataItem()
        : RsGxsNetTunnelItem(RS_PKT_SUBTYPE_GXS_NET_TUNNEL_TURTLE_SEARCH_GROUP_DATA),
          encrypted_group_data(NULL),
          encrypted_group_data_len(0)
    {}

    virtual ~RsGxsNetTunnelTurtleSearchGroupDataItem() {}

    uint16_t service ;
    unsigned char *encrypted_group_data ;
    uint32_t encrypted_group_data_len ;

    virtual void clear() { free(encrypted_group_data);encrypted_group_data=NULL;encrypted_group_data_len=0; }

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
	{
		RsTypeSerializer::serial_process<uint16_t>(j,ctx,service,"service") ;

		RsTypeSerializer::TlvMemBlock_proxy prox(encrypted_group_data,encrypted_group_data_len) ;
		RsTypeSerializer::serial_process(j,ctx,prox,"encrypted_group_data") ;
	}
};

class RsGxsNetTunnelSerializer: public RsServiceSerializer
{
public:
	RsGxsNetTunnelSerializer() :RsServiceSerializer(RS_SERVICE_TYPE_GXS_NET_TUNNEL) {}

	virtual RsItem *create_item(uint16_t service,uint8_t item_subtype) const
	{
		if(service != RS_SERVICE_TYPE_GXS_NET_TUNNEL)
		{
			GXS_NET_TUNNEL_ERROR() << "received item with wrong service ID " << std::hex << service << std::dec << std::endl;
			return NULL ;
		}

		switch(item_subtype)
		{
		case RS_PKT_SUBTYPE_GXS_NET_TUNNEL_VIRTUAL_PEER                : return new RsGxsNetTunnelVirtualPeerItem ;
		case RS_PKT_SUBTYPE_GXS_NET_TUNNEL_KEEP_ALIVE                  : return new RsGxsNetTunnelKeepAliveItem ;
		case RS_PKT_SUBTYPE_GXS_NET_TUNNEL_RANDOM_BIAS                 : return new RsGxsNetTunnelRandomBiasItem ;
		case RS_PKT_SUBTYPE_GXS_NET_TUNNEL_TURTLE_SEARCH_SUBSTRING     : return new RsGxsNetTunnelTurtleSearchSubstringItem;
		case RS_PKT_SUBTYPE_GXS_NET_TUNNEL_TURTLE_SEARCH_GROUP_REQUEST : return new RsGxsNetTunnelTurtleSearchGroupRequestItem;
		case RS_PKT_SUBTYPE_GXS_NET_TUNNEL_TURTLE_SEARCH_GROUP_SUMMARY : return new RsGxsNetTunnelTurtleSearchGroupSummaryItem;
		case RS_PKT_SUBTYPE_GXS_NET_TUNNEL_TURTLE_SEARCH_GROUP_DATA    : return new RsGxsNetTunnelTurtleSearchGroupDataItem;
		default:
			GXS_NET_TUNNEL_ERROR() << "type ID " << std::hex << (int)item_subtype << std::dec << " is not handled!" << std::endl;
			return NULL ;
		}
	}
};

//===========================================================================================================================================//
//                                                     Interface with rest of the software                                                   //
//===========================================================================================================================================//

uint16_t RsGxsNetTunnelService::serviceId() const
{
    return RS_SERVICE_TYPE_GXS_NET_TUNNEL;
}

bool RsGxsNetTunnelService::registerSearchableService(RsNetworkExchangeService *gxs_service)
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);

    mSearchableServices[gxs_service->serviceType()] = gxs_service ;

    return true;
}

class DataAutoDelete
{
public:
	DataAutoDelete(unsigned char *& data) : mData(data) {}
	~DataAutoDelete() { free(mData); mData=NULL ; }
	unsigned char *& mData;
};

RsGxsNetTunnelService::~RsGxsNetTunnelService()
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);

	// This is needed because we need to clear these structure in a mutex protected environment
	// Also this calls the destructor of the objects which calls the freeing of memory e.g. allocated in the incoming data list.

	mGroups.clear();
	mHandledHashes.clear();
	mVirtualPeers.clear();

	for(auto it(mIncomingData.begin());it!=mIncomingData.end();++it)
		for(auto it2(it->second.begin());it2!=it->second.end();++it2)
			delete it2->second;

	mIncomingData.clear();
}

bool RsGxsNetTunnelService::isDistantPeer(const RsGxsNetTunnelVirtualPeerId& virtual_peer, RsGxsGroupId& group_id)
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);

	auto it = mVirtualPeers.find(virtual_peer) ;

	if(it != mVirtualPeers.end())
	{
		group_id = it->second.group_id ;
		return true ;
	}
	else
		return false ;
}

bool RsGxsNetTunnelService::receiveTunnelData(uint16_t service_id, unsigned char *& data, uint32_t& data_len, RsGxsNetTunnelVirtualPeerId& virtual_peer)
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);

	std::list<std::pair<RsGxsNetTunnelVirtualPeerId,RsTlvBinaryData*> >& lst(mIncomingData[service_id]);

	if(lst.empty())
	{
		data = NULL;
		data_len = 0;
		return false ;
	}

	data = (unsigned char*)lst.front().second->bin_data ;
	data_len = lst.front().second->bin_len ;
	virtual_peer = lst.front().first;

	lst.front().second->bin_data = NULL ; // avoids deletion
	lst.front().second->bin_len = 0 ; // avoids deletion

	delete lst.front().second;
	lst.pop_front();

	return true;
}

bool RsGxsNetTunnelService::sendTunnelData(uint16_t /* service_id */,unsigned char *& data,uint32_t data_len,const RsGxsNetTunnelVirtualPeerId& virtual_peer)
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);
	// The item is serialized and encrypted using chacha20+SHA256, using the generic turtle encryption, and then sent to the turtle router.

	DataAutoDelete iad(data) ;	// This ensures the item is deleted whatsoever when leaving

	// 1 - find the virtual peer and the proper master key to encrypt with, and check that all the info is known

	auto it = mVirtualPeers.find(virtual_peer) ;

	if(it == mVirtualPeers.end())
	{
		GXS_NET_TUNNEL_ERROR() << "cannot find virtual peer " << virtual_peer << ". Data is dropped." << std::endl;
		return false ;
	}

	if(it->second.vpid_status != RsGxsNetTunnelVirtualPeerInfo::RS_GXS_NET_TUNNEL_VP_STATUS_ACTIVE)
	{
		GXS_NET_TUNNEL_ERROR() << "virtual peer " << virtual_peer << " is not active. Data is dropped." << std::endl;
		return false ;
	}

    // 2 - encrypt and send the item.

	RsTurtleGenericDataItem *encrypted_turtle_item = NULL ;

	if(!p3turtle::encryptData(data,data_len,it->second.encryption_master_key,encrypted_turtle_item))
	{
		GXS_NET_TUNNEL_ERROR() << "cannot encrypt. Something's wrong. Data is dropped." << std::endl;
		return false ;
	}

	mTurtle->sendTurtleData(it->second.turtle_virtual_peer_id,encrypted_turtle_item) ;

	return true ;
}

bool RsGxsNetTunnelService::getVirtualPeers(std::list<RsGxsNetTunnelVirtualPeerId>& peers)
{
	// This function has two effects:
	//  - return the virtual peers for this group
	//  - passively set the group as "managed", so that it we answer tunnel requests.

	RS_STACK_MUTEX(mGxsNetTunnelMtx);

	// update the hash entry if needed

	for(auto it(mVirtualPeers.begin());it!=mVirtualPeers.end();++it)
		peers.push_back(it->first) ;

#ifdef DEBUG_RSGXSNETTUNNEL
    GXS_NET_TUNNEL_DEBUG() << " returning " << peers.size() << " peers." << std::endl;
#endif

	return true ;
}

bool RsGxsNetTunnelService::requestDistantPeers(uint16_t service_id, const RsGxsGroupId& group_id)
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);

    // Now ask the turtle router to manage a tunnel for that hash.

	RsGxsNetTunnelGroupInfo& ginfo( mGroups[group_id] ) ;

	if(ginfo.group_policy < RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_POLICY_ACTIVE)
		ginfo.group_policy = RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_POLICY_ACTIVE;

	ginfo.hash         = calculateGroupHash(group_id) ;
	ginfo.service_id   = service_id;

	mHandledHashes[ginfo.hash] = group_id ;

	// we dont set the group policy here. It will only be set if no peers, or too few peers are available.
#ifdef DEBUG_RSGXSNETTUNNEL
    GXS_NET_TUNNEL_DEBUG() << " requesting peers for group " << group_id << std::endl;
#endif

	return true;
}

bool RsGxsNetTunnelService::releaseDistantPeers(uint16_t /* service_id */,const RsGxsGroupId& group_id)
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);

    // Ask turtle router to stop requesting tunnels for that hash.

	RsGxsNetTunnelGroupInfo& ginfo( mGroups[group_id] ) ;

    ginfo.group_policy = RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_POLICY_PASSIVE;
	ginfo.hash         = calculateGroupHash(group_id) ;

	mHandledHashes[ginfo.hash] = group_id ;	// yes, we do not remove, because we're supposed to answer tunnel requests from other peers.

	mTurtle->stopMonitoringTunnels(ginfo.hash) ;

#ifdef DEBUG_RSGXSNETTUNNEL
    GXS_NET_TUNNEL_DEBUG() << " releasing peers for group " << group_id << std::endl;
#endif
	return true;
}

void RsGxsNetTunnelService::dump() const
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);

	static std::string group_status_str[4] = {
	    std::string("[UNKNOWN          ]"),
	    std::string("[IDLE             ]"),
	    std::string("[TUNNELS_REQUESTED]"),
	    std::string("[VPIDS_AVAILABLE  ]")
	};

	static std::string group_policy_str[4] = {
	    	 std::string("[UNKNOWN   ]"),
	    	 std::string("[PASSIVE   ]"),
	    	 std::string("[ACTIVE    ]"),
	    	 std::string("[REQUESTING]"),
	};
	static std::string vpid_status_str[3] = {
	    	 std::string("[UNKNOWN  ]"),
		     std::string("[TUNNEL_OK]"),
		     std::string("[ACTIVE   ]")
	};

	std::cerr << "GxsNetTunnelService dump (this=" << (void*)this << ": " << std::endl;
	std::cerr << "  Managed GXS groups: " << std::endl;

	for(auto it(mGroups.begin());it!=mGroups.end();++it)
	{
		std::cerr << "    " << it->first << " hash: " << it->second.hash << "  policy: " << group_policy_str[it->second.group_policy] << " status: " << group_status_str[it->second.group_status] << "  Last contact: " << time(NULL) - it->second.last_contact << " secs ago" << std::endl;

		if(!it->second.virtual_peers.empty())
			std::cerr << "    virtual peers:" << std::endl;
		for(auto it2(it->second.virtual_peers.begin());it2!=it->second.virtual_peers.end();++it2)
			std::cerr << "      " << *it2 << std::endl;
	}

	std::cerr << "  Virtual peers: " << std::endl;
	for(auto it(mVirtualPeers.begin());it!=mVirtualPeers.end();++it)
		std::cerr << "    GXS Peer:" << it->first << " Turtle:" << it->second.turtle_virtual_peer_id
			      << "  status: " <<  vpid_status_str[it->second.vpid_status]
			      << "  group_id: " <<  it->second.group_id
		          << " direction: " << (int)it->second.side
		          << " last seen " << time(NULL)-it->second.last_contact << " secs ago"
			      << " ekey: " << RsUtil::BinToHex(it->second.encryption_master_key,RS_GXS_TUNNEL_CONST_EKEY_SIZE,10) << std::endl;

	std::cerr << "  Virtual peer turtle => GXS conversion table: " << std::endl;
	for(auto it(mTurtle2GxsPeer.begin());it!=mTurtle2GxsPeer.end();++it)
		std::cerr << "    " << it->first << " => " << it->second << std::endl;

	std::cerr << "  Hashes: " << std::endl;
	for(auto it(mHandledHashes.begin());it!=mHandledHashes.end();++it)
		std::cerr << "     hash: " << it->first << " GroupId: " << it->second << std::endl;

	std::cerr << "  Incoming data: " << std::endl;
	for(auto it(mIncomingData.begin());it!=mIncomingData.end();++it)
	{
		std::cerr << "    service " << std::hex << it->first << std::dec << std::endl;

		for(auto it2(it->second.begin());it2!=it->second.end();++it2)
			std::cerr << "     peer id " << it2->first << " " << (void*)it2->second << std::endl;
	}
}

//===========================================================================================================================================//
//                                                         Interaction with Turtle Router                                                    //
//===========================================================================================================================================//

void RsGxsNetTunnelService::connectToTurtleRouter(p3turtle *tr)
{
	mTurtle = tr ;
	mTurtle->registerTunnelService(this) ;
}

bool RsGxsNetTunnelService::handleTunnelRequest(const RsFileHash &hash,const RsPeerId& /* peer_id */)
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);
	// We simply check for wether a managed group has a hash that corresponds to the given hash.

	return mHandledHashes.find(hash) != mHandledHashes.end();
}

void RsGxsNetTunnelService::receiveTurtleData(const RsTurtleGenericTunnelItem *item, const RsFileHash& hash, const RsPeerId& turtle_virtual_peer_id, RsTurtleGenericTunnelItem::Direction direction)
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);

#ifdef DEBUG_RSGXSNETTUNNEL
	GXS_NET_TUNNEL_DEBUG() << " received turtle data for vpid " << turtle_virtual_peer_id << " for hash " << hash << " in direction " << (int)direction << std::endl;
#endif

	if(item->PacketSubType() != RS_TURTLE_SUBTYPE_GENERIC_DATA)
	{
		GXS_NET_TUNNEL_ERROR() << "item with type " << std::hex << item->PacketSubType() << std::dec << " received by GxsNetTunnel, but is not handled!" << std::endl;
		return;
	}
	// find the group id

	auto it4 = mHandledHashes.find(hash) ;

	if(it4 == mHandledHashes.end())
	{
		GXS_NET_TUNNEL_ERROR() << "Cannot find hash " << hash << " to be handled by GxsNetTunnel" << std::endl;
		return;
	}
	RsGxsGroupId group_id = it4->second;

	// Now check if we got an item to advertise a virtual peer

	unsigned char *data = NULL ;
	uint32_t data_size = 0 ;

	// generate the decryption key based on virtual peer id and group id

	uint8_t encryption_master_key[RS_GXS_TUNNEL_CONST_EKEY_SIZE] ;

	generateEncryptionKey(group_id,turtle_virtual_peer_id,encryption_master_key);

	if(!p3turtle::decryptItem(static_cast<const RsTurtleGenericDataItem*>(item),encryption_master_key,data,data_size))
	{
		GXS_NET_TUNNEL_ERROR() << "Cannot decrypt data!" << std::endl;

		if(data)
			free(data) ;

		return ;
	}

	uint16_t service_id = getRsItemService(getRsItemId(data));

	if(service_id == RS_SERVICE_TYPE_GXS_NET_TUNNEL)
	{
		RsItem *decrypted_item = RsGxsNetTunnelSerializer().deserialise(data,&data_size);
		RsGxsNetTunnelVirtualPeerItem *pid_item = dynamic_cast<RsGxsNetTunnelVirtualPeerItem*>(decrypted_item) ;

		if(!pid_item)		// this handles the case of a KeepAlive packet.
		{
			delete decrypted_item ;
			free(data);
			return ;
		}

#ifdef DEBUG_RSGXSNETTUNNEL
		GXS_NET_TUNNEL_DEBUG() << "    item is a virtual peer id item with vpid = "<< pid_item->virtual_peer_id
		                       << " for group " << group_id << ". Setting virtual peer." << std::endl;
#endif
		// we receive a virtual peer id, so we need to update the local information for this peer id

		mTurtle2GxsPeer[turtle_virtual_peer_id] = pid_item->virtual_peer_id ;

		RsGxsNetTunnelVirtualPeerInfo& vp_info(mVirtualPeers[pid_item->virtual_peer_id]) ;
        delete pid_item;

		vp_info.vpid_status = RsGxsNetTunnelVirtualPeerInfo::RS_GXS_NET_TUNNEL_VP_STATUS_ACTIVE ;					// status of the peer
		vp_info.side = direction;	                        // client/server
		vp_info.last_contact = time(NULL);					// last time some data was sent/recvd
		vp_info.group_id = group_id;
		vp_info.service_id = mGroups[group_id].service_id ;

		memcpy(vp_info.encryption_master_key,encryption_master_key,RS_GXS_TUNNEL_CONST_EKEY_SIZE);

		vp_info.turtle_virtual_peer_id = turtle_virtual_peer_id;  // turtle peer to use when sending data to this vpid.

		free(data);

		return ;
	}


	// item is a generic data item for the client. Let's store the data in the appropriate incoming data queue.

	auto it = mTurtle2GxsPeer.find(turtle_virtual_peer_id) ;

	if(it == mTurtle2GxsPeer.end())
	{
		GXS_NET_TUNNEL_ERROR() << "item received by GxsNetTunnel for vpid " << turtle_virtual_peer_id << " but this vpid is unknown! Removing this vpid from group " << group_id << std::endl;

		// this situation is inconsistent: the first item that should go through the tunnel is a virtual peer info, so if we don't have one, it means that
		// this virtual peer is dead. We should therefore remove it from the list of vpids for this group.

		mGroups[group_id].virtual_peers.erase(turtle_virtual_peer_id) ;

		free(data);
		return;
	}

	RsGxsNetTunnelVirtualPeerId gxs_vpid = it->second ;

	auto it2 = mVirtualPeers.find(gxs_vpid) ;

	if(it2 == mVirtualPeers.end())
	{
		GXS_NET_TUNNEL_ERROR() << "item received by GxsNetTunnel for GXS vpid " << gxs_vpid << " but the virtual peer id is missing!" << std::endl;
		free(data);
		return;
	}

	if(service_id != it2->second.service_id && service_id != RS_SERVICE_GXS_TYPE_GXSID)
	{
		GXS_NET_TUNNEL_ERROR() << " received an item for VPID " << gxs_vpid << " openned for service " << it2->second.service_id << " that is not for that service nor for GXS Id service (service=" << std::hex << service_id << std::dec << ". Rejecting!" << std::endl;
		free(data);
		return ;
	}
	it2->second.vpid_status = RsGxsNetTunnelVirtualPeerInfo::RS_GXS_NET_TUNNEL_VP_STATUS_ACTIVE ;					// status of the peer
	it2->second.last_contact = time(NULL);					// last time some data was sent/recvd from this peer

	mGroups[group_id].last_contact = time(NULL);			// last time some data as received for this group

#ifdef DEBUG_RSGXSNETTUNNEL
	GXS_NET_TUNNEL_DEBUG() << "item contains generic data for VPID " << gxs_vpid << ". service_id = " << std::hex << service_id << std::dec << ". Storing in incoming list" <<  std::endl;
#endif

	// push the data into the incoming data list

	RsTlvBinaryData *bind = new RsTlvBinaryData;
	bind->tlvtype = 0;
	bind->bin_len = data_size;
	bind->bin_data = data;

	mIncomingData[service_id].push_back(std::make_pair(gxs_vpid,bind)) ;
}

void RsGxsNetTunnelService::addVirtualPeer(const TurtleFileHash& hash, const TurtleVirtualPeerId& vpid,RsTurtleGenericTunnelItem::Direction dir)
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);

	auto it = mHandledHashes.find(hash) ;

	if(it == mHandledHashes.end())
	{
		std::cerr << "RsGxsNetTunnelService::addVirtualPeer(): error! hash " << hash << " is not handled. Cannot add vpid " << vpid << " in direction " << dir << std::endl;
		return ;
	}

#ifdef DEBUG_RSGXSNETTUNNEL
	GXS_NET_TUNNEL_DEBUG() << " adding virtual peer " << vpid << " for hash " << hash << " in direction " << dir << std::endl;
#endif
	const RsGxsGroupId group_id(it->second) ;

	RsGxsNetTunnelGroupInfo& ginfo( mGroups[group_id] ) ;
	ginfo.group_status = RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_STATUS_VPIDS_AVAILABLE ;
	ginfo.virtual_peers.insert(vpid);

	uint8_t encryption_master_key[RS_GXS_TUNNEL_CONST_EKEY_SIZE];

	generateEncryptionKey(group_id,vpid,encryption_master_key );

	// We need to send our own virtual peer id to the other end of the tunnel

	RsGxsNetTunnelVirtualPeerId net_service_virtual_peer = locked_makeVirtualPeerId(group_id);

#ifdef DEBUG_RSGXSNETTUNNEL
	GXS_NET_TUNNEL_DEBUG() << " sending back virtual peer name " << net_service_virtual_peer << " to end of tunnel" << std::endl;
#endif
	RsGxsNetTunnelVirtualPeerItem pitem ;
	pitem.virtual_peer_id = net_service_virtual_peer ;

	RsTemporaryMemory tmpmem( RsGxsNetTunnelSerializer().size(&pitem) ) ;
	uint32_t len = tmpmem.size();

	RsGxsNetTunnelSerializer().serialise(&pitem,tmpmem,&len);

	RsTurtleGenericDataItem *encrypted_turtle_item = NULL ;

	if(p3turtle::encryptData(tmpmem,len,encryption_master_key,encrypted_turtle_item))
		mPendingTurtleItems.push_back(std::make_pair(vpid,encrypted_turtle_item)) ;			// we cannot send directly because of turtle mutex locked before calling addVirtualPeer.
	else
		GXS_NET_TUNNEL_ERROR() << "cannot encrypt. Something's wrong. Data is dropped." << std::endl;
}

void RsGxsNetTunnelService::removeVirtualPeer(const TurtleFileHash& hash, const TurtleVirtualPeerId& vpid)
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);

#ifdef DEBUG_RSGXSNETTUNNEL
	GXS_NET_TUNNEL_DEBUG() << " removing virtual peer " << vpid << " for hash " << hash << std::endl;
#endif
	mTurtle2GxsPeer.erase(vpid);

	auto it = mHandledHashes.find(hash) ;

	if(it == mHandledHashes.end())
	{
		std::cerr << "RsGxsNetTunnelService::removeVirtualPeer(): error! hash " << hash << " is not handled. Cannot remove vpid " << vpid << std::endl;
		return ;
	}

	const RsGxsGroupId group_id(it->second) ;

	RsGxsNetTunnelGroupInfo& ginfo( mGroups[group_id] ) ;

	ginfo.virtual_peers.erase(vpid);

	if(ginfo.virtual_peers.empty())
	{
		ginfo.group_status = RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_STATUS_IDLE ;

#ifdef DEBUG_RSGXSNETTUNNEL
		GXS_NET_TUNNEL_DEBUG() << " no more virtual peers for group " << group_id << ": setting status to TUNNELS_REQUESTED" << std::endl;
#endif
	}
}

RsFileHash RsGxsNetTunnelService::calculateGroupHash(const RsGxsGroupId& group_id) const
{
	return RsDirUtil::sha1sum(group_id.toByteArray(),RsGxsGroupId::SIZE_IN_BYTES) ;
}

void RsGxsNetTunnelService::generateEncryptionKey(const RsGxsGroupId& group_id,const TurtleVirtualPeerId& vpid,unsigned char key[RS_GXS_TUNNEL_CONST_EKEY_SIZE])
{
	// The key is generated as H(group_id | vpid)
	// Because group_id is not known it shouldn't be possible to recover the key by observing the traffic.

	assert(Sha256CheckSum::SIZE_IN_BYTES == 32) ;

	unsigned char mem[RsGxsGroupId::SIZE_IN_BYTES + TurtleVirtualPeerId::SIZE_IN_BYTES] ;

	memcpy(mem                            ,group_id.toByteArray(),RsGxsGroupId::SIZE_IN_BYTES) ;
	memcpy(mem+RsGxsGroupId::SIZE_IN_BYTES,vpid.toByteArray()    ,TurtleVirtualPeerId::SIZE_IN_BYTES) ;

	memcpy( key, RsDirUtil::sha256sum(mem,RsGxsGroupId::SIZE_IN_BYTES+TurtleVirtualPeerId::SIZE_IN_BYTES).toByteArray(), RS_GXS_TUNNEL_CONST_EKEY_SIZE ) ;
}

//===========================================================================================================================================//
//                                                               Service parts                                                               //
//===========================================================================================================================================//

void RsGxsNetTunnelService::threadTick()
{
	while(!mPendingTurtleItems.empty())
	{
		auto& it(mPendingTurtleItems.front());

		mTurtle->sendTurtleData(it.first,it.second) ;
		mPendingTurtleItems.pop_front();
	}

	rstime_t now = time(NULL);

	// cleanup

	if(mLastAutoWash + 5 < now)
	{
		autowash();
		mLastAutoWash = now;
	}

	if(mLastKeepAlive + 20 < now)
	{
		mLastKeepAlive = now ;
		sendKeepAlivePackets() ;
	}

#ifdef DEBUG_RSGXSNETTUNNEL
	if(mLastDump + 10 < now)
	{
		mLastDump = now;
		dump();
	}
#endif

    for(uint32_t i=0;i<2;++i)
    {
        if(shouldStop())
            return;
        rstime::rs_usleep(500*1000) ; // 1 sec
    }
}

const Bias20Bytes& RsGxsNetTunnelService::locked_randomBias()
{
	if(mRandomBias.isNull())
	{
		mRandomBias = Bias20Bytes::random();
		IndicateConfigChanged();

        std::cerr << "Initialized RsGxsNetTunnel random bias to " << RsUtil::BinToHex(mRandomBias.toByteArray(),mRandomBias.SIZE_IN_BYTES) << std::endl;
	}

	return mRandomBias ;
}

RsGxsNetTunnelVirtualPeerId RsGxsNetTunnelService::locked_makeVirtualPeerId(const RsGxsGroupId& group_id)
{
	assert(RsPeerId::SIZE_IN_BYTES <= Sha1CheckSum::SIZE_IN_BYTES) ;// so that we can build the virtual PeerId from a SHA1 sum.

	// We compute sha1( GroupId | mRandomBias ) and trunk it to 16 bytes in order to compute a RsPeerId

	Bias20Bytes rb(locked_randomBias());

	unsigned char mem[group_id.SIZE_IN_BYTES + rb.SIZE_IN_BYTES];

	memcpy(mem                       ,group_id.toByteArray(),group_id.SIZE_IN_BYTES) ;
	memcpy(mem+group_id.SIZE_IN_BYTES,rb.toByteArray(),rb.SIZE_IN_BYTES) ;

	return RsGxsNetTunnelVirtualPeerId(RsDirUtil::sha1sum(mem,group_id.SIZE_IN_BYTES+rb.SIZE_IN_BYTES).toByteArray());
}


void RsGxsNetTunnelService::sendKeepAlivePackets()
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);
#ifdef DEBUG_RSGXSNETTUNNEL
	GXS_NET_TUNNEL_DEBUG() << " sending keep-alive packets. " << std::endl;
#endif

	// We send KA packets for each GXS virtual peer. The advantage is that unused tunnels will automatically die which eliminates duplicate tunnels
	// automatically. We only send from the client side.

	for(auto it(mVirtualPeers.begin());it!=mVirtualPeers.end();++it)
	{
		RsGxsNetTunnelVirtualPeerId own_gxs_vpid = locked_makeVirtualPeerId(it->second.group_id) ;

#ifdef DEBUG_RSGXSNETTUNNEL
		GXS_NET_TUNNEL_DEBUG() << "   virtual peer " << it->first << " through tunnel " << it->second.turtle_virtual_peer_id << " for group " << it->second.group_id ;
#endif

		if(own_gxs_vpid < it->first)
		{
#ifdef DEBUG_RSGXSNETTUNNEL
			std::cerr << ": sent!" << std::endl;
#endif
			RsGxsNetTunnelKeepAliveItem pitem ;
			RsTemporaryMemory tmpmem( RsGxsNetTunnelSerializer().size(&pitem) ) ;
			uint32_t len = tmpmem.size();

			RsGxsNetTunnelSerializer().serialise(&pitem,tmpmem,&len);

			RsTurtleGenericDataItem *encrypted_turtle_item = NULL ;

			if(p3turtle::encryptData(tmpmem,len,it->second.encryption_master_key,encrypted_turtle_item))
				mPendingTurtleItems.push_back(std::make_pair(it->second.turtle_virtual_peer_id,encrypted_turtle_item)) ;

			it->second.last_contact = time(NULL) ;
		}
#ifdef DEBUG_RSGXSNETTUNNEL
		else
			std::cerr << ": ignored!" << std::endl;
#endif
	}
}

void RsGxsNetTunnelService::autowash()
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);

#ifdef DEBUG_RSGXSNETTUNNEL
	GXS_NET_TUNNEL_DEBUG() << " performing per-group consistency test." << std::endl;
#endif
	for(auto it(mGroups.begin());it!=mGroups.end();++it)
	{
		RsGxsNetTunnelVirtualPeerId own_gxs_vpid = locked_makeVirtualPeerId(it->first) ;
		RsGxsNetTunnelGroupInfo& ginfo(it->second) ;
		bool should_monitor_tunnels = false ;

#ifdef DEBUG_RSGXSNETTUNNEL
		GXS_NET_TUNNEL_DEBUG() << " group " << it->first << ": " ;
#endif
		// check whether the group already has GXS virtual peers with suppliers. If so, we can set the GRP policy as passive.

		if(ginfo.group_policy >= RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_POLICY_ACTIVE)
		{
			bool found = false ;

			// check wether one virtual peer provided by GXS has ID > own ID. In this case we leave the priority to it.

			for(auto it2(ginfo.virtual_peers.begin());!found && it2!=ginfo.virtual_peers.end();++it2)
			{
				auto it3 = mTurtle2GxsPeer.find(*it2) ;

				if( it3 != mTurtle2GxsPeer.end() && it3->second < own_gxs_vpid)
					found = true ;
			}

			if(found)
			{
#ifdef DEBUG_RSGXSNETTUNNEL
				std::cerr << " active, with client-side peers : ";
#endif
				should_monitor_tunnels = false ;
				ginfo.group_policy = RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_POLICY_ACTIVE ;
			}
			else
			{
				should_monitor_tunnels = true ;
				ginfo.group_policy = RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_POLICY_REQUESTING ;
#ifdef DEBUG_RSGXSNETTUNNEL
				std::cerr << " active, and no client-side peers available : " ;
#endif
			}
		}
#ifdef DEBUG_RSGXSNETTUNNEL
		else
			std::cerr << " passive : ";
#endif

		// We should also check whether the group is supplied using another tunnel. If so, no need to request tunnels.
		// Otherwise the engine will keep requesting tunnels for all groups.
#warning CODE MISSING HERE

		if(should_monitor_tunnels)
		{
#ifdef DEBUG_RSGXSNETTUNNEL
			std::cerr << " requesting tunnels" << std::endl;
#endif
			mTurtle->monitorTunnels(ginfo.hash,this,false) ;
		}
		else
		{
#ifdef DEBUG_RSGXSNETTUNNEL
			std::cerr << " dropping tunnels" << std::endl;
#endif
			mTurtle->stopMonitoringTunnels(ginfo.hash);
		}
	}
}

bool RsGxsNetTunnelService::saveList(bool& cleanup, std::list<RsItem*>& save)
{
	RsGxsNetTunnelRandomBiasItem *it2 = new RsGxsNetTunnelRandomBiasItem() ;

	{
		RS_STACK_MUTEX(mGxsNetTunnelMtx);
		it2->mRandomBias = mRandomBias;
		std::cerr << "Saving RsGxsNetTunnel random bias to disc" << std::endl;
	}

	save.push_back(it2) ;
	cleanup = true ;

	return true;
}

bool RsGxsNetTunnelService::loadList(std::list<RsItem *> &load)
{
	RsGxsNetTunnelRandomBiasItem *rbsi ;

	for(auto it(load.begin());it!=load.end();++it)
	{
		if((rbsi = dynamic_cast<RsGxsNetTunnelRandomBiasItem*>(*it))!=NULL)
		{
			RS_STACK_MUTEX(mGxsNetTunnelMtx);
			mRandomBias = rbsi->mRandomBias;

			std::cerr << "Loaded RsGxsNetTunnel random bias from disc: " << RsUtil::BinToHex(mRandomBias.toByteArray(),mRandomBias.SIZE_IN_BYTES) << std::endl;
		}
		else
			GXS_NET_TUNNEL_ERROR() << " unknown item in config file: type=" << std::hex << (*it)->PacketId() << std::dec << std::endl;

		delete *it;
	}

	return true;
}

RsSerialiser *RsGxsNetTunnelService::setupSerialiser()
{
	RsSerialiser *ser = new RsSerialiser ;
	ser->addSerialType(new RsGxsNetTunnelSerializer) ;

	return ser ;
}

//===========================================================================================================================================//
//                                                               Turtle Search system                                                        //
//===========================================================================================================================================//

TurtleRequestId RsGxsNetTunnelService::turtleGroupRequest(const RsGxsGroupId& group_id,RsNetworkExchangeService *client_service)
{
    Sha1CheckSum hashed_group_id = RsDirUtil::sha1sum(group_id.toByteArray(),group_id.SIZE_IN_BYTES) ;

	GXS_NET_TUNNEL_DEBUG() << ": starting a turtle group request for grp \"" << group_id << "\" hashed to \"" << hashed_group_id << "\"" << std::endl;

	RsGxsNetTunnelTurtleSearchGroupRequestItem   search_item ;
	search_item.hashed_group_id = hashed_group_id ;
	search_item.service = client_service->serviceType() ;

    uint32_t size = RsGxsNetTunnelSerializer().size(&search_item) ;
    unsigned char *mem = (unsigned char*)rs_malloc(size) ;

    if(mem == NULL)
        return 0 ;

    RsGxsNetTunnelSerializer().serialise(&search_item,mem,&size);

    return mTurtle->turtleSearch(mem,size,this) ;
}

TurtleRequestId RsGxsNetTunnelService::turtleSearchRequest(const std::string& match_string,RsNetworkExchangeService *client_service)
{
    GXS_NET_TUNNEL_DEBUG() << ": starting a turtle search request for string \"" << match_string << "\"" << std::endl;

	RsGxsNetTunnelTurtleSearchSubstringItem   search_item ;
	search_item.substring_match = match_string ;
	search_item.service = client_service->serviceType() ;

    uint32_t size = RsGxsNetTunnelSerializer().size(&search_item) ;
    unsigned char *mem = (unsigned char*)rs_malloc(size) ;

    if(mem == NULL)
        return 0 ;

    RsGxsNetTunnelSerializer().serialise(&search_item,mem,&size);

	RS_STACK_MUTEX(mGxsNetTunnelMtx);
    return mTurtle->turtleSearch(mem,size,this) ;
}

bool RsGxsNetTunnelService::receiveSearchRequest(unsigned char *search_request_data,uint32_t search_request_data_len,unsigned char *& search_result_data,uint32_t& search_result_data_size,uint32_t& max_allowed_hits)
{
	GXS_NET_TUNNEL_DEBUG() << ": received a request." << std::endl;

	RsItem *item = RsGxsNetTunnelSerializer().deserialise(search_request_data,&search_request_data_len) ;

    RsGxsNetTunnelTurtleSearchSubstringItem *substring_sr = dynamic_cast<RsGxsNetTunnelTurtleSearchSubstringItem *>(item) ;

    if(substring_sr != NULL)
    {
		GXS_NET_TUNNEL_DEBUG() << "  : type is substring for service " << std::hex << (int)substring_sr->service << std::dec << std::endl;

        max_allowed_hits = RS_GXS_NET_TUNNEL_MAX_ALLOWED_HITS_GROUP_SEARCH ;

        std::list<RsGxsGroupSummary> results ;

		RS_STACK_MUTEX(mGxsNetTunnelMtx);

        auto it = mSearchableServices.find(substring_sr->service) ;

        if(it != mSearchableServices.end() && it->second->search(substring_sr->substring_match,results))
        {
			RsGxsNetTunnelTurtleSearchGroupSummaryItem search_result_item ;

			GXS_NET_TUNNEL_DEBUG() << "  : " << results.size() << " result found. Sending back." << std::endl;

            search_result_item.service = substring_sr->service ;
            search_result_item.group_infos = results ;

            search_result_data_size = RsGxsNetTunnelSerializer().size(&search_result_item) ;
			search_result_data = (unsigned char*)rs_malloc(search_result_data_size) ;

            delete item;

			if(search_result_data == NULL)
				return false ;

			RsGxsNetTunnelSerializer().serialise(&search_result_item,search_result_data,&search_result_data_size);

			return true ;
        }
    }

    RsGxsNetTunnelTurtleSearchGroupRequestItem *substring_gr = dynamic_cast<RsGxsNetTunnelTurtleSearchGroupRequestItem *>(item) ;

    if(substring_gr != NULL)
    {
		RS_STACK_MUTEX(mGxsNetTunnelMtx);
		auto it = mSearchableServices.find(substring_gr->service) ;

        max_allowed_hits = RS_GXS_NET_TUNNEL_MAX_ALLOWED_HITS_GROUP_DATA ;

        unsigned char *encrypted_group_data = NULL ;
        uint32_t encrypted_group_data_len = 0 ;

        if(it != mSearchableServices.end() && it->second->search(substring_gr->hashed_group_id,encrypted_group_data,encrypted_group_data_len))
        {
			RsGxsNetTunnelTurtleSearchGroupDataItem search_result_item ;

            search_result_item.service = substring_gr->service ;
            search_result_item.encrypted_group_data = encrypted_group_data ;
            search_result_item.encrypted_group_data_len = encrypted_group_data_len;

            search_result_data_size = RsGxsNetTunnelSerializer().size(&search_result_item) ;
			search_result_data = (unsigned char*)rs_malloc(search_result_data_size) ;

			if(search_result_data == NULL)
				return false ;

			RsGxsNetTunnelSerializer().serialise(&search_result_item,search_result_data,&search_result_data_size);

            delete item;
			return true ;
        }
    }

    delete item;
    return false ;
}

void RsGxsNetTunnelService::receiveSearchResult(TurtleSearchRequestId request_id,unsigned char *search_result_data,uint32_t search_result_data_len)
{
    RsItem *item = RsGxsNetTunnelSerializer().deserialise(search_result_data,&search_result_data_len);

	GXS_NET_TUNNEL_DEBUG() << "  : received search result for search request " << std::hex << request_id << "" << std::endl;

    RsGxsNetTunnelTurtleSearchGroupSummaryItem *result_gs = dynamic_cast<RsGxsNetTunnelTurtleSearchGroupSummaryItem *>(item) ;

    if(result_gs != NULL)
	{
		GXS_NET_TUNNEL_DEBUG() << "  : result is of type group summary result for service " << result_gs->service << std::dec << ": " << std::endl;

#ifdef DEBUG_RSGXSNETTUNNEL
		for(auto it(result_gs->group_infos.begin());it!=result_gs->group_infos.end();++it)
			std::cerr << "   group " << (*it).mGroupId << ": " << (*it).mGroupName << ", " << (*it).mNumberOfMessages << " messages, last is " << time(NULL)-(*it).mLastMessageTs << " secs ago." << std::endl;
#endif

		auto it = mSearchableServices.find(result_gs->service) ;

		if(it == mSearchableServices.end())
		{
			GXS_NET_TUNNEL_ERROR() << ": deserialized item is for service " << std::hex << result_gs->service << std::dec << " that is not in the searchable services list." << std::endl;
            delete item;
			return ;
		}

		it->second->receiveTurtleSearchResults(request_id,result_gs->group_infos) ;

		delete item;
		return ;
	}

    RsGxsNetTunnelTurtleSearchGroupDataItem *result_gd = dynamic_cast<RsGxsNetTunnelTurtleSearchGroupDataItem *>(item) ;

    if(result_gd != NULL)
    {
		GXS_NET_TUNNEL_DEBUG() << "  : result is of type group data for service " << result_gd->service << std::dec << ": " << std::endl;

		auto it = mSearchableServices.find(result_gd->service) ;

		if(it == mSearchableServices.end())
		{
			GXS_NET_TUNNEL_ERROR() << ": deserialized item is for service " << std::hex << result_gd->service << std::dec << " that is not in the searchable services list." << std::endl;
            delete item;
			return ;
		}

		it->second->receiveTurtleSearchResults(request_id,result_gd->encrypted_group_data,result_gd->encrypted_group_data_len) ;

        result_gd->encrypted_group_data = NULL ; // prevents deletion
		delete item;

		return ;
    }

	GXS_NET_TUNNEL_ERROR() << ": deserialized item is of unknown type. Dropping!" << std::endl;
}

void RsGxsNetTunnelService::getStatistics(std::map<RsGxsGroupId,RsGxsNetTunnelGroupInfo>& groups, std::map<RsGxsNetTunnelVirtualPeerId, RsGxsNetTunnelVirtualPeerInfo>& virtual_peers, std::map<TurtleVirtualPeerId, RsGxsNetTunnelVirtualPeerId> &turtle_vpid_to_net_tunnel_vpid, Bias20Bytes& bias      ) const
{
    groups = mGroups ;
    virtual_peers = mVirtualPeers ;
    turtle_vpid_to_net_tunnel_vpid = mTurtle2GxsPeer;
    bias = mRandomBias ;
}




















