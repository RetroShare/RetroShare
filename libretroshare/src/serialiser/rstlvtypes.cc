
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

#include "rstlvbase.h"
#include "rstlvtypes.h"
#include "rsbaseserial.h"
#include "util/rsprint.h"
#include <ostream>
#include <sstream>
#include <iomanip>
#include <iostream>

void  	RsTlvItem::TlvShallowClear()
{
	TlvClear(); /* unless overloaded! */
}

std::ostream &RsTlvItem::printBase(std::ostream &out, std::string clsName, uint16_t indent)
{
	printIndent(out, indent);
	out << "RsTlvItem: " << clsName << " Size: " << TlvSize() << "  ***********************";
	out << std::endl;
	return out;
}

std::ostream &RsTlvItem::printEnd(std::ostream &out, std::string clsName, uint16_t indent)
{
	printIndent(out, indent);
	out << "********************** " << clsName << " *********************";
	out << std::endl;
	return out;
}

std::ostream &printIndent(std::ostream &out, uint16_t indent)
{
	for(int i = 0; i < indent; i++)
	{
		out << " ";
	}
	return out;
}



/*!********************************** RsTlvFileBinaryData **********************************/


RsTlvBinaryData::RsTlvBinaryData(uint16_t t)
	:tlvtype(t), bin_len(0), bin_data(NULL)
{
	return;
}

RsTlvBinaryData::~RsTlvBinaryData()
{
	TlvClear();
}


/// used to allocate memory andinitialize binary data member 
bool     RsTlvBinaryData::setBinData(void *data, uint32_t size)
{
	/* ready to load */
	TlvClear();

	/* get mandatory */
	/* the rest of the TLV size binary data */
	bin_len = size;
	if (bin_len == 0)
	{
		bin_data = NULL;
		return true;
	}

	bin_data = malloc(bin_len);
	memcpy(bin_data, data, bin_len);
	return true;
}

void RsTlvBinaryData::TlvClear()
{
	if (bin_data)
	{
		free(bin_data);
	}
	TlvShallowClear();
}

void  	RsTlvBinaryData::TlvShallowClear()
{
	bin_data = NULL;
	bin_len = 0;
}

uint32_t RsTlvBinaryData::TlvSize()
{
	uint32_t s = TLV_HEADER_SIZE; /* header */

	if (bin_data != NULL) 
		s += bin_len; // len is the size of data

	return s;
}


bool     RsTlvBinaryData::SetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

		/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, tlvtype, tlvsize);

	/* add mandatory data */

	// Warning: this is actually not an error if bin_len=0, as it does not
	// corrupt the packet structure. We thus still return true in this case.
	//
	if (bin_data != NULL && bin_len > 0)
	{
		memcpy(&(((uint8_t *) data)[*offset]), bin_data, bin_len);
		*offset += bin_len;
	}
	return ok;
}



bool     RsTlvBinaryData::GetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	if (size < *offset + TLV_HEADER_SIZE)
	{
		return false; /* not enough space to get the header */
	}

	uint16_t tlvtype_in = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvsize < TLV_HEADER_SIZE)
	{
		return false; /* bad tlv size */
	}

	if (tlvtype != tlvtype_in) /* check type */
		return false;

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	bool ok = setBinData(&(((uint8_t *) data)[*offset]), tlvsize - TLV_HEADER_SIZE);
	(*offset) += bin_len;

	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvBinaryData::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
}

std::ostream &RsTlvBinaryData::print(std::ostream &out, uint16_t indent)
{
        uint16_t int_Indent = indent + 2;

        uint32_t i;
        std::ostringstream sout;
        printIndent(sout, indent);
        sout << "RsTlvBinaryData: Type: " << tlvtype << " Size: " << bin_len;
        sout << std::hex;
        for(i = 0; i < bin_len; i++)
        {
                if (i % 16 == 0)
                {
                        sout << std::endl;
        		printIndent(sout, int_Indent);
                }
                sout << std::setw(2) << std::setfill('0')
                        << (int) (((unsigned char *) bin_data)[i]) << ":";
        }
        sout << std::endl;
        out << sout.str();

        printEnd(out, "RsTlvBinaryData", indent);
        return out;

}

/************************************* Peer Id Set ************************************/

RsTlvPeerIdSet::RsTlvPeerIdSet(): RsTlvStringSet(TLV_TYPE_PEERSET) {}
RsTlvHashSet::RsTlvHashSet(): RsTlvStringSet(TLV_TYPE_HASHSET) {}


RsTlvStringSet::RsTlvStringSet(uint16_t type) :mType(type)
{
}

void RsTlvStringSet::TlvClear()
{
	ids.clear();

}

uint32_t RsTlvStringSet::TlvSize()
{

	uint32_t s = TLV_HEADER_SIZE; /* header */

	/* determine the total size of ids strings in list */

	std::list<std::string>::iterator it;
	
	for(it = ids.begin(); it != ids.end() ; ++it)
	{
		if (it->length() > 0)
			s += GetTlvStringSize(*it);
	}

	return s;
}


bool     RsTlvStringSet::SetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
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

	std::list<std::string>::iterator it;
	
	for(it = ids.begin(); it != ids.end() ; ++it)
	{
		if (it->length() > 0)
			ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_GENID, *it); 
	}

	return ok;

}


bool     RsTlvStringSet::GetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
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
std::ostream &RsTlvStringSet::print(std::ostream &out, uint16_t indent)
{
	printBase(out, "RsTlvStringSet", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "type:" << mType;
	out << std::endl;

	std::list<std::string>::iterator it;
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
std::ostream &RsTlvStringSet::printHex(std::ostream &out, uint16_t indent)
{
	printBase(out, "RsTlvStringSet", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "type:" << mType;
	out << std::endl;

	std::list<std::string>::iterator it;
	for(it = ids.begin(); it != ids.end() ; ++it)
	{
		printIndent(out, int_Indent);
		out << "id: 0x" << RsUtil::BinToHex(*it);
		out << std::endl;
	}

	printEnd(out, "RsTlvStringSet", indent);
	return out;

}


/************************************* Service Id Set ************************************/

void RsTlvServiceIdSet::TlvClear()
{
	ids.clear();

}

uint32_t RsTlvServiceIdSet::TlvSize()
{
	uint32_t s = TLV_HEADER_SIZE; /* header */

	/* determine the total size of ids strings in list */
	std::list<uint32_t>::iterator it;
	for(it = ids.begin(); it != ids.end() ; ++it)
	{
		if (*it > 0)
			s += GetTlvUInt32Size();
	}

	return s;
}

bool     RsTlvServiceIdSet::SetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

	
	/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_SERVICESET , tlvsize);

	/* determine the total size of ids strings in list */
	std::list<uint32_t>::iterator it;
	for(it = ids.begin(); it != ids.end() ; ++it)
	{
		if (*it > 0)
			ok &= SetTlvUInt32(data, tlvend, offset, TLV_TYPE_UINT32_SERID, *it);
	}

	return ok;

}

bool     RsTlvServiceIdSet::GetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	
	
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_SERVICESET) /* check type */
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
		
		if (tlvsubtype == TLV_TYPE_UINT32_SERID)
		{
			uint32_t newIds;
			ok &= GetTlvUInt32(data, tlvend, offset, TLV_TYPE_UINT32_SERID, &newIds);
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
                std::cerr << "RsTlvServiceIdSet::GetTlv() Warning extra bytes at end of item";
                std::cerr << std::endl;
#endif
                *offset = tlvend;
        }

	
	return ok;
}

/// print to screen RsTlvServiceSet contents
std::ostream &RsTlvServiceIdSet::print(std::ostream &out, uint16_t indent)
{
	printBase(out, "RsTlvServiceIdSet", indent);
	uint16_t int_Indent = indent + 2;

	std::list<uint32_t>::iterator it;
	for(it = ids.begin(); it != ids.end() ; ++it)
	{
		printIndent(out, int_Indent);
		out << "id:" << *it;
		out << std::endl;
	}

	printEnd(out, "RsTlvServiceIdSet", indent);
	return out;

}

/************************************* RsTlvKeyValue ************************************/

void RsTlvKeyValue::TlvClear()
{
	key = "";
	value = "";
}

uint32_t RsTlvKeyValue::TlvSize()
{
	uint32_t s = TLV_HEADER_SIZE; /* header */

	/* now add comment and title length of this tlv object */

	if (key.length() > 0)
		s += GetTlvStringSize(key); 
	if (value.length() > 0)
		s += GetTlvStringSize(value);

	return s;

}

bool  RsTlvKeyValue::SetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
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

bool  RsTlvKeyValue::GetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
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

std::ostream &RsTlvKeyValue::print(std::ostream &out, uint16_t indent)
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

uint32_t RsTlvKeyValueSet::TlvSize()
{

	uint32_t s = TLV_HEADER_SIZE; /* header */

	std::list<RsTlvKeyValue>::iterator it;
	
	if(!pairs.empty())
	{

		for(it = pairs.begin(); it != pairs.end() ; ++it)
			s += it->TlvSize();

	}

	return s;
}

bool  RsTlvKeyValueSet::SetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
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
		std::list<RsTlvKeyValue>::iterator it;

		for(it = pairs.begin(); it != pairs.end() ; ++it)
			ok &= it->SetTlv(data, size, offset);
	}
	

return ok;

}


bool  RsTlvKeyValueSet::GetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
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
std::ostream &RsTlvKeyValueSet::print(std::ostream &out, uint16_t indent)
{
	printBase(out, "RsTlvKeyValue", indent);
	uint16_t int_Indent = indent + 2;

	std::list<RsTlvKeyValue>::iterator it;

	for(it = pairs.begin(); it != pairs.end() ; ++it)
		it->print(out, indent);

	printEnd(out, "RsTlvKeyValue", indent);
	return out;
}

