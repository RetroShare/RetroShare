/*******************************************************************************
 * libretroshare/src/serialiser: rstlvidset.h                                  *
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

#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvitem.h"
#include "util/rsdeprecate.h"
#include <retroshare/rstypes.h>
#include <retroshare/rsgxsifacetypes.h>

#include <list>

/// @deprecated use plain std::set<> instead
template<class ID_CLASS,uint32_t TLV_TYPE> class RS_DEPRECATED_FOR(std::set<>) t_RsTlvIdSet
    : public RsTlvItem
{
	public:
		t_RsTlvIdSet() {}
		virtual ~t_RsTlvIdSet() {}

		virtual uint32_t TlvSize() const { return ID_CLASS::SIZE_IN_BYTES * ids.size() + TLV_HEADER_SIZE; }
		virtual void TlvClear(){ ids.clear() ; }
		virtual bool SetTlv(void *data, uint32_t size, uint32_t *offset) const
		{	/* must check sizes */
			uint32_t tlvsize = TlvSize();
			uint32_t tlvend  = *offset + tlvsize;

			if (size < tlvend)
				return false; /* not enough space */

			bool ok = true;

			/* start at data[offset] */
			ok = ok && SetTlvBase(data, tlvend, offset, TLV_TYPE, tlvsize);

            for(typename std::set<ID_CLASS>::const_iterator it(ids.begin());it!=ids.end();++it)
				ok = ok && (*it).serialise(data,tlvend,*offset) ;

			return ok ;
		}
		virtual bool GetTlv(void *data, uint32_t size, uint32_t *offset) 
		{
			if (size < *offset + TLV_HEADER_SIZE)
				return false;

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

			while(*offset + ID_CLASS::SIZE_IN_BYTES <= tlvend)
			{
				ID_CLASS id ;
				ok = ok && id.deserialise(data,tlvend,*offset) ;
                ids.insert(id) ;
			}
			if(*offset != tlvend)
				std::cerr << "(EE) deserialisaiton error in " << __PRETTY_FUNCTION__ << std::endl;
			else if(!ok)
				std::cerr << "(WW) something wrong in ID_CLASS.deserialise in " << __PRETTY_FUNCTION__ << std::endl;

			return *offset == tlvend ;
		}
		virtual std::ostream &print(std::ostream &out, uint16_t /* indent */) const
		{
            for(typename std::set<ID_CLASS>::const_iterator it(ids.begin());it!=ids.end();++it)
				out << (*it).toStdString() << ", " ;

			return out ;
		}
		virtual std::ostream &printHex(std::ostream &out, uint16_t /* indent */) const /* SPECIAL One */
		{
            for(typename std::set<ID_CLASS>::const_iterator it(ids.begin());it!=ids.end();++it)
				out << (*it).toStdString() << ", " ;

			return out ;
		}

        std::set<ID_CLASS> ids ;
};

typedef t_RsTlvIdSet<RsPeerId,      TLV_TYPE_PEERSET>	        RsTlvPeerIdSet ;
typedef t_RsTlvIdSet<RsPgpId,       TLV_TYPE_PGPIDSET>	        RsTlvPgpIdSet ;
typedef t_RsTlvIdSet<Sha1CheckSum,  TLV_TYPE_HASHSET> 	        RsTlvHashSet ;
typedef t_RsTlvIdSet<RsGxsId,       TLV_TYPE_GXSIDSET>          RsTlvGxsIdSet ;
typedef t_RsTlvIdSet<RsGxsMessageId,TLV_TYPE_GXSMSGIDSET>       RsTlvGxsMsgIdSet ;
typedef t_RsTlvIdSet<RsGxsCircleId, TLV_TYPE_GXSCIRCLEIDSET>    RsTlvGxsCircleIdSet ;
typedef t_RsTlvIdSet<RsNodeGroupId, TLV_TYPE_NODEGROUPIDSET>    RsTlvNodeGroupIdSet ;

class RS_DEPRECATED RsTlvServiceIdSet: public RsTlvItem
{
	public:
	 RsTlvServiceIdSet() { return; }
virtual ~RsTlvServiceIdSet() { return; }
virtual uint32_t TlvSize() const;
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const; 
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); 
virtual std::ostream &print(std::ostream &out, uint16_t indent) const;

	std::list<uint32_t> ids; /* Mandatory */
};


