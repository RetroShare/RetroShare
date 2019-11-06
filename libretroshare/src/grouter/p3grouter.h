/*******************************************************************************
 * libretroshare/src/grouter: p3grouter.h                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2013  Cyril Soler <csoler@users.sourceforge.net>              *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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
#include <fstream>
#include <list>

#include "retroshare/rsgrouter.h"
#include "retroshare/rstypes.h"
#include "retroshare/rstypes.h"
#include "retroshare/rsidentity.h"

#include "turtle/turtleclientservice.h"
#include "services/p3service.h"
#include "pqi/p3cfgmgr.h"
#include "util/rsdebug.h"
#include "groutertypes.h"
#include "groutermatrix.h"
#include "grouteritems.h"

// To be put in pqi/p3cfgmgr.h
static const uint32_t CONFIG_TYPE_GROUTER = 0x0016 ;

class p3LinkMgr ;
class p3turtle ;
class RsGixs ;
class RsGRouterItem ;
class RsGRouterGenericDataItem ;
class RsGRouterSignedReceiptItem ;
class RsGRouterAbstractMsgItem ;
class RsGRouterTransactionItem ;
class RsGRouterTransactionAcknItem ;

// This class is responsible for accepting data chunks and merging them into a final object. When the object is
// complete, it is de-serialised and returned as a RsGRouterGenericDataItem*.

class GRouterTunnelInfo
{
public:
    GRouterTunnelInfo() :first_tunnel_ok_TS(0), last_tunnel_ok_TS(0) {}

    // These two methods handle the memory management of buffers for each virtual peers.

    void addVirtualPeer(const TurtleVirtualPeerId& vpid) ;
    void removeVirtualPeer(const TurtleVirtualPeerId& vpid) ;

    std::set<TurtleVirtualPeerId> virtual_peers ;

    rstime_t first_tunnel_ok_TS ;	// timestamp when 1st tunnel was received.
    rstime_t last_tunnel_ok_TS ;	// timestamp when last tunnel was received.
};
class GRouterDataInfo
{
    // ! This class does not have a copy constructor that duplicates the incoming data buffer. This is on purpose!
public:
    GRouterDataInfo() : last_activity_TS(0)
    {
        incoming_data_buffer = NULL ;
    }

    void clear() { if(incoming_data_buffer!=NULL) delete incoming_data_buffer ; incoming_data_buffer = NULL ;}

    // These two methods handle the memory management of buffers for each virtual peers.

    RsGRouterAbstractMsgItem *addDataChunk(RsGRouterTransactionChunkItem *chunk_item) ;
    RsGRouterTransactionChunkItem *incoming_data_buffer ;

    rstime_t last_activity_TS ;
};

class p3GRouter: public RsGRouter, public RsTurtleClientService, public p3Service, public p3Config
{
public:
    p3GRouter(p3ServiceControl *sc,RsGixs *is) ;

    //===================================================//
    //         Router clients business                   //
    //===================================================//

    // This method allows to associate client ids (that are saved to disk) to client objects deriving
    // from GRouterClientService. The various services are responsible for regstering themselves to the
    // global router, with consistent ids. The services are stored in a map, and arriving objects are
    // passed on the correct service depending on the client id of the key they are reaching.
	virtual bool registerClientService(
	        RsServiceType serivceType, GRouterClientService* service );

    // Use this method to register/unregister a key that the global router will
    // forward in the network, so that is can be a possible destination for
    // global messages.
    //
    // 	auth_id     : The GXS key that will be used to sign the data Receipts.
    // 	contact_key : The key that is used to open tunnels
    // 	desc_str    : Any fixed length string (< 20 characters) to descript the address in words.
    // 	client_id   : Id of the client service to send the traffic to.
    // 	               The client ID should match the ID that has been registered using the previous method.
    //
    // Unregistering a key might not have an instantaneous effect, so the client is responsible for
    // discarding traffic that might later come for this key.
	//
	bool registerKey(
	        const RsGxsId& authentication_id, RsServiceType clientServiceType,
	        const std::string& description_string ) override;
	bool unregisterKey(const RsGxsId& key_id, RsServiceType sid) override;

    //===================================================//
    //         Routing clue collection methods           //
    //===================================================//

	virtual void addRoutingClue(
	        const RsGxsId& id, const RsPeerId& peer_id) override;

    //===================================================//
    //         Client/server request services            //
    //===================================================//

    // Sends an item to the given destination.  The router takes ownership of
    // the memory. That means item_data will be erase on return. The returned id should be
    // remembered by the client, so that he knows when the data has been received.
    // The client id is supplied so that the client can be notified when the data has been received.
	// Data is not modified by the global router.
	virtual bool sendData(
	        const RsGxsId& destination, RsServiceType client_id,
	        const uint8_t* data, uint32_t data_size, const RsGxsId& signing_id,
	        GRouterMsgPropagationId& id ) override;

    // Cancels a given sending order. If called too late, the message might already have left. But this will remove the item from the
    // re-try list.
    virtual bool cancel(GRouterMsgPropagationId mid) ;

    //===================================================//
    //                  Interface with RsGRouter         //
    //===================================================//

    // debug info from routing matrix
    // 	- list of known key ids
    // 	- list of clues/time_stamp for each key.
    // 	- real time routing probabilities
    //
    virtual bool getRoutingMatrixInfo(GRouterRoutingMatrixInfo& info) ;

    // debug info from routing cache
    // 	- Cache Items
    // 		* which message ids
    // 		* directions
    // 		* timestamp
    // 		* message type
    // 	- Cache state (memory size, etc)
    //
    virtual bool getRoutingCacheInfo(std::vector<GRouterRoutingCacheInfo>& info) ;

    //===================================================//
    //         Derived from p3Service                    //
    //===================================================//

    virtual RsServiceInfo getServiceInfo()
    {
        return RsServiceInfo(RS_SERVICE_TYPE_GROUTER,
                             SERVICE_INFO_APP_NAME,
                             SERVICE_INFO_APP_MAJOR_VERSION,
                             SERVICE_INFO_APP_MINOR_VERSION,
                             SERVICE_INFO_MIN_MAJOR_VERSION,
                             SERVICE_INFO_MIN_MINOR_VERSION) ;
    }

    virtual void setDebugEnabled(bool b) { _debug_enabled = b ; }

    virtual void connectToTurtleRouter(p3turtle *pt) ;

protected:
    //===================================================//
    //         Routing method handling                   //
    //===================================================//

    // Calls
    // 	- packet handling methods
    // 	- matrix updates
    //
    virtual int tick() ;

    static const std::string SERVICE_INFO_APP_NAME ;
    static const uint16_t SERVICE_INFO_APP_MAJOR_VERSION  =	1;
    static const uint16_t SERVICE_INFO_APP_MINOR_VERSION  =	0;
    static const uint16_t SERVICE_INFO_MIN_MAJOR_VERSION  =	1;
    static const uint16_t SERVICE_INFO_MIN_MINOR_VERSION  =	0;

    //===================================================//
    //         Interaction with turtle router            //
    //===================================================//

    uint16_t serviceId() const { return RS_SERVICE_TYPE_GROUTER; }
    virtual bool handleTunnelRequest(const RsFileHash& /*hash*/,const RsPeerId& /*peer_id*/) ;
    virtual void receiveTurtleData(const RsTurtleGenericTunnelItem */*item*/,const RsFileHash& /*hash*/,const RsPeerId& /*virtual_peer_id*/,RsTurtleGenericTunnelItem::Direction /*direction*/);
    virtual void addVirtualPeer(const TurtleFileHash& hash,const TurtleVirtualPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction dir) ;
    virtual void removeVirtualPeer(const TurtleFileHash& hash,const TurtleVirtualPeerId& virtual_peer_id) ;

private:
    //===================================================//
    //             Low level item sorting                //
    //===================================================//

    void handleLowLevelServiceItems() ;
    void handleLowLevelServiceItem(RsGRouterTransactionItem*) ;
    void handleLowLevelTransactionChunkItem(RsGRouterTransactionChunkItem *chunk_item);
    void handleLowLevelTransactionAckItem(RsGRouterTransactionAcknItem*) ;

	static Sha1CheckSum computeDataItemHash(RsGRouterGenericDataItem *data_item);

    std::ostream& grouter_debug() const
    {
        static std::ostream null(0);
        return _debug_enabled?(std::cerr):null;
    }

    void routePendingObjects() ;
    void handleTunnels() ;
    void autoWash() ;

    //===================================================//
    //             High level item sorting               //
    //===================================================//

	void handleIncoming();
	bool handleIncomingItem(std::unique_ptr<RsGRouterAbstractMsgItem> item);

	void handleIncomingReceiptItem(
	        std::unique_ptr<RsGRouterSignedReceiptItem> receiptItem );
	void handleIncomingDataItem(
	        std::unique_ptr<RsGRouterGenericDataItem> dataItem );

	bool locked_getLocallyRegisteredClientFromServiceId(
	        RsServiceType serviceType,
	        GRouterClientService*& clientServiceInstance );

    // utility functions
    //
    static float computeMatrixContribution(float base,uint32_t time_shift,float probability) ;
    static bool sliceDataItem(RsGRouterAbstractMsgItem *,std::list<RsGRouterTransactionChunkItem*>& chunks) ;

    uint32_t computeRandomDistanceIncrement(const RsPeerId& pid,const GRouterKeyId& destination_id) ;

    // signs an item with the given key.
    bool signDataItem(RsGRouterAbstractMsgItem *item,const RsGxsId& id) ;
    bool verifySignedDataItem(RsGRouterAbstractMsgItem *item, const RsIdentityUsage::UsageCode &info, uint32_t &error_status) ;
    bool encryptDataItem(RsGRouterGenericDataItem *item,const RsGxsId& destination_key) ;
    bool decryptDataItem(RsGRouterGenericDataItem *item) ;

    static Sha1CheckSum makeTunnelHash(const RsGxsId& destination,const GRouterServiceId& client);

    //bool locked_getGxsIdAndClientId(const TurtleFileHash &sum,RsGxsId& gxs_id,GRouterServiceId& client_id);
    bool locked_sendTransactionData(const RsPeerId& pid,const RsGRouterTransactionItem& item);

    void locked_collectAvailableFriends(const GRouterKeyId &gxs_id, const std::set<RsPeerId>& incoming_routes,uint32_t duplication_factor, std::map<RsPeerId, uint32_t> &friend_peers_and_duplication_factors);
    void locked_collectAvailableTunnels(const TurtleFileHash& hash, uint32_t total_duplication, std::map<RsPeerId, uint32_t> &tunnel_peers_and_duplication_factors);
    void locked_sendToPeers(RsGRouterGenericDataItem *data_item, const std::map<RsPeerId, uint32_t> &peers_and_duplication_factors);

    //===================================================//
    //                  p3Config methods                 //
    //===================================================//

    // Load/save the routing info, the pending items in transit, and the config variables.
    //
    virtual bool loadList(std::list<RsItem*>& items) ;
    virtual bool saveList(bool& cleanup,std::list<RsItem*>& items) ;

    virtual RsSerialiser *setupSerialiser() ;

    //===================================================//
    //                  Debug methods                    //
    //===================================================//

    // Prints the internal state of the router, for debug purpose.
    //
    void debugDump() ;

    //===================================================//
    //              Internal queues/variables            //
    //===================================================//

    // Stores the routing info
    // 	- list of known key ids
    // 	- list of clues/time_stamp for each key.
    // 	- real time routing probabilities
    //
    GRouterMatrix _routing_matrix ;

    // Stores the keys which identify the router's node. For each key, a structure holds:
    // 	- the client service
    // 	- flags
    // 	- usage time stamps
    //
    std::map<Sha1CheckSum, GRouterPublishedKeyInfo> _owned_key_ids ;

    // Registered services. These are known to the different peers with a common id,
    // so it's important to keep consistency here. This map is volatile, and re-created at each startup of
    // the software, when newly created services register themselves.

	std::map<RsServiceType, GRouterClientService*> _registered_services;

    // Stores the routing events.
    // 	- ongoing requests, waiting for return ACK
    // 	- pending items
    // Both a stored in 2 different lists, to allow a more efficient handling.
    //
    std::map<GRouterMsgPropagationId, GRouterRoutingInfo> _pending_messages;// pending messages

    // Stores virtual peers that appear/disappear as the result of the turtle router client
    //
    std::map<TurtleFileHash,GRouterTunnelInfo> _tunnels ;

    // Stores incoming data from any peers (virtual and real) into chunks that get aggregated until finished.
    //
    std::map<RsPeerId,GRouterDataInfo> _incoming_data_pipes ;

    // Queue of incoming items. Might be receipts or data. Should always be empty (not a storage place)
    std::list<RsGRouterAbstractMsgItem *> _incoming_items ;

    // Data handling methods
    //
    //void handleRecvDataItem(RsGRouterGenericDataItem *item);
    //void handleRecvReceiptItem(RsGRouterReceiptItem *item);

    // Pointers to other RS objects
    //
    p3ServiceControl *mServiceControl ;
    p3turtle *mTurtle ;
    RsGixs *mGixs ;
    //p3LinkMgr *mLinkMgr ;

    // Multi-thread protection mutex.
    //
    RsMutex grMtx ;

    // config update/save variables
    bool _changed ;
    bool _debug_enabled ;

    rstime_t _last_autowash_time ;
    rstime_t _last_matrix_update_time ;
    rstime_t _last_debug_output_time ;
    rstime_t _last_config_changed ;

    uint64_t _random_salt ;

	/** Temporarly store items that could not have been verified yet due to
	 * missing author key, attempt to handle them once in a while.
	 * The items are discarded if after mMissingKeyQueueEntryTimeout the key
	 * hasn't been received yet, and are not saved on RetroShare stopping. */
	std::list< std::pair<
	    std::unique_ptr<RsGRouterAbstractMsgItem>, rstime_t > > mMissingKeyQueue;
	RsMutex mMissingKeyQueueMtx; /// protect mMissingKeyQueue

	/// @see mMissingKeyQueue
	static constexpr rstime_t mMissingKeyQueueEntryTimeout = 600;

	/// @see mMissingKeyQueue
	static constexpr rstime_t mMissingKeyQueueCheckEvery = 30;

	/// @see mMissingKeyQueue
	rstime_t mMissingKeyQueueCheckLastCheck = 0;

	RS_SET_CONTEXT_DEBUG_LEVEL(2)
};
