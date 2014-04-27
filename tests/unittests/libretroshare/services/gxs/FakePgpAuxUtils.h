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

#pragma once

#include "pgp/pgpauxutils.h"

class FakePgpAuxUtils: public PgpAuxUtils
{
public:
	FakePgpAuxUtils(const RsPeerId& ownId);

	virtual const RsPgpId &getPGPOwnId();
	virtual RsPgpId getPGPId(const RsPeerId& sslid);

	virtual	bool getKeyFingerprint(const RsPgpId& id,PGPFingerprintType& fp) const;
	virtual bool VerifySignBin(const void *data, uint32_t len, unsigned char *sign, unsigned int signlen, const PGPFingerprintType& withfingerprint);
	virtual bool askForDeferredSelfSignature(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen,int& signature_result );

	virtual void addPeerListToPgpList(const std::list<RsPeerId> &ids);
	virtual void addPeerIdToPgpList(const RsPeerId &id);
	virtual bool getGPGAllList(std::list<RsPgpId> &ids);
private:
	RsPgpId mOwnId;
	std::list<RsPgpId> mPgpList;

};


