#pragma once

#include <typeinfo> // for typeid

#include "util/smallobject.h"
#include "retroshare/rstypes.h"
#include "serialiser/rsserializer.h"
#include "util/stacktrace.h"

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

		/// TODO: Do this make sense with the new serialization system?
		virtual void clear() = 0;

        virtual std::ostream &print(std::ostream &out, uint16_t /* indent */ = 0)
        {
           RsGenericSerializer::SerializeContext ctx(NULL,0,RsGenericSerializer::FORMAT_BINARY,RsGenericSerializer::SERIALIZATION_FLAG_NONE);
		   serial_process(RsGenericSerializer::PRINT,ctx) ;
           return out;
		}

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
		 * TODO: This should be made pure virtual as soon as all the codebase
		 * is ported to the new serialization system
		 */
		virtual void serial_process(RsGenericSerializer::SerializeJob,
		                            RsGenericSerializer::SerializeContext&)// = 0;
		{
			std::cerr << "(EE) RsItem::serial_process() called by an item using"
			          << "new serialization classes, but not derived! Class is "
			          << typeid(*this).name() << std::endl;
			print_stacktrace();
		}

	protected:
		uint32_t type;
		RsPeerId peerId;
		uint8_t _priority_level ;
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

	virtual void clear() {}
	virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

private:
	void *data;
	uint32_t len;
};
