#ifndef RS_TLV_GENERIC_PARAM_H
#define RS_TLV_GENERIC_PARAM_H

/*
 * libretroshare/src/serialiser: rstlvgenericparam.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2014 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#include "serialiser/rstlvtypes.h"
#include <map>

#include <stdlib.h>
#include <stdint.h>


/**** TLV *****
 * Generic Parameters / Maps.
 */


template<class T>
class RsTlvParamRef: public RsTlvItem
{
	public:
	 RsTlvParamRef(uint16_t param_type, T &p): mParamType(param_type), mParam(p) {}
virtual ~RsTlvParamRef() { return; }
virtual uint32_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	uint16_t mParamType;
	T &mParam;
};


#endif

