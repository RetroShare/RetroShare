#pragma once
/*
 * libretroshare/src/serialiser: rstlvitem.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie, Chris Parker
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

/*******************************************************************
 * These are the Compound TLV structures that must be (un)packed.
 *
 ******************************************************************/

#include <iosfwd>
#include <string>
#include <inttypes.h>

//! A base class for all tlv items 
/*! This class is provided to allow the serialisation and deserialization of compund 
tlv items 
*/
class RsTlvItem
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


class RsTlvUnit: public RsTlvItem
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


