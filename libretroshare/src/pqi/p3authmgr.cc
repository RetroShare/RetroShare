/*
 * libretroshare/src/pqi: p3authmgr.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#include "pqi/p3authmgr.h"

pqiAuthDetails::pqiAuthDetails()
	:trustLvl(0), ownsign(false), trusted(0)
{
	return;
}

p3DummyAuthMgr::p3DummyAuthMgr()
{
	/* for the truely dummy option */
	mOwnId = "OWNID";

	pqiAuthDetails ownDetails;
	ownDetails.id = mOwnId;
	ownDetails.name = "Youself";
	ownDetails.email = "me@me.com";
	ownDetails.location = "here";
	ownDetails.org = "me.com";

	ownDetails.trustLvl = 6;
	ownDetails.ownsign = true;
	ownDetails.trusted = true;

	/* ignoring fpr and signers */

	mPeerList[mOwnId] = ownDetails;

}

p3DummyAuthMgr::p3DummyAuthMgr(std::string ownId, std::list<pqiAuthDetails> peers)
{
	mOwnId = ownId;
	bool addedOwn = false;

	std::list<pqiAuthDetails>::iterator it;
	for(it = peers.begin(); it != peers.end(); it++)
	{
		mPeerList[it->id] = (*it);
		if (it->id == ownId)
		{
			addedOwn = true;
		}
	}
	if (!addedOwn)
	{
		pqiAuthDetails ownDetails;
		ownDetails.id = mOwnId;
		ownDetails.name = "Youself";
		ownDetails.email = "me@me.com";
		ownDetails.location = "here";
		ownDetails.org = "me.com";

		ownDetails.trustLvl = 6;
		ownDetails.ownsign = true;
		ownDetails.trusted = true;

		/* ignoring fpr and signers */

		mPeerList[mOwnId] = ownDetails;
	}
}
	
bool   p3DummyAuthMgr:: active()
{
	return true;
}

int     p3DummyAuthMgr::InitAuth(const char *srvr_cert, const char *priv_key, 
                                        const char *passwd)
{
	return 1;
}

bool    p3DummyAuthMgr::CloseAuth()
{
	return true;
}

int     p3DummyAuthMgr::setConfigDirectories(const char *cdir, const char *ndir)
{
	return 1;
}

std::string p3DummyAuthMgr::OwnId()
{
	return mOwnId;
}

bool	p3DummyAuthMgr::getAllList(std::list<std::string> &ids)
{
	std::map<std::string, pqiAuthDetails>::iterator it;
	for(it = mPeerList.begin(); it != mPeerList.end(); it++)
	{
		ids.push_back(it->first);
	}
	return true;
}

bool	p3DummyAuthMgr::getAuthenticatedList(std::list<std::string> &ids)
{
	std::map<std::string, pqiAuthDetails>::iterator it;
	for(it = mPeerList.begin(); it != mPeerList.end(); it++)
	{
		if (it->second.trustLvl > 3)
		{
			ids.push_back(it->first);
		}
	}
	return true;
}

bool	p3DummyAuthMgr::getUnknownList(std::list<std::string> &ids)
{
	std::map<std::string, pqiAuthDetails>::iterator it;
	for(it = mPeerList.begin(); it != mPeerList.end(); it++)
	{
		if (it->second.trustLvl <= 3)
		{
			ids.push_back(it->first);
		}
	}
	return true;
}

bool	p3DummyAuthMgr::isValid(std::string id)
{
	std::map<std::string, pqiAuthDetails>::iterator it;
	return (mPeerList.end() != mPeerList.find(id));
}


bool	p3DummyAuthMgr::isAuthenticated(std::string id)
{
	std::map<std::string, pqiAuthDetails>::iterator it;
	if (mPeerList.end() != (it = mPeerList.find(id)))
	{
		return (it->second.trustLvl > 3);
	}
	return false;
}

std::string p3DummyAuthMgr::getName(std::string id)
{
	std::map<std::string, pqiAuthDetails>::iterator it;
	if (mPeerList.end() != (it = mPeerList.find(id)))
	{
		return it->second.name;
	}
	std::string empty("");
	return empty;
}

bool	p3DummyAuthMgr::getDetails(std::string id, pqiAuthDetails &details)
{
	std::map<std::string, pqiAuthDetails>::iterator it;
	if (mPeerList.end() != (it = mPeerList.find(id)))
	{
		details = it->second;
		return true;
	}
	return false;
}

bool p3DummyAuthMgr::LoadCertificateFromString(std::string pem, std::string &id)
{
	return false;
}

std::string p3DummyAuthMgr::SaveCertificateToString(std::string id)
{
	std::string dummy("CERT STRING");
	return dummy;
}

bool p3DummyAuthMgr::LoadCertificateFromFile(std::string filename, std::string &id)
{
	return false;
}

bool p3DummyAuthMgr::SaveCertificateToFile(std::string id, std::string filename)
{
	return false;
}

		/* Signatures */
bool p3DummyAuthMgr::SignCertificate(std::string id)
{
	return false;
}

bool p3DummyAuthMgr::RevokeCertificate(std::string id)
{
	return false;
}

bool p3DummyAuthMgr::TrustCertificate(std::string id, bool trust)
{
	return false;
}

