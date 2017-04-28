#pragma once

/*
 * libretroshare/src/serialiser: rstlvbinary.h
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

	virtual uint32_t TlvSize() const;
	virtual void	 TlvClear(){}
	virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const;
	virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset);

    uint8_t *& mDataRef ;
    uint32_t & mSizeRef ;
    uint16_t tlvtype ;
};



