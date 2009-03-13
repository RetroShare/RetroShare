
/*
 * libretroshare/src/serialiser: rstlvbase.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie, Horatio, Chris Parker
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

#include <iostream>

#include "serialiser/rstlvbase.h"
#include "serialiser/rsbaseserial.h"

/*******************************************************************
 * These are the general TLV (un)packing routines.
 *
 * Data is Serialised into the following format
 *
 * -----------------------------------------
 * | TLV TYPE (2 bytes)| TLV LEN (2 bytes) |
 * -----------------------------------------
 * |                                       |
 * |         Data ....                     |
 * |                                       |
 * -----------------------------------------
 *
 * Size is the total size of the TLV Field (including the 4 byte header)
 *
 * Like the lowlevel packing routines. They are usually 
 * created in pairs - one to pack the data, the other to unpack.
 *
 * GetTlvXXX(void *data, uint32_t size, uint32_t *offset, XXX *out);
 * SetTlvXXX(void *data, uint32_t size, uint32_t *offset, XXX *in);
 *
 *
 * data - the base pointer to the serialised data.
 * size - size of the memory pointed to by data.
 * *offset - where we want to (un)pack the data.
 * 		This is incremented by the datasize.
 *
 * *in / *out - the data to (un)pack.
 *
 ******************************************************************/

//*********************

// A facility func
inline void* right_shift_void_pointer(void* p, uint32_t len) {

	return (void*)( (uint8_t*)p + len);
}
//*********************

#define TLV_BASE_DEBUG 1


const uint32_t TYPE_FIELD_BYTES = 2;

/**** Basic TLV Functions ****/
uint16_t GetTlvSize(void *data) {
	if (!data)
		return 0;

	uint16_t len;

	void * from =right_shift_void_pointer(data, sizeof(uint16_t));

	memcpy((void *)&len, from , sizeof(uint16_t));

	len = ntohs(len);

	return len;
}

uint16_t GetTlvType(void *data) {
	if (!data)
		return 0;

	uint16_t type;

	memcpy((void*)&type, data, TYPE_FIELD_BYTES);

	type = ntohs(type);

	return type;

}

//tested
bool SetTlvBase(void *data, uint32_t size, uint32_t *offset, uint16_t type,
		uint16_t len) {
	if (!data)
		return false;
	if (!offset)
		return false;
	if (size < *offset +4)
		return false;

	uint16_t type_n = htons(type);

	//copy type_n to (data+*offset)
	void* to = right_shift_void_pointer(data, *offset);
	memcpy(to , (void*)&type_n, sizeof(uint16_t));

	uint16_t len_n =htons(len);
	//copy len_n to (data + *offset +2)
	to = right_shift_void_pointer(to, sizeof(uint16_t));
	memcpy((void *)to, (void*)&len_n, sizeof(uint16_t));

	*offset += sizeof(uint16_t)*2;

	return true;
}

//tested
bool SetTlvSize(void *data, uint32_t size, uint16_t len) {
	if (!data)
		return false;

	if(size < sizeof(uint16_t)*2 )
		return false;
	
	uint16_t len_n = htons(len);

	void * to = (void*)((uint8_t *) data + sizeof(uint16_t));

	memcpy(to, (void*) &len_n, sizeof(uint16_t));

	return true;

}

/**** Generic TLV Functions ****
 * This have the same data (int or string for example), 
 * but they can have different types eg. a string could represent a name or a path, 
 * so we include a type parameter in the arguments
 */
//tested
bool SetTlvUInt32(void *data, uint32_t size, uint32_t *offset, uint16_t type,
		uint32_t out) 
{
	if (!data)
		return false;
	uint16_t tlvsize = GetTlvUInt32Size(); /* this will always be 8 bytes */
	uint32_t tlvend = *offset + tlvsize; /* where the data will extend to */
	if (size < tlvend)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "SetTlvUInt32() FAILED - not enough space. (or earlier)" << std::endl;
		std::cerr << "SetTlvUInt32() size: " << size << std::endl;
		std::cerr << "SetTlvUInt32() tlvsize: " << tlvsize << std::endl;
		std::cerr << "SetTlvUInt32() tlvend: " << tlvend << std::endl;
#endif
		return false;
	}

	bool ok = true;

	/* Now use the function we got to set the TlvHeader */
	/* function shifts offset to the new start for the next data */
	ok &= SetTlvBase(data, tlvend, offset, type, tlvsize);

#ifdef TLV_BASE_DEBUG
	if (!ok)
	{
		std::cerr << "SetTlvUInt32() SetTlvBase FAILED (or earlier)" << std::endl;
	}
#endif

	/* now set the UInt32 ( in rsbaseserial.h???) */
	ok &= setRawUInt32(data, tlvend, offset, out);

#ifdef TLV_BASE_DEBUG
	if (!ok)
	{
		std::cerr << "SetTlvUInt32() setRawUInt32 FAILED (or earlier)" << std::endl;
	}
#endif


	return ok;

}

//tested
bool GetTlvUInt32(void *data, uint32_t size, uint32_t *offset, 
		uint16_t type, uint32_t *in) 
{
	if (!data)
		return false;

	if (size < *offset + 4)
		return false;

	/* extract the type and size */
	void *tlvstart = right_shift_void_pointer(data, *offset);
	uint16_t tlvtype = GetTlvType(tlvstart);
	uint16_t tlvsize = GetTlvSize(tlvstart);

	/* check that there is size */
	uint32_t tlvend = *offset + tlvsize;
	if (size < tlvend)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "GetTlvUInt32() FAILED - not enough space." << std::endl;
		std::cerr << "GetTlvUInt32() size: " << size << std::endl;
		std::cerr << "GetTlvUInt32() tlvsize: " << tlvsize << std::endl;
		std::cerr << "GetTlvUInt32() tlvend: " << tlvend << std::endl;
#endif
		return false;
	}

	if (type != tlvtype)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "GetTlvUInt32() FAILED - Type mismatch" << std::endl;
		std::cerr << "GetTlvUInt32() type: " << type << std::endl;
		std::cerr << "GetTlvUInt32() tlvtype: " << tlvtype << std::endl;
#endif
		return false;
	}

	*offset += 4; /* step past header */

	bool ok = true;
	ok &= getRawUInt32(data, tlvend, offset, in);

	return ok;
}


uint32_t GetTlvUInt64Size() {
	return 4 + 8;
}

uint32_t GetTlvUInt32Size() {
	return 4 + 4;
}

uint32_t GetTlvUInt16Size() {
	return 4 + sizeof(uint16_t);

}

uint32_t GetTlvUInt8Size() {
	return 4 + sizeof(uint8_t);

}


bool SetTlvUInt64(void *data, uint32_t size, uint32_t *offset, uint16_t type,
		uint64_t out) 
{
	if (!data)
		return false;
	uint16_t tlvsize = GetTlvUInt64Size(); 
	uint32_t tlvend = *offset + tlvsize; 
	if (size < tlvend)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "SetTlvUInt64() FAILED - not enough space. (or earlier)" << std::endl;
		std::cerr << "SetTlvUInt64() size: " << size << std::endl;
		std::cerr << "SetTlvUInt64() tlvsize: " << tlvsize << std::endl;
		std::cerr << "SetTlvUInt64() tlvend: " << tlvend << std::endl;
#endif
		return false;
	}

	bool ok = true;

	/* Now use the function we got to set the TlvHeader */
	/* function shifts offset to the new start for the next data */
	ok &= SetTlvBase(data, tlvend, offset, type, tlvsize);

#ifdef TLV_BASE_DEBUG
	if (!ok)
	{
		std::cerr << "SetTlvUInt64() SetTlvBase FAILED (or earlier)" << std::endl;
	}
#endif

	/* now set the UInt64 ( in rsbaseserial.h???) */
	ok &= setRawUInt64(data, tlvend, offset, out);

#ifdef TLV_BASE_DEBUG
	if (!ok)
	{
		std::cerr << "SetTlvUInt64() setRawUInt64 FAILED (or earlier)" << std::endl;
	}
#endif

	return ok;

}

bool GetTlvUInt64(void *data, uint32_t size, uint32_t *offset, 
		uint16_t type, uint64_t *in) 
{
	if (!data)
		return false;

	if (size < *offset + 4)
		return false;

	/* extract the type and size */
	void *tlvstart = right_shift_void_pointer(data, *offset);
	uint16_t tlvtype = GetTlvType(tlvstart);
	uint16_t tlvsize = GetTlvSize(tlvstart);

	/* check that there is size */
	uint32_t tlvend = *offset + tlvsize;
	if (size < tlvend)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "GetTlvUInt64() FAILED - not enough space." << std::endl;
		std::cerr << "GetTlvUInt64() size: " << size << std::endl;
		std::cerr << "GetTlvUInt64() tlvsize: " << tlvsize << std::endl;
		std::cerr << "GetTlvUInt64() tlvend: " << tlvend << std::endl;
#endif
		return false;
	}

	if (type != tlvtype)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "GetTlvUInt64() FAILED - Type mismatch" << std::endl;
		std::cerr << "GetTlvUInt64() type: " << type << std::endl;
		std::cerr << "GetTlvUInt64() tlvtype: " << tlvtype << std::endl;
#endif
		return false;
	}

	*offset += 4; /* step past header */

	bool ok = true;
	ok &= getRawUInt64(data, tlvend, offset, in);

	return ok;
}


bool SetTlvString(void *data, uint32_t size, uint32_t *offset, 
			uint16_t type, std::string out) 
{
	if (!data)
		return false;
	uint16_t tlvsize = GetTlvStringSize(out); 
	uint32_t tlvend = *offset + tlvsize; /* where the data will extend to */

	if (size < tlvend)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "SetTlvString() FAILED - not enough space" << std::endl;
		std::cerr << "SetTlvString() size: " << size << std::endl;
		std::cerr << "SetTlvString() tlvsize: " << tlvsize << std::endl;
		std::cerr << "SetTlvString() tlvend: " << tlvend << std::endl;
#endif
		return false;
	}

	bool ok = true;
	ok &= SetTlvBase(data, tlvend, offset, type, tlvsize);

	void * to  = right_shift_void_pointer(data, *offset);

	uint16_t strlen = tlvsize - 4;
	memcpy(to, out.c_str(), strlen);

	*offset += strlen;

	return ok;
}

//tested
bool GetTlvString(void *data, uint32_t size, uint32_t *offset, 
			uint16_t type, std::string &in) 
{
	if (!data)
		return false;

	if (size < *offset + 4)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "GetTlvString() FAILED - not enough space" << std::endl;
		std::cerr << "GetTlvString() size: " << size << std::endl;
		std::cerr << "GetTlvString() *offset: " << *offset << std::endl;
#endif
		return false;
	}

	/* extract the type and size */
	void *tlvstart = right_shift_void_pointer(data, *offset);
	uint16_t tlvtype = GetTlvType(tlvstart);
	uint16_t tlvsize = GetTlvSize(tlvstart);

	/* check that there is size */
	uint32_t tlvend = *offset + tlvsize;
	if (size < tlvend)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "GetTlvString() FAILED - not enough space" << std::endl;
		std::cerr << "GetTlvString() size: " << size << std::endl;
		std::cerr << "GetTlvString() tlvsize: " << tlvsize << std::endl;
		std::cerr << "GetTlvString() tlvend: " << tlvend << std::endl;
#endif
		return false;
	}

	if (type != tlvtype)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "GetTlvString() FAILED - invalid type" << std::endl;
		std::cerr << "GetTlvString() type: " << type << std::endl;
		std::cerr << "GetTlvString() tlvtype: " << tlvtype << std::endl;
#endif
		return false;
	}

	char *strdata = (char *) right_shift_void_pointer(tlvstart, 4);
	uint16_t strsize = tlvsize - 4; /* remove the header */
	in = std::string(strdata, strsize);

	*offset += tlvsize; /* step along */
	return true;
}

uint32_t GetTlvStringSize(const std::string &in) {
	return 4 + in.size();
}


/* We must use a consistent wchar size for cross platform ness.
 * As unix uses 4bytes, and windows 2bytes? we'll go with 4bytes for maximum flexibility
 */

const uint32_t RS_WCHAR_SIZE = 4;

bool SetTlvWideString(void *data, uint32_t size, uint32_t *offset, 
			uint16_t type, std::wstring out) 
{
	if (!data)
		return false;
	uint16_t tlvsize = GetTlvWideStringSize(out); 
	uint32_t tlvend = *offset + tlvsize; /* where the data will extend to */

	if (size < tlvend)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "SetTlvWideString() FAILED - not enough space" << std::endl;
		std::cerr << "SetTlvWideString() size: " << size << std::endl;
		std::cerr << "SetTlvWideString() tlvsize: " << tlvsize << std::endl;
		std::cerr << "SetTlvWideString() tlvend: " << tlvend << std::endl;
#endif
		return false;
	}

	bool ok = true;
	ok &= SetTlvBase(data, tlvend, offset, type, tlvsize);

	uint16_t strlen = out.length();

	/* Must convert manually to ensure its always the same! */
	for(uint16_t i = 0; i < strlen; i++)
	{
		uint32_t widechar = out[i];
		ok &= setRawUInt32(data, tlvend, offset, widechar);
	}
	return ok;
}

//tested
bool GetTlvWideString(void *data, uint32_t size, uint32_t *offset, 
			uint16_t type, std::wstring &in) 
{
	if (!data)
		return false;

	if (size < *offset + 4)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "GetTlvWideString() FAILED - not enough space" << std::endl;
		std::cerr << "GetTlvWideString() size: " << size << std::endl;
		std::cerr << "GetTlvWideString() *offset: " << *offset << std::endl;
#endif
		return false;
	}

	/* extract the type and size */
	void *tlvstart = right_shift_void_pointer(data, *offset);
	uint16_t tlvtype = GetTlvType(tlvstart);
	uint16_t tlvsize = GetTlvSize(tlvstart);

	/* check that there is size */
	uint32_t tlvend = *offset + tlvsize;
	if (size < tlvend)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "GetTlvWideString() FAILED - not enough space" << std::endl;
		std::cerr << "GetTlvWideString() size: " << size << std::endl;
		std::cerr << "GetTlvWideString() tlvsize: " << tlvsize << std::endl;
		std::cerr << "GetTlvWideString() tlvend: " << tlvend << std::endl;
#endif
		return false;
	}

	if (type != tlvtype)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "GetTlvWideString() FAILED - invalid type" << std::endl;
		std::cerr << "GetTlvWideString() type: " << type << std::endl;
		std::cerr << "GetTlvWideString() tlvtype: " << tlvtype << std::endl;
#endif
		return false;
	}


	bool ok = true;
	/* remove the header, calc string length */
	*offset += 4;
	uint16_t strlen = (tlvsize - 4) / RS_WCHAR_SIZE; 

	/* Must convert manually to ensure its always the same! */
	for(uint16_t i = 0; i < strlen; i++)
	{
		uint32_t widechar;
		ok &= getRawUInt32(data, tlvend, offset, &widechar);
		in += widechar;
	}
	return ok;
}

uint32_t GetTlvWideStringSize(std::wstring &in) {
	return 4 + in.size() * RS_WCHAR_SIZE;
}


bool SetTlvIpAddrPortV4(void *data, uint32_t size, uint32_t *offset,
		uint16_t type, struct sockaddr_in *out) {
	if (!data)
		return false;

	uint16_t tlvsize = GetTlvIpAddrPortV4Size();
	uint32_t tlvend = *offset + tlvsize; /* where the data will extend to */

	if (size < tlvend)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "SetTlvIpAddrPortV4() FAILED - not enough space" << std::endl;
		std::cerr << "SetTlvIpAddrPortV4() size: " << size << std::endl;
		std::cerr << "SetTlvIpAddrPortV4() tlvsize: " << tlvsize << std::endl;
		std::cerr << "SetTlvIpAddrPortV4() tlvend: " << tlvend << std::endl;
#endif
		return false;
	}

	bool ok = true;
	ok &= SetTlvBase(data, tlvend, offset, type, tlvsize);

	/* now add the data .... (its already in network order) - so flip */
	uint32_t ipaddr = out->sin_addr.s_addr;
	ok &= setRawUInt32(data, tlvend, offset, ntohl(ipaddr));

	uint16_t port = out->sin_port;
	ok &= setRawUInt16(data, tlvend, offset, ntohs(port));

	return ok;
}

bool GetTlvIpAddrPortV4(void *data, uint32_t size, uint32_t *offset,
		uint16_t type, struct sockaddr_in *in) {
	if (!data)
		return false;

	if (size < *offset + 4)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "GetIpAddrPortV4() FAILED - not enough space" << std::endl;
		std::cerr << "GetIpAddrPortV4() size: " << size << std::endl;
		std::cerr << "GetIpAddrPortV4() *offset: " << *offset << std::endl;
#endif
		return false;
	}

	/* extract the type and size */
	void *tlvstart = right_shift_void_pointer(data, *offset);
	uint16_t tlvtype = GetTlvType(tlvstart);
	uint16_t tlvsize = GetTlvSize(tlvstart);

	/* check that there is size */
	uint32_t tlvend = *offset + tlvsize;
	if (size < tlvend)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "GetIpAddrPortV4() FAILED - not enough space" << std::endl;
		std::cerr << "GetIpAddrPortV4() size: " << size << std::endl;
		std::cerr << "GetIpAddrPortV4() tlvsize: " << tlvsize << std::endl;
		std::cerr << "GetIpAddrPortV4() tlvend: " << tlvend << std::endl;
#endif
		return false;
	}

	if (type != tlvtype)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "GetIpAddrPortV4() FAILED - invalid type" << std::endl;
		std::cerr << "GetIpAddrPortV4() type: " << type << std::endl;
		std::cerr << "GetIpAddrPortV4() tlvtype: " << tlvtype << std::endl;
#endif
		return false;
	}

	*offset += 4; /* skip header */

	bool ok = true;

	/* now get the data .... (its already in network order) - so flip */

	uint32_t ipaddr;
	ok &= getRawUInt32(data, tlvend, offset, &ipaddr);
	in->sin_family = AF_INET; /* set FAMILY */
	in->sin_addr.s_addr = htonl(ipaddr);

	uint16_t port;
	ok &= getRawUInt16(data, tlvend, offset, &port);
	in->sin_port = htons(port);

	return ok;
}

uint32_t GetTlvIpAddrPortV4Size() {
	return 4 + 4 + 2; /* header + 4 (IP) + 2 (Port) */
}

