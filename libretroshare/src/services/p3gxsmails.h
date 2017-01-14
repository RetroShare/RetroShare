#pragma once
/*
 * GXS Mailing Service
 * Copyright (C) 2016-2017  Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>

#include "retroshare/rsgxsifacetypes.h" // For RsGxsId, RsGxsCircleId
#include "gxs/gxstokenqueue.h" // For GxsTokenQueue
#include "serialiser/rsgxsmailitems.h" // For RS_SERVICE_TYPE_GXS_MAIL
#include "services/p3idservice.h" // For p3IdService


class p3GxsMails : public RsGenExchange, public GxsTokenQueue
{
public:
	p3GxsMails( RsGeneralDataService* gds, RsNetworkExchangeService* nes,
	            p3IdService *identities ) :
	    RsGenExchange( gds, nes, new RsGxsMailSerializer(),
	                   RS_SERVICE_TYPE_GXS_MAIL, identities,
	                   AuthenPolicy(),
	                   RS_GXS_DEFAULT_MSG_STORE_PERIOD ), // TODO: Discuss with Cyril about this
	    GxsTokenQueue(this) {}

	/// Public interface
	bool sendEmail( const RsGxsId& own_gxsid, const RsGxsId& recipient,
	                const std::string& body );

	/**
	 * @see GxsTokenQueue::handleResponse(uint32_t token, uint32_t req_type)
	 */
	virtual void handleResponse(uint32_t token, uint32_t req_type);

	/// @see RsGenExchange::service_tick()
	virtual void service_tick();

	/// @see RsGenExchange::getServiceInfo()
	virtual RsServiceInfo getServiceInfo() { return RsServiceInfo( RS_SERVICE_TYPE_GXS_MAIL, "GXS Mails", 0, 1, 0, 1 ); }

	/// @see RsGenExchange::service_CreateGroup(...)
	RsGenExchange::ServiceCreate_Return service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet&);

protected:
	/// @see RsGenExchange::notifyChanges(std::vector<RsGxsNotify *> &changes)
	void notifyChanges(std::vector<RsGxsNotify *> &changes);

private:

	/// @brief AuthenPolicy check nothing ATM TODO talk with Cyril how this should be
	static uint32_t AuthenPolicy() { return 0; }

	/// Types to mark GXS queries and answhers
	enum GxsReqResTypes { GROUPS_LIST, GROUP_CREATE };

	RsGxsGroupId preferredGroupId;

	/// Request groups list to GXS backend. Async method.
	bool requestGroupsList();

	/**
	 * Check if current preferredGroupId is the best against potentialGrId, if
	 * the passed one is better update it.
	 * Useful when GXS backend notifies groups changes, or when a reponse to an
	 * async grop request (@see GXS_REQUEST_TYPE_GROUP_*) is received.
	 * @return true if preferredGroupId has been supeseded by potentialGrId
	 *   false otherwise.
	 */
	bool inline supersedePreferredGroup(const RsGxsGroupId& potentialGrId)
	{
		std::cout << "supersedePreferredGroup(const RsGxsGroupId& potentialGrId) " << preferredGroupId << " <? " << potentialGrId << std::endl;
		if(preferredGroupId < potentialGrId)
		{
			preferredGroupId = potentialGrId;
			return true;
		}
		return false;
	}
};

/*
19:27:29 G10h4ck: About the scalability stuff i have thinked we can implement TTL mechanism like IP does
19:27:48 G10h4ck: better HTL Hop To Live
19:28:09 G10h4ck: one send an email with an associated HTL for example 2
19:28:32 G10h4ck: when a node receive that it decrement the HTL and store and forward it
19:29:03 G10h4ck: if a received mail has an HTL greater then the last we received we store that one
19:29:18 G10h4ck: this way we can control how much the mail should spread in the net
19:29:47 G10h4ck: HTL should be upper limited to a maximum
19:30:03 G10h4ck: so an abuser cant spread mails with 10000000 as HTL
19:30:30 G10h4ck: if a mail is received witha suspicios HTL it may be dropped or the HTL reduced to MAX_HTL
 */
