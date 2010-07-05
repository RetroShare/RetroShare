
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

#include "rstlvkeys.h"

#include "rstlvbase.h"
#include "rstlvtypes.h"
#include "rsbaseserial.h"
#include "util/rsprint.h"
#include <ostream>
#include <sstream>
#include <iomanip>
#include <iostream>

//#define TLV_DEBUG 1

/************************************* RsTlvSecurityKey ************************************/

RsTlvSecurityKey::RsTlvSecurityKey()
	:RsTlvItem(), keyFlags(0), startTS(0), endTS(0),  keyData(TLV_TYPE_KEY_EVP_PKEY)
{
	return;
}

void RsTlvSecurityKey::TlvClear()
{
	keyId.clear();
	keyFlags = 0;
	startTS = 0;
	endTS = 0;
	keyData.TlvClear();
}

/* clears keyData - but doesn't delete */
void RsTlvSecurityKey::ShallowClear()
{
	keyId.clear();
	keyFlags = 0;
	startTS = 0;
	endTS = 0;
	keyData.bin_data = 0;
	keyData.bin_len = 0;
}


uint32_t RsTlvSecurityKey::TlvSize()
{
	uint32_t s = TLV_HEADER_SIZE; /* header + 4 for size */

	/* now add comment and title length of this tlv object */

	s += GetTlvStringSize(keyId); 
	s += 4;
	s += 4;
	s += 4;
	s += keyData.TlvSize();

	return s;

}

bool  RsTlvSecurityKey::SetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvSecurityKey::SetTlv() Failed not enough space";
		std::cerr << std::endl;
#endif
		return false; /* not enough space */
	}

	bool ok = true;

	/* start at data[offset] */
        /* add mandatory parts first */

	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_SECURITYKEY, tlvsize);

	ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_KEYID, keyId);  
	ok &= setRawUInt32(data, tlvend, offset, keyFlags);
	ok &= setRawUInt32(data, tlvend, offset, startTS);
	ok &= setRawUInt32(data, tlvend, offset, endTS);
	ok &= keyData.SetTlv(data, tlvend, offset);  

	return ok;

}


bool  RsTlvSecurityKey::GetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	
	
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvSecurityKey::GetTlv() Fail, not enough space";
		std::cerr << std::endl;
#endif
		return false; /* not enough space */
	}

	if (tlvtype != TLV_TYPE_SECURITYKEY) /* check type */
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvSecurityKey::GetTlv() Fail, wrong type";
		std::cerr << std::endl;
#endif
		return false;
	}

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	ok &= GetTlvString(data, tlvend, offset, TLV_TYPE_STR_KEYID, keyId);
	ok &= getRawUInt32(data, tlvend, offset, &(keyFlags));
	ok &= getRawUInt32(data, tlvend, offset, &(startTS));
	ok &= getRawUInt32(data, tlvend, offset, &(endTS));
	ok &= keyData.GetTlv(data, tlvend, offset);  
   

	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvSecurityKey::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	if (!ok)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvSecurityKey::GetTlv() Failed somewhere ok == false";
		std::cerr << std::endl;
#endif
	}

	return ok;
	
}


std::ostream &RsTlvSecurityKey::print(std::ostream &out, uint16_t indent)
{ 
	printBase(out, "RsTlvSecurityKey", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "KeyId:" << keyId;
	out << std::endl;

	printIndent(out, int_Indent);
	out << "KeyFlags:" << keyFlags;
	out << std::endl;

	printIndent(out, int_Indent);
	out << "StartTS:" << startTS;
	out << std::endl;

	printIndent(out, int_Indent);
	out << "EndTS:" << endTS;
	out << std::endl;

	keyData.print(out, int_Indent);

	out << std::endl;
	
	printEnd(out, "RsTlvSecurityKey", indent);
	return out;
}




/************************************* RsTlvSecurityKeySet ************************************/

void RsTlvSecurityKeySet::TlvClear()
{
	groupId.clear();
	keys.clear(); //empty list
}

uint32_t RsTlvSecurityKeySet::TlvSize()
{

	uint32_t s = TLV_HEADER_SIZE; /* header */

	std::map<std::string, RsTlvSecurityKey>::iterator it;
	
	s += GetTlvStringSize(groupId); 

	if(!keys.empty())
	{

		for(it = keys.begin(); it != keys.end() ; ++it)
			s += (it->second).TlvSize();

	}

	return s;
}

bool  RsTlvSecurityKeySet::SetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvSecurityKeySet::SetTlv() Failed not enough space";
		std::cerr << std::endl;
#endif
		return false; /* not enough space */
	}

	bool ok = true;

		/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_SECURITYKEYSET , tlvsize);
	
	/* groupId */
	ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_GROUPID, groupId);

	if(!keys.empty())
	{
		std::map<std::string, RsTlvSecurityKey>::iterator it;

		for(it = keys.begin(); it != keys.end() ; ++it)
			ok &= (it->second).SetTlv(data, size, offset);
	}
	

return ok;

}


bool  RsTlvSecurityKeySet::GetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	

	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_SECURITYKEYSET) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	/* groupId */
	ok &= GetTlvString(data, tlvend, offset, TLV_TYPE_STR_GROUPID, groupId);

        /* while there is TLV  */
        while((*offset) + 2 < tlvend)
        {
                /* get the next type */
                uint16_t tlvsubtype = GetTlvType( &(((uint8_t *) data)[*offset]) );

                switch(tlvsubtype)
                {
                        case TLV_TYPE_SECURITYKEY:
			{
				RsTlvSecurityKey key;
				ok &= key.GetTlv(data, size, offset);
				if (ok)
				{
					keys[key.keyId] = key;
					key.ShallowClear(); /* so that the Map can get control - should be ref counted*/
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
		std::cerr << "RsTlvSecurityKeySet::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
}

// prints out contents of RsTlvSecurityKeySet
std::ostream &RsTlvSecurityKeySet::print(std::ostream &out, uint16_t indent)
{
	printBase(out, "RsTlvSecurityKeySet", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "GroupId: " << groupId;
	out << std::endl;

	std::map<std::string, RsTlvSecurityKey>::iterator it;
	for(it = keys.begin(); it != keys.end() ; ++it)
		(it->second).print(out, int_Indent);

	printEnd(out, "RsTlvSecurityKeySet", indent);
	return out;
}


/************************************* RsTlvSecurityKey ************************************/

RsTlvKeySignature::RsTlvKeySignature()
	:RsTlvItem(), signData(TLV_TYPE_SIGN_RSA_SHA1)
{
	return;
}

void RsTlvKeySignature::TlvClear()
{
	keyId.clear();
	signData.TlvClear();
}

/* clears signData - but doesn't delete */
void RsTlvKeySignature::ShallowClear()
{
	keyId.clear();
	signData.bin_data = 0;
	signData.bin_len = 0;
}

uint32_t RsTlvKeySignature::TlvSize()
{
	uint32_t s = TLV_HEADER_SIZE; /* header + 4 for size */

	s += GetTlvStringSize(keyId); 
	s += signData.TlvSize();
	return s;

}

bool  RsTlvKeySignature::SetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvKeySignature::SetTlv() Fail, not enough space";
		std::cerr << std::endl;
#endif
		return false; /* not enough space */
	}

	bool ok = true;

	/* start at data[offset] */
        /* add mandatory parts first */

	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_KEYSIGNATURE, tlvsize);

	ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_KEYID, keyId);  
	ok &= signData.SetTlv(data, tlvend, offset);  

	if (!ok)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvKeySignature::SetTlv() Failed somewhere";
		std::cerr << std::endl;
#endif
	}

	return ok;

}


bool  RsTlvKeySignature::GetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	
	
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint16_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvKeySignature::GetTlv() Not Enough Space";
		std::cerr << std::endl;
#endif
		return false; /* not enough space */
	}

	if (tlvtype != TLV_TYPE_KEYSIGNATURE) /* check type */
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvKeySignature::GetTlv() Type Fail";
		std::cerr << std::endl;
#endif
		return false;
	}

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	ok &= GetTlvString(data, tlvend, offset, TLV_TYPE_STR_KEYID, keyId);
	ok &= signData.GetTlv(data, tlvend, offset);  

	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvKeySignature::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
	
}


std::ostream &RsTlvKeySignature::print(std::ostream &out, uint16_t indent)
{ 
	printBase(out, "RsTlvKeySignature", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "KeyId:" << keyId;
	out << std::endl;

	signData.print(out, int_Indent);

	out << std::endl;
	
	printEnd(out, "RsTlvKeySignature", indent);
	return out;
}

