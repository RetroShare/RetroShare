/*******************************************************************************
 * libretroshare/src/turtle: rsturtleitem.h                                    *
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

#include "rsitems/rsserviceids.h"
#include "rsitems/rsitem.h"
#include "rsitems/itempriorities.h"

#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rsbaseserial.h"

#include "retroshare/rsturtle.h"
#include "retroshare/rsexpr.h"
#include "retroshare/rstypes.h"
#include "turtle/turtletypes.h"

#include "serialiser/rsserializer.h"

const uint8_t RS_TURTLE_SUBTYPE_STRING_SEARCH_REQUEST	= 0x01 ;
const uint8_t RS_TURTLE_SUBTYPE_FT_SEARCH_RESULT		= 0x02 ;
const uint8_t RS_TURTLE_SUBTYPE_OPEN_TUNNEL    			= 0x03 ;
const uint8_t RS_TURTLE_SUBTYPE_TUNNEL_OK      			= 0x04 ;
const uint8_t RS_TURTLE_SUBTYPE_FILE_REQUEST   			= 0x07 ;
const uint8_t RS_TURTLE_SUBTYPE_FILE_DATA      			= 0x08 ;
const uint8_t RS_TURTLE_SUBTYPE_REGEXP_SEARCH_REQUEST   = 0x09 ;
const uint8_t RS_TURTLE_SUBTYPE_GENERIC_DATA     		= 0x0a ;
const uint8_t RS_TURTLE_SUBTYPE_GENERIC_SEARCH_REQUEST  = 0x0b ;
const uint8_t RS_TURTLE_SUBTYPE_GENERIC_SEARCH_RESULT   = 0x0c ;
const uint8_t RS_TURTLE_SUBTYPE_FILE_MAP                = 0x10 ;
const uint8_t RS_TURTLE_SUBTYPE_FILE_MAP_REQUEST        = 0x11 ;
// const uint8_t RS_TURTLE_SUBTYPE_FILE_CRC                = 0x12 ; // unused
// const uint8_t RS_TURTLE_SUBTYPE_FILE_CRC_REQUEST        = 0x13 ;
const uint8_t RS_TURTLE_SUBTYPE_CHUNK_CRC               = 0x14 ;
const uint8_t RS_TURTLE_SUBTYPE_CHUNK_CRC_REQUEST       = 0x15 ;
const uint8_t RS_TURTLE_SUBTYPE_GENERIC_FAST_DATA   	= 0x16 ;


class TurtleSearchRequestInfo ;

/***********************************************************************************/
/*                           Basic Turtle Item Class                               */
/***********************************************************************************/

class RsTurtleItem: public RsItem
{
	public:
		RsTurtleItem(uint8_t turtle_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_TURTLE,turtle_subtype) {}
};

/***********************************************************************************/
/*                           Turtle Search Item classes                            */
/*                                Specific packets                                 */
/***********************************************************************************/

// Class hierarchy is
//
//     RsTurtleItem
//         |
//         +---- RsTurtleSearchRequestItem
//         |               |
//         |               +---- RsTurtleFileSearchRequestItem
//         |               |                 |
//         |               |                 +---- RsTurtleStringSearchRequestItem
//         |               |                 |
//         |               |                 +---- RsTurtleReqExpSearchRequestItem
//         |               |
//         |               +---- RsTurtleGenericSearchRequestItem
//         |
//         +---- RsTurtleSearchResultItem
//                         |
//                         +---- RsTurtleFTSearchResultItem
//                         |
//                         +---- RsTurtleGenericSearchResultItem
//

class RsTurtleSearchResultItem ;

class RsTurtleSearchRequestItem: public RsTurtleItem
{
	public:
        RsTurtleSearchRequestItem(uint32_t subtype) : RsTurtleItem(subtype), request_id(0), depth(0) { setPriorityLevel(QOS_PRIORITY_RS_TURTLE_SEARCH_REQUEST) ;}
        virtual ~RsTurtleSearchRequestItem() {}

		virtual RsTurtleSearchRequestItem *clone() const = 0 ;					// used for cloning in routing methods

		virtual std::string GetKeywords() = 0;
        virtual uint16_t serviceId() const= 0 ;

		uint32_t request_id ; 		// randomly generated request id.
		uint16_t depth ;				// Used for limiting search depth.
};

class RsTurtleFileSearchRequestItem: public RsTurtleSearchRequestItem
{
	public:
        RsTurtleFileSearchRequestItem(uint32_t subtype) : RsTurtleSearchRequestItem(subtype) {}
        virtual ~RsTurtleFileSearchRequestItem() {}

        virtual uint16_t serviceId() const { return RS_SERVICE_TYPE_FILE_TRANSFER ; }
		virtual void search(std::list<TurtleFileInfo> &) const =0;
};

class RsTurtleStringSearchRequestItem: public RsTurtleFileSearchRequestItem
{
	public:
		RsTurtleStringSearchRequestItem() : RsTurtleFileSearchRequestItem(RS_TURTLE_SUBTYPE_STRING_SEARCH_REQUEST) {}
        virtual ~RsTurtleStringSearchRequestItem() {}

		virtual void search(std::list<TurtleFileInfo> &) const ;

		std::string match_string ;	// string to match
		std::string GetKeywords() { return match_string; }

		virtual RsTurtleSearchRequestItem *clone() const { return new RsTurtleStringSearchRequestItem(*this) ; }

        void clear() { match_string.clear() ; }

	protected:
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

class RsTurtleRegExpSearchRequestItem: public RsTurtleFileSearchRequestItem
{
	public:
		RsTurtleRegExpSearchRequestItem() : RsTurtleFileSearchRequestItem(RS_TURTLE_SUBTYPE_REGEXP_SEARCH_REQUEST) {}
        virtual ~RsTurtleRegExpSearchRequestItem() {}

        RsRegularExpression::LinearizedExpression expr ;	// Reg Exp in linearised mode

		std::string GetKeywords()
		{
			RsRegularExpression::Expression *ex = RsRegularExpression::LinearizedExpression::toExpr(expr);
			std::string exs = ex->toStdString();
			delete ex;
			return exs;
		}

		virtual void search(std::list<TurtleFileInfo> &) const ;

		virtual RsTurtleSearchRequestItem *clone() const { return new RsTurtleRegExpSearchRequestItem(*this) ; }
		void clear() { expr = RsRegularExpression::LinearizedExpression(); }
	protected:
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

class RsTurtleGenericSearchRequestItem: public RsTurtleSearchRequestItem
{
	public:
		RsTurtleGenericSearchRequestItem()
            : RsTurtleSearchRequestItem(RS_TURTLE_SUBTYPE_GENERIC_SEARCH_REQUEST),
              service_id(0),
              search_data_len(0),
              request_type(0),
              search_data(nullptr)
        {}
        virtual ~RsTurtleGenericSearchRequestItem() { clear(); }

        uint16_t service_id ;		// service to search
        uint32_t search_data_len ;
        uint8_t  request_type ;     // type of request. This is used to limit the number of responses.
        unsigned char *search_data ;

		std::string GetKeywords() ;
        virtual uint16_t serviceId() const { return service_id ; }

		virtual RsTurtleSearchRequestItem *clone() const ;
        virtual uint32_t requestType() const { return request_type; }

		void clear() { free(search_data); search_data=NULL; search_data_len=0; }

	protected:
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

    private:
        RsTurtleGenericSearchRequestItem(const RsTurtleGenericSearchRequestItem&); // make the object non copi-able.
        RsTurtleGenericSearchRequestItem& operator=(const RsTurtleGenericSearchRequestItem&) { return *this;}
};
class RsTurtleSearchResultItem: public RsTurtleItem
{
	public:
        RsTurtleSearchResultItem(uint8_t subtype) : RsTurtleItem(subtype), request_id(0) { setPriorityLevel(QOS_PRIORITY_RS_TURTLE_SEARCH_RESULT) ;}

		TurtleSearchRequestId request_id ;	// Randomly generated request id.

        virtual uint32_t count() const =0;
        virtual void pop() =0;

		virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)=0;
        virtual RsTurtleSearchResultItem *duplicate() const =0;
};

class RsTurtleFTSearchResultItem: public RsTurtleSearchResultItem
{
	public:
        RsTurtleFTSearchResultItem() : RsTurtleSearchResultItem(RS_TURTLE_SUBTYPE_FT_SEARCH_RESULT){}

		std::list<TurtleFileInfo> result ;

        void clear() { result.clear() ; }
        uint32_t count() const { return result.size() ; }
        virtual void pop() { result.pop_back() ;}
        virtual RsTurtleSearchResultItem *duplicate() const { return new RsTurtleFTSearchResultItem(*this) ; }
	protected:
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

class RsTurtleGenericSearchResultItem: public RsTurtleSearchResultItem
{
	public:
        RsTurtleGenericSearchResultItem() : RsTurtleSearchResultItem(RS_TURTLE_SUBTYPE_GENERIC_SEARCH_RESULT){}
        virtual ~RsTurtleGenericSearchResultItem() {}

        uint32_t count() const { return result_data_len/50 ; }	// This is a blind size estimate. We should probably use the actual size to limit search results.
        virtual void pop() {}

        unsigned char *result_data ;
        uint32_t result_data_len ;

        virtual RsTurtleSearchResultItem *duplicate() const ;
		void clear() { free(result_data); result_data=NULL; result_data_len=0; }
	protected:
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

/***********************************************************************************/
/*                           Turtle Tunnel Item classes                            */
/***********************************************************************************/

class RsTurtleOpenTunnelItem: public RsTurtleItem
{
	public:
        RsTurtleOpenTunnelItem() : RsTurtleItem(RS_TURTLE_SUBTYPE_OPEN_TUNNEL), request_id(0), partial_tunnel_id(0), depth(0) { setPriorityLevel(QOS_PRIORITY_RS_TURTLE_OPEN_TUNNEL) ;}

		TurtleFileHash file_hash ;	 // hash to match
		uint32_t request_id ;		 // randomly generated request id.
		uint32_t partial_tunnel_id ; // uncomplete tunnel id. Will be completed at destination.
		uint16_t depth ;			 // Used for limiting search depth.

        void clear() { file_hash.clear() ;}
	protected:
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

class RsTurtleTunnelOkItem: public RsTurtleItem
{
	public:
        RsTurtleTunnelOkItem() : RsTurtleItem(RS_TURTLE_SUBTYPE_TUNNEL_OK), tunnel_id(0), request_id(0) { setPriorityLevel(QOS_PRIORITY_RS_TURTLE_TUNNEL_OK) ;}

		uint32_t tunnel_id ;		// id of the tunnel. Should be identical for a tunnel between two same peers for the same hash.
		uint32_t request_id ;	// randomly generated request id corresponding to the intial request.

        void clear() {}
	protected:
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

/***********************************************************************************/
/*                           Generic turtle packets for tunnels                    */
/***********************************************************************************/

class RsTurtleGenericTunnelItem: public RsTurtleItem
{
	public:
        RsTurtleGenericTunnelItem(uint8_t sub_packet_id) : RsTurtleItem(sub_packet_id), direction(0), tunnel_id(0) { setPriorityLevel(QOS_PRIORITY_RS_TURTLE_GENERIC_ITEM);}
        virtual ~RsTurtleGenericTunnelItem() {}

		typedef uint32_t Direction ;
		static const Direction DIRECTION_CLIENT = 0x001 ;
		static const Direction DIRECTION_SERVER = 0x002 ;

		/// Does this packet stamps tunnels when it passes through ?
		/// This is used for keeping trace weither tunnels are active or not.

		virtual bool shouldStampTunnel() const = 0 ;

		/// All tunnels derived from RsTurtleGenericTunnelItem should have a tunnel id to
		/// indicate which tunnel they are travelling through.

		virtual TurtleTunnelId tunnelId() const { return tunnel_id ; }

		/// Indicate weither the packet is a client packet (goign back to the
		/// client) or a server packet (going to the server. Typically file
		/// requests are server packets, whereas file data are client packets.

		virtual Direction travelingDirection() const { return direction ; }
		virtual void setTravelingDirection(Direction d) { direction = d; }

		Direction direction ;	// This does not need to be serialised. It's only used by the client services, optionnally,
										// and is set by the turtle router according to which direction the item travels.

		uint32_t tunnel_id ;		// Id of the tunnel to travel through
};

/***********************************************************************************/
/*                           Specific Turtle Transfer items                        */
/***********************************************************************************/

// This item can be used by any service to pass-on arbitrary data into a tunnel.
//
class RsTurtleGenericDataItem: public RsTurtleGenericTunnelItem
{
	public:
        RsTurtleGenericDataItem() : RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_GENERIC_DATA), data_size(0), data_bytes(0) { setPriorityLevel(QOS_PRIORITY_RS_TURTLE_FILE_REQUEST);}
		virtual ~RsTurtleGenericDataItem() { if(data_bytes != NULL) free(data_bytes) ; }

		virtual bool shouldStampTunnel() const { return true ; }

		uint32_t data_size ;
		void *data_bytes ;

        void clear()
        {
            free(data_bytes) ;
            data_bytes = NULL ;
            data_size = 0;
        }
	protected:
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

// Same, but with a fact priority. Can rather be used for e.g. distant chat.
//
class RsTurtleGenericFastDataItem: public RsTurtleGenericTunnelItem
{
	public:
        RsTurtleGenericFastDataItem() : RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_GENERIC_FAST_DATA), data_size(0), data_bytes(0) { setPriorityLevel(QOS_PRIORITY_RS_TURTLE_GENERIC_FAST_DATA);}
		virtual ~RsTurtleGenericFastDataItem() { if(data_bytes != NULL) free(data_bytes) ; }

		virtual bool shouldStampTunnel() const { return true ; }

		uint32_t data_size ;
		void *data_bytes ;

        void clear()
        {
            free(data_bytes) ;
            data_bytes = NULL ;
            data_size = 0;
        }
	protected:
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};
/***********************************************************************************/
/*                           Turtle Serialiser class                               */
/***********************************************************************************/

class RsTurtleSerialiser: public RsServiceSerializer
{
	public:
		RsTurtleSerialiser() : RsServiceSerializer(RS_SERVICE_TYPE_TURTLE) {}

		virtual RsItem *create_item(uint16_t service,uint8_t item_subtype) const;

		// This is used by the turtle router to add services to its serialiser.
		// Client services are only used for deserialising, since the serialisation is
		// performed using the overloaded virtual functions above.
		//
		void registerClientService(RsTurtleClientService *service) { _client_services.push_back(service) ; }

	private:
		std::vector<RsTurtleClientService *> _client_services ;
};

