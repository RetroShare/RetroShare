/*******************************************************************************
 * libretroshare/src/retroshare: rsturtle.h                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2009-2018 by Cyril Soler <csoler@users.sourceforge.net>           *
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

#include <inttypes.h>
#include <string>
#include <list>
#include <vector>

#include "serialiser/rstlvbinary.h"
#include "retroshare/rstypes.h"
#include "retroshare/rsgxsifacetypes.h"
#include "serialiser/rsserializable.h"

namespace RsRegularExpression { class LinearizedExpression ; }
class RsTurtleClientService ;

class RsTurtle;

/**
 * Pointer to global instance of RsTurtle service implementation
 */
extern RsTurtle* rsTurtle;

typedef uint32_t TurtleRequestId ;
typedef RsPeerId TurtleVirtualPeerId;

struct TurtleFileInfo : RsSerializable
{
	TurtleFileInfo() : size(0) {}

	uint64_t size;    /// File size
	RsFileHash hash;  /// File hash
	std::string name; /// File name

	/// @see RsSerializable::serial_process
	void serial_process( RsGenericSerializer::SerializeJob j,
						 RsGenericSerializer::SerializeContext& ctx )
	{
		RS_SERIAL_PROCESS(size);
		RS_SERIAL_PROCESS(hash);

		// Use String TLV serial process for retrocompatibility
		RsTypeSerializer::serial_process(
		            j, ctx, TLV_TYPE_STR_NAME, name, "name" );
	}
} RS_DEPRECATED_FOR(TurtleFileInfoV2);

struct TurtleTunnelRequestDisplayInfo
{
	uint32_t request_id ;     // Id of the request
	RsPeerId source_peer_id ; // Peer that relayed the request
	uint32_t age ;            // Age in seconds
	uint32_t depth ;          // Depth of the request. Might be altered.
};
struct TurtleSearchRequestDisplayInfo
{
	uint32_t request_id ;     // Id of the request
	RsPeerId source_peer_id ; // Peer that relayed the request
	uint32_t age ;            // Age in seconds
	uint32_t depth ;          // Depth of the request. Might be altered.
	uint32_t hits ;
	std::string keywords;
};

class TurtleTrafficStatisticsInfo
{
	public:
		float unknown_updn_Bps ;	// unknown data transit bitrate (in Bytes per sec.)
		float data_up_Bps ;			// upload (in Bytes per sec.)
		float data_dn_Bps ;			// download (in Bytes per sec.)
		float tr_up_Bps ;				// tunnel requests upload bitrate (in Bytes per sec.)
		float tr_dn_Bps ;				// tunnel requests dnload bitrate (in Bytes per sec.)
		float total_up_Bps ;			// turtle network management bitrate (in Bytes per sec.)
		float total_dn_Bps ;			// turtle network management bitrate (in Bytes per sec.)

		std::vector<float> forward_probabilities ;	// probability to forward a TR as a function of depth.
};

// Interface class for turtle hopping.
//
//   This class mainly interacts with the turtle router, that is responsible
//   for routing turtle packets between peers, accepting/forwarding search
//   requests and dowloading files.
//
//   As seen from here, the interface is really simple.
//
class RsTurtle
{
public:
		RsTurtle() {}
		virtual ~RsTurtle() {}

		// This is saved permanently.
		virtual void setEnabled(bool) = 0 ;
		virtual bool enabled() const = 0 ;

		// This is temporary, used by Operating Mode.
		virtual void setSessionEnabled(bool) = 0 ;
		virtual bool sessionEnabled() const = 0 ;

		/** Lauches a search request through the pipes, and immediately returns
		 * the request id, which will be further used by client services to
		 * handle results as they come back. */
		virtual TurtleRequestId turtleSearch(
		        unsigned char *search_bin_data, uint32_t search_bin_data_len,
		        RsTurtleClientService* client_service ) = 0;

		// Initiates tunnel handling for the given file hash.  tunnels.  Launches
		// an exception if an error occurs during the initialization process. The
		// turtle router itself does not initiate downloads, it only maintains
		// tunnels for the given hash. The download should be driven by the file
        // transfer module by calling ftServer::FileRequest().
        // Aggressive mode causes the turtle router to regularly re-ask tunnels in addition to the ones already
        // available without replacing them. In non aggressive mode, we wait for all tunnels to die before asking
        // for new tunnels.
		//
        virtual void monitorTunnels(const RsFileHash& file_hash,RsTurtleClientService *client_service,bool use_aggressive_mode) = 0 ;

		// Tells the turtle router to stop handling tunnels for the given file hash. Traditionally this should
		// be called after calling ftServer::fileCancel().
		//
		virtual void stopMonitoringTunnels(const RsFileHash& file_hash) = 0 ;

		/// Adds a client tunnel service. This means that the service will be added 
		/// to the list of services that might respond to tunnel requests.
		/// Example tunnel services include:
		///
		///	p3ChatService:		tunnels correspond to private distant chatting
		///	ftServer		 : 	tunnels correspond to file data transfer
		///
		virtual void registerTunnelService(RsTurtleClientService *service) = 0;

		virtual std::string getPeerNameForVirtualPeerId(const RsPeerId& virtual_peer_id) = 0;
		
		// Get info from the turtle router. I use std strings to hide the internal structs.
		//
		virtual void getInfo(std::vector<std::vector<std::string> >&,std::vector<std::vector<std::string> >&,
									std::vector<TurtleSearchRequestDisplayInfo>&,std::vector<TurtleTunnelRequestDisplayInfo>&) const = 0;

		// Get info about turtle traffic. See TurtleTrafficStatisticsInfo members for details.
		//
		virtual void getTrafficStatistics(TurtleTrafficStatisticsInfo& info) const = 0;

		// Convenience function.
		virtual bool isTurtlePeer(const RsPeerId& peer_id) const = 0 ;

		// Hardcore handles
		virtual void setMaxTRForwardRate(int max_tr_up_rate) = 0 ;
		virtual int getMaxTRForwardRate() const = 0 ;
};
