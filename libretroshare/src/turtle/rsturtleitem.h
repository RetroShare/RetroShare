#pragma once

#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rsbaseserial.h"
#include "rsiface/rsturtle.h"
#include "rsiface/rsexpr.h"
#include "serialiser/rsserviceids.h"
#include "turtle/turtletypes.h"

const uint8_t RS_TURTLE_SUBTYPE_STRING_SEARCH_REQUEST	= 0x01 ;
const uint8_t RS_TURTLE_SUBTYPE_SEARCH_RESULT  			= 0x02 ;
const uint8_t RS_TURTLE_SUBTYPE_OPEN_TUNNEL    			= 0x03 ;
const uint8_t RS_TURTLE_SUBTYPE_TUNNEL_OK      			= 0x04 ;
const uint8_t RS_TURTLE_SUBTYPE_CLOSE_TUNNEL   			= 0x05 ;
const uint8_t RS_TURTLE_SUBTYPE_TUNNEL_CLOSED  			= 0x06 ;
const uint8_t RS_TURTLE_SUBTYPE_FILE_REQUEST   			= 0x07 ;
const uint8_t RS_TURTLE_SUBTYPE_FILE_DATA      			= 0x08 ;
const uint8_t RS_TURTLE_SUBTYPE_REGEXP_SEARCH_REQUEST = 0x09 ;

/***********************************************************************************/
/*                           Basic Turtle Item Class                               */
/***********************************************************************************/

class RsTurtleItem: public RsItem
{
	public:
		RsTurtleItem(uint8_t turtle_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_TURTLE,turtle_subtype) {}

		virtual bool serialize(void *data,uint32_t& size) = 0 ;	// Isn't it better that items can serialize themselves ?
		virtual uint32_t serial_size() = 0 ; 							// deserialise is handled using a constructor

		virtual void clear() {} 
};

/***********************************************************************************/
/*                           Turtle Search Item classes                            */
/***********************************************************************************/

class RsTurtleSearchResultItem: public RsTurtleItem
{
	public:
		RsTurtleSearchResultItem() : RsTurtleItem(RS_TURTLE_SUBTYPE_SEARCH_RESULT) {}
		RsTurtleSearchResultItem(void *data,uint32_t size) ;		// deserialization

		TurtleSearchRequestId request_id ;	// Randomly generated request id.

		uint16_t depth ;							// The depth of a search result is obfuscated in this way:
														// 	If the actual depth is 1, this field will be 1.
														// 	If the actual depth is > 1, this field is a larger arbitrary integer. 
														
		std::list<TurtleFileInfo> result ;

		virtual std::ostream& print(std::ostream& o, uint16_t) ;

	protected:
		virtual bool serialize(void *data,uint32_t& size) ;
		virtual uint32_t serial_size() ;
};

class RsTurtleSearchRequestItem: public RsTurtleItem
{
	public:
		RsTurtleSearchRequestItem(uint32_t subtype) : RsTurtleItem(subtype) {}

		virtual RsTurtleSearchRequestItem *clone() const = 0 ;						// used for cloning in routing methods
		virtual void performLocalSearch(std::list<TurtleFileInfo>&) const = 0 ;	// abstracts the search method

		uint32_t request_id ; 		// randomly generated request id.
		uint16_t depth ;				// Used for limiting search depth.
};

class RsTurtleStringSearchRequestItem: public RsTurtleSearchRequestItem
{
	public:
		RsTurtleStringSearchRequestItem() : RsTurtleSearchRequestItem(RS_TURTLE_SUBTYPE_STRING_SEARCH_REQUEST) {} 
		RsTurtleStringSearchRequestItem(void *data,uint32_t size) ;
			
		std::string match_string ;	// string to match

		virtual RsTurtleSearchRequestItem *clone() const { return new RsTurtleStringSearchRequestItem(*this) ; }
		virtual void performLocalSearch(std::list<TurtleFileInfo>&) const ;

		virtual std::ostream& print(std::ostream& o, uint16_t) ;
	protected:
		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};

class RsTurtleRegExpSearchRequestItem: public RsTurtleSearchRequestItem
{
	public:
		RsTurtleRegExpSearchRequestItem() : RsTurtleSearchRequestItem(RS_TURTLE_SUBTYPE_REGEXP_SEARCH_REQUEST) {} 
		RsTurtleRegExpSearchRequestItem(void *data,uint32_t size) ;

		LinearizedExpression expr ;	// Reg Exp in linearised mode

		virtual RsTurtleSearchRequestItem *clone() const { return new RsTurtleRegExpSearchRequestItem(*this) ; }
		virtual void performLocalSearch(std::list<TurtleFileInfo>&) const ;

		virtual std::ostream& print(std::ostream& o, uint16_t) ;
	protected:
		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};

/***********************************************************************************/
/*                           Turtle Tunnel Item classes                            */
/***********************************************************************************/

class RsTurtleOpenTunnelItem: public RsTurtleItem
{
	public:
		RsTurtleOpenTunnelItem() : RsTurtleItem(RS_TURTLE_SUBTYPE_OPEN_TUNNEL) {}
		RsTurtleOpenTunnelItem(void *data,uint32_t size) ;		// deserialization

		TurtleFileHash file_hash ;	  // hash to match
		uint32_t request_id ;		  // randomly generated request id.
		uint32_t partial_tunnel_id ; // uncomplete tunnel id. Will be completed at destination.
		uint16_t depth ;				  // Used for limiting search depth.

		virtual std::ostream& print(std::ostream& o, uint16_t) ;

	protected:
		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};

class RsTurtleTunnelOkItem: public RsTurtleItem
{
	public:
		RsTurtleTunnelOkItem() : RsTurtleItem(RS_TURTLE_SUBTYPE_TUNNEL_OK) {}
		RsTurtleTunnelOkItem(void *data,uint32_t size) ;		// deserialization

		uint32_t tunnel_id ;		// id of the tunnel. Should be identical for a tunnel between two same peers for the same hash.
		uint32_t request_id ;	// randomly generated request id corresponding to the intial request.

		virtual std::ostream& print(std::ostream& o, uint16_t) ;

	protected:
		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};

#ifdef A_VIRER
class RsTurtleCloseTunnelItem: public RsTurtleItem
{
	public:
		RsTurtleCloseTunnelItem() : RsTurtleItem(RS_TURTLE_SUBTYPE_CLOSE_TUNNEL) {}
		RsTurtleCloseTunnelItem(void *data,uint32_t size) ;		// deserialization

		uint32_t tunnel_id ;		// id of the tunnel to close.

		virtual std::ostream& print(std::ostream& o, uint16_t) ;
	protected:
		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};

class RsTurtleTunnelClosedItem: public RsTurtleItem
{
	public:
		RsTurtleTunnelClosedItem() : RsTurtleItem(RS_TURTLE_SUBTYPE_TUNNEL_CLOSED) {}
		RsTurtleTunnelClosedItem(void *data,uint32_t size) ;		// deserialization

		uint32_t tunnel_id ;		// id of the tunnel to close.

		virtual std::ostream& print(std::ostream& o, uint16_t) ;
	protected:
		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};
#endif

/***********************************************************************************/
/*                           Turtle File Transfer item classes                     */
/***********************************************************************************/

class RsTurtleFileRequestItem: public RsTurtleItem
{
	public:
		RsTurtleFileRequestItem() : RsTurtleItem(RS_TURTLE_SUBTYPE_FILE_REQUEST) {}
		RsTurtleFileRequestItem(void *data,uint32_t size) ;		// deserialization

		uint32_t tunnel_id ;		// id of the tunnel to travel through
		uint64_t chunk_offset ;
		uint32_t chunk_size ;

		virtual std::ostream& print(std::ostream& o, uint16_t) ;
	protected:
		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};

class RsTurtleFileDataItem: public RsTurtleItem
{
	public:
		RsTurtleFileDataItem() : RsTurtleItem(RS_TURTLE_SUBTYPE_FILE_DATA) {}
		~RsTurtleFileDataItem() ;
		RsTurtleFileDataItem(void *data,uint32_t size) ;		// deserialization

		uint32_t tunnel_id ;		// id of the tunnel to travel through
		uint64_t chunk_offset ;	// offset in the file
		uint32_t chunk_size ;	// size of the file chunk
		void    *chunk_data ;	// actual data.

		virtual std::ostream& print(std::ostream& o, uint16_t) ;

		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};

/***********************************************************************************/
/*                           Turtle Serialiser class                               */
/***********************************************************************************/

class RsTurtleSerialiser: public RsSerialType
{
	public:
		RsTurtleSerialiser() : RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_TURTLE) {}

		virtual uint32_t 	size (RsItem *item) 
		{ 
			return dynamic_cast<RsTurtleItem *>(item)->serial_size() ;
		}
		virtual bool serialise(RsItem *item, void *data, uint32_t *size) 
		{ 
			return dynamic_cast<RsTurtleItem *>(item)->serialize(data,*size) ;
		}
		virtual RsItem *deserialise (void *data, uint32_t *size) ;
};

