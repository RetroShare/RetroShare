/*******************************************************************************
 * libretroshare/src/serialiser: rstlvgenericparam.h                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2014 by Robert Fernie <retroshare@lunamutt.com>                   *
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
#ifndef RS_TLV_GENERIC_PARAM_H
#define RS_TLV_GENERIC_PARAM_H

#include "serialiser/rstlvitem.h"

#if 0
#include <map>

#include <stdlib.h>
#include <stdint.h>
#endif


/**** TLV *****
 * Generic Parameters / Maps.
 */


template<class T>
class RsTlvParamRef: public RsTlvItem
{
	public:
	 RsTlvParamRef(uint16_t param_type, T &p): mParamType(param_type), mParam(p) {}
virtual ~RsTlvParamRef() { return; }
virtual uint32_t TlvSize() const;
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const; 
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); 
virtual std::ostream &print(std::ostream &out, uint16_t indent) const;

	uint16_t mParamType;
	T &mParam;
};


#endif

