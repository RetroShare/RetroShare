/*******************************************************************************
 * libretroshare/src/serialiser: rstlvstring.cc                                *
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
#include "serialiser/rstlvstring.h"

#include "serialiser/rstlvbase.h"
#include "util/rsprint.h"
#include <iostream>

// #define TLV_DEBUG 1

/************************************* Peer Id Set ************************************/


RsTlvStringSet::RsTlvStringSet(uint16_t type) :mType(type)
{
}


bool     RsTlvStringSet::SetTlv(void *data, uint32_t size, uint32_t *offset) const 
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;


		/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, mType , tlvsize);

	/* determine the total size of ids strings in list */

	std::list<std::string>::const_iterator it;

	for(it = ids.begin(); it != ids.end() ; ++it)
	{
		if (it->length() > 0)
			ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_GENID, *it);
	}

	return ok;

}


void RsTlvStringSet::TlvClear()
{
	ids.clear();

}

uint32_t RsTlvStringSet::TlvSize() const
{

	uint32_t s = TLV_HEADER_SIZE; /* header */

	/* determine the total size of ids strings in list */

	std::list<std::string>::const_iterator it;

	for(it = ids.begin(); it != ids.end() ; ++it)
	{
		if (it->length() > 0)
			s += GetTlvStringSize(*it);
	}

	return s;
}


bool     RsTlvStringSet::GetTlv(void *data, uint32_t size, uint32_t *offset)
{	
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	

	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;



	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != mType) /* check type */
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
	
		if (tlvsubtype == TLV_TYPE_STR_GENID)
		{
			std::string newIds;
			ok &= GetTlvString(data, tlvend, offset, TLV_TYPE_STR_GENID, newIds);
			if(ok)
			{
				ids.push_back(newIds);
				
			}
		}
		else
		{
			/* Step past unknown TLV TYPE */
			ok &= SkipUnknownTlv(data, tlvend, offset);
		}

		if (!ok)
		{
			break;
		}
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
		std::cerr << "RsTlvPeerIdSet::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
}

/// print to screen RsTlvStringSet contents
std::ostream &RsTlvStringSet::print(std::ostream &out, uint16_t indent) const
{
	printBase(out, "RsTlvStringSet", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "type:" << mType;
	out << std::endl;

	std::list<std::string>::const_iterator it;
	for(it = ids.begin(); it != ids.end() ; ++it)
	{
		printIndent(out, int_Indent);
		out << "id:" << *it;
		out << std::endl;
	}

	printEnd(out, "RsTlvStringSet", indent);
	return out;

}

/// print to screen RsTlvStringSet contents
std::ostream &RsTlvStringSet::printHex(std::ostream &out, uint16_t indent) const
{
	printBase(out, "RsTlvStringSet", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "type:" << mType;
	out << std::endl;

	std::list<std::string>::const_iterator it;
	for(it = ids.begin(); it != ids.end() ; ++it)
	{
		printIndent(out, int_Indent);
		out << "id: 0x" << RsUtil::BinToHex(*it);
		out << std::endl;
	}

	printEnd(out, "RsTlvStringSet", indent);
	return out;

}


/************************************* String Set Ref ************************************/
/* This is exactly the same as StringSet, but it uses an alternative list.
 */
RsTlvStringSetRef::RsTlvStringSetRef(uint16_t type, std::list<std::string> &refids)
	:mType(type), ids(refids)
{
}

void RsTlvStringSetRef::TlvClear()
{
    ids.clear();

}

uint32_t RsTlvStringSetRef::TlvSize() const
{

	uint32_t s = TLV_HEADER_SIZE; /* header */

	/* determine the total size of ids strings in list */

	std::list<std::string>::const_iterator it;
	
	for(it = ids.begin(); it != ids.end() ; ++it)
	{
		if (it->length() > 0)
			s += GetTlvStringSize(*it);
	}

	return s;
}


bool     RsTlvStringSetRef::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

	
		/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, mType , tlvsize);

	/* determine the total size of ids strings in list */

	std::list<std::string>::const_iterator it;
	
	for(it = ids.begin(); it != ids.end() ; ++it)
	{
		if (it->length() > 0)
			ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_GENID, *it); 
	}

	return ok;

}


bool     RsTlvStringSetRef::GetTlv(void *data, uint32_t size, uint32_t *offset)
{	
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	

	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;



	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != mType) /* check type */
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
	
		if (tlvsubtype == TLV_TYPE_STR_GENID)
		{
			std::string newIds;
			ok &= GetTlvString(data, tlvend, offset, TLV_TYPE_STR_GENID, newIds);
			if(ok)
			{
				ids.push_back(newIds);
				
			}
		}
		else
		{
			/* Step past unknown TLV TYPE */
			ok &= SkipUnknownTlv(data, tlvend, offset);
		}

		if (!ok)
		{
			break;
		}
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
		std::cerr << "RsTlvPeerIdSetRef::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
}

/// print to screen RsTlvStringSet contents
std::ostream &RsTlvStringSetRef::print(std::ostream &out, uint16_t indent) const
{
	printBase(out, "RsTlvStringSetRef", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "type:" << mType;
	out << std::endl;

	std::list<std::string>::const_iterator it;
	for(it = ids.begin(); it != ids.end() ; ++it)
	{
		printIndent(out, int_Indent);
		out << "id:" << *it;
		out << std::endl;
	}

	printEnd(out, "RsTlvStringSetRef", indent);
	return out;

}

