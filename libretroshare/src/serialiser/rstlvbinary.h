/*******************************************************************************
 * libretroshare/src/serialiser: rstlvbinary.h                                 *
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

#include "serialiser/rstlvitem.h"


class RsTlvBinaryData: public RsTlvItem
{
public:
	RsTlvBinaryData();
	RsTlvBinaryData(uint16_t t);
	RsTlvBinaryData(const RsTlvBinaryData& b); // as per rule of three
	void operator=(const RsTlvBinaryData& b); // as per rule of three
        
	virtual ~RsTlvBinaryData(); // as per rule of three
        
	virtual	uint32_t TlvSize() const;
	virtual	void	 TlvClear(); /*! Initialize fields to empty legal values ( "0", "", etc) */
	virtual	void	 TlvShallowClear(); /*! Don't delete the binary data */

	/// Serialise.
	/*! Serialise Tlv to buffer(*data) of 'size' bytes starting at *offset */
	virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const;

	/// Deserialise.
	/*! Deserialise Tlv buffer(*data) of 'size' bytes starting at *offset */
	virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); 
	virtual std::ostream &print(std::ostream &out, uint16_t indent) const; /*! Error/Debug util function */

	// mallocs the necessary size, and copies data into the allocated buffer in bin_data
	bool    setBinData(const void *data, uint32_t size);

	uint16_t tlvtype;	/// set/checked against TLV input 
	uint32_t bin_len;	/// size of malloc'ed data (not serialised) 
	void    *bin_data;	/// mandatory
};

// This class is mainly used for on-the-fly serialization

class RsTlvBinaryDataRef: public RsTlvItem
{
public:
    RsTlvBinaryDataRef(uint16_t type,uint8_t *& data_ref,uint32_t& size_ref) : mDataRef(data_ref),mSizeRef(size_ref),tlvtype(type) {}
	virtual ~RsTlvBinaryDataRef() {}

	virtual std::ostream &print(std::ostream &out, uint16_t indent) const;

	virtual uint32_t TlvSize() const;
	virtual void	 TlvClear(){}
	virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const;
	virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset);

    uint8_t *& mDataRef ;
    uint32_t & mSizeRef ;
    uint16_t tlvtype ;
};



