/*******************************************************************************
 * libretroshare/src/serialiser: rstlvkeyvalue.cc                              *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008 by Robert Fernie,Chris Parker <retroshare@lunamutt.com>      *
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
#include "rstlvkeyvalue.h"
#include "rstlvbase.h"

// #define TLV_DEBUG 1

void RsTlvKeyValue::TlvClear()
{
	key = "";
	value = "";
}

uint32_t RsTlvKeyValue::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; /* header */

	/* now add comment and title length of this tlv object */

	if (key.length() > 0)
		s += GetTlvStringSize(key); 
	if (value.length() > 0)
		s += GetTlvStringSize(value);

	return s;

}

bool  RsTlvKeyValue::SetTlv(void *data, uint32_t size, uint32_t *offset) const 
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

		/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_KEYVALUE, tlvsize);


	
		/* now optional ones */
	if (key.length() > 0)
		ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_KEY, key);  // no base tlv type for title?
	if (value.length() > 0)
		ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_VALUE, value); // no base tlv type for comment?

return ok;

}

bool  RsTlvKeyValue::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	
	
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_KEYVALUE) /* check type */
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
			case TLV_TYPE_STR_KEY:
				ok &= GetTlvString(data, tlvend, offset, TLV_TYPE_STR_KEY, key);
				break;
			case TLV_TYPE_STR_VALUE:
				ok &= GetTlvString(data, tlvend, offset,  TLV_TYPE_STR_VALUE, value);
				break;
			default:
				ok &= SkipUnknownTlv(data, tlvend, offset);
				break;

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
		std::cerr << "RsTlvKeyValue::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
	
}

std::ostream &RsTlvKeyValue::print(std::ostream &out, uint16_t indent) const
{ 
	printBase(out, "RsTlvKeyValue", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Key:" << key;
	printIndent(out, int_Indent);
	out << "Value:" << value;
	out << std::endl;
	

	printEnd(out, "RsTlvKeyValue", indent);
	return out;
}

/************************************* RsTlvKeyValueSet ************************************/

void RsTlvKeyValueSet::TlvClear()
{
	pairs.clear(); //empty list
}

uint32_t RsTlvKeyValueSet::TlvSize() const
{

	uint32_t s = TLV_HEADER_SIZE; /* header */

	std::list<RsTlvKeyValue>::const_iterator it;
	
	if(!pairs.empty())
	{

		for(it = pairs.begin(); it != pairs.end() ; ++it)
			s += it->TlvSize();

	}

	return s;
}

bool  RsTlvKeyValueSet::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

		/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_KEYVALUESET , tlvsize);
	

	if(!pairs.empty())
	{
		std::list<RsTlvKeyValue>::const_iterator it;

		for(it = pairs.begin(); it != pairs.end() ; ++it)
			ok &= it->SetTlv(data, size, offset);
	}
	

return ok;

}


bool  RsTlvKeyValueSet::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	

	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_KEYVALUESET) /* check type */
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
                        case TLV_TYPE_KEYVALUE:
			{
				RsTlvKeyValue kv;
				ok &= kv.GetTlv(data, size, offset);
				if (ok)
				{
					pairs.push_back(kv);
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
		std::cerr << "RsTlvKeyValueSet::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
}

/// prints out contents of RsTlvKeyValueSet
std::ostream &RsTlvKeyValueSet::print(std::ostream &out, uint16_t indent) const
{
	printBase(out, "RsTlvKeyValue", indent);

	std::list<RsTlvKeyValue>::const_iterator it;

	for(it = pairs.begin(); it != pairs.end() ; ++it)
		it->print(out, indent);

	printEnd(out, "RsTlvKeyValue", indent);
	return out;
}

