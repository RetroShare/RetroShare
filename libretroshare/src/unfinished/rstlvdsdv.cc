
/*
 * libretroshare/src/serialiser: rstlvtypes.cc
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

#include "rstlvdsdv.h"
#include "rsbaseserial.h"


/************************************* RsTlvDsdvEndPoint ************************************/

RsTlvDsdvEndPoint::RsTlvDsdvEndPoint()
	:RsTlvItem(), idType(0)
{
	return;
}

void RsTlvDsdvEndPoint::TlvClear()
{
	idType = 0;
	anonChunk.clear();
	serviceId.clear();
}

uint32_t RsTlvDsdvEndPoint::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; /* header + 4 + str + str */

	s += 4; // idType;
	s += GetTlvStringSize(anonChunk);
	s += GetTlvStringSize(serviceId);

	return s;

}

bool  RsTlvDsdvEndPoint::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

	/* start at data[offset] */
        /* add mandatory parts first */

	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_DSDV_ENDPOINT, tlvsize);

	ok &= setRawUInt32(data, tlvend, offset, idType);
        ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_GENID, anonChunk);
        ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_HASH_SHA1, serviceId);
	return ok;

}


bool  RsTlvDsdvEndPoint::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	
	
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_DSDV_ENDPOINT) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	ok &= getRawUInt32(data, tlvend, offset, &(idType));
	ok &= GetTlvString(data, tlvend, offset, TLV_TYPE_STR_GENID, anonChunk);
	ok &= GetTlvString(data, tlvend, offset, TLV_TYPE_STR_HASH_SHA1, serviceId);

	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvDsdvEndPoint::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
	
}


std::ostream &RsTlvDsdvEndPoint::print(std::ostream &out, uint16_t indent) const
{ 
	printBase(out, "RsTlvDsdvEndPoint", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "idType:" << idType;
	out << std::endl;

	printIndent(out, int_Indent);
	out << "AnonChunk:" << anonChunk;
	out << std::endl;

	printIndent(out, int_Indent);
	out << "ServiceId:" << serviceId;
	out << std::endl;

	printEnd(out, "RsTlvDsdvEndPoint", indent);
	return out;
}


/************************************* RsTlvDsdvEntry ************************************/

RsTlvDsdvEntry::RsTlvDsdvEntry()
	:RsTlvItem(), sequence(0), distance(0)
{
	return;
}

void RsTlvDsdvEntry::TlvClear()
{
	endPoint.TlvClear();
	sequence = 0;
	distance = 0;
}

uint32_t RsTlvDsdvEntry::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; /* header + EndPoint.Size + 4 + 4 */

	s += endPoint.TlvSize();
	s += 4; // sequence;
	s += 4; // distance;

	return s;

}

bool  RsTlvDsdvEntry::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

	/* start at data[offset] */
        /* add mandatory parts first */

	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_DSDV_ENTRY, tlvsize);

	ok &= endPoint.SetTlv(data, size, offset);
	ok &= setRawUInt32(data, tlvend, offset, sequence);
	ok &= setRawUInt32(data, tlvend, offset, distance);

	return ok;

}


bool  RsTlvDsdvEntry::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	
	
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_DSDV_ENTRY) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	ok &= endPoint.GetTlv(data, size, offset);
	ok &= getRawUInt32(data, tlvend, offset, &(sequence));
	ok &= getRawUInt32(data, tlvend, offset, &(distance));
   

	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvDsdvEntry::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
	
}


std::ostream &RsTlvDsdvEntry::print(std::ostream &out, uint16_t indent) const
{ 
	printBase(out, "RsTlvDsdvEntry", indent);
	uint16_t int_Indent = indent + 2;

	endPoint.print(out, int_Indent);

	printIndent(out, int_Indent);
	out << "Sequence:" << sequence;
	out << std::endl;

	printIndent(out, int_Indent);
	out << "Distance:" << distance;
	out << std::endl;
	
	printEnd(out, "RsTlvDsdvEntry", indent);
	return out;
}


/************************************* RsTlvDsdvEntrySet ************************************/

#if 0
RsTlvDsdvEntrySet::RsTlvDsdvEntrySet()
{

}

void RsTlvDsdvEntrySet::TlvClear()
{
	entries.clear();
}

uint32_t RsTlvDsdvEntrySet::TlvSize()
{

	uint32_t s = TLV_HEADER_SIZE; /* header */

	std::list<RsTlvDsdvEntry>::iterator it;
	

	if(!entries.empty())
	{

		for(it = entries.begin(); it != entries.end() ; ++it)
			s += it->TlvSize();

	}

	return s;
}

bool  RsTlvDsdvEntrySet::SetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

		/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_DSDV_ENTRY_SET , tlvsize);
	
	if(!entries.empty())
	{
		std::list<RsTlvDsdvEntry>::iterator it;

		for(it = entries.begin(); it != entries.end() ; ++it)
			ok &= it->SetTlv(data, size, offset);
	}
	

	return ok;

}


bool  RsTlvDsdvEntrySet::GetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	

	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_DSDV_ENTRY_SET) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

        /* while there is TLV  */
        while((*offset) + 2 < tlvend)
        {
                /* get the next type */
                uint16_t tlvsubtype = GetTlvType( &(((uint8_t *) data)[*offset]) );

                switch(tlvsubtype)
                {
                        case TLV_TYPE_DSDV_ENTRY:
			{
				RsTlvDsdvEntry entry;
				ok &= entry.GetTlv(data, size, offset);
				if (ok)
				{
					entries.push_back(entry);
				}
			}
				break;
                        default:
                                ok &= SkipUnknownTlv(data, tlvend, offset);
                                break;

                }

                if (!ok)
			break;
	}
   

		
	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvDsdvEntrySet::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
}

// prints out contents of RsTlvDsdvEntrySet
std::ostream &RsTlvDsdvEntrySet::print(std::ostream &out, uint16_t indent)
{
	printBase(out, "RsTlvDsdvEntrySet", indent);
	uint16_t int_Indent = indent + 2;

	std::list<RsTlvDsdvEntry>::iterator it;
	for(it = entries.begin(); it != entries.end() ; ++it)
		it->print(out, int_Indent);

	printEnd(out, "RsTlvDsdvEntrySet", indent);
	return out;
}


#endif


