/*
 * libretroshare/src/services: turtleclientservice.h
 *
 * Services for RetroShare.
 *
 * Copyright 2013 by Cyril Soler
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
 * Please report all bugs and problems to "csoler@users.sourceforge.net".
 *
 */

// This class is the parent class for any service that will use the turtle router to distribute its packets.
// Typical representative clients include:
//
// 	p3ChatService:		opens tunnels to distant peers for chatting
// 	ftServer:			searches and open tunnels to distant sources for file transfer
//
#pragma once

#include <string>
#include <stdlib.h>
#include <serialiser/rsserial.h>
#include <turtle/rsturtleitem.h>

class RsItem ;

class RsTurtleClientService
{
	public:
		// Handling of tunnel request for the given hash. Most of the time, it's a search in a predefined list.
		// The output info_string is used by the turtle router to display info about tunnels it manages. It is
		// not passed to the tunnel.

		virtual bool handleTunnelRequest(const std::string& hash,const std::string& peer_id) { return false ; }
		
		// This method is called by the turtle router to send data that comes out of a turtle tunnel.
		// The turtle router stays responsible for the memory management of data. Most of the  time the
		// data chunk is a serialized item to be de-serialized by the client service.
		//
		// Parameters:
		// 		virtual_peer_id	: name of the tunnel that sent the data
		// 		data					: memory chunk for the data
		// 		size					: size of data
		// 		item->direction	: direction of travel:
		// 										RsTurtleGenericTunnelItem::DIRECTION_CLIENT: the service is acting as a client 
		// 										RsTurtleGenericTunnelItem::DIRECTION_CLIENT: the service is acting as a server
		//
		// 			Most of the time this parameter is not used by services, except when some info (such as chunk maps, chat items, etc) go 
		// 			both ways, and their nature cannot suffice to determine where they should be handled.
		//
		// By default (if not overloaded), the method will just free the data, as any subclass should do as well.
		// Note: p3turtle stays owner of the item, so the client should not delete it!
		//
		virtual void receiveTurtleData(RsTurtleGenericTunnelItem *item,const std::string& hash,const std::string& virtual_peer_id,RsTurtleGenericTunnelItem::Direction direction) 
		{ 
			std::cerr << "!!!!!! Received Data from turtle router, but the client service is not handling it !!!!!!!!!!" << std::endl ; 
		}

		// Method for deserialising specific items of the client service. The
		// method has a default behavior of not doing anything, since most client
		// services might only use the generic item already provided by the turtle
		// router: RsTurtleGenericDataItem

		virtual RsTurtleGenericTunnelItem *deserialiseItem(void *data, uint32_t size) const { return NULL ; }

		// These methods are called by the turtle router to add/remove virtual peers when tunnels are created/deleted
		//
		virtual void addVirtualPeer(const TurtleFileHash& hash,const TurtleVirtualPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction dir) = 0 ;
		virtual void removeVirtualPeer(const TurtleFileHash& hash,const TurtleVirtualPeerId& virtual_peer_id) = 0 ;
};


