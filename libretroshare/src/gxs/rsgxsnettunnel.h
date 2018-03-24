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
//      * tunnels are only used one-way. If a distant peers wants to sync the same group, he'll have to open his own tunnel, with a different ID.
//      * each group will produced multiple tunnels, but each tunnel with have exactly one virtual peer ID

typedef RsPeerId RsGxsNetTunnelVirtualPeerId ;

class RsGxsNetTunnelItem ;

struct RsGxsNetTunnelVirtualPeerInfo
{
	enum {   RS_GXS_NET_TUNNEL_VP_STATUS_UNKNOWN   = 0x00,		// unknown status.
		     RS_GXS_NET_TUNNEL_VP_STATUS_TUNNEL_OK = 0x01,		// tunnel has been established and we're waiting for virtual peer id
		     RS_GXS_NET_TUNNEL_VP_STATUS_ACTIVE    = 0x02		// virtual peer id is known. Data can transfer.
	     };

	RsGxsNetTunnelVirtualPeerInfo() : vpid_status(RS_GXS_NET_TUNNEL_VP_STATUS_UNKNOWN) { memset(encryption_master_key,0,16) ; }

	uint8_t vpid_status ;					// status of the peer
	uint8_t side ;	                        // client/server
	uint8_t encryption_master_key[32] ;		// key from which the encryption key is derived for each virtual peer (using H(master_key | random IV))
	time_t  last_contact ;					// last time some data was sent/recvd

	RsGxsNetTunnelVirtualPeerId net_service_virtual_peer ;  // anonymised peer that is used to communicate with client services
	RsGxsGroupId                group_id ;		            // group id

	std::list<RsItem*> incoming_items ;
	std::list<RsItem*> outgoing_items ;
};

struct RsGxsNetTunnelGroupInfo
{
	enum {	RS_GXS_NET_TUNNEL_GRP_STATUS_UNKNOWN            = 0x00,	// unknown status
		    RS_GXS_NET_TUNNEL_GRP_STATUS_TUNNELS_REQUESTED  = 0x01,	// waiting for turtle to send some virtual peers.
		    RS_GXS_NET_TUNNEL_GRP_STATUS_VPIDS_AVAILABLE    = 0x02	// some virtual peers are available
	};

	RsGxsNetTunnelGroupInfo() : group_status(RS_GXS_NET_TUNNEL_GRP_STATUS_UNKNOWN),last_contact(0) {}

	uint8_t        group_status ;
	time_t         last_contact ;
	TurtleFileHash hash ;

	std::map<TurtleVirtualPeerId, RsGxsNetTunnelVirtualPeerInfo> virtual_peers ;
};

class RsGxsNetTunnelService: public RsTurtleClientService, public RsTickingThread
{
public:
	  RsGxsNetTunnelService() ;

	  /*!
	   * \brief start managing tunnels for this group
	   *	@param group_id group for which tunnels should be requested
	   */
      bool manage(const RsGxsGroupId& group_id) ;

	  /*!
	   * \brief Stop managing tunnels for this group
	   *	@param group_id group for which tunnels should be released
	   */
      bool release(const RsGxsGroupId&group_id) ;


	  /*!
	   * \brief sendItem
	   *               send data to this virtual peer, and takes memory ownership (deletes the item)
	   * \param item           item to send
	   * \param virtual_peer   destination virtual peer
	   * \return
	   *               true if succeeded.
	   */
      bool sendItem(RsItem *& item, const RsGxsNetTunnelVirtualPeerId& virtual_peer) ;

	  /*!
	   * \brief receivedItem
	   *                 returns the next received item from the given virtual peer.
	   * \param virtual_peer
	   * \return
	   */
      RsItem *receivedItem(const RsGxsNetTunnelVirtualPeerId& virtual_peer) ;

	  /*!
	   * \brief Get the list of active virtual peers for a given group. This implies that the tunnel is up and
	   * alive.
	   */
      bool getVirtualPeers(const RsGxsGroupId&, std::list<RsPeerId>& peers) ; 					// returns the virtual peers for this group

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

	  static const uint32_t RS_GXS_TUNNEL_CONST_RANDOM_BIAS_SIZE = 16 ;
	  static const uint32_t RS_GXS_TUNNEL_CONST_EKEY_SIZE        = 32 ;

	  std::map<RsGxsGroupId,RsGxsNetTunnelGroupInfo> mGroups ;	// groups on the client and server side

	  std::map<RsGxsNetTunnelVirtualPeerId, std::pair<RsGxsGroupId,TurtleVirtualPeerId> > mVirtualPeers ;	// current virtual peers,
	  std::map<RsFileHash, RsGxsGroupId>                                   mHandledHashes ; // hashes asked to turtle

	  /*!
	   * \brief Generates the hash to request tunnels for this group. This hash is only used by turtle, and is used to
	   * 		hide the real group id.
	   */

	  RsFileHash calculateGroupHash(const RsGxsGroupId&group_id) const ;

	  /*!
	   * \brief makeVirtualPeerIdForGroup creates a virtual peer id that can be used and that will be constant accross time, whatever the
	   * 		tunnel ID and turtle virtual peer id. This allows RsGxsNetService to keep sync-ing the data consistently.
	   */

	  RsGxsNetTunnelVirtualPeerId makeServerVirtualPeerIdForGroup(const RsGxsGroupId&group_id) const ;

	  void generateEncryptionKey(const RsGxsGroupId& group_id,const TurtleVirtualPeerId& vpid,unsigned char key[RS_GXS_TUNNEL_CONST_EKEY_SIZE]) const ;

	  uint8_t mRandomBias[RS_GXS_TUNNEL_CONST_RANDOM_BIAS_SIZE] ; // constant accross reboots. Allows to disguise the real SSL id while providing a consistent value accross time.

	  mutable RsMutex mGxsNetTunnelMtx;
};

