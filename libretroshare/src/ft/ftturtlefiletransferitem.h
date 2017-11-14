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
		RsTurtleFileRequestItem() : RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_FILE_REQUEST), chunk_offset(0), chunk_size(0) { setPriorityLevel(QOS_PRIORITY_RS_TURTLE_FILE_REQUEST);}

		virtual bool shouldStampTunnel() const { return false ; }
		virtual Direction travelingDirection() const { return DIRECTION_SERVER ; }

		uint64_t chunk_offset ;
		uint32_t chunk_size ;

        void clear() {}
	protected:
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

class RsTurtleFileDataItem: public RsTurtleGenericTunnelItem
{
	public:
		RsTurtleFileDataItem() : RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_FILE_DATA), chunk_offset(0), chunk_size(0), chunk_data(NULL) { setPriorityLevel(QOS_PRIORITY_RS_TURTLE_FILE_DATA) ;}
        ~RsTurtleFileDataItem() { clear() ; }

		virtual bool shouldStampTunnel() const { return true ; }
		virtual Direction travelingDirection() const { return DIRECTION_CLIENT ; }

        void clear()
        {
            free(chunk_data);
            chunk_data = NULL ;
            chunk_size = 0 ;
            chunk_offset = 0 ;
        }

		uint64_t chunk_offset ;	// offset in the file
		uint32_t chunk_size ;	// size of the file chunk
		void    *chunk_data ;	// actual data.

		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

class RsTurtleFileMapRequestItem: public RsTurtleGenericTunnelItem			
{
	public:
		RsTurtleFileMapRequestItem() : RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_FILE_MAP_REQUEST) { setPriorityLevel(QOS_PRIORITY_RS_TURTLE_FILE_MAP_REQUEST) ;}

		virtual bool shouldStampTunnel() const { return false ; }

        void clear() {}
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

class RsTurtleFileMapItem: public RsTurtleGenericTunnelItem			
{
	public:
		RsTurtleFileMapItem() : RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_FILE_MAP) { setPriorityLevel(QOS_PRIORITY_RS_TURTLE_FILE_MAP) ;}

		virtual bool shouldStampTunnel() const { return false ; }

		CompressedChunkMap compressed_map ;	// Map info for the file in compressed format. Each *bit* in the array uint's says "I have" or "I don't have"
										// by default, we suppose the peer has all the chunks. This info will thus be and-ed 
										// with the default file map for this source.
												
        void clear() { compressed_map._map.clear() ;}
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

class RsTurtleChunkCrcRequestItem: public RsTurtleGenericTunnelItem			
{
	public:
		RsTurtleChunkCrcRequestItem() : RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_CHUNK_CRC_REQUEST), chunk_number(0) { setPriorityLevel(QOS_PRIORITY_RS_CHUNK_CRC_REQUEST);}

		virtual bool shouldStampTunnel() const { return false ; }
		virtual Direction travelingDirection() const { return DIRECTION_SERVER ; }

		uint32_t chunk_number ; // id of the chunk to CRC.

        void clear() {}
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

class RsTurtleChunkCrcItem: public RsTurtleGenericTunnelItem			
{
	public:
		RsTurtleChunkCrcItem() : RsTurtleGenericTunnelItem(RS_TURTLE_SUBTYPE_CHUNK_CRC), chunk_number(0) { setPriorityLevel(QOS_PRIORITY_RS_CHUNK_CRC);}

		virtual bool shouldStampTunnel() const { return true ; }
		virtual Direction travelingDirection() const { return DIRECTION_CLIENT ; }

		uint32_t chunk_number ;
		Sha1CheckSum check_sum ;

        void clear() { check_sum.clear() ;}
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};
