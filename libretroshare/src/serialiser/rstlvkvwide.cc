/*
 * libretroshare/src/serialiser: rstlvkvwide.cc
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
 
#include "rstlvbase.h"
#include "rstlvkvwide.h"
#include "rsbaseserial.h"
#include "util/rsprint.h"
#include <ostream>
#include <sstream>
#include <iomanip>
#include <iostream>


void RsTlvKeyValueWide::TlvClear()
{
	wKey.clear();
 	wValue.clear();
}
 
uint16_t RsTlvKeyValueWide::TlvSize()
{
	uint32_t s =  4; /* header size */
	s += GetTlvWideStringSize(wKey);
	s += GetTlvWideStringSize(wValue);
	
	return s;
}

std::ostream &RsTlvKeyValueWide::print(std::ostream &out, uint16_t indent)
{
	printBase(out, "RsTlvKeyValueWide", indent);
	uint16_t int_Indent = indent + 2;
	std::string cnv_str(wKey.begin(), wKey.end()); // to convert to string
	
	printIndent(out, int_Indent);
	out << "wKey:" << cnv_str;
	cnv_str.clear();
	cnv_str.assign(wValue.begin(), wValue.end());
	printIndent(out, int_Indent);
	out << "wValue:" << cnv_str;
	out << std::endl;
	
	printEnd(out, "RsTlvKeyValuewide", indent);
	return out;
}

bool RsTlvKeyValueWide::SetTlv(void *data, uint32_t size, uint32_t *offset)
{

	/* must check sizes */
	uint16_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

		/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_WKEYVALUE, tlvsize);


	
		/* now optional ones */
	if (wKey.length() > 0)
		ok &= SetTlvWideString(data, tlvend, offset, TLV_TYPE_WSTR_KEY, wKey);  // no base tlv type for title?
	if (wValue.length() > 0)
		ok &= SetTlvWideString(data, tlvend, offset, TLV_TYPE_WSTR_VALUE, wValue); // no base tlv type for comment?

	return ok;
}

bool RsTlvKeyValueWide::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	if (size < *offset + 4)
		return false;	
	
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint16_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_WKEYVALUE ) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += 4;

	/* while there is TLV  */
	while((*offset) + 2 < tlvend)
	{
		/* get the next type */
		uint16_t tlvsubtype = GetTlvType( &(((uint8_t *) data)[*offset]) );
		
		switch(tlvsubtype)
		{
			case TLV_TYPE_WSTR_KEY:
				ok &= GetTlvWideString(data, tlvend, offset, TLV_TYPE_WSTR_KEY, wKey);
				break;
			case TLV_TYPE_WSTR_VALUE:
				ok &= GetTlvWideString(data, tlvend, offset,  TLV_TYPE_WSTR_VALUE, wValue);
				break;
			default:
				break;

		}

		if (!ok)
		{
			return false;
		}
	}
   
	return ok;
}

/******************************************* Wide Key Value set *************************************/


void RsTlvKeyValueWideSet::TlvClear()
{
	wPairs.clear();
}

uint16_t RsTlvKeyValueWideSet::TlvSize()
{
	uint32_t s = 4; /* header size */
	
	std::list<RsTlvKeyValueWide>::iterator  it;
	
	for(it = wPairs.begin(); it != wPairs.end(); it++)
	{
		s += it->TlvSize();
	}
	
	return s;
}

std::ostream &RsTlvKeyValueWideSet::print(std::ostream &out, uint16_t indent)
{
	printBase(out, "RsTlvKeyValueWide", indent);
	uint16_t int_Indent = indent + 2;
	
	std::list<RsTlvKeyValueWide>::iterator  it;

	for(it = wPairs.begin(); it != wPairs.end(); it++)
	{
		it->print(out, int_Indent);
	}	
	
	printEnd(out, "RsTlvKeyValuewide", indent);
	return out;
}

bool RsTlvKeyValueWideSet::SetTlv(void *data, uint32_t size, uint32_t* offset)
{

	/* must check sizes */
	uint16_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

		/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_WKEYVALUESET, tlvsize);
	std::list<RsTlvKeyValueWide>::iterator  it;
	
	for(it = wPairs.begin(); it != wPairs.end(); it++)
	{
		ok &= it->SetTlv(data, size, offset);
	}

	return ok;
}

bool RsTlvKeyValueWideSet::GetTlv(void *data, uint32_t size, uint32_t* offset)
{
	if (size < *offset + 4)
		return false;	
	
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint16_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_WKEYVALUESET) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += 4;

	/* while there is TLV  */
	while((*offset) + 2 < tlvend)
	{

		RsTlvKeyValueWide wPair;
		
		ok &= wPair.GetTlv(data, size, offset);
		
		wPairs.push_back(wPair);

		if (!ok)
		{
			return false;
		}
	}
   
	return ok;
}
