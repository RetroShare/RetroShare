/*******************************************************************************
 * unittests/libretroshare/services/gxs/FakePgpAuxUtils.cc                     *
 *                                                                             *
 * Copyright 2014      by Robert Fernie    <retroshare.team@gmail.com>         *
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

#include "FakePgpAuxUtils.h"

#include <algorithm>

FakePgpAuxUtils::FakePgpAuxUtils(const RsPeerId& ownId)
{
	mOwnId = getPGPId(ownId);
	addPeerIdToPgpList(ownId);
}

void FakePgpAuxUtils::addPeerListToPgpList(const std::list<RsPeerId> &ids)
{
	std::list<RsPeerId>::const_iterator it;
	for(it = ids.begin(); it != ids.end(); it++)
	{
		addPeerIdToPgpList(*it);
	}
}

void FakePgpAuxUtils::addPeerIdToPgpList(const RsPeerId &id)
{
	RsPgpId pgpId = getPGPId(id);
	if (mPgpList.end() == std::find(mPgpList.begin(), mPgpList.end(), pgpId))
	{
		mPgpList.push_back(pgpId);
	}
}

const RsPgpId & FakePgpAuxUtils::getPGPOwnId()
{
	return mOwnId;
}

RsPgpId FakePgpAuxUtils::getPGPId(const RsPeerId& sslid)
{
	/* convert an sslId */
	std::string idstring = sslid.toStdString();
	idstring.resize(RsPgpId::SIZE_IN_BYTES*2, '0');
	RsPgpId pgpId(idstring);

#if 0
	std::cerr << "FakePgpAuxUtils::getPGPId()";
	std::cerr << " RsPeerId: " << sslid.toStdString();
	std::cerr << " RsPgpId: " << pgpId.toStdString();
	std::cerr << std::endl;
#endif
	return pgpId;
}

bool FakePgpAuxUtils::getKeyFingerprint(const RsPgpId& id,PGPFingerprintType& fp) const
{
	/* convert an sslId */
	std::string idstring = id.toStdString();
	idstring.resize(PGPFingerprintType::SIZE_IN_BYTES*2, '0');
	PGPFingerprintType pgpfp(idstring);

#if 0
	std::cerr << "FakePgpAuxUtils::getKeyFingerprint()";
	std::cerr << " RsPeerId: " << id.toStdString();
	std::cerr << " RsPgpId: " << pgpfp.toStdString();
	std::cerr << std::endl;
#endif

	fp = pgpfp;
	return true;
}

bool FakePgpAuxUtils::parseSignature(unsigned char* /*sign*/, unsigned int /*signlen*/, RsPgpId& /*issuer*/) const
{
	return true;
}

bool FakePgpAuxUtils::VerifySignBin(const void* /*data*/, uint32_t /*len*/, unsigned char* /*sign*/, unsigned int /*signlen*/, const PGPFingerprintType& /*withfingerprint*/)
{
	return true;
}

bool FakePgpAuxUtils::getGPGAllList(std::list<RsPgpId> &ids)
{
	ids = mPgpList;
	return true;
}

bool FakePgpAuxUtils::askForDeferredSelfSignature(const void* /*data*/, const uint32_t /*len*/, unsigned char *sign, unsigned int *signlen,int& signature_result, std::string /*reason = ""*/ )
{
	for(unsigned int i = 0; i < *signlen; i++)
	{
		sign[i] = 0;
	}
	signature_result = 1;
	return true;
}

