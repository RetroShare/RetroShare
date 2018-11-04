/*******************************************************************************
 * libretroshare/src/serialiser: rstlvbinary.cc                                *
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
#include "util/rsmemory.h"
#include "serialiser/rstlvbinary.h"

#include "serialiser/rstlvbase.h"
#include <stdlib.h>
#include <sstream>
#include <iomanip>

// #define TLV_DEBUG 1

/*!********************************** RsTlvFileBinaryData **********************************/


RsTlvBinaryData::RsTlvBinaryData()
	:tlvtype(0), bin_len(0), bin_data(NULL)
{
}

RsTlvBinaryData::RsTlvBinaryData(uint16_t t)
	:tlvtype(t), bin_len(0), bin_data(NULL)
{
}

RsTlvBinaryData::RsTlvBinaryData(const RsTlvBinaryData &b)
    : tlvtype(b.tlvtype), bin_len(0) , bin_data(NULL) {

    setBinData(b.bin_data, b.bin_len);
}

RsTlvBinaryData::~RsTlvBinaryData()
{
	TlvClear();
}

void RsTlvBinaryData::operator =(const RsTlvBinaryData& b){

    setBinData(b.bin_data, b.bin_len);
    tlvtype = b.tlvtype;
}

/// used to allocate memory andinitialize binary data member 
bool     RsTlvBinaryData::setBinData(const void *data, uint32_t size)
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

	bin_data = rs_malloc(bin_len);
    
    	if(bin_data == NULL)
            return false ;
        
	memcpy(bin_data, data, bin_len);
	return true;
}

void RsTlvBinaryData::TlvClear()
{
	free(bin_data);
	TlvShallowClear();
}

void RsTlvBinaryData::TlvShallowClear()
{
	bin_data = NULL;
	bin_len = 0;
}

uint32_t RsTlvBinaryData::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; /* header */

	if (bin_data != NULL) 
		s += bin_len; // len is the size of data

	return s;
}


bool     RsTlvBinaryData::SetTlv(void *data, uint32_t size, uint32_t *offset) const
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



bool     RsTlvBinaryData::GetTlv(void *data, uint32_t size, uint32_t *offset)
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
std::ostream &RsTlvBinaryDataRef::print(std::ostream &out, uint16_t indent) const
{
        uint16_t int_Indent = indent + 2;

        uint32_t i;
        std::ostringstream sout;
        printIndent(sout, indent);
        sout << "RsTlvBinaryData: Type: " << tlvtype << " Size: " << mSizeRef;
        sout << std::hex;

        for(i = 0; i < mSizeRef; i++)
        {
                if (i % 16 == 0)
                {
                        sout << std::endl;
        		printIndent(sout, int_Indent);
                }
                sout << std::setw(2) << std::setfill('0')
                        << (int) (((unsigned char *) mDataRef)[i]) << ":";
        }

        sout << std::endl;
        out << sout.str();

        printEnd(out, "RsTlvBinaryData", indent);
        return out;

}

std::ostream &RsTlvBinaryData::print(std::ostream &out, uint16_t indent) const
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

bool     RsTlvBinaryDataRef::SetTlv(void *data, uint32_t size, uint32_t *offset) const
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
	if (mDataRef != NULL && mSizeRef > 0)
	{
		memcpy(&(((uint8_t *) data)[*offset]), mDataRef, mSizeRef);
		*offset += mSizeRef;
	}
	return ok;
}
bool     RsTlvBinaryDataRef::GetTlv(void *data, uint32_t size, uint32_t *offset)
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
		return false; /* bad tlv size */

	if (tlvtype != tlvtype_in) /* check type */
		return false;

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	mDataRef = (uint8_t*)rs_malloc(tlvsize - TLV_HEADER_SIZE) ;

	if(mDataRef == NULL)
		return false ;

	mSizeRef = tlvsize - TLV_HEADER_SIZE;

	memcpy(mDataRef,&(((uint8_t *) data)[*offset]), tlvsize - TLV_HEADER_SIZE);
	*offset += mSizeRef;

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

	return true;
}

uint32_t RsTlvBinaryDataRef::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; /* header */

	if (mDataRef != NULL)
		s += mSizeRef; // len is the size of data

	return s;
}
