#pragma once

/*
 * libretroshare/src/serialiser: rstlvkeys.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2008 by Robert Fernie
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
 ******************************************************************/

#include "serialiser/rstlvitem.h"
#include "serialiser/rstlvbinary.h"
#include "retroshare/rsgxsifacetypes.h"

#include <map>

const uint32_t RSTLV_KEY_TYPE_MASK              = 0x000f;
const uint32_t RSTLV_KEY_TYPE_PUBLIC_ONLY       = 0x0001;
const uint32_t RSTLV_KEY_TYPE_FULL              = 0x0002;

const uint32_t RSTLV_KEY_DISTRIB_PUBLIC_deprecated = 0x0010;// was used as PUBLISH flag. Probably a typo.

const uint32_t RSTLV_KEY_DISTRIB_PUBLISH        = 0x0020;
const uint32_t RSTLV_KEY_DISTRIB_ADMIN          = 0x0040;
const uint32_t RSTLV_KEY_DISTRIB_IDENTITY       = 0x0080;
const uint32_t RSTLV_KEY_DISTRIB_MASK           = 0x00f0;

class RsTlvSecurityKey: public RsTlvItem
{
	public:
		RsTlvSecurityKey();
		virtual ~RsTlvSecurityKey() {}

		virtual uint32_t TlvSize() const;
		virtual void	 TlvClear();
		virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const; 
		virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); 
		virtual std::ostream &print(std::ostream &out, uint16_t indent) const;

		/* clears KeyData - but doesn't delete - to transfer ownership */
		void ShallowClear(); 

		RsGxsId keyId;		// Mandatory :
		uint32_t keyFlags;		// Mandatory ;
		uint32_t startTS;		// Mandatory : 
		uint32_t endTS;			// Mandatory : 
		RsTlvBinaryData keyData; 	// Mandatory : 
};

class RsTlvSecurityKeySet: public RsTlvItem
{
	public:
	 RsTlvSecurityKeySet() { return; }
virtual ~RsTlvSecurityKeySet() { return; }
virtual uint32_t TlvSize() const;
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const; 
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); 
virtual std::ostream &print(std::ostream &out, uint16_t indent) const;

	std::string groupId;				// Mandatory :
	std::map<RsGxsId, RsTlvSecurityKey> keys;	// Mandatory :
};


class RsTlvKeySignature: public RsTlvItem
{
	public:
		RsTlvKeySignature();
		virtual ~RsTlvKeySignature() { return; }
		virtual uint32_t TlvSize() const;
		virtual void	 TlvClear();
		virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const; 
		virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); 
		virtual std::ostream &print(std::ostream &out, uint16_t indent) const;

		void	ShallowClear(); /* clears signData - but doesn't delete */

		RsGxsId keyId;		// Mandatory :
		RsTlvBinaryData signData; 	// Mandatory :
};

typedef uint32_t SignType;

class RsTlvKeySignatureSet : public RsTlvItem
{
public:
    RsTlvKeySignatureSet();
    virtual ~RsTlvKeySignatureSet() { return; }
    virtual uint32_t TlvSize() const;
    virtual void	 TlvClear();
    virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const; 
    virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); 
    virtual std::ostream &print(std::ostream &out, uint16_t indent) const;

    std::map<SignType, RsTlvKeySignature> keySignSet; // mandatory
};


