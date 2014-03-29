#pragma once

/*
 * libretroshare/src/serialiser: rstlvkeyvalue.h
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

#include "serialiser/rstlvitem.h"
#include <list>

class RsTlvKeyValue: public RsTlvItem
{
	public:
	 RsTlvKeyValue() { return; }
	 RsTlvKeyValue(const std::string& k,const std::string& v): key(k),value(v) {}
virtual ~RsTlvKeyValue() { return; }
virtual uint32_t TlvSize() const;
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const; 
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); 
virtual std::ostream &print(std::ostream &out, uint16_t indent) const;

	std::string key;	/// Mandatory : For use in hash tables
	std::string value;	/// Mandatory : For use in hash tables
};

class RsTlvKeyValueSet: public RsTlvItem
{
	public:
	 RsTlvKeyValueSet() { return; }
virtual ~RsTlvKeyValueSet() { return; }
virtual uint32_t TlvSize() const;
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const; 
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset);
virtual std::ostream &print(std::ostream &out, uint16_t indent) const;

	std::list<RsTlvKeyValue> pairs; /// For use in hash tables 
};


