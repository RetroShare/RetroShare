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

class LinearizedExpression ;

class RsTurtle;
extern RsTurtle   *rsTurtle ;

typedef uint32_t TurtleRequestId ;

// This is the structure used to send back results of the turtle search 
// to the notifyBase class, or send info to the GUI.

struct TurtleFileInfo
{
	std::string hash ;
	std::string name ;
	uint64_t size ;
};

struct TurtleRequestDisplayInfo
{
	uint32_t request_id ;			// Id of the request
	std::string source_peer_id ;	// Peer that relayed the request
	uint32_t age ;						// Age in seconds
	uint32_t depth ;					// Depth of the request. Might be altered.
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
		enum FileSharingStrategy { SHARE_ENTIRE_NETWORK, SHARE_FRIENDS_ONLY } ;

		RsTurtle() {}
		virtual ~RsTurtle() {}

		// Lauches a search request through the pipes, and immediately returns
		// the request id, which will be further used by the gui to store results
		// as they come back.
		//
		virtual TurtleRequestId turtleSearch(const std::string& match_string) = 0 ;
		virtual TurtleRequestId turtleSearch(const LinearizedExpression& expr) = 0 ;

		// Sets the file sharing strategy. It concerns all local files. It would
		// be better to handle this for each file, of course.

		void setFileSharingStrategy(FileSharingStrategy f) { _sharing_strategy = f ; }

		// Initiates tunnel handling for the given file hash.  tunnels.  Launches
		// an exception if an error occurs during the initialization process. The
		// turtle router itself does not initiate downloads, it only maintains
		// tunnels for the given hash. The download should be driven by the file
		// transfer module by calling ftServer::FileRequest().
		//
		virtual void monitorFileTunnels(const std::string& name,const std::string& file_hash,uint64_t size) = 0 ;

		// Tells the turtle router to stop handling tunnels for the given file hash. Traditionally this should
		// be called after calling ftServer::fileCancel().
		//
		virtual void stopMonitoringFileTunnels(const std::string& file_hash) = 0 ;

		// Get info from the turtle router. I use std strings to hide the internal structs.
		virtual void getInfo(std::vector<std::vector<std::string> >&,std::vector<std::vector<std::string> >&,
									std::vector<TurtleRequestDisplayInfo>&,std::vector<TurtleRequestDisplayInfo>&) const = 0;

		// Convenience function.
		virtual bool isTurtlePeer(const std::string& peer_id) const = 0 ;

	protected:
		FileSharingStrategy _sharing_strategy ;
};

#endif
