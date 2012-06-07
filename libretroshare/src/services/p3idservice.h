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
#include "services/p3gxsservice.h"

#include "retroshare/rsidentity.h"

#include <map>
#include <string>

/* 
 * Identity Service
 *
 */


class p3IdService: public p3GxsService, public RsIdentity
{
	public:

	p3IdService(uint16_t type);

virtual int	tick();

	public:


        /* changed? */
virtual bool updated();

	/* Interface now a request / poll / answer system */

	/* Data Requests */
virtual bool requestIdentityList(uint32_t &token);
virtual bool requestIdentities(uint32_t &token, const std::list<std::string> &ids);
virtual bool requestIdReputations(uint32_t &token, const std::list<std::string> &ids);
virtual bool requestIdPeerOpinion(uint32_t &token, const std::string &aboutId, const std::string &peerId);
//virtual bool requestIdGpgDetails(uint32_t &token, const std::list<std::string> &ids);

	/* Poll */
virtual uint32_t requestStatus(const uint32_t token);

	/* Retrieve Data */
virtual bool getIdentityList(const uint32_t token, std::list<std::string> &ids);
virtual bool getIdentity(const uint32_t token, RsIdData &data);
virtual bool getIdReputation(const uint32_t token, RsIdReputation &reputation);
virtual bool getIdPeerOpinion(const uint32_t token, RsIdOpinion &opinion);
//virtual bool getIdGpgDetails(const uint32_t token, RsIdGpgDetails &gpgData);

	/* Updates */
virtual bool updateIdentity(RsIdData &data);
virtual bool updateOpinion(RsIdOpinion &opinion);



	/* below here not part of the interface */
bool fakeprocessrequests();

virtual bool InternalgetIdentityList(std::list<std::string> &ids);
virtual bool InternalgetIdentity(const std::string &id, RsIdData &data);
virtual bool InternalgetIdReputation(const std::string &id, RsIdReputation &reputation);
virtual bool InternalgetIdPeerOpinion(const std::string &aboutid, const std::string &peerid, RsIdOpinion &opinion);

#if 0
        /* changed? */
virtual bool updated();

virtual bool getIdentityList(std::list<std::string> &ids);

virtual bool getIdentity(const std::string &id, RsIdData &data);
virtual bool getIdReputation(const std::string &id, RsIdReputation &reputation);
virtual bool getIdPeerOpinion(const std::string &aboutid, const std::string &peerid, RsIdOpinion &opinion);

virtual bool getGpgIdDetails(const std::string &id, std::string &gpgName, std::string &gpgEmail);

virtual bool updateIdentity(RsIdData &data);
virtual bool updateOpinion(RsIdOpinion &opinion);

#endif

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
