/*
 * libretroshare/src/gxs: rsgxsnettunnel.h
 *
 * General Data service, interface for RetroShare.
 *
 * Copyright 2011-2011 by Robert Fernie, Evi-Parker Christopher
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
 * 			a particular GXS group. For each group, a set of virtual peers, corresponding to active tunnels will be made available
 * 			to RsGxsNetService.
 *
 * 		  It is the responsibility of RsGxsNetService to activate/desactivate tunnels for each particular group, depending on wether the group
 * 		  is already available at friends or not.
 */

//   Proposed protocol:
//      * request tunnels based on H(GroupId)
//      * encrypt tunnel data using chacha20+HMAC-SHA256 using AEAD( GroupId, 96bits IV, tunnel ID ) (similar to what FT does)
//      * when tunnel is established, exchange virtual peer names: vpid = H( GroupID | Random bias )
//      * when vpid is known, notify the client (GXS net service) which can use the virtual peer to sync
//
//      * only use a single tunnel per virtual peer ID
//
//              Client  ------------------ TR(H(GroupId)) -------------->  Server
//
//              Client  <-------------------- T OK ----------------------  Server
//
//                               Here, a turtle vpid is known
//
//           [Encrypted traffic using H(GroupId, 96bits IV, tunnel ID)]
//
//              Client  <--------- VPID = H( Random IV | GroupId ) ------  Server
//                 |                                                         |
//                 +--------------> Mark the virtual peer active <-----------+
//
//                         Here, a consistent virtual peer ID is known

typedef RsPeerId RsGxsNetTunnelVirtualPeerId ;

struct RsGxsNetTunnelVirtualPeerInfo
{
	enum {   RS_GXS_NET_TUNNEL_VP_STATUS_UNKNOWN   = 0x00,		// unknown status.
		     RS_GXS_NET_TUNNEL_VP_STATUS_TUNNEL_OK = 0x01,		// tunnel has been established and we're waiting for virtual peer id
		     RS_GXS_NET_TUNNEL_VP_STATUS_ACTIVE    = 0x02		// virtual peer id is known. Data can transfer.
	     };

	uint8_t vpid_status ;
	RsGxsNetTunnelVirtualPeerId net_service_virtual_peer ;
	uint8_t side ;	// client/server
};

struct RsGxsNetTunnelInfo
{
	enum {	RS_GXS_NET_TUNNEL_GRP_STATUS_UNKNOWN            = 0x00,	// unknown status
		    RS_GXS_NET_TUNNEL_GRP_STATUS_TUNNELS_REQUESTED  = 0x01,	// waiting for turtle to send some virtual peers.
		    RS_GXS_NET_TUNNEL_GRP_STATUS_VPIDS_AVAILABLE    = 0x02	// some virtual peers are available
	};

	uint8_t group_status ;
	uint8_t encryption_master_key[16] ;		// key from which the encryption key is derived for each virtual peer (using H(master_key | random IV))

	std::map<TurtleVirtualPeerId, RsGxsNetTunnelVirtualPeerInfo> virtual_peers ;
};

class RsGxsNetTunnelService
{
public:
	  RsGxsNetTunnelService() {}

	  /*!
	   * \brief start managing tunnels for this group
	   *	@param group_id group for which tunnels should be requested
	   */
      bool manageTunnels(const RsGxsGroupId&) ;													

	  /*!
	   * \brief Stop managing tunnels for this group
	   *	@param group_id group for which tunnels should be released
	   */
      bool releaseTunnels(const RsGxsGroupId&) ;													

	  /*!
	   * sends data to this virtual peer ID 
	   */
      bool sendData(const unsigned char *data,uint32_t size,const RsGxsNetTunnelVirtualPeerId& virtual_peer) ;		// send data to this virtual peer

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

private:
	  std::map<RsGxsGroupId,RsGxsNetTunnelInfo> mClientGroups ;	// groups on the client side
	  std::map<RsGxsGroupId,RsGxsNetTunnelInfo> mServerGroups ;	// groups on the server side

	  std::map<RsGxsNetTunnelVirtualPeerId, std::pair<RsGxsGroupId,TurtleVirtualPeerId> > mVirtualPeers ;

	  /*!
	   * \brief Generates the hash to request tunnels for this group. This hash is only used by turtle, and is used to
	   * 		hide the real group id.
	   */

	  RsFileHash makeRequestHash(const RsGxsGroupId&) const ;

	  /*!
	   * \brief makeVirtualPeerIdForGroup creates a virtual peer id that can be used and that will be constant accross time, whatever the
	   * 		tunnel ID and turtle virtual peer id. This allows RsGxsNetService to keep sync-ing the data consistently.
	   */

	  RsGxsNetTunnelVirtualPeerInfo makeVirtualPeerIdForGroup(const RsGxsGroupId&) const ;

	  uint8_t mRandomBias[16] ; // constant accross reboots. Allows to disguise the real SSL id while providing a consistent value accross time.
};

