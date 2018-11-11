/*******************************************************************************
 * libretroshare/src/grouter: grouterclientservice.h                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2013 by Cyril Soler <csoler@users.sourceforge.net>                *
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

// This class is the parent class for any service that will use the global router to distribute its packets.
// Typical representative clients include:
//
// 	p3msgservice:		sends distant messages and advertise messaging keys
//
#pragma once

#include <string>
#include <stdlib.h>
#include <grouter/grouteritems.h>

struct RsItem;

static const uint32_t GROUTER_CLIENT_SERVICE_DATA_STATUS_UNKNOWN  = 0x0000 ;	// unused.
static const uint32_t GROUTER_CLIENT_SERVICE_DATA_STATUS_RECEIVED = 0x0001 ;	// sent when data has been received and a receipt is available.
static const uint32_t GROUTER_CLIENT_SERVICE_DATA_STATUS_FAILED   = 0x0002 ;	// sent if the global router cannot send after a while

class GRouterClientService
{
public:
    // This method is called by the turtle router to send data that comes out of a turtle tunnel.
    // The turtle router stays responsible for the memory management of data. Most of the  time the
    // data chunk is a serialized item to be de-serialized by the client service.
    //
    // Parameters:
    // 		item            : global router item. Handled by the client service.
    // 		destination_key : key that is associated with this item. Can be useful for the client.
    //
    // GRouter stays owner of the item, so the client should not delete it!
    //
    virtual void receiveGRouterData(const RsGxsId& destination_key,const RsGxsId& /*signing_key*/, GRouterServiceId &/*client_id*/, uint8_t */*data*/, uint32_t /*data_size*/)
    { 
	    std::cerr << "!!!!!! Received Data from global router, but the client service is not handling it !!!!!!!!!!" << std::endl ; 
	    std::cerr << "   destination key_id = " << destination_key.toStdString() << std::endl;
    }

    // This method is called by the global router when a message has been received, or cannot be sent, etc.
    //
    virtual void notifyDataStatus(const GRouterMsgPropagationId& received_id,const RsGxsId& signer_id,uint32_t data_status)
    {
	    std::cerr << "!!!!!! Received Data status from global router, but the client service is not handling it !!!!!!!!!!" << std::endl ;
	    std::cerr << "   message ID  = " << received_id << std::endl;
	    std::cerr << "   data status = " << data_status << std::endl;
	    std::cerr << "   signer ID   = " << signer_id << std::endl;
    }

    // This function is mandatory. It should do two things:
    // 	1 - keep a pointer to the global router, so as to be able to send data (e.g. copy pt into a local variable)
    // 	2 - call pt->registerTunnelService(this), so that the TR knows that service and can send back information to it.
    //
    virtual void connectToGlobalRouter(p3GRouter *pt) = 0 ;

    // should be derived to determine wether the client accepts data from this peer or not. If not, the data is dropped.

    virtual bool acceptDataFromPeer(const RsGxsId& gxs_id) =0;
};


