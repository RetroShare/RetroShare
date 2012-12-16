#ifndef RS_TLV_KEY_TYPES_H
#define RS_TLV_KEY_TYPES_H

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

#include <map>
#include "serialiser/rstlvtypes.h"

const uint32_t RSTLV_KEY_TYPE_MASK              = 0x000f;
const uint32_t RSTLV_KEY_DISTRIB_MASK           = 0x00f0;
const uint32_t RSTLV_KEY_TYPE_PUBLIC_ONLY       = 0x0001;
const uint32_t RSTLV_KEY_TYPE_FULL              = 0x0002;
const uint32_t RSTLV_KEY_TYPE_SHARED            = 0x0004;
const uint32_t RSTLV_KEY_DISTRIB_PUBLIC         = 0x0010;
const uint32_t RSTLV_KEY_DISTRIB_PRIVATE        = 0x0020;
const uint32_t RSTLV_KEY_DISTRIB_ADMIN          = 0x0040;
const uint32_t RSTLV_KEY_DISTRIB_IDENTITY       = 0x0080;


class RsTlvSecurityKey: public RsTlvItem
{
	public:
	 RsTlvSecurityKey();
virtual ~RsTlvSecurityKey() { return; }
virtual uint32_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	/* clears KeyData - but doesn't delete - to transfer ownership */
	void ShallowClear(); 

	std::string keyId;		// Mandatory :
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
virtual uint32_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	std::string groupId;				// Mandatory :
	std::map<std::string, RsTlvSecurityKey> keys;	// Mandatory :
};


class RsTlvKeySignature: public RsTlvItem
{
	public:
	 RsTlvKeySignature();
virtual ~RsTlvKeySignature() { return; }
virtual uint32_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	void	ShallowClear(); /* clears signData - but doesn't delete */

	std::string keyId;		// Mandatory :
        RsTlvBinaryData signData; 	// Mandatory :
	// NO Certificates in Signatures... add as separate data type.
};

typedef uint32_t SignType;

class RsTlvKeySignatureSet : public RsTlvItem
{
public:
    RsTlvKeySignatureSet();
    virtual ~RsTlvKeySignatureSet() { return; }
    virtual uint32_t TlvSize();
    virtual void	 TlvClear();
    virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
    virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
    virtual std::ostream &print(std::ostream &out, uint16_t indent);

    std::map<SignType, RsTlvKeySignature> keySignSet; // mandatory
};


#endif

