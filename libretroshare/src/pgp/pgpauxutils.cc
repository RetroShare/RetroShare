/*******************************************************************************
 * libretroshare/src/pgp: pgpauxutils.cc                                       *
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
#include "pgp/pgpauxutils.h"

#include "pqi/authgpg.h"
#include "rsserver/p3face.h"
#include "retroshare/rsiface.h"
#include "retroshare/rspeers.h"

PgpAuxUtilsImpl::PgpAuxUtilsImpl()
{
	return;
}


const RsPgpId& PgpAuxUtilsImpl::getPGPOwnId() 
{
    return AuthPGP::getPgpOwnId();
}

RsPgpId PgpAuxUtilsImpl::getPgpId(const RsPeerId& sslid)
{
	return rsPeers->getGPGId(sslid);
}

bool PgpAuxUtilsImpl::getKeyFingerprint(const RsPgpId& id,PGPFingerprintType& fp) const
{
    return AuthPGP::getKeyFingerprint(id, fp);
}

bool PgpAuxUtilsImpl::VerifySignBin(const void *data, 
		uint32_t len, 
		unsigned char *sign, 
		unsigned int signlen, 
		const PGPFingerprintType& withfingerprint)

{
    return AuthPGP::VerifySignBin(data, len, sign, signlen, withfingerprint);
}

bool PgpAuxUtilsImpl::getPgpAllList(std::list<RsPgpId> &ids)
{
    return AuthPGP::getPgpAllList(ids);
}

bool PgpAuxUtilsImpl::parseSignature(unsigned char *sign, unsigned int signlen, RsPgpId& issuer) const 
{
    return AuthPGP::parseSignature(sign,signlen,issuer);
}




