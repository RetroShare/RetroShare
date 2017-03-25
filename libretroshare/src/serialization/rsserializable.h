#pragma once

#include "serialiser/rsserial.h"

class RsItem ;
class SerializeContext;

#define SERIALIZE_ERROR() std::cerr << __PRETTY_FUNCTION__ << " : " 

class RsSerializable: public RsItem
{
public:
	typedef enum { SIZE_ESTIMATE = 0x01, SERIALIZE = 0x02, DESERIALIZE  = 0x03} SerializeJob ;

	RsSerializable(uint8_t version,uint16_t service,uint8_t id)
		: RsItem(version,service,id)
	{
	}

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

	virtual void serial_process(SerializeJob j,SerializeContext& ctx) = 0;
};

