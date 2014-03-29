/*
 * libretroshare/src/services: rsgrouteritems.h
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

#pragma once

#include "serialiser/rsserial.h"
#include "serialiser/rsserviceids.h"
#include "retroshare/rstypes.h"

#include "retroshare/rsgrouter.h"
#include "p3grouter.h"

const uint8_t RS_PKT_SUBTYPE_GROUTER_PUBLISH_KEY	= 0x01 ;			// used to publish a key
const uint8_t RS_PKT_SUBTYPE_GROUTER_ACK           = 0x03 ;			// acknowledgement of data received
const uint8_t RS_PKT_SUBTYPE_GROUTER_DATA          = 0x05 ;			// used to send data to a destination

const uint8_t RS_PKT_SUBTYPE_GROUTER_MATRIX_CLUES  = 0x80 ;			// item to save matrix clues
const uint8_t RS_PKT_SUBTYPE_GROUTER_FRIENDS_LIST  = 0x82 ;			// item to save friend lists
const uint8_t RS_PKT_SUBTYPE_GROUTER_ROUTING_INFO  = 0x85 ;			// item to save routing info

const uint8_t QOS_PRIORITY_RS_GROUTER_PUBLISH_KEY = 3 ;				// slow items. No need to congest the network with this.
const uint8_t QOS_PRIORITY_RS_GROUTER_ACK         = 3 ;
const uint8_t QOS_PRIORITY_RS_GROUTER_DATA        = 3 ;

const uint32_t RS_GROUTER_ACK_STATE_RECEIVED            = 0x0001 ;		// data was received, directly
const uint32_t RS_GROUTER_ACK_STATE_RECEIVED_INDIRECTLY = 0x0002 ;		// data was received indirectly
const uint32_t RS_GROUTER_ACK_STATE_GIVEN_UP            = 0x0003 ;		// data was given up. No route.
const uint32_t RS_GROUTER_ACK_STATE_NO_ROUTE            = 0x0004 ;		// data was given up. No route.
const uint32_t RS_GROUTER_ACK_STATE_UNKNOWN             = 0x0005 ;		// unknown destination key
const uint32_t RS_GROUTER_ACK_STATE_TOO_FAR             = 0x0006 ;		// dropped because of distance

/***********************************************************************************/
/*                           Basic GRouter Item Class                              */
/***********************************************************************************/

class RsGRouterItem: public RsItem
{
	public:
		RsGRouterItem(uint8_t grouter_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_GROUTER,grouter_subtype) {}

		virtual bool serialise(void *data,uint32_t& size) const = 0 ;	
		virtual uint32_t serial_size() const = 0 ; 						

		virtual void clear() = 0 ;
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) = 0;

	protected:
		bool serialise_header(void *data, uint32_t& pktsize, uint32_t& tlvsize, uint32_t& offset) const;
};

/***********************************************************************************/
/*                                Helper base classes                              */
/***********************************************************************************/

class RsGRouterNonCopyableObject
{
	public:
		RsGRouterNonCopyableObject() {}
	private:
		RsGRouterNonCopyableObject(const RsGRouterNonCopyableObject&) {}
		RsGRouterNonCopyableObject operator=(const RsGRouterNonCopyableObject&) { return *this ;}
};

class RsGRouterProofOfWorkObject
{
	public:
		RsGRouterProofOfWorkObject() {}

		virtual bool serialise(void *data,uint32_t& size) const =0;	
		virtual uint32_t serial_size() const =0;	

		virtual bool checkProofOfWork() ;		// checks that the serialized object hashes down to a hash beginning with LEADING_BYTES_SIZE zeroes
		virtual bool updateProofOfWork() ;		// computes the pow_bytes so that the hash starts with LEADING_BYTES_SIZE zeroes.

		static bool checkProofOfWork(unsigned char *mem,uint32_t size) ;

		static const int POW_PAYLOAD_SIZE = 8 ;
		static const int PROOF_OF_WORK_REQUESTED_BYTES = 4 ;

		unsigned char pow_bytes[POW_PAYLOAD_SIZE] ;  // 8 bytes to put at the beginning of the serialized packet, so that 
												// the hash starts with a fixed number of zeroes.
};

/***********************************************************************************/
/*                                Specific packets                                 */
/***********************************************************************************/

class RsGRouterPublishKeyItem: public RsGRouterItem, public RsGRouterProofOfWorkObject
{
	public:
		RsGRouterPublishKeyItem() : RsGRouterItem(RS_PKT_SUBTYPE_GROUTER_PUBLISH_KEY) { setPriorityLevel(QOS_PRIORITY_RS_GROUTER_PUBLISH_KEY) ; }

		virtual bool serialise(void *data,uint32_t& size) const ;	
		virtual uint32_t serial_size() const ; 						

		virtual void clear() {} 
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) ;

		// packet data
		//
		GRouterKeyPropagationId diffusion_id ;
		GRouterKeyId published_key ;
		uint32_t service_id ;
		float randomized_distance ;
		std::string  description_string ;
		PGPFingerprintType fingerprint ;

};

class RsGRouterGenericDataItem: public RsGRouterItem, public RsGRouterNonCopyableObject
{
	public:
		RsGRouterGenericDataItem() : RsGRouterItem(RS_PKT_SUBTYPE_GROUTER_DATA) { setPriorityLevel(QOS_PRIORITY_RS_GROUTER_DATA) ; }
		virtual ~RsGRouterGenericDataItem() { clear() ; }

		virtual bool serialise(void *data,uint32_t& size) const ;	
		virtual uint32_t serial_size() const ; 						

		virtual void clear() 
		{
			free(data_bytes); 
			data_bytes=NULL;
		} 
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) ;

		RsGRouterGenericDataItem *duplicate() const ;

		// packet data
		//
		GRouterMsgPropagationId routing_id ;
		GRouterKeyId destination_key ;
		uint32_t randomized_distance ;

		uint32_t data_size ;
		uint8_t *data_bytes;
};

class RsGRouterACKItem: public RsGRouterItem
{
	public:
		RsGRouterACKItem() : RsGRouterItem(RS_PKT_SUBTYPE_GROUTER_ACK) { setPriorityLevel(QOS_PRIORITY_RS_GROUTER_ACK) ; }

		virtual bool serialise(void *data,uint32_t& size) const ;	
		virtual uint32_t serial_size() const ; 						

		virtual void clear() {} 
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) ;

		// packet data
		//
		GRouterMsgPropagationId mid ; 		// message id to which this ack is a response
		uint32_t state ; 							// packet was delivered, not delivered, bounced, etc
};

// Items for saving the routing matrix information.

class RsGRouterMatrixCluesItem: public RsGRouterItem
{
	public:
		RsGRouterMatrixCluesItem() : RsGRouterItem(RS_PKT_SUBTYPE_GROUTER_MATRIX_CLUES) 
		{ setPriorityLevel(0) ; }	// this item is never sent through the network

		virtual bool serialise(void *data,uint32_t& size) const ;	
		virtual uint32_t serial_size() const ; 						

		virtual void clear() {} 
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) ;

		// packet data
		//
		GRouterKeyId destination_key ;
		std::list<RoutingMatrixHitEntry> clues ;
};

class RsGRouterMatrixFriendListItem: public RsGRouterItem
{
	public:
		RsGRouterMatrixFriendListItem() : RsGRouterItem(RS_PKT_SUBTYPE_GROUTER_FRIENDS_LIST) 
		{ setPriorityLevel(0) ; }	// this item is never sent through the network

		virtual bool serialise(void *data,uint32_t& size) const ;	
		virtual uint32_t serial_size() const ; 						

		virtual void clear() {} 
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) ;

		// packet data
		//
		std::vector<RsPeerId> reverse_friend_indices ;
};

class RsGRouterRoutingInfoItem: public RsGRouterItem, public GRouterRoutingInfo, public RsGRouterNonCopyableObject
{
	public:
		RsGRouterRoutingInfoItem() : RsGRouterItem(RS_PKT_SUBTYPE_GROUTER_ROUTING_INFO) 
		{ setPriorityLevel(0) ; }	// this item is never sent through the network

		virtual ~RsGRouterRoutingInfoItem() { clear() ; }
		
		virtual bool serialise(void *data,uint32_t& size) const ;	
		virtual uint32_t serial_size() const ; 						

		virtual void clear() 
		{
			if(data_item != NULL)
				delete data_item ;
			data_item = NULL ;
			tried_friends.clear() ;
		}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) ;
};

/***********************************************************************************/
/*                                Serialisation                                    */
/***********************************************************************************/

class RsGRouterSerialiser: public RsSerialType
{
	public:
		RsGRouterSerialiser() : RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_GROUTER) {}

		virtual uint32_t 	size (RsItem *item) 
		{ 
			return dynamic_cast<RsGRouterItem *>(item)->serial_size() ;
		}
		virtual bool serialise(RsItem *item, void *data, uint32_t *size) 
		{ 
			return dynamic_cast<RsGRouterItem *>(item)->serialise(data,*size) ;
		}
		virtual RsItem *deserialise (void *data, uint32_t *size) ;

	private:
		RsGRouterPublishKeyItem       *deserialise_RsGRouterPublishKeyItem(void *data,uint32_t size) const ;
		RsGRouterGenericDataItem      *deserialise_RsGRouterGenericDataItem(void *data,uint32_t size) const ;
		RsGRouterACKItem              *deserialise_RsGRouterACKItem(void *data,uint32_t size) const ;
		RsGRouterMatrixCluesItem      *deserialise_RsGRouterMatrixCluesItem(void *data,uint32_t size) const ;
		RsGRouterMatrixFriendListItem *deserialise_RsGRouterMatrixFriendListItem(void *data,uint32_t size) const ;
		RsGRouterRoutingInfoItem      *deserialise_RsGRouterRoutingInfoItem(void *data,uint32_t size) const ;
};


