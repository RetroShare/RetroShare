/*******************************************************************************
 * libretroshare/src/pgp: pgpauxutils.h                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2014 Robert Fernie  <drbob@lunamutt.com>                          *
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

#include "retroshare/rsids.h"
#include "retroshare/rstypes.h"
#include "util/rsdeprecate.h"

/* This is a small collection of PGP functions that are widely used in libretroshare.
 * This interface class allows these functions to be easily mocked for testing.
 */

class PgpAuxUtils
{
	public:
	virtual ~PgpAuxUtils(){}

	virtual const RsPgpId &getPGPOwnId()  = 0;
    virtual RsPgpId getPgpId(const RsPeerId& sslid) = 0;
    virtual bool getPgpAllList(std::list<RsPgpId> &ids) = 0;
	virtual	bool getKeyFingerprint(const RsPgpId& id,PGPFingerprintType& fp) const = 0;

	virtual bool parseSignature(unsigned char *sign, unsigned int signlen, RsPgpId& issuer) const =0;
	virtual bool VerifySignBin(const void *data, uint32_t len, unsigned char *sign, unsigned int signlen, const PGPFingerprintType& withfingerprint) = 0;
};

class PgpAuxUtilsImpl: public PgpAuxUtils
{
public:
	PgpAuxUtilsImpl();

	virtual const RsPgpId &getPGPOwnId();
    virtual RsPgpId getPgpId(const RsPeerId& sslid);

	virtual bool parseSignature(unsigned char *sign, unsigned int signlen, RsPgpId& issuer) const ;
	virtual	bool getKeyFingerprint(const RsPgpId& id,PGPFingerprintType& fp) const;
	virtual bool VerifySignBin(const void *data, uint32_t len, unsigned char *sign, unsigned int signlen, const PGPFingerprintType& withfingerprint);
    virtual bool getPgpAllList(std::list<RsPgpId> &ids);
};


