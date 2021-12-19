/*******************************************************************************
 * libretroshare/src/rsitems: rsitem.h                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2018-2021  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>       *
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
#pragma once

#include <typeinfo> // for typeid

#include "util/smallobject.h"
#include "retroshare/rstypes.h"
#include "serialiser/rsserializer.h"
#include "serialiser/rsserializable.h"
#include "util/stacktrace.h"
#include "rsitems/itempriorities.h"
#include "rsitems/rsserviceids.h"


#include <typeinfo>

struct RsItem : RsMemoryManagement::SmallObject, RsSerializable
{
	explicit RsItem(uint32_t t);
	RsItem(uint8_t ver, uint8_t cls, uint8_t t, uint8_t subtype);
#ifdef DO_STATISTICS
	void *operator new(size_t s) ;
	void operator delete(void *,size_t s) ;
#endif

	virtual ~RsItem();

	/** TODO: Does the existence of this method make sense with the new
	 * serialization system? **/
	virtual void clear()
	{
		RS_ERR("Called without being overridden, report to developers");
		print_stacktrace();
	}

	/// @deprecated use << ostream operator instead
	RS_DEPRECATED_FOR("<< ostream operator")
	virtual std::ostream &print(std::ostream &out, uint16_t /* indent */ = 0)
	{
		RsGenericSerializer::SerializeContext ctx(
		            nullptr, 0, RsSerializationFlags::NONE );
		serial_process(RsGenericSerializer::PRINT,ctx);
		return out;
	}

	void print_string(std::string &out, uint16_t indent = 0);

	/// source / destination id
	const RsPeerId& PeerId() const { return peerId; }
	void PeerId(const RsPeerId& id) { peerId = id; }

	/// complete id
	uint32_t PacketId() const;

	/// id parts
	uint8_t PacketVersion();
	uint8_t PacketClass();
	uint8_t PacketType();
	uint8_t PacketSubType() const;

	/** For Service Packets, @deprecated use the costructor with priority
	 * paramether instead */
	RS_DEPRECATED RsItem(uint8_t ver, uint16_t service, uint8_t subtype);

	/// For Service Packets
	RsItem( uint8_t ver, RsServiceType service, uint8_t subtype,
	        RsItemPriority prio );

	uint16_t PacketService() const; /* combined Packet class/type (mid 16bits) */
	void setPacketService(uint16_t service);

	inline uint8_t priority_level() const { return _priority_level ;}
	inline void setPriorityLevel(uint8_t l) { _priority_level = l ;}

#ifdef RS_DEAD_CODE
	/*
	 * TODO: This default implementation should be removed and childs structs
	 * implement ::serial_process(...) as soon as all the codebase is ported to
	 * the new serialization system
	 */
	virtual void serial_process(RsGenericSerializer::SerializeJob,
	                            RsGenericSerializer::SerializeContext&)// = 0;
	{
		RS_ERR( "called by an item using new serialization system without "
		        "overriding Class is: ", typeid(*this).name() );
		print_stacktrace();
	}
#endif //def RS_DEAD_CODE

protected:
	uint32_t type;
	RsPeerId peerId;
	RsItemPriority _priority_level;
};

/// TODO: Do this make sense with the new serialization system?
class RsRawItem: public RsItem
{
public:
	RsRawItem(uint32_t t, uint32_t size) : RsItem(t), len(size)
	{ data = rs_malloc(len); }
	virtual ~RsRawItem() { free(data); }

	uint32_t getRawLength() { return len; }
	void * getRawData() { return data; }

//	virtual void clear() override {}
	virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

	virtual void serial_process(RsGenericSerializer::SerializeJob,
	                            RsGenericSerializer::SerializeContext&) override
	{
		RS_ERR( "called by an item using new serialization system ",
		        typeid(*this).name() );
		print_stacktrace();
	}

private:
	void *data;
	uint32_t len;
};
