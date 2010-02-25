#ifndef RS_TUNNEL_ITEMS_H
#define RS_TUNNEL_ITEMS_H

/*
 * libretroshare/src/serialiser: rschannelitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie.
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
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include <map>

#include "serialiser/rstlvbase.h"
#include "serialiser/rsbaseserial.h"
#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvkeys.h"

const uint8_t RS_TUNNEL_SUBTYPE_DATA	= 0x01 ;
const uint8_t RS_TUNNEL_SUBTYPE_HANDSHAKE	= 0x02 ;

/***********************************************************************************/
/*                           Basic Tunnel Item Class                               */
/***********************************************************************************/

class RsTunnelItem: public RsItem
{
	public:
		RsTunnelItem(uint8_t tunnel_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_TUNNEL,tunnel_subtype) {}

		virtual bool serialize(void *data,uint32_t& size) = 0 ;	// Isn't it better that items can serialize themselves ?
                virtual uint32_t serial_size() { return  0;}

		virtual void clear() {}
};

class RsTunnelDataItem: public RsTunnelItem
{
	public:
		RsTunnelDataItem() : RsTunnelItem(RS_TUNNEL_SUBTYPE_DATA) {}
		RsTunnelDataItem(void *data,uint32_t size) ;		// deserialization

		uint32_t encoded_data_len;
		void *encoded_data;

		std::string sourcePeerId ;
		std::string relayPeerId ;
                std::string destPeerId ;

		std::ostream& print(std::ostream& o, uint16_t) ;

		bool serialize(void *data,uint32_t& size) ;
		uint32_t serial_size() ;
};

class RsTunnelHandshakeItem: public RsTunnelItem
{
        public:
                RsTunnelHandshakeItem() : RsTunnelItem(RS_TUNNEL_SUBTYPE_HANDSHAKE) {}
                RsTunnelHandshakeItem(void *data,uint32_t size) ;		// deserialization

                std::string sourcePeerId ;
                std::string relayPeerId ;
                std::string destPeerId ;
                std::string sslCertPEM ;
                uint32_t connection_accepted;

                std::ostream& print(std::ostream& o, uint16_t) ;

                bool serialize(void *data,uint32_t& size) ;
                uint32_t serial_size() ;
};

class RsTunnelSerialiser: public RsSerialType
{
	public:
		RsTunnelSerialiser() : RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_TUNNEL) {}

		virtual uint32_t 	size(RsItem *item)
		{
		    RsTunnelItem * rst;
		    if (NULL != (rst = dynamic_cast<RsTunnelItem *>(item)))
		    {
			    return rst->serial_size() ;
		    } else {
			    std::cerr << "RsTunnelSerialiser::size() problem, not a RsTunnelItem." << std::endl;
		    }
		    return 0;
		}
		virtual bool serialise(RsItem *item, void *data, uint32_t *size)
		{
		    RsTunnelItem * rst;
		    if (NULL != (rst = dynamic_cast<RsTunnelItem *>(item)))
		    {
			    return rst->serialize(data,*size) ;
		    } else {
			    std::cerr << "RsTunnelSerialiser::serialise() problem, not a RsTunnelItem." << std::endl;
		    }
		    return false;
		}
		virtual RsItem *deserialise (void *data, uint32_t *size) ;
};

#endif /* RS_TUNNEL_ITEMS_H */
