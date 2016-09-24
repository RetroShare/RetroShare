#ifndef RETROSHARE_TURTLE_GUI_INTERFACE_H
#define RETROSHARE_TURTLE_GUI_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsturtle.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2009 by Cyril Soler.
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
 * Please report all bugs and problems to "csoler@users.sourceforge.net"
 *
 */

#pragma once

#include <inttypes.h>
#include <string>
#include <list>
#include <vector>

#include "retroshare/rstypes.h"

namespace RsRegularExpression { class LinearizedExpression ; }
class RsTurtleClientService ;

class RsTurtle;
extern RsTurtle   *rsTurtle ;

typedef uint32_t TurtleRequestId ;

// This is the structure used to send back results of the turtle search 
// to the notifyBase class, or send info to the GUI.

struct TurtleFileInfo
{
	RsFileHash hash ;
	std::string name ;
	uint64_t size ;
};

struct TurtleRequestDisplayInfo
{
	uint32_t request_id ;			// Id of the request
	RsPeerId source_peer_id ;	// Peer that relayed the request
	uint32_t age ;						// Age in seconds
	uint32_t depth ;					// Depth of the request. Might be altered.
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

		// Lauches a search request through the pipes, and immediately returns
		// the request id, which will be further used by the gui to store results
		// as they come back.
		//
		virtual TurtleRequestId turtleSearch(const std::string& match_string) = 0 ;
        virtual TurtleRequestId turtleSearch(const RsRegularExpression::LinearizedExpression& expr) = 0 ;

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

		// Get info from the turtle router. I use std strings to hide the internal structs.
		//
		virtual void getInfo(std::vector<std::vector<std::string> >&,std::vector<std::vector<std::string> >&,
									std::vector<TurtleRequestDisplayInfo>&,std::vector<TurtleRequestDisplayInfo>&) const = 0;

		// Get info about turtle traffic. See TurtleTrafficStatisticsInfo members for details.
		//
		virtual void getTrafficStatistics(TurtleTrafficStatisticsInfo& info) const = 0;

		// Convenience function.
		virtual bool isTurtlePeer(const RsPeerId& peer_id) const = 0 ;

		// Hardcore handles
		virtual void setMaxTRForwardRate(int max_tr_up_rate) = 0 ;
		virtual int getMaxTRForwardRate() const = 0 ;
};

#endif
