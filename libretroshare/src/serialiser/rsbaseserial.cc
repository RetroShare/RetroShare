/*******************************************************************************
 * libretroshare/src/serialiser: rsbaseserial.cc                               *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie <retroshare@lunamutt.com>              *
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

#include <stdlib.h>	/* Included because GCC4.4 wants it */
#include <string.h> 	/* Included because GCC4.4 wants it */

#include "retroshare/rstypes.h"
#include "serialiser/rsbaseserial.h"
#include "util/rsnet.h"
#include "util/rstime.h"

#include <iostream>
#include <cstdint>

/* UInt8 get/set */

bool getRawUInt8(const void *data, uint32_t size, uint32_t *offset, uint8_t *out)
{
	/* first check there is space */
	if (size < *offset + 1)
	{
		std::cerr << "(EE) Cannot deserialise uint8_t: not enough size." << std::endl;
		return false;
	}
	void *buf = (void *) &(((uint8_t *) data)[*offset]);

	/* extract the data */
	memcpy(out, buf, sizeof(uint8_t));
	(*offset) += 1;

	return true;
}
	
bool setRawUInt8(void *data, uint32_t size, uint32_t *offset, uint8_t in)
{
	/* first check there is space */
	if (size < *offset + 1)
	{
		std::cerr << "(EE) Cannot serialise uint8_t: not enough size." << std::endl;
		return false;
	}

	void *buf = (void *) &(((uint8_t *) data)[*offset]);

	/* pack it in */
	memcpy(buf, &in, sizeof(uint8_t));

	(*offset) += 1;
	return true;
}
/* UInt16 get/set */

bool getRawUInt16(const void *data, uint32_t size, uint32_t *offset, uint16_t *out)
{
	/* first check there is space */
	if (size < *offset + 2)
	{
		std::cerr << "(EE) Cannot deserialise uint16_t: not enough size." << std::endl;
		return false;
	}
	void *buf = (void *) &(((uint8_t *) data)[*offset]);

	/* extract the data */
	uint16_t netorder_num;
	memcpy(&netorder_num, buf, sizeof(uint16_t));

	(*out) = ntohs(netorder_num);
	(*offset) += 2;
	return true;
}
	
bool setRawUInt16(void *data, uint32_t size, uint32_t *offset, uint16_t in)
{
	/* first check there is space */
	if (size < *offset + 2)
	{
		std::cerr << "(EE) Cannot serialise uint16_t: not enough size." << std::endl;
		return false;
	}

	void *buf = (void *) &(((uint8_t *) data)[*offset]);

	/* convert the data to the right format */
	uint16_t netorder_num = htons(in);

	/* pack it in */
	memcpy(buf, &netorder_num, sizeof(uint16_t));

	(*offset) += 2;
	return true;
}

/* UInt32 get/set */

bool getRawUInt32(const void *data, uint32_t size, uint32_t *offset, uint32_t *out)
{
	/* first check there is space */
	if (size < *offset + 4)
	{
		std::cerr << "(EE) Cannot deserialise uint32_t: not enough size." << std::endl;
		return false;
	}
	void *buf = (void *) &(((uint8_t *) data)[*offset]);

	/* extract the data */
	uint32_t netorder_num;
	memcpy(&netorder_num, buf, sizeof(uint32_t));

	(*out) = ntohl(netorder_num);
	(*offset) += 4;
	return true;
}
	
bool setRawUInt32(void *data, uint32_t size, uint32_t *offset, uint32_t in)
{
	/* first check there is space */
	if (size < *offset + 4)
	{
		std::cerr << "(EE) Cannot serialise uint32_t: not enough size." << std::endl;
		return false;
	}

	void *buf = (void *) &(((uint8_t *) data)[*offset]);

	/* convert the data to the right format */
	uint32_t netorder_num = htonl(in);

	/* pack it in */
	memcpy(buf, &netorder_num, sizeof(uint32_t));

	(*offset) += 4;
	return true;
}

/* UInt64 get/set */

bool getRawUInt64(const void *data, uint32_t size, uint32_t *offset, uint64_t *out)
{
	/* first check there is space */
	if (size < *offset + 8)
	{
		std::cerr << "(EE) Cannot deserialise uint64_t: not enough size." << std::endl;
		return false;
	}
	void *buf = (void *) &(((uint8_t *) data)[*offset]);

	/* extract the data */
	uint64_t netorder_num;
	memcpy(&netorder_num, buf, sizeof(uint64_t));

	(*out) = ntohll(netorder_num);
	(*offset) += 8;
	return true;
}
	
bool setRawUInt64(void *data, uint32_t size, uint32_t *offset, uint64_t in)
{
	/* first check there is space */
	if (size < *offset + 8)
	{
		std::cerr << "(EE) Cannot serialise uint64_t: not enough size." << std::endl;
		return false;
	}

	void *buf = (void *) &(((uint8_t *) data)[*offset]);

	/* convert the data to the right format */
	uint64_t netorder_num = htonll(in);

	/* pack it in */
	memcpy(buf, &netorder_num, sizeof(uint64_t));

	(*offset) += 8;
	return true;
}

bool getRawUFloat32(const void *data, uint32_t size, uint32_t *offset, float& f)
{
	uint32_t n ;
	if(!getRawUInt32(data, size, offset, &n) )
		return false ;

	f = 1.0f/ ( n/(float)(~(uint32_t)0)) - 1.0f ;

	return true ;
}

bool setRawUFloat32(void *data,uint32_t size,uint32_t *offset,float f)
{
	uint32_t sz = 4;

	if ( !data || size <= *offset || size < sz + *offset )
	{
		std::cerr << "(EE) not enough room. SIZE+offset=" << sz+*offset << " and size is only " << size << std::endl;
		return false;
	}
	if(f < 0.0f)
	{
		std::cerr << "(EE) Cannot serialise invalid negative float value " << f << " in " << __PRETTY_FUNCTION__ << std::endl;
		return false ;
	}

	// This serialisation is quite accurate. The max relative error is approx.
	// 0.01% and most of the time less than 1e-05% The error is well distributed
	// over numbers also.
	//
	uint32_t n = (f < 1e-7)?(~(uint32_t)0): ((uint32_t)( (1.0f/(1.0f+f) * (~(uint32_t)0)))) ;

	return setRawUInt32(data, size, offset, n);
}


uint32_t getRawStringSize(const std::string &outStr)
{
	return outStr.length() + 4;
}

bool getRawString(const void *data, uint32_t size, uint32_t *offset, std::string &outStr)
{
	outStr.clear();

	uint32_t len = 0;
	if (!getRawUInt32(data, size, offset, &len))
	{
                std::cerr << "getRawString() get size failed" << std::endl;
		return false;
	}

	/* check there is space for string */
	if(len > size || size-len < *offset) // better than if(size < *offset + len) because it avoids integer overflow
	{
		std::cerr << "getRawString() not enough size" << std::endl;
		print_stacktrace();
		return false;
	}
	uint8_t *buf = &(((uint8_t *) data)[*offset]);
    
        for (uint32_t i = 0; i < len; i++)
	{
		outStr += buf[i];
	}

	(*offset) += len;
	return true;
}

bool setRawString(void *data, uint32_t size, uint32_t *offset, const std::string &inStr)
{
	uint32_t len = inStr.length();
	/* first check there is space */
    
    	if(size < 4 || len > size-4 || size-len-4 < *offset) // better than if(size < *offset + len + 4) because it avoids integer overflow
	{
                std::cerr << "setRawString() Not enough size" << std::endl;
		return false;
	}

	if (!setRawUInt32(data, size, offset, len))
	{
                std::cerr << "setRawString() set size failed" << std::endl;
		return false;
	}

	void *buf = (void *) &(((uint8_t *) data)[*offset]);

	/* pack it in */
	memcpy(buf, inStr.c_str(), len);

	(*offset) += len;
	return true;
}

bool getRawTimeT(const void *data,uint32_t size,uint32_t *offset,rstime_t& t)
{
	uint64_t T;
	bool res = getRawUInt64(data,size,offset,&T);
	t = T;

	if(t < 0) // [[unlikely]]
		std::cerr << __PRETTY_FUNCTION__ << " got a negative time: " << t
		          << " this seems fishy, report to the developers!" << std::endl;

	return res;
}
bool setRawTimeT(void *data, uint32_t size, uint32_t *offset, const rstime_t& t)
{
	if(t < 0) // [[unlikely]]
		std::cerr << __PRETTY_FUNCTION__ << " got a negative time: " << t
		          << " this seems fishy, report to the developers!" << std::endl;

	return setRawUInt64(data,size,offset,t) ;
}
