/*
 * libretroshare/src/services: p3idservice.h
 *
 * Identity interface for RetroShare.
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

#ifndef P3_IDENTITY_SERVICE_HEADER
#define P3_IDENTITY_SERVICE_HEADER

#include "services/p3service.h"

#include "retroshare/rsidentity.h"

#include <map>
#include <string>

/* 
 * Indentity Service
 *
 */

class p3IdService: public p3Service, public RsIdentity
{
	public:

	p3IdService(uint16_t type);

virtual int	tick();

	public:

        /* changed? */
virtual bool updated();

virtual bool getIdentityList(std::list<std::string> &ids);

virtual bool getIdentity(const std::string &id, RsIdData &data);
virtual bool getIdReputation(const std::string &id, RsIdReputation &reputation);
virtual bool getIdPeerOpinion(const std::string &aboutid, const std::string &peerid, RsIdOpinion &opinion);

virtual bool getGpgIdDetails(const std::string &id, std::string &gpgName, std::string &gpgEmail);

virtual bool updateIdentity(RsIdData &data);
virtual bool updateOpinion(RsIdOpinion &opinion);

virtual void generateDummyData();

	private:

std::string genRandomId();

	RsMutex mIdMtx;

	/***** below here is locked *****/

	bool mUpdated;

	std::map<std::string, RsIdData> mIds;
	std::map<std::string, std::map<std::string, RsIdOpinion> >  mOpinions;

	std::map<std::string, RsIdReputation> mReputations; // this is created locally.

};

#endif 
