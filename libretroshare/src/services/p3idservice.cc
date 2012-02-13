/*
 * libretroshare/src/services p3idservice.cc
 *
 * Id interface for RetroShare.
 *
 * Copyright 2012-2012 by Robert Fernie.
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

#include "services/p3idservice.h"

#include "util/rsrandom.h"
#include <retroshare/rspeers.h>
#include <sstream>

/****
 * #define ID_DEBUG 1
 ****/

RsIdentity *rsIdentity = NULL;


p3IdService::p3IdService(uint16_t type)
	:p3Service(type), mIdMtx("p3IdService"), mUpdated(true)
{
     	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
	return;
}


int	p3IdService::tick()
{
	std::cerr << "p3IdService::tick()";
	std::cerr << std::endl;
	
	return 0;
}

bool p3IdService::updated()
{
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

	if (mUpdated)
	{
		mUpdated = false;
		return true;
	}
	return false;
}

bool p3IdService::getGpgIdDetails(const std::string &id, std::string &gpgName, std::string &gpgEmail)
{
	gpgName = "aGpgName";
	gpgEmail = "a@GpgMailAddress";

	return true;
}

bool p3IdService::getIdentityList(std::list<std::string> &ids)
{
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

        std::map<std::string, RsIdData>::iterator it;
	for(it = mIds.begin(); it != mIds.end(); it++)
	{
		ids.push_back(it->first);
	}
	
	return false;
}

bool p3IdService::getIdentity(const std::string &id, RsIdData &data)
{
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
	
        std::map<std::string, RsIdData>::iterator it;
	it = mIds.find(id);
	if (it == mIds.end())
	{
		return false;
	}
	
	data = it->second;
	return true;
}

bool p3IdService::getIdReputation(const std::string &id, RsIdReputation &reputation)
{
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
	
        std::map<std::string, RsIdReputation>::iterator it;
	it = mReputations.find(id);
	if (it == mReputations.end())
	{
		return false;
	}
	
	reputation = it->second;
	return true;
}


bool p3IdService::getIdPeerOpinion(const std::string &aboutid, const std::string &peerId, RsIdOpinion &opinion)
{
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
	
        std::map<std::string, std::map<std::string, RsIdOpinion> >::iterator it;
        std::map<std::string, RsIdOpinion>::iterator oit;
	it = mOpinions.find(aboutid);
	if (it == mOpinions.end())
	{
		return false;
	}

	oit = it->second.find(peerId);
	if (oit == it->second.end())
	{
		return false;
	}
	
	opinion = oit->second;
	return true;
}


/* details are updated  */
bool p3IdService::updateIdentity(RsIdData &data)
{
	if (data.mKeyId.empty())
	{
		/* new photo */

		/* generate a temp id */
		data.mKeyId = genRandomId();

		if (data.mIdType & RSID_TYPE_REALID)
		{
			data.mGpgIdHash = genRandomId();
		}
		else
		{
			data.mGpgIdHash = "";
		}
		
	}
	
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

	mUpdated = true;

	/* add / modify */
	mIds[data.mKeyId] = data;

	return true;
}




bool p3IdService::updateOpinion(RsIdOpinion &opinion)
{
	if (opinion.mKeyId.empty())
	{
		/* new photo */
		std::cerr << "p3IdService::updateOpinion() Missing KeyId";
		std::cerr << std::endl;
		return false;
	}
	
	/* check if its a mod or new page */
	if (opinion.mPeerId.empty())
	{
		std::cerr << "p3IdService::updateOpinion() Missing PeerId";
		std::cerr << std::endl;
		return false;
	}

	std::cerr << "p3IdService::updateOpinion() KeyId: " << opinion.mKeyId;
	std::cerr << std::endl;
	std::cerr << "p3IdService::updateOpinion() PeerId: " << opinion.mPeerId;
	std::cerr << std::endl;

	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

	mUpdated = true;

	std::map<std::string, std::map<std::string, RsIdOpinion> >::iterator it;
	std::map<std::string, RsIdOpinion>::iterator oit;

	it = mOpinions.find(opinion.mKeyId);
	if (it == mOpinions.end())
	{
		std::map<std::string, RsIdOpinion> emptyMap;
		mOpinions[opinion.mKeyId] = emptyMap;

		it = mOpinions.find(opinion.mKeyId);
	}

	(it->second)[opinion.mPeerId] = opinion;
	return true;
}



	
std::string p3IdService::genRandomId()
{
	std::string randomId;
	for(int i = 0; i < 20; i++)
	{
		randomId += (char) ('a' + (RSRandom::random_u32() % 26));
	}
	
	return randomId;
}
	


void p3IdService::generateDummyData()
{
	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

	/* grab all the gpg ids... and make some ids */

	std::list<std::string> gpgids;
	std::list<std::string>::iterator it;
	
	rsPeers->getGPGAllList(gpgids);

	std::string ownId = rsPeers->getGPGOwnId();
	gpgids.push_back(ownId);

	int i;
	for(it = gpgids.begin(); it != gpgids.end(); it++)
	{
		/* create one or two for each one */
		int nIds = 1 + (RSRandom::random_u32() % 2);
		for(i = 0; i < nIds; i++)
		{
			RsIdData id;

                	RsPeerDetails details;

			id.mKeyId = genRandomId();
			id.mIdType = RSID_TYPE_REALID;
			id.mGpgIdHash = genRandomId();

                	if (rsPeers->getPeerDetails(*it, details))
			{
				std::ostringstream out;
				out << details.name << "_" << i + 1;

				id.mNickname = out.str();
				id.mGpgIdKnown = true;
			
				id.mGpgId = *it;
				id.mGpgName = details.name;
				id.mGpgEmail = details.email;

				if (*it == ownId)
				{
					id.mIdType |= RSID_RELATION_YOURSELF;
				}
				else if (rsPeers->isGPGAccepted(*it))
				{
					id.mIdType |= RSID_RELATION_FRIEND;
				}
				else
				{
					id.mIdType |= RSID_RELATION_OTHER;
				}
				
			}
			else
			{
				std::cerr << "p3IdService::generateDummyData() missing" << std::endl;
				std::cerr << std::endl;

				id.mIdType |= RSID_RELATION_OTHER;
				id.mNickname = genRandomId();
				id.mGpgIdKnown = false;
			}

			mIds[id.mKeyId] = id;
		}
	}

#define MAX_RANDOM_GPGIDS	1000
#define MAX_RANDOM_PSEUDOIDS	5000

	int nFakeGPGs = (RSRandom::random_u32() % MAX_RANDOM_GPGIDS);
	int nFakePseudoIds = (RSRandom::random_u32() % MAX_RANDOM_PSEUDOIDS);

	/* make some fake gpg ids */
	for(i = 0; i < nFakeGPGs; i++)
	{
		RsIdData id;

                RsPeerDetails details;

		id.mKeyId = genRandomId();
		id.mIdType = RSID_TYPE_REALID;
		id.mGpgIdHash = genRandomId();

		id.mIdType |= RSID_RELATION_OTHER;
		id.mNickname = genRandomId();
		id.mGpgIdKnown = false;
		id.mGpgId = "";
		id.mGpgName = "";
		id.mGpgEmail = "";

		mIds[id.mKeyId] = id;
	}

	/* make lots of pseudo ids */
	for(i = 0; i < nFakePseudoIds; i++)
	{
		RsIdData id;

                RsPeerDetails details;

		id.mKeyId = genRandomId();
		id.mIdType = RSID_TYPE_PSEUDONYM;
		id.mGpgIdHash = "";

		id.mNickname = genRandomId();
		id.mGpgIdKnown = false;
		id.mGpgId = "";
		id.mGpgName = "";
		id.mGpgEmail = "";

		mIds[id.mKeyId] = id;
	}

	mUpdated = true;

	return;
}



std::string rsIdTypeToString(uint32_t idtype)
{
	std::string str;
	if (idtype & RSID_TYPE_REALID)
	{
		str += "GPGID ";
	}
	if (idtype & RSID_TYPE_PSEUDONYM)
	{
		str += "PSEUDO ";
	}
	if (idtype & RSID_RELATION_YOURSELF)
	{
		str += "YOURSELF ";
	}
	if (idtype & RSID_RELATION_FRIEND)
	{
		str += "FRIEND ";
	}
	if (idtype & RSID_RELATION_FOF)
	{
		str += "FOF ";
	}
	if (idtype & RSID_RELATION_OTHER)
	{
		str += "OTHER ";
	}
	if (idtype & RSID_RELATION_UNKNOWN)
	{
		str += "UNKNOWN ";
	}
	return str;
}



