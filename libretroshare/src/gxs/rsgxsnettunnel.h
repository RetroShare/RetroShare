/*
 * libretroshare/src/gxs: rsgxsnettunnel.h
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

#include <map>

#include <turtle/p3turtle.h>

/*!
 * \brief The RsGxsNetTunnelService class takes care of requesting tunnels to the turtle router, through which it is possible to sync
 *        a particular GXS group. For each group, a set of virtual peers, corresponding to active tunnels will be made available
 *        to RsGxsNetService.
 *
 *        It is the responsibility of RsGxsNetService to activate/desactivate tunnels for each particular group, depending on wether the group
 *        is already available at friends or not.
 *
 *        Tunnel management is done at the level of groups rather than services, because we would like to keep the possibility to not
 *        request tunnels for some groups which do not need it, and only request tunnels for specific groups that cannot be provided
 *        by direct connections.
 */

//   Protocol:
//      * request tunnels based on H(GroupId)
//      * encrypt tunnel data using chacha20+HMAC-SHA256 using AEAD( GroupId, 96bits IV, tunnel ID ) (similar to what FT does)
//      * when tunnel is established, exchange virtual peer names: vpid = H( GroupID | Random bias )
//      * when vpid is known, notify the client (GXS net service) which can use the virtual peer to sync
//
//      * only use a single tunnel per virtual peer ID
//                                                                                        -
//              Client  ------------------ TR(H(GroupId)) -------------->  Server         |
//                                                                                        |   Turtle
//              Client  <-------------------- T OK ----------------------  Server         |
//                                                                                        -
//                               Here, a turtle vpid is known                             |   [ addVirtualPeer() called by turtle ]
//                                                                                        -
//           [Encrypted traffic using H(GroupId | Tunnel ID, 96bits IV)]                  |
//                                                                                        |
//              Client  <--------- VPID = H( Random IV | GroupId ) ------  Server         |
//                 |                                                         |            |
//                 +--------------> Mark the virtual peer active <-----------+            |    Encrypted traffic decoded locally and sorted
//                                                                                        |
//                         Here, a consistent virtual peer ID is known                    |
//                                                                                        |
//              Client  <------------------- GXS Data ------------------>  Server         |
//                                                                                        -
//   Notes:
//      * tunnels are established symetrically. If a distant peers wants to sync the same group, they'll have to open a single tunnel, with a different ID.
//        Groups therefore have two states:
//          - managed : the group can be used to answer tunnel requests. If server tunnels are established, the group will be synced with these peers
//          - tunneled: the group will actively request tunnels. If tunnels are established both ways, the same virtual peers will be used so the tunnels are "merged".
//                        * In practice, that means one of the two tunnels will not be used and therefore die.
//                        * If a tunneled group already has enough virtual peers, it will not request for tunnels itself.
//
//           Group policy      | Request tunnels   |    SyncWithPeers      |  Item receipt
//         --------------------+-------------------+-----------------------+----------------
//             Passive         |        no         |  If peers present     | If peers present
//             Active          |  yes, if no peers |  If peers present     | If peers present
//                             |                   |                       |
//
//      * when a service has the DistSync flag set, groups to sync are communicated passively to the GxsNetTunnel service when requesting distant peers.
//        However, a call should be made to set a particular group policy to "ACTIVE" for group that do not have peers and need some.
//
//      * services also need to retrieve GXS data items that come out of tunnels. These will be available as (data,len) type, since they are not de-serialized.
//
//      * GxsNetService stores data information (such as update maps) per peerId, so it makes sense to use the same PeerId for all groups of a given service
//        Therefore, virtual peers are stored separately from groups, because each one can sync multiple groups.
//
//      * virtual peers are also shared among services. This reduces the required amount of tunnels and tunnel requests to send.

typedef RsPeerId RsGxsNetTunnelVirtualPeerId ;

class RsGxsNetTunnelItem ;

struct RsGxsNetTunnelVirtualPeerProvidingSet
{
	std::set<RsGxsGroupId>      provided_groups ;
	std::list<RsTlvBinaryData*> incoming_data ;
};

struct RsGxsNetTunnelVirtualPeerInfo
{
	enum {   RS_GXS_NET_TUNNEL_VP_STATUS_UNKNOWN   = 0x00,		// unknown status.
		     RS_GXS_NET_TUNNEL_VP_STATUS_TUNNEL_OK = 0x01,		// tunnel has been established and we're waiting for virtual peer id
		     RS_GXS_NET_TUNNEL_VP_STATUS_ACTIVE    = 0x02		// virtual peer id is known. Data can transfer.
	     };

	RsGxsNetTunnelVirtualPeerInfo() : vpid_status(RS_GXS_NET_TUNNEL_VP_STATUS_UNKNOWN) { memset(encryption_master_key,0,32) ; }
	~RsGxsNetTunnelVirtualPeerInfo() ;

	uint8_t vpid_status ;					// status of the peer
	uint8_t side ;	                        // client/server
	uint8_t encryption_master_key[32] ;		// key from which the encryption key is derived for each virtual peer (using H(master_key | random IV))
	time_t  last_contact ;					// last time some data was sent/recvd

	TurtleVirtualPeerId 		turtle_virtual_peer_id ;  // turtle peer to use when sending data to this vpid.
	RsGxsGroupId                group_id ;		          // group id

	std::map<uint16_t,RsGxsNetTunnelVirtualPeerProvidingSet> providing_set;	// partial list of groups provided by this virtual peer id, based on tunnel results, for each service
};

struct RsGxsNetTunnelGroupInfo
{
	enum GroupStatus {
		    RS_GXS_NET_TUNNEL_GRP_STATUS_UNKNOWN            = 0x00,	// unknown status
		    RS_GXS_NET_TUNNEL_GRP_STATUS_IDLE               = 0x01,	// no virtual peers requested, just waiting
		    RS_GXS_NET_TUNNEL_GRP_STATUS_TUNNELS_REQUESTED  = 0x02,	// virtual peers requested, and waiting for turtle to answer
		    RS_GXS_NET_TUNNEL_GRP_STATUS_VPIDS_AVAILABLE    = 0x03	// some virtual peers are available. Data can be read/written
	};

	enum GroupPolicy {
		    RS_GXS_NET_TUNNEL_GRP_POLICY_UNKNOWN            = 0x00,	// nothing has been set
		    RS_GXS_NET_TUNNEL_GRP_POLICY_PASSIVE            = 0x01,	// group is available for server side tunnels, but does not explicitely request tunnels
		    RS_GXS_NET_TUNNEL_GRP_POLICY_ACTIVE             = 0x02,	// group explicitely request tunnels, if none available
    };

	RsGxsNetTunnelGroupInfo() : group_policy(RS_GXS_NET_TUNNEL_GRP_POLICY_PASSIVE),group_status(RS_GXS_NET_TUNNEL_GRP_STATUS_IDLE),last_contact(0),service_id(0) {}

	GroupPolicy    group_policy ;
	GroupStatus    group_status ;
	time_t         last_contact ;
	TurtleFileHash hash ;
	uint16_t       service_id ;

	std::set<RsGxsNetTunnelVirtualPeerId> virtual_peers ; // list of which virtual peers provide this group. Can me more than 1.
};

class RsGxsNetTunnelService: public RsTurtleClientService, public RsTickingThread
{
public:
	  RsGxsNetTunnelService() ;
	  virtual ~RsGxsNetTunnelService() ;

	  /*!
	   * \brief Manage tunnels for this group
	   *	@param group_id group for which tunnels should be released
	   */
      bool requestPeers(const RsGxsGroupId&group_id) ;

	  /*!
	   * \brief Stop managing tunnels for this group
	   *	@param group_id group for which tunnels should be released
	   */
      bool releasePeers(const RsGxsGroupId&group_id) ;

	  /*!
	   * \brief Get the list of active virtual peers for a given group. This implies that a tunnel is up and
	   *        alive. This function also "registers" the group which allows to handle tunnel requests in the server side.
	   */
      bool getVirtualPeers(uint16_t service_id, std::list<RsGxsNetTunnelVirtualPeerId>& peers) ; 					// returns the virtual peers for this group

	  /*!
	   * \brief sendData
	   *               send data to this virtual peer, and takes memory ownership (deletes the item)
	   * \param item           item to send
	   * \param virtual_peer   destination virtual peer
	   * \return
	   *               true if succeeded.
	   */
      bool sendData(unsigned char *& data, uint32_t data_len, const RsGxsNetTunnelVirtualPeerId& virtual_peer) ;

	  /*!
	   * \brief receivedItem
	   *                 returns the next received item from the given virtual peer.
	   * \param virtual_peer
	   * \return
	   */
      RsItem *receivedItem(const RsGxsNetTunnelVirtualPeerId& virtual_peer) ;

	  /*!
	   * \brief dumps all information about monitored groups.
	   */
	  void dump() const;

	  // other methods are still missing.
	  //  - derived from p3Config, to load/save data
	  //  - method to respond to tunnel requests, probably using RsGxsNetService
	  //  - method to encrypt/decrypt data and send/receive to/from turtle.

	  virtual void connectToTurtleRouter(p3turtle *tr) ;

	  // Overloaded from RsTickingThread

	  void data_tick() ;

protected:
	  // interaction with turtle router

	  virtual bool handleTunnelRequest(const RsFileHash &hash,const RsPeerId& peer_id) ;
	  virtual void receiveTurtleData(RsTurtleGenericTunnelItem *item,const RsFileHash& hash,const RsPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction direction) ;
	  void addVirtualPeer(const TurtleFileHash&, const TurtleVirtualPeerId&,RsTurtleGenericTunnelItem::Direction dir) ;
	  void removeVirtualPeer(const TurtleFileHash&, const TurtleVirtualPeerId&) ;

	  p3turtle 	*mTurtle ;
private:
	  void autowash() ;
	  void handleIncoming(RsGxsNetTunnelItem *item) ;

	  static const uint32_t RS_GXS_TUNNEL_CONST_RANDOM_BIAS_SIZE = 20 ;
	  static const uint32_t RS_GXS_TUNNEL_CONST_EKEY_SIZE        = 32 ;

	  std::map<RsGxsGroupId,RsGxsNetTunnelGroupInfo> mGroups ;	// groups on the client and server side

	  std::map<RsGxsNetTunnelVirtualPeerId, RsGxsNetTunnelVirtualPeerInfo> mVirtualPeers ;	// current virtual peers, which group they provide, and how to talk to them through turtle
	  std::map<RsFileHash, RsGxsGroupId>                                   mHandledHashes ; // hashes asked to turtle. Used to answer tunnel requests
	  std::map<TurtleVirtualPeerId, RsGxsNetTunnelVirtualPeerId>           mTurtle2GxsPeer ; // convertion table to find GXS peer id from turtle

	  /*!
	   * \brief Generates the hash to request tunnels for this group. This hash is only used by turtle, and is used to
	   * 		hide the real group id.
	   */

	  RsFileHash calculateGroupHash(const RsGxsGroupId&group_id) const ;

	  /*!
	   * \brief makeVirtualPeerIdForGroup creates a virtual peer id that can be used and that will be constant accross time, whatever the
	   * 		tunnel ID and turtle virtual peer id. This allows RsGxsNetService to keep sync-ing the data consistently.
	   */

	  RsGxsNetTunnelVirtualPeerId locked_makeVirtualPeerId() const ;

	  static void generateEncryptionKey(const RsGxsGroupId& group_id,const TurtleVirtualPeerId& vpid,unsigned char key[RS_GXS_TUNNEL_CONST_EKEY_SIZE]) ;

	  uint8_t mRandomBias[RS_GXS_TUNNEL_CONST_RANDOM_BIAS_SIZE] ; // constant accross reboots. Allows to disguise the real SSL id while providing a consistent value accross time.

	  mutable RsMutex mGxsNetTunnelMtx;
};

