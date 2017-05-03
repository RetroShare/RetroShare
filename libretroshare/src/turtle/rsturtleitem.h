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
const uint8_t RS_TURTLE_SUBTYPE_SEARCH_RESULT  			= 0x02 ;
const uint8_t RS_TURTLE_SUBTYPE_OPEN_TUNNEL    			= 0x03 ;
const uint8_t RS_TURTLE_SUBTYPE_TUNNEL_OK      			= 0x04 ;
const uint8_t RS_TURTLE_SUBTYPE_CLOSE_TUNNEL   			= 0x05 ;
const uint8_t RS_TURTLE_SUBTYPE_TUNNEL_CLOSED  			= 0x06 ;
const uint8_t RS_TURTLE_SUBTYPE_FILE_REQUEST   			= 0x07 ;
const uint8_t RS_TURTLE_SUBTYPE_FILE_DATA      			= 0x08 ;
const uint8_t RS_TURTLE_SUBTYPE_REGEXP_SEARCH_REQUEST   = 0x09 ;
const uint8_t RS_TURTLE_SUBTYPE_GENERIC_DATA     		= 0x0a ;
const uint8_t RS_TURTLE_SUBTYPE_FILE_MAP                = 0x10 ;
const uint8_t RS_TURTLE_SUBTYPE_FILE_MAP_REQUEST        = 0x11 ;
const uint8_t RS_TURTLE_SUBTYPE_CHUNK_CRC               = 0x14 ;
const uint8_t RS_TURTLE_SUBTYPE_CHUNK_CRC_REQUEST       = 0x15 ;

// const uint8_t RS_TURTLE_SUBTYPE_FILE_CRC                = 0x12 ; // unused
// const uint8_t RS_TURTLE_SUBTYPE_FILE_CRC_REQUEST        = 0x13 ;

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

class RsTurtleSearchResultItem: public RsTurtleItem
{
	public:
        RsTurtleSearchResultItem() : RsTurtleItem(RS_TURTLE_SUBTYPE_SEARCH_RESULT), request_id(0), depth(0) { setPriorityLevel(QOS_PRIORITY_RS_TURTLE_SEARCH_RESULT) ;}

		TurtleSearchRequestId request_id ;	// Randomly generated request id.

		uint16_t depth ;					// The depth of a search result is obfuscated in this way:
											// 	If the actual depth is 1, this field will be 1.
											// 	If the actual depth is > 1, this field is a larger arbitrary integer.
		std::list<TurtleFileInfo> result ;

        void clear() { result.clear() ; }
	protected:
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

};

class RsTurtleSearchRequestItem: public RsTurtleItem
{
	public:
        RsTurtleSearchRequestItem(uint32_t subtype) : RsTurtleItem(subtype), request_id(0), depth(0) { setPriorityLevel(QOS_PRIORITY_RS_TURTLE_SEARCH_REQUEST) ;}
		virtual RsTurtleSearchRequestItem *clone() const = 0 ;					// used for cloning in routing methods

		virtual void performLocalSearch(std::list<TurtleFileInfo>&) const = 0 ;	// abstracts the search method

		uint32_t request_id ; 		// randomly generated request id.
		uint16_t depth ;				// Used for limiting search depth.
};

class RsTurtleStringSearchRequestItem: public RsTurtleSearchRequestItem
{
	public:
		RsTurtleStringSearchRequestItem() : RsTurtleSearchRequestItem(RS_TURTLE_SUBTYPE_STRING_SEARCH_REQUEST) {} 
			
		std::string match_string ;	// string to match

		virtual RsTurtleSearchRequestItem *clone() const { return new RsTurtleStringSearchRequestItem(*this) ; }
		virtual void performLocalSearch(std::list<TurtleFileInfo>&) const ;

        void clear() { match_string.clear() ; }

	protected:
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

class RsTurtleRegExpSearchRequestItem: public RsTurtleSearchRequestItem
{
	public:
		RsTurtleRegExpSearchRequestItem() : RsTurtleSearchRequestItem(RS_TURTLE_SUBTYPE_REGEXP_SEARCH_REQUEST) {} 

        RsRegularExpression::LinearizedExpression expr ;	// Reg Exp in linearised mode

		virtual RsTurtleSearchRequestItem *clone() const { return new RsTurtleRegExpSearchRequestItem(*this) ; }
		virtual void performLocalSearch(std::list<TurtleFileInfo>&) const ;

		void clear() { expr = RsRegularExpression::LinearizedExpression(); }
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

		TurtleFileHash file_hash ;	  // hash to match
		uint32_t request_id ;		  // randomly generated request id.
		uint32_t partial_tunnel_id ; // uncomplete tunnel id. Will be completed at destination.
		uint16_t depth ;				  // Used for limiting search depth.

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

