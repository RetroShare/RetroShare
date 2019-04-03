/*******************************************************************************
 * libretroshare/src/serialiser: rstlvitem.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie,Chris Parker <retroshare@lunamutt.com> *
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

/*******************************************************************
 * These are the Compound TLV structures that must be (un)packed.
 *
 ******************************************************************/

#include "util/rsdeprecate.h"

#include <iosfwd>
#include <string>
#include <inttypes.h>

/*! A base class for all tlv items
 * This class is provided to allow the serialisation and deserialization of
 * compund tlv items
 * @deprecated TLV serialization system is deprecated!
 */
class RS_DEPRECATED_FOR(RsSerializable) RsTlvItem
{
	public:
	 RsTlvItem() { return; }
virtual ~RsTlvItem() { return; }
virtual uint32_t TlvSize() const = 0;
virtual void	 TlvClear() = 0;
virtual	void	 TlvShallowClear(); /*! Don't delete allocated data */
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const = 0; /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset) = 0; /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent) const = 0;
std::ostream &printBase(std::ostream &out, std::string clsName, uint16_t indent) const;
std::ostream &printEnd(std::ostream &out, std::string clsName, uint16_t indent) const;
};

std::ostream &printIndent(std::ostream &out, uint16_t indent);

class RS_DEPRECATED_FOR(RsSerializable) RsTlvUnit: public RsTlvItem
{
	public:
	 RsTlvUnit(uint16_t tlv_type);
virtual ~RsTlvUnit() { return; }
virtual uint32_t TlvSize() const;
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const;
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset);

virtual uint16_t TlvType() const { return mTlvType; }

// These Functions need to be implemented.
//virtual void	 TlvClear() = 0;
//virtual std::ostream &print(std::ostream &out, uint16_t indent) = 0 const;

virtual uint32_t TlvSizeUnit() const = 0;
virtual bool     SetTlvUnit(void *data, uint32_t size, uint32_t *offset) const = 0;
virtual bool     GetTlvUnit(void *data, uint32_t size, uint32_t *offset) = 0;


	private:
	uint16_t mTlvType;
};


