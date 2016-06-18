/*
 * libretroshare/src/pgp: pgpauxutils.h
 *
 * PGP interface for RetroShare.
 *
 * Copyright 2014-2014 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#pragma once

#include "retroshare/rsids.h"
#include "retroshare/rstypes.h"

/* This is a small collection of PGP functions that are widely used in libretroshare.
 * This interface class allows these functions to be easily mocked for testing.
 */

class PgpAuxUtils
{
	public:

	virtual const RsPgpId &getPGPOwnId()  = 0;
	virtual RsPgpId getPGPId(const RsPeerId& sslid) = 0;
	virtual bool getGPGAllList(std::list<RsPgpId> &ids) = 0;
	virtual	bool getKeyFingerprint(const RsPgpId& id,PGPFingerprintType& fp) const = 0;

	virtual bool parseSignature(unsigned char *sign, unsigned int signlen, RsPgpId& issuer) const =0;
	virtual bool VerifySignBin(const void *data, uint32_t len, unsigned char *sign, unsigned int signlen, const PGPFingerprintType& withfingerprint) = 0;
	virtual bool askForDeferredSelfSignature(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen,int& signature_result ) = 0;
};

class PgpAuxUtilsImpl: public PgpAuxUtils
{
public:
	PgpAuxUtilsImpl();

	virtual const RsPgpId &getPGPOwnId();
	virtual RsPgpId getPGPId(const RsPeerId& sslid);

	virtual bool parseSignature(unsigned char *sign, unsigned int signlen, RsPgpId& issuer) const ;
	virtual	bool getKeyFingerprint(const RsPgpId& id,PGPFingerprintType& fp) const;
	virtual bool VerifySignBin(const void *data, uint32_t len, unsigned char *sign, unsigned int signlen, const PGPFingerprintType& withfingerprint);
	virtual bool getGPGAllList(std::list<RsPgpId> &ids);
    virtual bool askForDeferredSelfSignature(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen,int& signature_result );

};


