#pragma once
/*
 * libretroshare/src/serialiser: rstlvitem.h
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

#include <iosfwd>
#include <string>
#include <inttypes.h>
#include <vector>

#include "serialiser/rstlvbase.h"

/**
 * @brief The RsSerializable Interface
 * This interface is supposed to be implemented by serializable items in
 * RetroShare like RsTlvItem and RsSerializableUInt32. To inherit from
 * RsSerializable doesn't imply that the serialzation will be in TLV format in
 * fact RsSerializableUInt32 and similar that are meant as building block of
 * RsTlvItem and derivatives serialize as raw data not as TLV.
 * If you are about to implement a new TLV you don't need to inherit direclty
 * from RsSerializable as more functionalities you are probably interested in
 * are already provided by @see RsAutoTlvItem.
 */
class RsSerializable
{
public:
	/**
	 * @brief serialize this object to the given buffer
	 * @param data Chunk of memory were to dump the serialized data
	 * @param size Size of memory chunk
	 * @param offset Readed to determine at witch offset start writing data,
	 *        written to inform caller were written data ends, the updated value
	 *        is usually passed by the caller to serialize of another
	 *        RsSerializable so it can write on the same chunk of memory just
	 *        after where this RsSerializable has been serialized.
	 * @return true if serialization successed, false otherwise
	 */
	virtual bool serialize(uint8_t data[], uint32_t size,
	                       uint32_t &offset) const = 0;

	/**
	 * @brief deserialize Populate members by deserializing the given buffer
	 * @param data Readonly data to read to generate the deserialized object
	 * @param size Size of memory chunk
	 * @param offset Readed to determine at witch offset start reading data,
	 *        written to inform caller were read ended, the updated value is
	 *        usually passed by the caller to deserialize of another RsTlvItem
	 *        so it can read the same chunk of memory just after where the
	 *        deserialization of this RsTlvItem has ended.
	 * @return true if deserialization succeded false otherwise
	 */
	virtual bool deserialize(const uint8_t data[], uint32_t size,
	                         uint32_t &offset) = 0;

	/**
	 * @brief serializedSize calculate the size in bytes of minimum buffer
	 * needed to serialize this element
	 * @return the calculated size
	 */
	virtual uint32_t serializedSize() const = 0;
};

/**
 * A base class for all Type Length Value items
 * If you are about to implement a new TLV you don't need to inherit direclty
 * from RsTlvItem as more functionalities you are probably interested in are
 * already provided by @see RsAutoTlvItem
 */
class RsTlvItem : public RsSerializable
{
public:
	RsTlvItem() {}
	virtual ~RsTlvItem() {}

	/**
	 * @see RsSerializable
	 * Must be overriden by derived classes.
	 * By default it call the deprecated version for retrocompatibility but this
	 * should be made pure virtual in future.
	 * @return the calculated size
	 */
	virtual uint32_t serializedSize() const { return TlvSize(); }

	/**
	 * @brief clear members and delete them if dinamically allocated.
	 * Must be overriden by derived classes.
	 * By default it call the deprecated version for retrocompatibility but this
	 * should be made pure virtual in future.
	 */
	virtual void clear() { TlvClear(); }

	/**
	 * @brief shallowClear is meant to clear members without deleting them if
	 * dinamically allocated.
	 * Should be overriden by derived classes.
	 * If not overridden call clear().
	 */
	virtual void shallowClear() { clear(); }

	/** @see RsSerializable
	 * By default it call the deprecated version for retrocompatibility but this
	 * should be made pure virtual in future.
	 */
	virtual bool serialize(uint8_t data[], uint32_t size, uint32_t &offset) const;

	/** @see RsSerializable
	 * By default it call the deprecated version for retrocompatibility but this
	 * should be made pure virtual in future.
	 */
	virtual bool deserialize(const uint8_t data[], uint32_t size, uint32_t &offset);

	virtual std::ostream &print(std::ostream &out, uint16_t indent) const = 0;
	std::ostream &printBase(std::ostream &out, std::string clsName, uint16_t indent) const;
	std::ostream &printEnd(std::ostream &out, std::string clsName, uint16_t indent) const;

	/**
	 * @brief getTlvType parse TLV type
	 * @param data @see deserialize
	 * @return TLV type parsed from TLV
	 */
	static uint16_t getTlvType(const uint8_t data[]);

	/**
	 * @brief getTlvSize parse TLV size
	 * @param data @see deserialize
	 * @return TLV size parsed from TLV length
	 */
	static uint32_t getTlvSize(const uint8_t data[]);

	/**
	 * @brief SkipTlv skip TLV without concerning about type or value
	 * @param data @see deserialize
	 * @param size @see deserialize
	 * @param offset @see deserialize
	 * @return true if skipping success, false otherwise
	 */
	static bool skipTlv(const uint8_t data[], uint32_t size, uint32_t &offset);

	/**
	 * @brief setTlvHeader initialize a TLV header to the given size and lenght
	 * @param data @see serialize
	 * @param size @see serialize
	 * @param offset @see serialize
	 * @param type TLV item type
	 * @param lenght TLV item lenght
	 * @return true if success, false otherwise
	 */
	static bool setTlvHeader(uint8_t data[], uint32_t size, uint32_t &offset,
	                         uint16_t type, uint32_t lenght);

	/// DEPRECATED: @see deserialize
	virtual bool GetTlv(void *data, uint32_t size, uint32_t *offset)
	{ return deserialize((const uint8_t *) data, size, *offset); }
	/// DEPRECATED: @see serialize
	virtual bool SetTlv(void *data, uint32_t size, uint32_t *offset) const
	{ return serialize((uint8_t *)data, size, *offset); }
	/// DEPRECATED @see shallowClear
	virtual	void TlvShallowClear() { shallowClear(); }
	/// DEPRECATED @see clear
	virtual void TlvClear() { clear(); }
	/// DEPRECATED @see tlvSize
	virtual uint32_t TlvSize() const { return serializedSize(); }
};

class RsSerializableUInt8 : public RsSerializable
{
public:
	RsSerializableUInt8() {}
	RsSerializableUInt8(uint8_t v) : value(v) {}
	bool serialize(uint8_t data[], uint32_t size, uint32_t &offset) const;
	bool deserialize(const uint8_t data[], uint32_t size, uint32_t &offset);
	uint32_t serializedSize() const { return 1; }
	uint8_t value;
};

class RsSerializableUInt16 : public RsSerializable
{
public:
	RsSerializableUInt16() {}
	RsSerializableUInt16(uint16_t v) : value(v) {}
	bool serialize(uint8_t data[], uint32_t size, uint32_t &offset) const;
	bool deserialize(const uint8_t data[], uint32_t size, uint32_t &offset);
	uint32_t serializedSize() const { return 2; }
	uint16_t value;
};

class RsSerializableUInt32 : public RsSerializable
{
public:
	RsSerializableUInt32() {}
	RsSerializableUInt32(uint32_t v) : value(v) {}
	bool serialize(uint8_t data[], uint32_t size, uint32_t &offset) const;
	bool deserialize(const uint8_t data[], uint32_t size, uint32_t &offset);
	uint32_t serializedSize() const { return 4; }
	uint32_t value;
};

class RsSerializableUInt64 : public RsSerializable
{
public:
	RsSerializableUInt64() {}
	RsSerializableUInt64(uint64_t v) : value(v) {}
	bool serialize(uint8_t data[], uint32_t size, uint32_t &offset) const;
	bool deserialize(const uint8_t data[], uint32_t size, uint32_t &offset);
	uint32_t serializedSize() const { return 8; }
	uint64_t value;
};

class RsSerializableUFloat32 : public RsSerializable
{
public:
	RsSerializableUFloat32() {}
	RsSerializableUFloat32(float v) : value(v) {}
	bool serialize(uint8_t data[], uint32_t size, uint32_t &offset) const;
	bool deserialize(const uint8_t data[], uint32_t size, uint32_t &offset);
	uint32_t serializedSize() const { return 4; }
	float value;
};

class RsSerializableTimeT : public RsSerializable
{
public:
	RsSerializableTimeT() {}
	RsSerializableTimeT(time_t v) : value(v) {}
	bool serialize(uint8_t data[], uint32_t size, uint32_t &offset) const;
	bool deserialize(const uint8_t data[], uint32_t size, uint32_t &offset);
	uint32_t serializedSize() const { return 8; }
	time_t value;
};

class RsSerializableString : public RsSerializable
{
public:
	RsSerializableString() {}
	RsSerializableString(const std::string& v) : value(v) {}
	bool serialize(uint8_t data[], uint32_t size, uint32_t &offset) const;
	bool deserialize(const uint8_t data[], uint32_t size, uint32_t &offset);
	uint32_t serializedSize() const { return value.length() + 4; }
	std::string value;
};

/**
 * @brief The base class for automatized [de]serialization of TLV items
 * Inherit from this class if you are about to implement a new TLV item or are
 * renewing an existing one, retrocompatibility with @see RsTlvItem is kept both
 * in terms of API and serialized format.
 * Use RsSerializable members as building blocks of your derived RsAutoTlvItem
 * then register them for automatic [de]serialization in constructor with
 * RsAutoTlvItem.registerSerializable, this way your TLV item will benefit of
 * automatic [de]serialization without writing a single line of boring
 * [de]serialization code.
 */
class RsAutoTlvItem : RsTlvItem
{
public:
	///@brief initialize at_type to 0 @see RsAutoTlvItem.at_type
	RsAutoTlvItem() : at_type(0) { registerSerializable(&at_type); }
	///@brief initialize at_type to given value @see RsAutoTlvItem.at_type
	RsAutoTlvItem(uint16_t type) : at_type(type) { registerSerializable(&at_type); }

	/**
	 * @brief register an RsSerializable member for automatic serialization
	 * @param subitem non NULL pointer to the member
	 * This method has be designed to be used with meber of the same object, but
	 * it doesn't check if this assumption is true, passing a pointer to an
	 * external RsSerializable may work but may cause unpredictable behaviour as
	 * we don't keep track of external objects.
	 */
	void registerSerializable(RsSerializable * subitem);

	/**
	 * @brief Serialize this object in TLV format automatically
	 * Serialize this object in TLV format by automatically serilizing members
	 * registered (@see RsAutoTlvItem.registerSerializable) in internal
	 * automatic [de]serialization register.
	 * When inheriting from this class you should not need to override this
	 * method.
	 * @see RsSerializable for params and return details.
	 */
	bool serialize(uint8_t data[], uint32_t size, uint32_t &offset) const;

	/**
	 * @brief Deserialize this object in TLV format automatically
	 * Deserialize this object in TLV format by automatically deserilizing
	 * members registered (@see RsAutoTlvItem.registerSerializable) in internal
	 * automatic [de]serialization register.
	 * When inheriting from this class you should not need to override this
	 * method.
	 * @see RsSerializable for params and return details.
	 */
	bool deserialize(const uint8_t data[], uint32_t size, uint32_t &offset);

	/// @see RsSerializable.serializedSize()
	uint32_t serializedSize() const;

	/**
	 * @brief at_type TLV type for this item
	 * The initialization value influence the deserialization bahaviour, if 0
	 * the serialized TLV type is not checked and at_type is setted to that
	 * type, otherwise the serialized TLV type is checked agaist at_type, if
	 * they match the deserialization continues else the TLV is skipped without
	 * concern.
	 * On serialization this value is used as serialized TLV type.
	 */
	RsSerializableUInt16 at_type;

private:
	std::vector<RsSerializable*> at_register;
};

std::ostream &printIndent(std::ostream &out, uint16_t indent);
