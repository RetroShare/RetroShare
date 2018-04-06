/*
 * libretroshare/src/gxs: rsgxsnettunnel.cc
 *
 * General Data service, interface for RetroShare.
 *
 * Copyright 2018-2018 by Cyril Soler
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
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
 * Please report all bugs and problems to "retroshare.project@gmail.com"
 *
 */

#include "util/rsdir.h"
#include "util/rstime.h"
#include "retroshare/rspeers.h"
#include "serialiser/rstypeserializer.h"
#include "rsgxsnettunnel.h"

#define DEBUG_RSGXSNETTUNNEL 1

#define GXS_NET_TUNNEL_NOT_IMPLEMENTED() { std::cerr << __PRETTY_FUNCTION__ << ": not yet implemented." << std::endl; }
#define GXS_NET_TUNNEL_DEBUG()             std::cerr << time(NULL) << " : GXS_NET_TUNNEL : " << __FUNCTION__ << " : "
#define GXS_NET_TUNNEL_ERROR()             std::cerr << "(EE) GXS_NET_TUNNEL ERROR : "


RsGxsNetTunnelService::RsGxsNetTunnelService(): mGxsNetTunnelMtx("GxsNetTunnel") {}

//===========================================================================================================================================//
//                                                               Transport Items                                                             //
//===========================================================================================================================================//

const uint16_t RS_SERVICE_TYPE_GXS_NET_TUNNEL = 0x2233 ;

const uint8_t  RS_PKT_SUBTYPE_GXS_NET_TUNNEL_VIRTUAL_PEER = 0x01 ;
const uint8_t  RS_PKT_SUBTYPE_GXS_NET_TUNNEL_KEEP_ALIVE   = 0x02 ;

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
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx) {}
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
		case RS_PKT_SUBTYPE_GXS_NET_TUNNEL_VIRTUAL_PEER: return new RsGxsNetTunnelVirtualPeerItem ;
		case RS_PKT_SUBTYPE_GXS_NET_TUNNEL_KEEP_ALIVE  : return new RsGxsNetTunnelKeepAliveItem ;
		default:
			GXS_NET_TUNNEL_ERROR() << "type ID " << std::hex << item_subtype << std::dec << " is not handled!" << std::endl;
			return NULL ;
		}
	}
};

//===========================================================================================================================================//
//                                                     Interface with rest of the software                                                   //
//===========================================================================================================================================//

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
		for(auto it2((*it).second.begin());it2!=(*it).second.end();++it2)
			delete (*it2).second;

	mIncomingData.clear();
}

bool RsGxsNetTunnelService::isDistantPeer(const RsGxsNetTunnelVirtualPeerId& virtual_peer)
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);

	return mVirtualPeers.find(virtual_peer) != mVirtualPeers.end();
}

bool RsGxsNetTunnelService::receiveData(uint16_t service_id,unsigned char *& data,uint32_t& data_len,RsGxsNetTunnelVirtualPeerId& virtual_peer)
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);

	std::list<std::pair<RsGxsNetTunnelVirtualPeerId,RsTlvBinaryData*> >& lst(mIncomingData[service_id]) ;

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

bool RsGxsNetTunnelService::sendData(unsigned char *& data,uint32_t data_len,const RsGxsNetTunnelVirtualPeerId& virtual_peer)
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

bool RsGxsNetTunnelService::getVirtualPeers(uint16_t service_id, std::list<RsGxsNetTunnelVirtualPeerId>& peers)
{
	// This function has two effects:
	//  - return the virtual peers for this group
	//  - passively set the group as "managed", so that it we answer tunnel requests.

	RS_STACK_MUTEX(mGxsNetTunnelMtx);

	// update the hash entry if needed

	for(auto it(mVirtualPeers.begin());it!=mVirtualPeers.end();++it)
		if(it->second.providing_set.find(service_id) != it->second.providing_set.end())
			peers.push_back(it->first) ;

#ifdef DEBUG_RSGXSNETTUNNEL
    GXS_NET_TUNNEL_DEBUG() << " service " << std::hex << service_id << std::dec << " returning " << peers.size() << " peers." << std::endl;
#endif

	return true ;
}

bool RsGxsNetTunnelService::requestPeers(uint16_t service_id,const RsGxsGroupId& group_id)
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);

    // Now ask the turtle router to manage a tunnel for that hash.

	RsGxsNetTunnelGroupInfo& ginfo( mGroups[group_id] ) ;

    ginfo.group_policy = RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_POLICY_ACTIVE;
	ginfo.hash         = calculateGroupHash(group_id) ;
	ginfo.service_id   = service_id;

	mHandledHashes[ginfo.hash] = group_id ;

	// we dont set the group policy here. It will only be set if no peers, or too few peers are available.
#ifdef DEBUG_RSGXSNETTUNNEL
    GXS_NET_TUNNEL_DEBUG() << " service " << std::hex << service_id << std::dec << " requesting peers for group " << group_id << std::endl;
#endif

	return true;
}

bool RsGxsNetTunnelService::releasePeers(uint16_t service_id, const RsGxsGroupId& group_id)
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);

    // Ask turtle router to stop requesting tunnels for that hash.

	RsGxsNetTunnelGroupInfo& ginfo( mGroups[group_id] ) ;

    ginfo.group_policy = RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_POLICY_PASSIVE;
	ginfo.hash         = calculateGroupHash(group_id) ;
	ginfo.service_id   = service_id;

	mHandledHashes[ginfo.hash] = group_id ;	// yes, we do not remove, because we're supposed to answer tunnel requests from other peers.

	mTurtle->stopMonitoringTunnels(ginfo.hash) ;

#ifdef DEBUG_RSGXSNETTUNNEL
    GXS_NET_TUNNEL_DEBUG() << " service " << std::hex << service_id << std::dec << " releasing peers for group " << group_id << std::endl;
#endif
	return true;
}

RsGxsNetTunnelVirtualPeerId RsGxsNetTunnelService::locked_makeVirtualPeerId() const
{
	assert(RsPeerId::SIZE_IN_BYTES <= Sha1CheckSum::SIZE_IN_BYTES) ;

	// We compute sha1( SSL_id | mRandomBias ) and trunk it to 16 bytes in order to compute a RsPeerId

	RsPeerId ssl_id = rsPeers->getOwnId() ;

	unsigned char mem[RsPeerId::SIZE_IN_BYTES /*+ RsGxsGroupId::SIZE_IN_BYTES */ + RS_GXS_TUNNEL_CONST_RANDOM_BIAS_SIZE];

	memcpy(mem                                                    ,ssl_id.toByteArray()  ,RsPeerId::SIZE_IN_BYTES) ;
  //memcpy(mem+RsPeerId::SIZE_IN_BYTES                            ,group_id.toByteArray(),RsGxsGroupId::SIZE_IN_BYTES) ;
	memcpy(mem+RsPeerId::SIZE_IN_BYTES /*+RsGxsGroupId::SIZE_IN_BYTES*/,mRandomBias           ,RS_GXS_TUNNEL_CONST_RANDOM_BIAS_SIZE) ;

	return RsGxsNetTunnelVirtualPeerId(RsDirUtil::sha1sum(mem,RsPeerId::SIZE_IN_BYTES+RsGxsGroupId::SIZE_IN_BYTES+RS_GXS_TUNNEL_CONST_RANDOM_BIAS_SIZE).toByteArray());
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

	static std::string group_policy_str[3] = {
	    	 std::string("[UNKNOWN]"),
	    	 std::string("[PASSIVE]"),
	    	 std::string("[ACTIVE ]"),
	};
	static std::string vpid_status_str[3] = {
	    	 std::string("[UNKNOWN  ]"),
		     std::string("[TUNNEL_OK]"),
		     std::string("[ACTIVE   ]")
	};

	std::cerr << "GxsNetTunnelService dump: " << std::endl;
	std::cerr << "Managed GXS groups: " << std::endl;

	for(auto it(mGroups.begin());it!=mGroups.end();++it)
	{
		std::cerr << "  " << it->first << " hash: " << it->second.hash << "  policy: " << group_policy_str[it->second.group_policy] << " status: " << group_status_str[it->second.group_status] << "  Last contact: " << time(NULL) - it->second.last_contact << " secs ago" << std::endl;
		std::cerr << "  virtual peers:" << std::endl;
		for(auto it2(it->second.virtual_peers.begin());it2!=it->second.virtual_peers.end();++it2)
			std::cerr << "    " << *it2 << std::endl;
	}

	std::cerr << "Virtual peers: " << std::endl;
	for(auto it(mVirtualPeers.begin());it!=mVirtualPeers.end();++it)
	{
		std::cerr << "  GXS Peer:" << it->first << " Turtle:" << it->second.turtle_virtual_peer_id
			      << "  status: " <<  vpid_status_str[it->second.vpid_status] << " direction: "
			      << (int)it->second.side << " last seen " << time(NULL)-it->second.last_contact
			      << " ekey: " << RsUtil::BinToHex(it->second.encryption_master_key,RS_GXS_TUNNEL_CONST_EKEY_SIZE,10) << std::endl;

		for(auto it2(it->second.providing_set.begin());it2!=it->second.providing_set.end();++it2)
			std::cerr << "    service " << std::hex << it2->first << std::dec << "   " << it2->second.provided_groups.size() << " groups" << std::endl;
	}

	std::cerr << "Hashes: " << std::endl;
	for(auto it(mHandledHashes.begin());it!=mHandledHashes.end();++it)
		std::cerr << "   hash: " << it->first << " GroupId: " << it->second << std::endl;

	std::cerr << "Incoming data: " << std::endl;
	for(auto it(mIncomingData.begin());it!=mIncomingData.end();++it)
		for(auto it2(it->second.begin());it2!=it->second.end();++it2)
			std::cerr << "   service " << std::hex << it->first << std::dec << " peer id " << it2->first << " " << (void*)it2->second << std::endl;
}

//===========================================================================================================================================//
//                                                         Interaction with Turtle Router                                                    //
//===========================================================================================================================================//

void RsGxsNetTunnelService::connectToTurtleRouter(p3turtle *tr)
{
	mTurtle = tr ;
	mTurtle->registerTunnelService(this) ;
}

bool RsGxsNetTunnelService::handleTunnelRequest(const RsFileHash &hash,const RsPeerId& peer_id)
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);
	// We simply check for wether a managed group has a hash that corresponds to the given hash.

	return mHandledHashes.find(hash) != mHandledHashes.end();
}

void RsGxsNetTunnelService::receiveTurtleData(RsTurtleGenericTunnelItem *item,const RsFileHash& hash,const RsPeerId& turtle_virtual_peer_id,RsTurtleGenericTunnelItem::Direction direction)
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

	if(!p3turtle::decryptItem(static_cast<RsTurtleGenericDataItem*>(item),encryption_master_key,data,data_size))
	{
		GXS_NET_TUNNEL_ERROR() << "Cannot decrypt data!" << std::endl;

		if(data)
			free(data) ;

		return ;
	}

	if(getRsItemService(getRsItemId(data)) == RS_SERVICE_TYPE_GXS_NET_TUNNEL)
	{
		RsItem *decrypted_item = RsGxsNetTunnelSerializer().deserialise(data,&data_size);
		RsGxsNetTunnelVirtualPeerItem *pid_item = dynamic_cast<RsGxsNetTunnelVirtualPeerItem*>(decrypted_item) ;

		if(!pid_item)
		{
			delete decrypted_item ;
			return ;
		}

		uint16_t service_id = mGroups[group_id].service_id ;

#ifdef DEBUG_RSGXSNETTUNNEL
		GXS_NET_TUNNEL_DEBUG() << "    item is a virtual peer id item with vpid = "<< pid_item->virtual_peer_id
		                       << " for group " << group_id << " in service " << std::hex << service_id << std::dec
		                       << ". Setting virtual peer." << std::endl;
#endif
		// we receive a virtual peer id, so we need to update the local information for this peer id

		mTurtle2GxsPeer[turtle_virtual_peer_id] = pid_item->virtual_peer_id ;

		RsGxsNetTunnelVirtualPeerInfo& vp_info(mVirtualPeers[pid_item->virtual_peer_id]) ;

		vp_info.vpid_status = RsGxsNetTunnelVirtualPeerInfo::RS_GXS_NET_TUNNEL_VP_STATUS_ACTIVE ;					// status of the peer
		vp_info.side = direction;	                        // client/server
		vp_info.last_contact = time(NULL);					// last time some data was sent/recvd
		vp_info.providing_set[service_id].provided_groups.insert(group_id);

		memcpy(vp_info.encryption_master_key,encryption_master_key,RS_GXS_TUNNEL_CONST_EKEY_SIZE);

		vp_info.turtle_virtual_peer_id = turtle_virtual_peer_id;  // turtle peer to use when sending data to this vpid.

		free(data);
	}
	else
	{
		// item is a generic data item for the client. Let's store the data in the appropriate incoming data queue.

		auto it = mTurtle2GxsPeer.find(turtle_virtual_peer_id) ;

		if(it == mTurtle2GxsPeer.end())
		{
			GXS_NET_TUNNEL_ERROR() << "item received by GxsNetTunnel for vpid " << turtle_virtual_peer_id << " but this vpid is unknown!" << std::endl;
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

		uint16_t service_id = getRsItemService(getRsItemId(data)) ;

#ifdef DEBUG_RSGXSNETTUNNEL
		GXS_NET_TUNNEL_DEBUG() << "item contains generic data for service " << std::hex << service_id << std::dec << " for VPID " << gxs_vpid << ". Storing in incoming list" <<  std::endl;
#endif

		// push the data into the service incoming data list

		RsTlvBinaryData *bind = new RsTlvBinaryData;
		bind->tlvtype = 0;
		bind->bin_len = data_size;
		bind->bin_data = data;

		mIncomingData[service_id].push_back(std::make_pair(gxs_vpid,bind)) ;
	}
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

	RsGxsNetTunnelVirtualPeerId net_service_virtual_peer = locked_makeVirtualPeerId();

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
		ginfo.group_status = RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_STATUS_TUNNELS_REQUESTED ;

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

void RsGxsNetTunnelService::data_tick()
{
	while(!mPendingTurtleItems.empty())
	{
		auto& it(mPendingTurtleItems.front());

		mTurtle->sendTurtleData(it.first,it.second) ;
		mPendingTurtleItems.pop_front();
	}

	rstime::rs_usleep(1*1000*1000); // 1 sec

	time_t now = time(NULL);

	// cleanup

	static time_t last_autowash = time(NULL);

	if(last_autowash + 5 < now)
	{
		autowash();
		last_autowash = now;
	}

	static time_t last_dump = time(NULL);

	if(last_dump + 10 < now)
	{
		last_dump = now;
		dump();
	}
}

void RsGxsNetTunnelService::autowash()
{
	RS_STACK_MUTEX(mGxsNetTunnelMtx);

	for(auto it(mGroups.begin());it!=mGroups.end();++it)
	{
		// check if the group is in active or passive mode, in which case make sure that turtle manages tunnels or not.
		// if active, then we check wether they are tunnels are not. If not, we ask turtle to monitor.

		RsGxsNetTunnelGroupInfo& ginfo(it->second) ;

		if(ginfo.group_policy == RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_POLICY_ACTIVE && ginfo.virtual_peers.empty())
		{
			mTurtle->monitorTunnels(ginfo.hash,this,false) ;
			ginfo.group_status = RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_STATUS_TUNNELS_REQUESTED;
		}

		if(ginfo.group_policy == RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_POLICY_PASSIVE)
		{
			mTurtle->stopMonitoringTunnels(ginfo.hash);
			ginfo.group_status = RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_STATUS_IDLE;
		}
	}
}


