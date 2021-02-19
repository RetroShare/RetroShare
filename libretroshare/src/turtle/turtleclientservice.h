/*******************************************************************************
 * libretroshare/src/turtle: rsturtleclientservice.h                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2013-2018 by Cyril Soler <csoler@users.sourceforge.net>           *
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

#include <string>
#include <stdlib.h>

#include "serialiser/rsserial.h"
#include "turtle/rsturtleitem.h"
#include "util/rsdebug.h"

struct RsItem;
class p3turtle ;

/** This class is the parent class for any service that will use the turtle
 * router to distribute its packets.
 * Typical representative clients include:
 * 	p3ChatService: opens tunnels to distant peers for chatting
 * 	ftServer:      searches and open tunnels to distant sources for file
 * 	               transfer
 */
class RsTurtleClientService
{
	public:
    	/*!
         * \brief serviceId
         * 			Returns the ID of the client service. This is used to pass the ID to search requests, from the client services
         * \return
         * 			The service ID.
         */

    	virtual uint16_t serviceId() const
		{
			std::cerr << "!!!!!! Received request for service ID in turtle router client, but the client service is not handling it !!!!!!!" << std::endl ;
            return 0 ;
    	}

		/*!
		 * \brief handleTunnelRequest
		              Handling of tunnel request for the given hash. To be derived by the service in order to tell the turtle router
                      whether the service handles this hash or not. Most of the time, it's a search in a predefined list.

		 * \return true if the service
		 */
		virtual bool handleTunnelRequest(const RsFileHash& /*hash*/,const RsPeerId& /*peer_id*/) { return false ; }
		
	    /*!
		 * \brief receiveTurtleData
		 *           This method is called by the turtle router to send data that comes out of a turtle tunnel, and should
		 * 			 be overloaded by the client service.
		 *           The turtle router stays responsible for the memory management of data. Most of the  time the
		 *           data chunk is a serialized item to be de-serialized by the client service.
		 *
		 *           Parameters:
		 *           		virtual_peer_id	: name of the tunnel that sent the data
		 *           		data					: memory chunk for the data
		 *           		size					: size of data
		 *           		item->direction	: direction of travel:
		 *           										RsTurtleGenericTunnelItem::DIRECTION_CLIENT: the service is acting as a client
		 *           										RsTurtleGenericTunnelItem::DIRECTION_CLIENT: the service is acting as a server
		 *
		 *           			Most of the time this parameter is not used by services, except when some info (such as chunk maps, chat items, etc) go
		 *           			both ways, and their nature cannot suffice to determine where they should be handled.
		 *
		 *           By default (if not overloaded), the method will just free the data, as any subclass should do as well.
		 *           Note: p3turtle stays owner of the item, so the client should not delete it!
		 */
        virtual void receiveTurtleData(const RsTurtleGenericTunnelItem * /* item */,const RsFileHash& /*hash*/,const RsPeerId& /*virtual_peer_id*/,RsTurtleGenericTunnelItem::Direction /*direction*/)
		{ 
			std::cerr << "!!!!!! Received Data from turtle router, but the client service is not handling it !!!!!!!!!!" << std::endl ; 
		}

	/*!
	* This method is called by the turtle router to notify the client of a
	* search request in the form generic data.
	* The returned result contains the serialised generic result returned by the
	* client service.
	* The turtle router keeps the memory ownership over search_request_data
	* \param search_request_data generic serialized search data
	* \param search_request_data_len  length of the serialized search data
	* \param search_result_data generic serialized search result data
	* \param search_result_data_len length of the serialized search result data
	* \param max_allowed_hits max number of hits allowed to be sent back and
	* forwarded
	* \return true if matching results are available, false otherwise.
	*/
	virtual bool receiveSearchRequest(
	        unsigned char *search_request_data, uint32_t search_request_data_len,
	        unsigned char *& search_result_data, uint32_t& search_result_data_len,
	        uint32_t& max_allows_hits )
	{
		/* Suppress unused warning this way and not commenting the param names
		 * so doxygen match documentation against params */
		(void) search_request_data; (void) search_request_data_len;
		(void) search_result_data; (void) search_result_data_len;
		(void) max_allows_hits;

		RS_WARN( "Received search request from turtle router, but the client "
		         "is not handling it!" );
		return false;
	}

    	/*!
         * \brief receiveSearchResult
         * 			This method is called by the turtle router to notify the client of a search result. The result is serialized for the current class to read.
         *
         * \param search_result_data  result data. Memory ownership is owned by the turtle router. So do not delete!
         * \param search_result_data  length of result data
         */
    	virtual void receiveSearchResult(TurtleSearchRequestId /* request_id */,unsigned char * /*search_result_data*/,uint32_t  /*search_result_data_len*/)
		{
			std::cerr << "!!!!!! Received search result from turtle router, but the client service who requested it is not handling it !!!!!!!!!!" << std::endl ;
		}

	    /*!
		 * \brief serializer
		 *           Method for creating specific items of the client service. The
		 *           method has a default behavior of not doing anything, since most client
		 *           services might only use the generic item already provided by the turtle
		 *           router: RsTurtleGenericDataItem
		 *
		 * \return the client's serializer is returned
		 */
		virtual RsServiceSerializer *serializer() { return NULL ; }

		 /*!
		 * \brief addVirtualPeer
		 *           These methods are called by the turtle router to notify the client in order to add/remove virtual peers when tunnels are created/deleted
		 *           These methods must be overloaded, because a service which does not care about tunel being openned or closed is not supposed to need tunnels.
		 *
		 * \param hash                hash that the tunnel responds to
		 * \param virtual_peer_id     virtual peer id provided by turtle to allow the client to send data into this tunnel. This peer is related to the tunnel itself
		 *                              rather than to its destination. As such, multiple peer ids may actually send data to the same computer because multiple tunnels
		 *                              arrive at the same location.
		 * \param dir                 dir indicates which side the cient will be talking to: CLIENT means that the client is the server. SERVER means that the client acts
		 *                            as a client (and therefore actually requested the tunnel).
		 */
		virtual void addVirtualPeer(const TurtleFileHash& hash,const TurtleVirtualPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction dir) = 0 ;
		virtual void removeVirtualPeer(const TurtleFileHash& hash,const TurtleVirtualPeerId& virtual_peer_id) = 0 ;

	    /*!
		 * \brief connectToTurtleRouter
		 *           This function must be overloaded by the client. It should do two things:
		 *           	1 - keep a pointer to the turtle router, so as to be able to send data (e.g. store pt into a local variable)
		 *           	2 - call pt->registerTunnelService(this), so that the TR knows that service and can send back information to it.
		 *
		 * \param pt A pointer to the turtle router.
		 */
		virtual void connectToTurtleRouter(p3turtle *pt) = 0 ;
};


