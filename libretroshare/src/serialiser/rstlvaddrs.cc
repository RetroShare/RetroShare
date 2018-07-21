/*******************************************************************************
 * libretroshare/src/serialiser: rstlvaddr.cc                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2010 by Robert Fernie <retroshare@lunamutt.com>                   *
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
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvaddrs.h"
#include "serialiser/rsbaseserial.h"

/************************************* RsTlvIpAddress ************************************/

RsTlvIpAddress::RsTlvIpAddress()
	:RsTlvItem()
{
	sockaddr_storage_clear(addr);
	return;
}

void RsTlvIpAddress::TlvClear()
{
	sockaddr_storage_clear(addr);
}

uint32_t RsTlvIpAddress::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; 
	switch(addr.ss_family)
	{
		default:
		case 0:
			break;
		case AF_INET:
			s += GetTlvIpAddrPortV4Size(); 
			break;
		case AF_INET6:
			s += GetTlvIpAddrPortV6Size(); 
			break;
	}
	return s;
}

bool  RsTlvIpAddress::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

	/* start at data[offset] */
        /* add mandatory parts first */

	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_ADDRESS, tlvsize);

	switch(addr.ss_family)
	{
		default:
		case 0:
			break;
		case AF_INET:
			ok &= SetTlvIpAddrPortV4(data, tlvend, offset, TLV_TYPE_IPV4, (struct sockaddr_in *) &addr);
			break;

		case AF_INET6:
			ok &= SetTlvIpAddrPortV6(data, tlvend, offset, TLV_TYPE_IPV6, (struct sockaddr_in6 *) &addr);
			break;
	}
	return ok;

}


bool  RsTlvIpAddress::GetTlv(void *data, uint32_t size, uint32_t *offset) 
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	
	
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_ADDRESS) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	if (*offset == tlvend)
	{
		/* empty address */
		return ok;
	}

	uint16_t iptype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	switch(iptype)
	{
		case TLV_TYPE_IPV4:
			ok &= GetTlvIpAddrPortV4(data, tlvend, offset, TLV_TYPE_IPV4, (struct sockaddr_in *) &addr);
			break;
		case TLV_TYPE_IPV6:
			ok &= GetTlvIpAddrPortV6(data, tlvend, offset, TLV_TYPE_IPV6, (struct sockaddr_in6 *) &addr);
			break;
		default:
			break;
	}

	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvIpAddress::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
	
}


std::ostream &RsTlvIpAddress::print(std::ostream &out, uint16_t indent) const
{ 
	printBase(out, "RsTlvIpAddress", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Address:" << sockaddr_storage_tostring(addr) << std::endl;
	
	printEnd(out, "RsTlvIpAddress", indent);
	return out;
}




/************************************* RsTlvIpAddressInfo ************************************/

RsTlvIpAddressInfo::RsTlvIpAddressInfo()
	:RsTlvItem(), seenTime(0), source(0)
{
	addr.TlvClear();
	return;
}

void RsTlvIpAddressInfo::TlvClear()
{
	addr.TlvClear();
	seenTime = 0;
	source = 0;
}

uint32_t RsTlvIpAddressInfo::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; /* header + IpAddr + 8 for time & 4 for size */

	s += addr.TlvSize();
	s += 8; // seenTime
	s += 4; // source

	return s;

}

bool  RsTlvIpAddressInfo::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

	/* start at data[offset] */
        /* add mandatory parts first */

	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_ADDRESS_INFO, tlvsize);

	ok &= addr.SetTlv(data, tlvend, offset);
	ok &= setRawUInt64(data, tlvend, offset, seenTime);
	ok &= setRawUInt32(data, tlvend, offset, source);

	return ok;

}


bool  RsTlvIpAddressInfo::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	
	
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_ADDRESS_INFO) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	ok &= addr.GetTlv(data, tlvend, offset);
	ok &= getRawUInt64(data, tlvend, offset, &(seenTime));
	ok &= getRawUInt32(data, tlvend, offset, &(source));
   

	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvIpAddressInfo::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
	
}


std::ostream &RsTlvIpAddressInfo::print(std::ostream &out, uint16_t indent) const
{ 
	printBase(out, "RsTlvIpAddressInfo", indent);
	uint16_t int_Indent = indent + 2;

	addr.print(out, int_Indent);

	printIndent(out, int_Indent);
	out << "SeenTime:" << seenTime;
	out << std::endl;

	printIndent(out, int_Indent);
	out << "Source:" << source;
	out << std::endl;
	
	printEnd(out, "RsTlvIpAddressInfo", indent);
	return out;
}



#if 0
/************************************* RsTlvIpAddrSet ************************************/

void RsTlvIpAddrSet::TlvClear()
{
	addrs.clear();
}

uint32_t RsTlvIpAddrSet::TlvSize() const
{

	uint32_t s = TLV_HEADER_SIZE; /* header */

	std::list<RsTlvIpAddressInfo>::iterator it;
	

	if(!addrs.empty())
	{

		for(it = addrs.begin(); it != addrs.end() ; ++it)
			s += it->TlvSize();

	}

	return s;
}

bool  RsTlvIpAddrSet::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

		/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_ADDRESS_SET , tlvsize);
	
	if(!addrs.empty())
	{
		std::list<RsTlvIpAddressInfo>::iterator it;

		for(it = addrs.begin(); it != addrs.end() ; ++it)
			ok &= it->SetTlv(data, size, offset);
	}
	

return ok;

}


bool  RsTlvIpAddrSet::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	

	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_ADDRESS_SET) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

        /* while there is TLV  */
        while((*offset) + 2 < tlvend)
        {
                /* get the next type */
                uint16_t tlvsubtype = GetTlvType( &(((uint8_t *) data)[*offset]) );

                switch(tlvsubtype)
                {
                        case TLV_TYPE_ADDRESS_INFO:
			{
				RsTlvIpAddressInfo addr;
				ok &= addr.GetTlv(data, size, offset);
				if (ok)
				{
					addrs.push_back(addr);
				}
			}
				break;
                        default:
                                ok &= SkipUnknownTlv(data, tlvend, offset);
                                break;

                }

                if (!ok)
			break;
	}
   

		
	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvIpAddrSet::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
}

// prints out contents of RsTlvIpAddrSet
std::ostream &RsTlvIpAddrSet::print(std::ostream &out, uint16_t indent) const
{
	printBase(out, "RsTlvIpAddrSet", indent);
	uint16_t int_Indent = indent + 2;

	std::list<RsTlvIpAddressInfo>::iterator it;
	for(it = addrs.begin(); it != addrs.end() ; ++it)
		it->print(out, int_Indent);

	printEnd(out, "RsTlvIpAddrSet", indent);
	return out;
}


/************************************* RsTlvIpAddressInfo ************************************/

#endif
