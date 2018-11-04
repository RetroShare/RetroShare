/*******************************************************************************
 * libretroshare/src/grouter: grouteritems.h                                   *
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
#pragma once

#include "util/rsmemory.h"

#include "serialiser/rsserial.h"
#include "serialiser/rstlvkeys.h"
#include "rsitems/rsserviceids.h"
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
		explicit RsGRouterItem(uint8_t grouter_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_GROUTER,grouter_subtype) {}

        virtual ~RsGRouterItem() {}

		virtual void clear() = 0 ;
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
    explicit RsGRouterAbstractMsgItem(uint8_t pkt_subtype) : RsGRouterItem(pkt_subtype), flags(0) {}
    virtual ~RsGRouterAbstractMsgItem() {}

    GRouterMsgPropagationId routing_id ;
    GRouterKeyId destination_key ;
    GRouterServiceId service_id ;
    RsTlvKeySignature signature ;		// signs mid+destination_key+state
	uint32_t flags ; 					// packet was delivered, not delivered, bounced, etc
};

class RsGRouterGenericDataItem: public RsGRouterAbstractMsgItem, public RsGRouterNonCopyableObject
{
    public:
        RsGRouterGenericDataItem() : RsGRouterAbstractMsgItem(RS_PKT_SUBTYPE_GROUTER_DATA), data_size(0), data_bytes(NULL), duplication_factor(0) { setPriorityLevel(QOS_PRIORITY_RS_GROUTER) ; }
        virtual ~RsGRouterGenericDataItem() { clear() ; }

        virtual void clear()
        {
            free(data_bytes);
            data_bytes=NULL;
        }

		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

        RsGRouterGenericDataItem *duplicate() const ;

        // packet data
        //
        uint32_t data_size ;
        uint8_t *data_bytes;
        uint32_t duplication_factor ;	// number of duplicates allowed. Should be capped at each de-serialise operation!
};

class RsGRouterSignedReceiptItem: public RsGRouterAbstractMsgItem
{
    public:
        RsGRouterSignedReceiptItem() : RsGRouterAbstractMsgItem(RS_PKT_SUBTYPE_GROUTER_SIGNED_RECEIPT) { setPriorityLevel(QOS_PRIORITY_RS_GROUTER) ; }
		virtual ~RsGRouterSignedReceiptItem() {}

        virtual void clear() {}
		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

        RsGRouterSignedReceiptItem *duplicate() const ;

        // packet data
        //
		Sha1CheckSum data_hash ;		// avoids an attacker to re-use a given signed receipt. This is the hash of the enceypted data.
};

// Low-level data items

class RsGRouterTransactionItem: public RsGRouterItem
{
    public:
        explicit RsGRouterTransactionItem(uint8_t pkt_subtype) : RsGRouterItem(pkt_subtype) {}

		virtual ~RsGRouterTransactionItem() {}

        virtual void clear() =0;

		virtual RsGRouterTransactionItem *duplicate() const = 0 ;
};

class RsGRouterTransactionChunkItem: public RsGRouterTransactionItem, public RsGRouterNonCopyableObject
{
public:
	RsGRouterTransactionChunkItem() : RsGRouterTransactionItem(RS_PKT_SUBTYPE_GROUTER_TRANSACTION_CHUNK), chunk_start(0), chunk_size(0), total_size(0), chunk_data(NULL) { setPriorityLevel(QOS_PRIORITY_RS_GROUTER) ; }

	virtual ~RsGRouterTransactionChunkItem()  { free(chunk_data) ; }

	virtual void clear() {}
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	virtual RsGRouterTransactionItem *duplicate() const
	{
		RsGRouterTransactionChunkItem *item = new RsGRouterTransactionChunkItem ;
		*item = *this ;	// copy all fields
		item->chunk_data = (uint8_t*)rs_malloc(chunk_size) ;	// deep copy memory chunk

		if(item->chunk_data == NULL)
			return NULL ;

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

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	virtual void clear() {}

	virtual RsGRouterTransactionItem *duplicate() const  { return new RsGRouterTransactionAcknItem(*this) ; }

	GRouterMsgPropagationId propagation_id ;
};

// Items for saving the routing matrix information.

class RsGRouterMatrixCluesItem: public RsGRouterItem
{
	public:
        RsGRouterMatrixCluesItem() : RsGRouterItem(RS_PKT_SUBTYPE_GROUTER_MATRIX_CLUES)
		{ setPriorityLevel(0) ; }	// this item is never sent through the network

		virtual void clear() {} 
		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		// packet data
		//
		GRouterKeyId destination_key ;
		std::list<RoutingMatrixHitEntry> clues ;
};

class RsGRouterMatrixTrackItem: public RsGRouterItem
{
	public:
        RsGRouterMatrixTrackItem() : RsGRouterItem(RS_PKT_SUBTYPE_GROUTER_MATRIX_TRACK), time_stamp(0)
		{ setPriorityLevel(0) ; }	// this item is never sent through the network

		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		virtual void clear() {} 

		// packet data
		//
		RsGxsMessageId message_id ;
		RsPeerId provider_id ;
		rstime_t time_stamp ;
};
class RsGRouterMatrixFriendListItem: public RsGRouterItem
{
	public:
		RsGRouterMatrixFriendListItem() : RsGRouterItem(RS_PKT_SUBTYPE_GROUTER_FRIENDS_LIST) 
        { setPriorityLevel(0) ; }	// this item is never sent through the network

		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
		virtual void clear() {}

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
		
		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		virtual void clear() 
		{
            if(data_item != NULL) delete data_item ;
            if(receipt_item != NULL) delete receipt_item ;

            data_item = NULL ;
            receipt_item = NULL ;
        }
};

/***********************************************************************************/
/*                                Serialisation                                    */
/***********************************************************************************/

class RsGRouterSerialiser: public RsServiceSerializer
{
public:
    explicit RsGRouterSerialiser(SerializationFlags flags = SERIALIZATION_FLAG_NONE) : RsServiceSerializer(RS_SERVICE_TYPE_GROUTER,RsGenericSerializer::FORMAT_BINARY,flags) {}

    virtual RsItem *create_item(uint16_t service,uint8_t subtype) const ;
};


