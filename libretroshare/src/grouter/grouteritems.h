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
#include "serialiser/rstlvkeys.h"
#include "serialiser/rsserviceids.h"
#include "retroshare/rstypes.h"

#include "retroshare/rsgrouter.h"
#include "groutermatrix.h"

const uint8_t RS_PKT_SUBTYPE_GROUTER_PUBLISH_KEY                 = 0x01 ;	// used to publish a key
const uint8_t RS_PKT_SUBTYPE_GROUTER_ACK_deprecated              = 0x03 ;	// don't use!
const uint8_t RS_PKT_SUBTYPE_GROUTER_SIGNED_RECEIPT_deprecated   = 0x04 ;	// don't use!
const uint8_t RS_PKT_SUBTYPE_GROUTER_DATA_deprecated             = 0x05 ;	// don't use!
const uint8_t RS_PKT_SUBTYPE_GROUTER_DATA_deprecated2            = 0x06 ;	// don't use!
const uint8_t RS_PKT_SUBTYPE_GROUTER_DATA                        = 0x07 ;	// used to send data to a destination (Signed by source)
const uint8_t RS_PKT_SUBTYPE_GROUTER_SIGNED_RECEIPT              = 0x08 ;	// long-distance acknowledgement of data received
const uint8_t RS_PKT_SUBTYPE_GROUTER_TRANSACTION_CHUNK           = 0x10 ;	// chunk of data. Used internally.
const uint8_t RS_PKT_SUBTYPE_GROUTER_TRANSACTION_ACKN            = 0x11 ;	// acknowledge for finished transaction. Not necessary, but increases fiability.
const uint8_t RS_PKT_SUBTYPE_GROUTER_MATRIX_CLUES                = 0x80 ;	// item to save matrix clues
const uint8_t RS_PKT_SUBTYPE_GROUTER_FRIENDS_LIST                = 0x82 ;	// item to save friend lists
const uint8_t RS_PKT_SUBTYPE_GROUTER_ROUTING_INFO                = 0x93 ;	//
const uint8_t RS_PKT_SUBTYPE_GROUTER_MATRIX_TRACK                = 0x94 ;	// item to save matrix track info

const uint8_t QOS_PRIORITY_RS_GROUTER = 4 ;			// relevant for items that travel through friends


/***********************************************************************************/
/*                           Basic GRouter Item Class                              */
/***********************************************************************************/

class RsGRouterItem: public RsItem
{
	public:
		RsGRouterItem(uint8_t grouter_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_GROUTER,grouter_subtype) {}

        virtual ~RsGRouterItem() {}

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
    protected:
		RsGRouterNonCopyableObject(const RsGRouterNonCopyableObject&) {}
		RsGRouterNonCopyableObject operator=(const RsGRouterNonCopyableObject&) { return *this ;}
};

/***********************************************************************************/
/*                                Specific packets                                 */
/***********************************************************************************/

// This abstract item class encapsulates 2 types of signed items. All have signature, destination key
// and routing ID. Sub-items are responsible for providing the serialised data to be signed for
// both signing and checking.

class RsGRouterAbstractMsgItem: public RsGRouterItem
{
public:
    RsGRouterAbstractMsgItem(uint8_t pkt_subtype) : RsGRouterItem(pkt_subtype) {}
    virtual ~RsGRouterAbstractMsgItem() {}

    virtual uint32_t signed_data_size() const = 0 ;
    virtual bool serialise_signed_data(void *data,uint32_t& size) const = 0 ;

    GRouterMsgPropagationId routing_id ;
    GRouterKeyId destination_key ;
    GRouterServiceId service_id ;
    RsTlvKeySignature signature ;		// signs mid+destination_key+state
        uint32_t flags ; 			// packet was delivered, not delivered, bounced, etc
};

class RsGRouterGenericDataItem: public RsGRouterAbstractMsgItem, public RsGRouterNonCopyableObject
{
    public:
        RsGRouterGenericDataItem() : RsGRouterAbstractMsgItem(RS_PKT_SUBTYPE_GROUTER_DATA) { setPriorityLevel(QOS_PRIORITY_RS_GROUTER) ; }
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
        uint32_t data_size ;
        uint8_t *data_bytes;

        uint32_t randomized_distance ;	// number of hops (tunnel wise. Does not preclude of the real distance)

    // utility methods for signing data
    virtual uint32_t signed_data_size() const ;
        virtual bool serialise_signed_data(void *data,uint32_t& size) const ;
};

class RsGRouterSignedReceiptItem: public RsGRouterAbstractMsgItem
{
    public:
        RsGRouterSignedReceiptItem() : RsGRouterAbstractMsgItem(RS_PKT_SUBTYPE_GROUTER_SIGNED_RECEIPT) { setPriorityLevel(QOS_PRIORITY_RS_GROUTER) ; }
    virtual ~RsGRouterSignedReceiptItem() {}

        virtual bool serialise(void *data,uint32_t& size) const ;
        virtual uint32_t serial_size() const ;

        virtual void clear() {}
        virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) ;

        RsGRouterSignedReceiptItem *duplicate() const ;

        // packet data
        //
    Sha1CheckSum data_hash ;		// avoids an attacker to re-use a given signed receipt. This is the hash of the enceypted data.

    // utility methods for signing data
    virtual uint32_t signed_data_size() const ;
        virtual bool serialise_signed_data(void *data,uint32_t& size) const ;
};

// Low-level data items

class RsGRouterTransactionItem: public RsGRouterItem
{
    public:
        RsGRouterTransactionItem(uint8_t pkt_subtype) : RsGRouterItem(pkt_subtype) {}

    virtual ~RsGRouterTransactionItem() {}

        virtual bool serialise(void *data,uint32_t& size) const =0;
        virtual uint32_t serial_size() const =0;
        virtual void clear() =0;

    virtual RsGRouterTransactionItem *duplicate() const = 0 ;
};

class RsGRouterTransactionChunkItem: public RsGRouterTransactionItem, public RsGRouterNonCopyableObject
{
    public:
        RsGRouterTransactionChunkItem() : RsGRouterTransactionItem(RS_PKT_SUBTYPE_GROUTER_TRANSACTION_CHUNK) { setPriorityLevel(QOS_PRIORITY_RS_GROUTER) ; }

    virtual ~RsGRouterTransactionChunkItem()  { free(chunk_data) ; }

        virtual bool serialise(void *data,uint32_t& size) const ;
        virtual uint32_t serial_size() const ;

        virtual void clear() {}
        virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) ;

    virtual RsGRouterTransactionItem *duplicate() const
    {
        RsGRouterTransactionChunkItem *item = new RsGRouterTransactionChunkItem ;
        *item = *this ;	// copy all fields
        item->chunk_data = (uint8_t*)malloc(chunk_size) ;	// deep copy memory chunk
        memcpy(item->chunk_data,chunk_data,chunk_size) ;
        return item ;
    }

        GRouterMsgPropagationId propagation_id ;
        uint32_t chunk_start ;
        uint32_t chunk_size ;
        uint32_t total_size ;
        uint8_t *chunk_data ;
};
class RsGRouterTransactionAcknItem: public RsGRouterTransactionItem
{
    public:
        RsGRouterTransactionAcknItem() : RsGRouterTransactionItem(RS_PKT_SUBTYPE_GROUTER_TRANSACTION_ACKN) { setPriorityLevel(QOS_PRIORITY_RS_GROUTER) ; }
    virtual ~RsGRouterTransactionAcknItem() {}

        virtual bool serialise(void *data,uint32_t& size) const ;
        virtual uint32_t serial_size() const ;

        virtual void clear() {}
        virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) ;

    virtual RsGRouterTransactionItem *duplicate() const  { return new RsGRouterTransactionAcknItem(*this) ; }

        GRouterMsgPropagationId propagation_id ;
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

class RsGRouterMatrixTrackItem: public RsGRouterItem
{
	public:
        RsGRouterMatrixTrackItem() : RsGRouterItem(RS_PKT_SUBTYPE_GROUTER_MATRIX_TRACK)
		{ setPriorityLevel(0) ; }	// this item is never sent through the network

		virtual bool serialise(void *data,uint32_t& size) const ;	
		virtual uint32_t serial_size() const ; 						

		virtual void clear() {} 
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) ;

		// packet data
		//
		RsGxsMessageId message_id ;
        	uint32_t provider_id ;
            	time_t time_stamp ;
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
            if(data_item != NULL) delete data_item ;
            if(receipt_item != NULL) delete receipt_item ;

            data_item = NULL ;
            receipt_item = NULL ;
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
        RsGRouterItem *gitem = dynamic_cast<RsGRouterItem *>(item);
        if (!gitem)
        {
            return 0;
        }
        return gitem->serial_size() ;
    }
    virtual bool serialise(RsItem *item, void *data, uint32_t *size)
    {
        RsGRouterItem *gitem = dynamic_cast<RsGRouterItem *>(item);
        if (!gitem)
        {
            return false;
        }
        return gitem->serialise(data,*size) ;
    }
    virtual RsItem *deserialise (void *data, uint32_t *size) ;

private:
    RsGRouterGenericDataItem      *deserialise_RsGRouterGenericDataItem(void *data,uint32_t size) const ;
    RsGRouterTransactionChunkItem *deserialise_RsGRouterTransactionChunkItem(void *data,uint32_t size) const ;
    RsGRouterTransactionAcknItem  *deserialise_RsGRouterTransactionAcknItem(void *data,uint32_t size) const ;
    RsGRouterSignedReceiptItem    *deserialise_RsGRouterSignedReceiptItem(void *data,uint32_t size) const ;
    RsGRouterMatrixCluesItem      *deserialise_RsGRouterMatrixCluesItem(void *data,uint32_t size) const ;
    RsGRouterMatrixTrackItem      *deserialise_RsGRouterMatrixTrackItem(void *data,uint32_t size) const ;
    RsGRouterMatrixFriendListItem *deserialise_RsGRouterMatrixFriendListItem(void *data,uint32_t size) const ;
    RsGRouterRoutingInfoItem      *deserialise_RsGRouterRoutingInfoItem(void *data,uint32_t size) const ;
};


