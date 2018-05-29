/*******************************************************************************
 * libretroshare/src/serialiser: rstlvstring.h                                 *
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

#include "serialiser/rstlvitem.h"

#include <list>

class RsTlvStringSet: public RsTlvItem
{
	public:
explicit RsTlvStringSet(uint16_t type);
virtual ~RsTlvStringSet() { return; }
virtual uint32_t TlvSize() const;
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const;
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); 
virtual std::ostream &print(std::ostream &out, uint16_t indent) const;
virtual std::ostream &printHex(std::ostream &out, uint16_t indent) const;

	uint16_t mType;
	std::list<std::string> ids; /* Mandatory */
};

class RsTlvStringSetRef: public RsTlvItem
{
	public:
     RsTlvStringSetRef(uint16_t type, std::list<std::string> &refids);
virtual ~RsTlvStringSetRef() { return; }
virtual uint32_t TlvSize() const;
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const; 
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); 
virtual std::ostream &print(std::ostream &out, uint16_t indent) const;

	uint16_t mType;
    std::list<std::string> &ids; /* Mandatory */
};


