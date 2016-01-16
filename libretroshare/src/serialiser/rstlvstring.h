#pragma once

/*
 * libretroshare/src/serialiser: rstlvstring.h
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

/**
 * DEPRECATED!
 * TODO: 2016/01/08 This file as misleading naming, the classes you find here
 * are not proper StringSets in fact they do assumption on type of string!
 * @see rstlvlist.h for a proper implementation!!
 */


#include "serialiser/rstlvitem.h"

#include <list>

class RsTlvStringSet: public RsTlvItem
{
public:
	RsTlvStringSet(uint16_t type);
	virtual ~RsTlvStringSet() {}
	virtual uint32_t TlvSize() const;
	virtual void TlvClear();
	virtual bool SetTlv(void *data, uint32_t size, uint32_t *offset) const;
	virtual bool GetTlv(void *data, uint32_t size, uint32_t *offset);
	virtual std::ostream &print(std::ostream &out, uint16_t indent) const;
	virtual std::ostream &printHex(std::ostream &out, uint16_t indent) const;

	uint16_t mType;
	std::list<std::string> ids; /* Mandatory */
};

class RsTlvStringSetRef: public RsTlvItem
{
public:
	RsTlvStringSetRef(uint16_t type, std::list<std::string> &refids);
	virtual ~RsTlvStringSetRef() {}
	virtual uint32_t TlvSize() const;
	virtual void TlvClear();
	virtual bool SetTlv(void *data, uint32_t size, uint32_t *offset) const;
	virtual bool GetTlv(void *data, uint32_t size, uint32_t *offset);
	virtual std::ostream &print(std::ostream &out, uint16_t indent) const;

	uint16_t mType;
	std::list<std::string> &ids; /* Mandatory */
};
