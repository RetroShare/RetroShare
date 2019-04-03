/*******************************************************************************
 * libretroshare/src/serialiser: rstlvkeys.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008 by Robert Fernie <retroshare@lunamutt.com>                   *
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
 ******************************************************************/

#include "serialiser/rstlvitem.h"
#include "serialiser/rstlvbinary.h"
#include "serialiser/rstlvbase.h"
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

// Old class for RsTlvSecurityKey. Is kept for backward compatibility, but should not be serialised anymore

class RsTlvRSAKey: public RsTlvItem
{
public:
	RsTlvRSAKey();
    virtual bool     checkKey() const = 0 ;	// this pure virtual forces people to explicitly declare if they use a public or a private key.

    virtual uint32_t TlvSize() const;
    virtual void 	TlvClear();
    virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const; 
    virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); 
    virtual std::ostream& print(std::ostream &out, uint16_t indent) const;

    /* clears KeyData - but doesn't delete - to transfer ownership */
    void ShallowClear(); 

    uint32_t getKeyTypeTlv(void *data, uint32_t size, uint32_t *offset) const;

    RsGxsId keyId;		// Mandatory :
    uint32_t keyFlags;		// Mandatory ;
    uint32_t startTS;		// Mandatory : 
    uint32_t endTS;		// Mandatory : 
    RsTlvBinaryData keyData; 	// Mandatory : 
};

// The two classes below are by design incompatible, making it impossible to pass a private key as a public key

class RsTlvPrivateRSAKey: public RsTlvRSAKey
{    
public:
	RsTlvPrivateRSAKey():RsTlvRSAKey() {}
	virtual ~RsTlvPrivateRSAKey() {}

	virtual bool checkKey() const  ;
};
class RsTlvPublicRSAKey: public RsTlvRSAKey
{
public:
	RsTlvPublicRSAKey():RsTlvRSAKey() {}
	virtual ~RsTlvPublicRSAKey() {}

	virtual bool checkKey() const  ;
};

class RsTlvSecurityKeySet: public RsTlvItem
{
public:
	RsTlvSecurityKeySet() { return; }
	virtual ~RsTlvSecurityKeySet() { return; }
        
    	// creates the public keys that are possible missing although the private keys are present.
    
        void createPublicFromPrivateKeys(); 
        
	virtual uint32_t TlvSize() const;
	virtual void	 TlvClear();
	virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) const; 
	virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); 
	virtual std::ostream &print(std::ostream &out, uint16_t indent) const;

	std::string groupId;					// Mandatory :
	std::map<RsGxsId, RsTlvPublicRSAKey> public_keys;	// Mandatory :
	std::map<RsGxsId, RsTlvPrivateRSAKey> private_keys;	// Mandatory :
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


