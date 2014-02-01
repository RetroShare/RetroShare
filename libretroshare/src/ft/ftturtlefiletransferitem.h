/*
 * libretroshare/src/services: ftturtlefiletransferitem.h
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

#include <stdint.h>
#include <turtle/rsturtleitem.h>

/***********************************************************************************/
/*                           Turtle File Transfer item classes                     */
/***********************************************************************************/

class RsTurtleFileRequestItem: public RsTurtleGenericTunnelItem
{
	public:
		RsTurtleFileRequestItem() : RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_FILE_REQUEST) { setPriorityLevel(QOS_PRIORITY_RS_TURTLE_FILE_REQUEST);}
		RsTurtleFileRequestItem(void *data,uint32_t size) ;		// deserialization

		virtual bool shouldStampTunnel() const { return false ; }
		virtual Direction travelingDirection() const { return DIRECTION_SERVER ; }

		uint64_t chunk_offset ;
		uint32_t chunk_size ;

		virtual std::ostream& print(std::ostream& o, uint16_t) ;
	protected:
		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};

class RsTurtleFileDataItem: public RsTurtleGenericTunnelItem
{
	public:
		RsTurtleFileDataItem() : RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_FILE_DATA) { setPriorityLevel(QOS_PRIORITY_RS_TURTLE_FILE_DATA) ;}
		~RsTurtleFileDataItem() ;
		RsTurtleFileDataItem(void *data,uint32_t size) ;		// deserialization

		virtual bool shouldStampTunnel() const { return true ; }
		virtual Direction travelingDirection() const { return DIRECTION_CLIENT ; }

		uint64_t chunk_offset ;	// offset in the file
		uint32_t chunk_size ;	// size of the file chunk
		void    *chunk_data ;	// actual data.

		virtual std::ostream& print(std::ostream& o, uint16_t) ;

		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};

class RsTurtleFileMapRequestItem: public RsTurtleGenericTunnelItem			
{
	public:
		RsTurtleFileMapRequestItem() : RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_FILE_MAP_REQUEST) { setPriorityLevel(QOS_PRIORITY_RS_TURTLE_FILE_MAP_REQUEST) ;}
		RsTurtleFileMapRequestItem(void *data,uint32_t size) ;		// deserialization

		virtual bool shouldStampTunnel() const { return false ; }

		virtual std::ostream& print(std::ostream& o, uint16_t) ;

		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};

class RsTurtleFileMapItem: public RsTurtleGenericTunnelItem			
{
	public:
		RsTurtleFileMapItem() : RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_FILE_MAP) { setPriorityLevel(QOS_PRIORITY_RS_TURTLE_FILE_MAP) ;}
		RsTurtleFileMapItem(void *data,uint32_t size) ;		// deserialization

		virtual bool shouldStampTunnel() const { return false ; }

		CompressedChunkMap compressed_map ;	// Map info for the file in compressed format. Each *bit* in the array uint's says "I have" or "I don't have"
										// by default, we suppose the peer has all the chunks. This info will thus be and-ed 
										// with the default file map for this source.
												
		virtual std::ostream& print(std::ostream& o, uint16_t) ;

		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};

class RsTurtleFileCrcRequestItem: public RsTurtleGenericTunnelItem			
{
	public:
		RsTurtleFileCrcRequestItem() : RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_FILE_CRC_REQUEST) { setPriorityLevel(QOS_PRIORITY_RS_FILE_CRC_REQUEST);}
		RsTurtleFileCrcRequestItem(void *data,uint32_t size) ;		// deserialization

		virtual bool shouldStampTunnel() const { return false ; }
		virtual Direction travelingDirection() const { return DIRECTION_SERVER ; }

		virtual std::ostream& print(std::ostream& o, uint16_t) ;

		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};

class RsTurtleChunkCrcRequestItem: public RsTurtleGenericTunnelItem			
{
	public:
		RsTurtleChunkCrcRequestItem() : RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_CHUNK_CRC_REQUEST) { setPriorityLevel(QOS_PRIORITY_RS_CHUNK_CRC_REQUEST);}
		RsTurtleChunkCrcRequestItem(void *data,uint32_t size) ;		// deserialization

		virtual bool shouldStampTunnel() const { return false ; }
		virtual Direction travelingDirection() const { return DIRECTION_SERVER ; }

		uint32_t chunk_number ; // id of the chunk to CRC.
												
		virtual std::ostream& print(std::ostream& o, uint16_t) ;

		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};

class RsTurtleChunkCrcItem: public RsTurtleGenericTunnelItem			
{
	public:
		RsTurtleChunkCrcItem() : RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_CHUNK_CRC) { setPriorityLevel(QOS_PRIORITY_RS_CHUNK_CRC);}
		RsTurtleChunkCrcItem(void *data,uint32_t size) ;		// deserialization

		virtual bool shouldStampTunnel() const { return true ; }
		virtual Direction travelingDirection() const { return DIRECTION_CLIENT ; }

		uint32_t chunk_number ;
		Sha1CheckSum check_sum ;

		virtual std::ostream& print(std::ostream& o, uint16_t) ;
		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};
