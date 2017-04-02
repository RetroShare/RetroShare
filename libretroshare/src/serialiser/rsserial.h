#ifndef RS_BASE_SERIALISER_H
#define RS_BASE_SERIALISER_H

/*
 * libretroshare/src/serialiser: rsserial.h
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

#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>
#include <iosfwd>
#include <stdlib.h>
#include <stdint.h>

#include "util/rsmemory.h"
#include "retroshare/rstypes.h"

/*******************************************************************
 * This is the Top-Level serialiser/deserialise, 
 *
 * Data is Serialised into the following format
 *
 * -----------------------------------------
 * |    TYPE (4 bytes) | Size (4 bytes)    |
 * -----------------------------------------
 * |                                       |
 * |         Data ....                     |
 * |                                       |
 * -----------------------------------------
 *
 * Size is the total size of the packet (including the 8 byte header)
 * Type is composed of:
 *
 * 8 bits: Version (0x01)
 * 8 bits: Class  
 * 8 bits: Type
 * 8 bits: SubType
 ******************************************************************/

#include <util/smallobject.h>
#include "itempriorities.h"

const uint8_t RS_PKT_VERSION1        = 0x01;
const uint8_t RS_PKT_VERSION_SERVICE = 0x02;

const uint8_t RS_PKT_CLASS_BASE      = 0x01;
const uint8_t RS_PKT_CLASS_CONFIG    = 0x02;

const uint8_t RS_PKT_SUBTYPE_DEFAULT = 0x01; /* if only one subtype */

class SerializeContext ;

class RsItem: public RsMemoryManagement::SmallObject
{
	public:
		RsItem(uint32_t t);
		RsItem(uint8_t ver, uint8_t cls, uint8_t t, uint8_t subtype);
#ifdef DO_STATISTICS
		void *operator new(size_t s) ;
		void operator delete(void *,size_t s) ;
#endif

		virtual ~RsItem();
		virtual void clear() = 0;
		virtual std::ostream &print(std::ostream &out, uint16_t indent = 0) = 0;
		void print_string(std::string &out, uint16_t indent = 0);

		/* source / destination id */
		const RsPeerId& PeerId() const { return peerId; }
		void        PeerId(const RsPeerId& id) { peerId = id; }

		/* complete id */
		uint32_t PacketId() const;

		/* id parts */
		uint8_t  PacketVersion();
		uint8_t  PacketClass();
		uint8_t  PacketType();
		uint8_t  PacketSubType() const;

		/* For Service Packets */
		RsItem(uint8_t ver, uint16_t service, uint8_t subtype);
		uint16_t  PacketService() const; /* combined Packet class/type (mid 16bits) */
		void    setPacketService(uint16_t service);

		inline uint8_t priority_level() const { return _priority_level ;}
		inline void setPriorityLevel(uint8_t l) { _priority_level = l ;}

		/**
		 * @brief serialize this object to the given buffer
		 * @param Job to do: serialise or deserialize.
		 * @param data Chunk of memory were to dump the serialized data
		 * @param size Size of memory chunk
		 * @param offset Readed to determine at witch offset start writing data,
		 *        written to inform caller were written data ends, the updated value
		 *        is usually passed by the caller to serialize of another
		 *        RsSerializable so it can write on the same chunk of memory just
		 *        after where this RsSerializable has been serialized.
		 * @return true if serialization successed, false otherwise
		 */
		typedef enum { SIZE_ESTIMATE = 0x01, SERIALIZE = 0x02, DESERIALIZE  = 0x03} SerializeJob ;

		virtual void serial_process(SerializeJob j,SerializeContext& ctx) 
		{
			std::cerr << "(EE) RsItem::serial_process() called by an item using new serialization classes, but not derived! " << std::endl;
		}

	private:
		uint32_t type;
		RsPeerId peerId;
		uint8_t _priority_level ;
};


class RsSerialType
{
	public:
	RsSerialType(uint32_t t); /* only uses top 24bits */
	RsSerialType(uint8_t ver, uint8_t cls, uint8_t t);
	RsSerialType(uint8_t ver, uint16_t service);

virtual     ~RsSerialType();
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);
	
uint32_t    PacketId() const;
	private:
uint32_t type;
};


class RsSerialiser
{
	public:
	RsSerialiser();
	~RsSerialiser();
	bool        addSerialType(RsSerialType *type);

	uint32_t    size(RsItem *);
	bool        serialise  (RsItem *item, void *data, uint32_t *size);
	RsItem *    deserialise(void *data, uint32_t *size);
	
	
	private:
	std::map<uint32_t, RsSerialType *> serialisers;
};

bool     setRsItemHeader(void *data, uint32_t size, uint32_t type, uint32_t pktsize);

/* Extract Header Information from Packet */
uint32_t getRsItemId(void *data);
uint32_t getRsItemSize(void *data);

uint8_t  getRsItemVersion(uint32_t type);
uint8_t  getRsItemClass(uint32_t type);
uint8_t  getRsItemType(uint32_t type);
uint8_t  getRsItemSubType(uint32_t type);

uint16_t  getRsItemService(uint32_t type);

/* size constants */
uint32_t getRsPktBaseSize();
uint32_t getRsPktMaxSize();



/* helper fns for printing */
std::ostream &printRsItemBase(std::ostream &o, std::string n, uint16_t i);
std::ostream &printRsItemEnd(std::ostream &o, std::string n, uint16_t i);

/* defined in rstlvtypes.cc - redeclared here for ease */
std::ostream &printIndent(std::ostream &out, uint16_t indent);
/* Wrapper class for data that is serialised somewhere else */

class RsRawItem: public RsItem
{
public:
	RsRawItem(uint32_t t, uint32_t size) : RsItem(t), len(size)
	{ data = rs_malloc(len); }
	virtual ~RsRawItem() { free(data); }

	uint32_t getRawLength() { return len; }
	void * getRawData() { return data; }

	virtual void clear() {}
	virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

private:
	void *data;
	uint32_t len;
};


#endif /* RS_BASE_SERIALISER_H */
