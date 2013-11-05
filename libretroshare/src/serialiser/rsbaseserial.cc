
/*
 * libretroshare/src/serialiser: rsbaseserial.cc
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

#include <stdlib.h>	/* Included because GCC4.4 wants it */
#include <string.h> 	/* Included because GCC4.4 wants it */

#include "retroshare/rstypes.h"
#include "serialiser/rsbaseserial.h"
#include "util/rsnet.h"

#include <iostream>

/* UInt8 get/set */

bool getRawUInt8(void *data, uint32_t size, uint32_t *offset, uint8_t *out)
{
	/* first check there is space */
	if (size < *offset + 1)
	{
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
		return false;
	}

	void *buf = (void *) &(((uint8_t *) data)[*offset]);

	/* pack it in */
	memcpy(buf, &in, sizeof(uint8_t));

	(*offset) += 1;
	return true;
}
/* UInt16 get/set */

bool getRawUInt16(void *data, uint32_t size, uint32_t *offset, uint16_t *out)
{
	/* first check there is space */
	if (size < *offset + 2)
	{
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

bool getRawUInt32(void *data, uint32_t size, uint32_t *offset, uint32_t *out)
{
	/* first check there is space */
	if (size < *offset + 4)
	{
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

bool getRawUInt64(void *data, uint32_t size, uint32_t *offset, uint64_t *out)
{
	/* first check there is space */
	if (size < *offset + 8)
	{
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

bool getRawUFloat32(void *data,uint32_t size,uint32_t *offset,float& f)
{
	uint32_t n ;
	if(!getRawUInt32(data, size, offset, &n) )
		return false ;

	f = 1.0f/ ( n/(float)(~(uint32_t)0)) - 1.0f ;

	return true ;
}

bool setRawUFloat32(void *data,uint32_t size,uint32_t *offset,float f)
{
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

bool getRawSha1(void *data,uint32_t size,uint32_t *offset,Sha1CheckSum& cs)
{
	uint32_t len = 20 ; // SHA1 length in bytes

	/* check there is space for string */
	if (size < *offset + len)
	{
		std::cerr << "getRawSha1() not enough size" << std::endl;
		return false;
	}
	bool ok = true ;

	ok = ok && getRawUInt32(data, size, offset, &cs.fourbytes[0]) ;
	ok = ok && getRawUInt32(data, size, offset, &cs.fourbytes[1]) ;
	ok = ok && getRawUInt32(data, size, offset, &cs.fourbytes[2]) ;
	ok = ok && getRawUInt32(data, size, offset, &cs.fourbytes[3]) ;
	ok = ok && getRawUInt32(data, size, offset, &cs.fourbytes[4]) ;

	return ok ;
}

bool setRawSha1(void *data,uint32_t size,uint32_t *offset,const Sha1CheckSum& cs)
{
	uint32_t len = 20 ; // SHA1 length in bytes

	if (size < *offset + len)
	{
		std::cerr << "setRawSha1() Not enough size" << std::endl;
		return false;
	}

	bool ok = true ;
	/* pack it in */
	ok = ok && setRawUInt32(data, size, offset, cs.fourbytes[0]);
	ok = ok && setRawUInt32(data, size, offset, cs.fourbytes[1]);
	ok = ok && setRawUInt32(data, size, offset, cs.fourbytes[2]);
	ok = ok && setRawUInt32(data, size, offset, cs.fourbytes[3]);
	ok = ok && setRawUInt32(data, size, offset, cs.fourbytes[4]);

	return true ;
}

bool getRawString(void *data, uint32_t size, uint32_t *offset, std::string &outStr)
{
	uint32_t len = 0;
	if (!getRawUInt32(data, size, offset, &len))
	{
                std::cerr << "getRawString() get size failed" << std::endl;
		return false;
	}

	/* check there is space for string */
	if (size < *offset + len)
	{
                std::cerr << "getRawString() not enough size" << std::endl;
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
	if (size < *offset + 4 + len)
	{
//#ifdef RSSERIAL_DEBUG
                std::cerr << "setRawString() Not enough size" << std::endl;
//#endif
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

