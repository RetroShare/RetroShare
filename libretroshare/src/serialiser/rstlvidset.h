#pragma once

/*
 * libretroshare/src/serialiser: rstlvidset.h
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

#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvitem.h"

#include <retroshare/rstypes.h>
#include <retroshare/rsgxsifacetypes.h>

#include <list>

template<class ID_CLASS,uint32_t TLV_TYPE> class t_RsTlvIdSet: public RsTlvItem
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

			for(typename std::list<ID_CLASS>::const_iterator it(ids.begin());it!=ids.end();++it)
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
				ids.push_back(id) ;
			}
			if(*offset != tlvend)
				std::cerr << "(EE) deserialisaiton error in " << __PRETTY_FUNCTION__ << std::endl;
			return *offset == tlvend ;
		}
		virtual std::ostream &print(std::ostream &out, uint16_t /* indent */) const
		{
			for(typename std::list<ID_CLASS>::const_iterator it(ids.begin());it!=ids.end();++it)
				out << (*it).toStdString() << ", " ;

			return out ;
		}
		virtual std::ostream &printHex(std::ostream &out, uint16_t /* indent */) const /* SPECIAL One */
		{
			for(typename std::list<ID_CLASS>::const_iterator it(ids.begin());it!=ids.end();++it)
				out << (*it).toStdString() << ", " ;

			return out ;
		}

		std::list<ID_CLASS> ids ;
};

typedef t_RsTlvIdSet<RsPeerId,TLV_TYPE_PEERSET>		RsTlvPeerIdSet ;
typedef t_RsTlvIdSet<RsPgpId,TLV_TYPE_PGPIDSET>	RsTlvPgpIdSet ;
typedef t_RsTlvIdSet<Sha1CheckSum,TLV_TYPE_HASHSET> 	RsTlvHashSet ;
typedef t_RsTlvIdSet<RsGxsId,TLV_TYPE_GXSIDSET> 	RsTlvGxsIdSet ;
typedef t_RsTlvIdSet<RsGxsCircleId,TLV_TYPE_GXSCIRCLEIDSET> 	RsTlvGxsCircleIdSet ;

class RsTlvServiceIdSet: public RsTlvItem
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


