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

/* DISCLAIMER!
 *
 * This file has exceptionally long lines that are ugly, this may seems a
 * violation of libretroshare coding style but is not because those lines became
 * even uglier if breaked.
 *
 * For more information about libretroshare coding style see:
 * https://github.com/RetroShare/RetroShare/wiki/Coding
 */

/**
 * @file rsautotlvitem.h
 * @brief RetroShare automatic TLV [de]serialization.
 * When an object is serialized all type information handled by the language is
 * lost, this cause that on deserialization the code have to simply trust the
 * data and try convert it to the expected object, if everything go as expected
 * we obtain the "same" object that the other RetroShare instance sent to us,
 * but the assumption that everything goes as expected depends on the fact that
 * the two istances uses the same code and that the software has no bugs which
 * is usually not true.
 * To reduce the intrinsic unsafenes of deserialization a TLV (Type, Lenght,
 * Value) format should be adopted, for RsItem members (RsItem itself already
 * have his own format), with an unsigned 16bit field used as type, and then a
 * 32bit field used as leght. A serialized TLV would look like this:
 *
 * -----------------------------
 * |      (T) Type 2 bytes     |
 * |---------------------------|
 * |     (L) Lenght 4 bytes    |
 * |---------------------------|
 * |     (V) Value L bytes     |
 * |                           |
 * |                           |
 * -----------------------------
 *
 * Resuming a serialized RsItem containing two TLV (let's call them TLV1 and
 * TLV2 for simplicity) members would look like:
 *
 * -----------------------------
 * |    RsItem type 4 bytes    |
 * |---------------------------|
 * |   RsItem Lenght 4 bytes   |
 * |---------------------------|
 * |        T1  2 bytes        |
 * |---------------------------|
 * |        L1  4 bytes        |
 * |---------------------------|
 * |                           |
 * |        V1 L1 bytes        |
 * |                           |
 * |---------------------------|
 * |        T2  2 bytes        |
 * |---------------------------|
 * |        L2  4 bytes1       |
 * |---------------------------|
 * |                           |
 * |        V2 L2 bytes        |
 * |                           |
 * -----------------------------
 *
 * TLV type has local meaning, so there is no need to keep them unique accross
 * RetroShare code but it is suggested to keep them unique inside the same
 * RsSerializable, type check is not enforced by default to enforce it the
 * CHECK_TYPE flag must be set on the TLV type, more flags are available like
 * MANDATORY that enforces the type is checked and that the member is populated
 * if the deserialization is successful @see RsAutoTlvSerializable::TlvFlags for
 * more details about TLV flags.
 *
 * Take advantage of this is very simple the following example should be enoungh
 * to understand how to develop your own TLV:

class ExampleHomeAddressTlv : public RsAutoTlvSerializable
{
public:
	ExampleHomeAddressTlv(uint16_t type=0) : RsAutoTlvSerializable(type),
	  // street has type 1 and is mandatory
	  street("Via dei ciliegi", MANDATORY|1),
	  // street_number type is checked but is not a mandatory member
	  street_number(39, CHECK_TYPE|2),
	  // zip_code has type 3 and is mandatory
	  zip_code(47833, MANDATORY|3),
	  // door has type 4 and is not mandatory and type is not checked
	  door("", ~MANDATORY&4),
	  // You can automatically [de]serialize raw standard types supported by
	  // RsAutoTlvSerializable but without TLV checks
	  odd("")
	{
		registerSerializable(&ExampleHomeAddressTlv::street);
		registerSerializable(&ExampleHomeAddressTlv::street_number);
		registerSerializable(&ExampleHomeAddressTlv::zip_code);
		registerSerializable(&ExampleHomeAddressTlv::odd);
		//registerSerializable(&ExampleHomeAddressTlv::another_member);
	}

	RsAutoTlvWrap<std::string> street;
	RsAutoTlvWrap<uint32_t> street_number;
	RsAutoTlvWrap<uint32_t> zip_code;
	RsAutoTlvWrap<std::string> door;
	std::string odd;
	//MyRsSerializable another_member;
};

 */

#include <iosfwd>
#include <string>
#include <inttypes.h>
#include <vector>

#include "serialiser/rstlvbase.h"
#include "serialiser/rsautoserialize.h"

/**
 * @deprecated @see RsAutoTlvSerializable instead
 *
 * @brief A base class for Type Length Value items
 * If you are about to implement a new TLV you should <b>not</b> inherit
 * direclty from RsTlvItem as this class is kept just for retrocompatibility
 * purpose and should be not used anymore @see RsAutoTlvSerializable instead.
 */
class RsTlvItem : public RsSerializable
{
public:
	RsTlvItem() {}
	virtual ~RsTlvItem() {}
	virtual uint32_t TlvSize() const;

	/**
	 * @brief clear members and delete them if dinamically allocated.
	 * Must be overriden by derived classes.
	 * By default it call the deprecated version for retrocompatibility but this
	 * should be made pure virtual in future.
	 */
	virtual void TlvClear();

	/**
	 * @brief shallowClear is meant to clear members without deleting them if
	 * dinamically allocated.
	 * Should be overriden by derived classes.
	 * If not overridden call clear().
	 */
	virtual	void TlvShallowClear() { TlvClear(); }

	virtual bool GetTlv(void *data, uint32_t size, uint32_t *offset)
	{ return deserialize((const uint8_t *) data, size, *offset); }
	virtual bool SetTlv(void *data, uint32_t size, uint32_t *offset) const
	{ return serialize((uint8_t *)data, size, *offset); }
	virtual std::ostream &print(std::ostream &out, uint16_t indent) const;
	std::ostream &printBase(std::ostream &out, std::string clsName, uint16_t indent) const;
	std::ostream &printEnd(std::ostream &out, std::string clsName, uint16_t indent) const;

	/// Forward compatibility with @see RsSerializable
	virtual uint32_t serialSize() const;
	virtual bool serialize(uint8_t data[], uint32_t size, uint32_t &offset) const;
	virtual bool deserialize(const uint8_t data[], uint32_t size, uint32_t &offset);
};

/**
 * @brief The base class for automatized TLV [de]serialization.
 * Inherit from this class if you are about to implement a new TLV serializable
 * or are renewing an existing one, retrocompatibility with @see RsTlvItem is
 * kept both in terms of API and serialized format.
 * Use class members as building blocks of your derived RsAutoTlvSerializable
 * then register them for automatic [de]serialization in constructor with
 * registerSerializable(...), this way your class will benefit of automatic TLV
 * [de]serialization without writing a single line of boring code.
 * In addition to what provided by @see RsAutoSerializable this class provide
 * extra safety by checking Type on the TLV encoded received data against the
 * initialization type of the item, @see TlvFlags permits to control the
 * strictness of the check.
 */
class RsAutoTlvSerializable : public RsAutoSerializable, public RsTlvItem
{
public:
	/**
	 * @brief [de]serialization behavior flags
	 * This flags are matched against TLV type to determine [de]serialization
	 * behaviour.
	 */
	enum TlvFlags
	{
		/**
		 * If CHECK_TYPE flag is set deserialize(...) check if the buffer match
		 * the expected TLV type, if it doesn't match skipTlv(...) is returned
		 * leaving the members untouched.
		 */
		CHECK_TYPE = 0x8000,

		/**
		 * If MANDATORY flag is set deserialize(...) check if the buffer match
		 * the expected TLV type, if it doesn't match restore the previous state
		 * and return false to inform the caller that the serialization failed.
		 */
		MANDATORY = CHECK_TYPE + 0x4000, // = 0xc000
	};

	/**
	 * @brief initialize at_type to given value and register members for
	 * automatic [de]serialization @see RsAutoTlvItem.at_type for more details.
	 */
	RsAutoTlvSerializable(uint16_t type=0) : at_type(type)
	{
		registerSerializable(&RsAutoTlvSerializable::at_type);
		registerSerializable(&RsAutoTlvSerializable::at_lenght);
	}

	/**
	 * @brief @see RsSerializable
	 * Set at_lenght and then call @see RsAutoSerializable::serialize(...)
	 */
	bool serialize(uint8_t data[], uint32_t size, uint32_t &offset) const;

	/**
	 * @brief @see RsSerializable
	 * Check conformance of buffer with TLV type and setted @see TlvFlags and if
	 * ok call * @see RsAutoSerializable::deserialize(...)
	 */
	bool deserialize(const uint8_t data[], uint32_t size, uint32_t &offset);

	/**
	 * @brief Get Item TLV type setted during initalization or deserialization.
	 * @return Item TLV type.
	 */
	uint16_t getTlvItemType() { return at_type; }

protected:
	/**
	 * @brief SkipTlv skip TLV without concerning about type or value
	 * @param data @see deserialize
	 * @param size @see deserialize
	 * @param offset @see deserialize
	 * @return true if skipping success, false otherwise
	 */
	static bool skipTlv(const uint8_t data[], uint32_t size, uint32_t &offset);

private:
	/**
	 * @brief at_type TLV type for this item
	 * The initialization value influence the deserialization bahaviour,
	 * @see TlvFlags.
	 * On serialization this value is used as serialized TLV type.
	 */
	uint16_t at_type;

	/// @brief Store TLV lenght
	uint32_t at_lenght;
};

/**
 * Template to generate RsAutoTlvSerializable wrappers for standard types
 * supported by @see RsAutoSerializable
 */
template<typename N> class RsAutoTlvWrap : public RsAutoTlvSerializable
{
public:
	RsAutoTlvWrap(const N &val, uint16_t type=0) : RsAutoTlvSerializable(type),
	    value(val) { registerSerializable(&RsAutoTlvWrap::value); }
	N value;
};

std::ostream &printIndent(std::ostream &out, uint16_t indent);
