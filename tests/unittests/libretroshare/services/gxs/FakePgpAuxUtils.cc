/*
 * unittests/libretroshare/services: fakepgpauxutils.h
 *
 * Identity interface for RetroShare.
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
	std::cerr << "FakePgpAuxUtils::getPGPId()";
	/* convert an sslId */
	std::string idstring = sslid.toStdString();
	idstring.resize(RsPgpId::SIZE_IN_BYTES*2, '0');
	RsPgpId pgpId(idstring);

	std::cerr << " RsPeerId: " << sslid.toStdString();
	std::cerr << " RsPgpId: " << pgpId.toStdString();
	std::cerr << std::endl;
	return pgpId;
}


bool FakePgpAuxUtils::getKeyFingerprint(const RsPgpId& id,PGPFingerprintType& fp) const
{
	std::cerr << "FakePgpAuxUtils::getKeyFingerprint()";
	/* convert an sslId */
	std::string idstring = id.toStdString();
	idstring.resize(PGPFingerprintType::SIZE_IN_BYTES*2, '0');
	PGPFingerprintType pgpfp(idstring);

	std::cerr << " RsPeerId: " << id.toStdString();
	std::cerr << " RsPgpId: " << pgpfp.toStdString();
	std::cerr << std::endl;

	fp = pgpfp;
	return true;
}

bool FakePgpAuxUtils::VerifySignBin(const void *data, uint32_t len, unsigned char *sign, unsigned int signlen, const PGPFingerprintType& withfingerprint)
{
	return true;
}

bool FakePgpAuxUtils::getGPGAllList(std::list<RsPgpId> &ids)
{
	ids = mPgpList;
	return true;
}


bool FakePgpAuxUtils::askForDeferredSelfSignature(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen,int& signature_result )
{
	for(int i = 0; i < *signlen; i++)
	{
		sign[i] = 0;
	}
	signature_result = 1;
	return true;
}

