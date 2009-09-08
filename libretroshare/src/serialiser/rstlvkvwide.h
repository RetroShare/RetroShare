#ifndef RSTLVKVWIDE_H_
#define RSTLVKVWIDE_H_

/*
 * libretroshare/src/serialiser: rstlvkvwide.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Chris Parker
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

#include "rstlvtypes.h"

class RsTlvKeyValueWide: public RsTlvItem
{
	public:
	
	RsTlvKeyValueWide() { return;}
		virtual ~RsTlvKeyValueWide() { return;}
	
virtual uint32_t TlvSize();
virtual void TlvClear();
virtual bool SetTlv(void *data, uint32_t size, uint32_t *offset);
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	std::wstring wKey;
	std::wstring wValue;
};


class RsTlvKeyValueWideSet : public RsTlvItem
{
	public:
	
	RsTlvKeyValueWideSet() { return;}
	virtual ~RsTlvKeyValueWideSet() { return; }
	
virtual uint32_t TlvSize();
virtual void TlvClear();
virtual bool SetTlv(void *data, uint32_t size, uint32_t *offset);
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	std::list<RsTlvKeyValueWide> wPairs;
};


#endif /*RSTLVKVWIDE_H_*/
