/*******************************************************************************
 * unittests/libretroshare/services/gxs/FakePgpAuxUtils.h                      *
 *                                                                             *
 * Copyright 2014      by Robert Fernie    <retroshare.project@gmail.com>      *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
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
 ******************************************************************************/

#pragma once

#include "pgp/pgpauxutils.h"

class FakePgpAuxUtils: public PgpAuxUtils
{
public:
	FakePgpAuxUtils(const RsPeerId& ownId);

	virtual const RsPgpId &getPGPOwnId();
	virtual RsPgpId getPgpId(const RsPeerId& sslid);
	virtual	bool getKeyFingerprint(const RsPgpId& id,PGPFingerprintType& fp) const;

	virtual bool parseSignature(unsigned char *sign, unsigned int signlen, RsPgpId& issuer) const;
	virtual bool VerifySignBin(const void *data, uint32_t len, unsigned char *sign, unsigned int signlen, const PGPFingerprintType& withfingerprint);

	virtual void addPeerListToPgpList(const std::list<RsPeerId> &ids);
	virtual void addPeerIdToPgpList(const RsPeerId &id);
	virtual bool getPgpAllList(std::list<RsPgpId> &ids);
private:
	RsPgpId mOwnId;
	std::list<RsPgpId> mPgpList;

};


