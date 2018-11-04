/*******************************************************************************
 * libretroshare/src/serialiser: rstlvlist.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008 by Robert Fernie <retroshare@lunamutt.com>                   *
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

#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvitem.h"

#include <list>

// #define TLV_DEBUG_LIST 1

template<class TLV_CLASS,uint32_t TLV_TYPE> class t_RsTlvList: public RsTlvItem
{
	public:
		t_RsTlvList() {}
		virtual ~t_RsTlvList() {}

		virtual uint32_t TlvSize() const
		{ 
			uint32_t size = TLV_HEADER_SIZE;
			typename std::list<TLV_CLASS>::const_iterator it;
			for(it = mList.begin();it != mList.end(); ++it)
			{
				size += it->TlvSize();
			}
			return size;
		}
		virtual void TlvClear(){ mList.clear(); }
		virtual bool SetTlv(void *data, uint32_t size, uint32_t *offset) const
		{	
			uint32_t tlvsize = TlvSize();
			uint32_t tlvend  = *offset + tlvsize;
			/* must check sizes */

			if (size < tlvend)
			{
#ifdef TLV_DEBUG_LIST
				std::cerr << "RsTlvList::SetTlv() Not enough size";
				std::cerr << std::endl;
#endif
				return false; /* not enough space */
			}

			bool ok = true;

			/* start at data[offset] */
			ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE, tlvsize);

			typename std::list<TLV_CLASS>::const_iterator it;
			for(it = mList.begin();it != mList.end(); ++it)
			{
				ok &= it->SetTlv(data,tlvend,offset) ;
			}
			return ok ;
		}
		virtual bool GetTlv(void *data, uint32_t size, uint32_t *offset)
		{
			if (size < *offset + TLV_HEADER_SIZE)
			{
#ifdef TLV_DEBUG_LIST
				std::cerr << "RsTlvList::GetTlv() Not enough size";
				std::cerr << std::endl;
#endif
				return false;
			}

			uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
			uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
			uint32_t tlvend = *offset + tlvsize;

			if (size < tlvend)    /* check size */
				return false; /* not enough space */

			if (tlvtype != TLV_TYPE) /* check type */
				return false;

			bool ok = true;

			/* ready to load */
			TlvClear();

			/* skip the header */
			(*offset) += TLV_HEADER_SIZE;

			/* while there is TLV : 2 (type) + 4 (len) */
			while(ok && ((*offset) + 6 < tlvend))
			{
				TLV_CLASS item;
				ok &= item.GetTlv(data,tlvend,offset);
				if (ok)
				{
					mList.push_back(item);
				}
			}

			if(*offset != tlvend)
			{
				std::cerr << "(EE) deserialisation error in " << __PRETTY_FUNCTION__ << std::endl;
			}
			return *offset == tlvend ;
		}

		virtual std::ostream &print(std::ostream &out, uint16_t indent) const
		{
			printBase(out, "Template TlvList", indent);
			typename std::list<TLV_CLASS>::const_iterator it;
			for(it = mList.begin();it != mList.end(); ++it)
			{
				it->print(out, indent + 2) ;
			}
			printEnd(out, "Template TlvList", indent);
			return out ;
		}

		std::list<TLV_CLASS> mList;
};



