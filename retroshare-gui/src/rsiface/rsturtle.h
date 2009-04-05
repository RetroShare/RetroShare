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

#include <inttypes.h>
#include <string>
#include <list>

class RsTurtle;
extern RsTurtle   *rsTurtle ;

typedef uint32_t TurtleRequestId ;

// This is the structure used to send back results of the turtle search 
// to the notifyBase class.

struct TurtleFileInfo
{
	std::string hash ;
	std::string name ;
	uint64_t size ;
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
		RsTurtle() { _sharing_strategy = SHARE_ENTIRE_NETWORK ;}
		virtual ~RsTurtle() {}

		enum FileSharingStrategy { SHARE_ENTIRE_NETWORK, SHARE_FRIENDS_ONLY } ;

		// Lauches a search request through the pipes, and immediately returns
		// the request id, which will be further used by the gui to store results
		// as they come back.
		//
		virtual TurtleRequestId turtleSearch(const std::string& match_string) = 0 ;

		// Launches a complete download file operation: diggs one or more
		// tunnels.  Launches an exception if an error occurs during the
		// initialization process.
		//
		virtual void turtleDownload(const std::string& file_hash) = 0 ;

		// Sets the file sharing strategy. It concerns all local files. It would
		// be better to handle this for each file, of course.

		void setFileSharingStrategy(FileSharingStrategy f) { _sharing_strategy = f ; }

	protected:
		FileSharingStrategy _sharing_strategy ;
};

#endif
