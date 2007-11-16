
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

#include <netinet/in.h>
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

	uint32_t *intdata = (uint32_t *) right_shift_void_pointer(tlvstart, 4);
	*in = ntohl(*intdata);

	*offset += tlvsize; /* step along */
	return true;
}

uint32_t GetTlvUInt32Size() {
	return 8;
}

uint32_t GetTlvUInt16Size() {
	return sizeof(uint16_t);

}

uint32_t GetTlvUInt8Size() {
	return sizeof(uint8_t);

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

uint32_t GetTlvStringSize(std::string &in) {
	return 4 + in.size();
}

//How to handle structure sockaddr_in? Convert it to void*?
bool SetTlvIpAddrPortV4(void *data, uint32_t size, uint32_t *offset,
		uint16_t type, struct sockaddr_in *out) {
	if (!data)
		return false;

	if (size < *offset + sizeof(uint16_t)*2+ sizeof(sockaddr_in))
		return false;

	//	struct sockaddr_in out_n = *out;
	//	out_n. sin_port = htons(out_n.sin_port);
	//	out_n.sin_addr = htonl(out_n.sin_addr);

	void * to = right_shift_void_pointer(data, *offset);
	memcpy(to, (void *)&type, sizeof(uint16_t));

	to = right_shift_void_pointer(to, sizeof(uint16_t));
	uint16_t len = sizeof(sockaddr_in);
	memcpy(to, (void*)&len, sizeof(uint16_t));

	to = right_shift_void_pointer(to, sizeof(uint16_t));
	memcpy(to, (void *)out, sizeof(sockaddr_in));

	*offset += sizeof(uint16_t)*2+ sizeof(sockaddr_in);

	return true;

}

bool GetTlvIpAddrPortV4(void *data, uint32_t size, uint32_t *offset,
		uint16_t type, struct sockaddr_in *in) {
	if (!data)
		return false;

	if (size < *offset + sizeof(uint16_t)*2+ sizeof(sockaddr_in))
		return false;

	void * from = right_shift_void_pointer(data, *offset +2);

	uint16_t len;
	memcpy((void *)&len, from, sizeof(uint16_t));
	len = ntohs(len);

	if (len != sizeof(sockaddr_in))
		return false;

	from = right_shift_void_pointer(from, sizeof(uint16_t));

	memcpy((void*)in, from, sizeof(sockaddr_in));

	*offset += sizeof(uint16_t)*2+ len;
	return true;

}

//intention?
bool GetTlvIpAddrPortV4Size() {
	return sizeof(uint16_t)*2+ sizeof(sockaddr_in);
}

bool SetRawUInt32(void* data, uint32_t size, uint32_t* offset, uint32_t out) {
	if (!data)
		return false;
	if (size < *offset + sizeof(uint32_t))
		return false;

	uint32_t out_n = htonl(out);

	memcpy(right_shift_void_pointer(data, *offset), (void*)&out_n,
			sizeof(uint32_t));

	*offset += sizeof(uint32_t);
	return true;
}

// additional serializer of data
/*
bool SetTlvBinData(void* data, uint32_t size, uint32_t* offset, uint16_t type, void* data_bin, uint32_t len_tlv)
 {
	if (!data)
		return false;
	if (size < *offset + len)
		return false;

	setTlvBase(data, size, offset, TLV_TYPE_DATA, len_tlv);

	memcpy(right_shift_void_pointer(data, *offset), data_bin,
			len);

	*offset += len;
	return true;
}

*/

