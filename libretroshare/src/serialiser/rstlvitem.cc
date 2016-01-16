
/*
 * libretroshare/src/serialiser: rstlvtypes.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie, Chris Parker
 * Copyright (C) 2016 Gioacchino Mazzurco <gio@eigenlab.org>
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

#include "rstlvitem.h"
#include "rstlvbase.h"
#include "util/stacktrace.h"
#include <iostream>

std::ostream &RsTlvItem::printBase(std::ostream &out, std::string clsName,
                                   uint16_t indent) const
{
	printIndent(out, indent);
	out << "RsTlvItem: " << clsName << " Size: " << TlvSize()
	    << "  ***********************" << std::endl;
	return out;
}

std::ostream &RsTlvItem::printEnd(std::ostream &out, std::string clsName,
                                  uint16_t indent) const
{
	printIndent(out, indent);
	out << "***************** " << clsName << " ****************" << std::endl;
	return out;
}

std::ostream &printIndent(std::ostream &out, uint16_t indent)
{
	for(int i = 0; i < indent; i++) out << " ";
	return out;
}

bool RsTlvItem::deserialize(const uint8_t data[], uint32_t size,
                            uint32_t &offset)
{
	std::cerr << "RsTlvItem::deserialize you must implement type safe "
	          << "deserialize for your item instead of relying on deprecated "
	          << "GetTlv that is type unsafe" << std::endl;
	print_stacktrace();
	return GetTlv((void *) data, size, &offset);
}
bool RsTlvItem::serialize(uint8_t data[], uint32_t size, uint32_t &offset) const
{
	std::cerr << "RsTlvItem::serialize you should implement type safe serialize"
	          << " for your TLV instead of relying on deprecated SetTlv" << std::endl;
	print_stacktrace();
	return SetTlv((void *)data, size, &offset);
}

uint32_t RsTlvItem::getTlvSize(const uint8_t data[])
{
	if (!data) return 0;
	uint32_t len;
	const void * from = data + TLV_HEADER_TYPE_SIZE;
	memcpy((void *)&len, from , TLV_HEADER_LEN_SIZE);
	len = ntohl(len);
	return len;
}

uint16_t RsTlvItem::getTlvType(const uint8_t data[])
{
	if (!data) return 0;
	uint16_t type;
	memcpy((void*)&type, data, TLV_HEADER_TYPE_SIZE);
	type = ntohs(type);
	return type;
}

bool RsTlvItem::skipTlv(const uint8_t data[], uint32_t size,
                               uint32_t &offset)
{
	if (!data) return false;
	if (size < offset + TLV_HEADER_SIZE) return false;

	const uint8_t * tlvstart = data + offset;
	uint32_t tlvsize = getTlvSize(tlvstart);
	uint32_t tlvend = offset + tlvsize;
	if (size < tlvend)
	{
#ifdef TLV_BASE_DEBUG
		std::cerr << "SkipUnknownTlv() FAILED - not enough space. Size: "
		          << size << " tlvsize: " << tlvsize << " tlvend: " << tlvend
		          << std::endl;
#endif
		return false;
	}

	offset = tlvend;
	return true;
}

bool RsTlvItem::setTlvHeader(uint8_t data[], uint32_t size,
                           uint32_t &offset, uint16_t type, uint32_t lenght)
{
	if ( !data || (size < offset + TLV_HEADER_SIZE) ) return false;

	uint16_t type_n = htons(type);
	uint8_t * to = data + offset;
	memcpy((void*)to, (void*)&type_n, TLV_HEADER_TYPE_SIZE);

	uint32_t len_n = htonl(lenght);
	to += TLV_HEADER_TYPE_SIZE;
	memcpy((void *)to, (void*)&len_n, TLV_HEADER_LEN_SIZE);

	offset += TLV_HEADER_SIZE;
	return true;
}

void RsAutoTlvItem::registerSerializable(RsSerializable * subitem)
{
	if(subitem) at_register.push_back(subitem);
	else
	{
		std::cerr << "RsAutoTlvItem::registerSerializable tryng to register a "
		          << "NULL pointer to subitem, this MUST NOT happen, notify "
		          << "developers ASAP" << std::endl;
	}
}

bool RsAutoTlvItem::serialize(uint8_t data[], uint32_t size,
                              uint32_t &offset) const
{
	if ( !data || (size < offset + TLV_HEADER_SIZE) ) return false;

	uint32_t origOffset = offset;
	offset += TLV_HEADER_SIZE; // skip header ATM

	bool ok = true;
	std::vector<RsSerializable*>::const_iterator it;
	for( it = at_register.begin(); ok && it != at_register.end(); ++it)
		ok &= (*it)->serialize(data, size, offset);

	ok &= at_type.serialize(data, TLV_HEADER_TYPE_SIZE, origOffset);

	RsSerializableUInt32 tlvSize(offset - origOffset);
	ok &= tlvSize.serialize(data, TLV_HEADER_LEN_SIZE, origOffset);

	return ok;
}
bool RsAutoTlvItem::deserialize(const uint8_t data[], uint32_t size,
                                uint32_t &offset)
{
	if ( !data || (size < offset + TLV_HEADER_SIZE) ) return false;

	uint32_t origOffest = offset;
	bool ok = true;

	RsSerializableUInt16 tlvType;
	ok &= tlvType.deserialize(data, size, offset);
	if(at_type.value && at_type.value != tlvType.value)
	{
		std::cerr << "RsAutoTlvItem::deserialize wrong TLV type got: "
		          << tlvType.value << " expected: "<< at_type.value << std::endl;
		return skipTlv(data, size, origOffest);
	}
	else at_type.value = tlvType.value;

	RsSerializableUInt32 tlvSize;
	ok &= tlvSize.deserialize(data, size, offset);
	uint32_t tlvEnd = offset + tlvSize.value;
	if (size < tlvEnd) return false; // not enough space

	std::vector<RsSerializable*>::const_iterator it;
	for(it = at_register.begin(); ok && it != at_register.end(); ++it)
		ok &= (*it)->deserialize(data, size, offset);

	if (offset != tlvEnd)
	{
		std::cerr << "RsTlvIpAddressInfo::GetTlv() Warning extra bytes at end "
		          << "of item" << std::endl;
		offset = tlvEnd;
	}

	return ok;
}
uint32_t RsAutoTlvItem::serializedSize() const
{
	uint32_t accumulator = TLV_HEADER_SIZE;
	std::vector<RsSerializable*>::const_iterator it;
	for(it = at_register.begin(); it != at_register.end(); ++it)
		accumulator += (*it)->serializedSize();
	return accumulator;
}

bool RsSerializableUInt8::serialize(uint8_t data[], uint32_t size,
                                    uint32_t &offset) const
{
	if (size < offset + 1) return false;
	data[1] = value;
	offset += 1;
	return true;
}
bool RsSerializableUInt8::deserialize(const uint8_t data[], uint32_t size,
                                      uint32_t &offset)
{
	if (size < offset + 1) return false;
	value = data[offset];
	offset += 1;
	return true;
}

bool RsSerializableUInt16::serialize(uint8_t data[], uint32_t size,
                                     uint32_t &offset) const
{
	if (size < offset + 2) return false;
	uint16_t tmp = htons(value);
	memcpy(data+offset, &tmp, 2);
	offset += 2;
	return true;
}
bool RsSerializableUInt16::deserialize(const uint8_t data[], uint32_t size,
                                       uint32_t &offset)
{
	if (size < offset + 2) return false; /* first check there is space */
	memcpy(&value, data+offset, 2);
	value = ntohs(value);
	offset += 2;
	return true;
}

bool RsSerializableUInt32::serialize(uint8_t data[], uint32_t size,
                                     uint32_t &offset) const
{
	if (size < offset + 4) return false; // first check there is space
	uint32_t tmp = htonl(value); // convert the data to the right format
	memcpy(data+offset, &tmp, 4);
	offset += 4;
	return true;
}
bool RsSerializableUInt32::deserialize(const uint8_t data[], uint32_t size,
                                       uint32_t &offset)
{
	if (size < offset + 4) return false;
	memcpy(&value, data+offset, 4);
	value = ntohl(value);
	offset += 4;
	return true;
}

bool RsSerializableUInt64::serialize(uint8_t data[], uint32_t size,
                                     uint32_t &offset) const
{
	if (size < offset + 8) return false; // first check there is space
	uint64_t tmp = htonll(value); // convert the data to the right format
	memcpy(data+offset, &tmp, 8);
	offset += 8;
	return true;
}
bool RsSerializableUInt64::deserialize(const uint8_t data[], uint32_t size,
                                       uint32_t &offset)
{
	if (size < offset + 8) return false;
	memcpy(&value, data+offset, 8);
	value = ntohll(value);
	offset += 8;
	return true;
}

bool RsSerializableUFloat32::serialize(uint8_t data[], uint32_t size,
                                       uint32_t &offset) const
{
	if (size < offset + 4) return false; // first check there is space
	if(value < 0.0f)
	{
		std::cerr << "(EE) Cannot serialise invalid negative float value "
		          << value << " in " << __PRETTY_FUNCTION__ << std::endl;
		return false;
	}

	/* This serialisation is quite accurate. The max relative error is approx.
	 * 0.01% and most of the time less than 1e-05% The error is well distributed
	 * over numbers also. */
	RsSerializableUInt32 n;
	if(value < 1e-7) n.value = (~(uint32_t)0);
	else n.value = ((uint32_t)( (1.0f/(1.0f+value) * (~(uint32_t)0))));

	return n.serialize(data, size, offset);
}
bool RsSerializableUFloat32::deserialize(const uint8_t data[], uint32_t size,
                                         uint32_t &offset)
{
	RsSerializableUInt32 n;
	if(!n.deserialize(data, size, offset)) return false;
	value = 1.0f/ ( n.value/(float)(~(uint32_t)0)) - 1.0f;
	return true;
}

bool RsSerializableTimeT::serialize(uint8_t data[], uint32_t size,
                                       uint32_t &offset) const
{
	RsSerializableUInt64 n(value);
	return n.serialize(data, size, offset);
}

bool RsSerializableTimeT::deserialize(const uint8_t data[], uint32_t size,
                                      uint32_t &offset)
{
	RsSerializableUInt64 n;
	bool ok = n.deserialize(data, size, offset);
	value = n.value;
	return ok;
}

bool RsSerializableString::serialize(uint8_t data[], uint32_t size,
                                     uint32_t &offset) const
{
	RsSerializableUInt32 len(value.length());
	if (size < offset + 4 + len.value) return false;
	if (!len.serialize(data, size, offset)) return false;
	memcpy(data+offset, value.c_str(), len.value);
	offset += len.value;
	return true;
}
bool RsSerializableString::deserialize(const uint8_t data[], uint32_t size,
                                 uint32_t &offset)
{
	RsSerializableUInt32 strLen;
	if (!strLen.deserialize(data, size, offset)) return false;
	if (size < offset + strLen.value) return false;
	value.clear();
	value.insert(0, (char*)data+offset, strLen.value);
	offset += strLen.value;
	return true;
}
