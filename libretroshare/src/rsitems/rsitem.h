#pragma once

#include "util/smallobject.h"
#include "retroshare/rstypes.h"
#include "serialization/rsserializer.h"

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

		virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */)
		{
			std::cerr << "(EE) RsItem::serial_process() called by an item using new serialization classes, but not derived! Class is " << typeid(*this).name() << std::endl;
		}

	protected:
		uint32_t type;
		RsPeerId peerId;
		uint8_t _priority_level ;
};

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
