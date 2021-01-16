/*******************************************************************************
 * libretroshare/src/services: p3wire.h                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2020 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef P3_WIRE_SERVICE_HEADER
#define P3_WIRE_SERVICE_HEADER

#include "retroshare/rswire.h"
#include "gxs/rsgenexchange.h"

#include <map>
#include <string>

/* 
 * Wire Service
 *
 *
 */

class p3Wire: public RsGenExchange, public RsWire
{
public:
	p3Wire(RsGeneralDataService* gds, RsNetworkExchangeService* nes, RsGixs *gixs);
	virtual RsServiceInfo getServiceInfo();
	static uint32_t wireAuthenPolicy();

protected:
	virtual void notifyChanges(std::vector<RsGxsNotify*>& changes) ;

public:
	virtual void service_tick();

	virtual RsTokenService* getTokenService();

	/* Specific Service Data */
	virtual bool getGroupData(const uint32_t &token, std::vector<RsWireGroup> &groups) override;
	virtual bool getPulseData(const uint32_t &token, std::vector<RsWirePulse> &pulses) override;

	virtual bool createGroup(uint32_t &token, RsWireGroup &group) override;
	virtual bool createPulse(uint32_t &token, RsWirePulse &pulse) override;

	// Blocking Interfaces.
	virtual bool createGroup(RsWireGroup &group) override;
	virtual bool updateGroup(uint32_t &token, RsWireGroup &group) override;
	virtual bool getGroups(const std::list<RsGxsGroupId> grpIds, std::vector<RsWireGroup> &groups) override;

	bool editWire(RsWireGroup& wire) override;

	// New Interfaces.
	// Blocking, request structures for display.
	virtual bool createOriginalPulse(const RsGxsGroupId &grpId, RsWirePulseSPtr pPulse) override;
	virtual bool createReplyPulse(RsGxsGroupId grpId, RsGxsMessageId msgId,
		RsGxsGroupId replyWith, uint32_t reply_type,
		RsWirePulseSPtr pPulse) override;

#if 0
	virtual bool createReplyPulse(uint32_t &token, RsWirePulse &pulse) override;
	virtual bool createRepublishPulse(uint32_t &token, RsWirePulse &pulse) override;
	virtual bool createLikePulse(uint32_t &token, RsWirePulse &pulse) override;
#endif

	virtual bool getWireGroup(const RsGxsGroupId &groupId, RsWireGroupSPtr &grp) override;
	virtual bool getWirePulse(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId, RsWirePulseSPtr &pPulse) override;

	virtual bool getPulsesForGroups(const std::list<RsGxsGroupId> &groupIds, std::list<RsWirePulseSPtr> &pulsePtrs) override;

	virtual bool getPulseFocus(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId, int type, RsWirePulseSPtr &pPulse) override;

private:
	// Internal Service Data.
	// They should eventually all be here.
	bool getRelatedPulseData(const uint32_t &token, std::vector<RsWirePulse> &pulses);
	bool getGroupPtrData(const uint32_t &token,
		std::map<RsGxsGroupId, RsWireGroupSPtr> &groups);
	bool getPulsePtrData(const uint32_t &token, std::list<RsWirePulseSPtr> &pulses);

	// util functions fetching data.
	bool fetchPulse(RsGxsGroupId grpId, RsGxsMessageId msgId, RsWirePulseSPtr &pPulse);
	bool updatePulse(RsWirePulseSPtr pPulse, int levels);
	bool updatePulseChildren(RsWirePulseSPtr pParent,  uint32_t token);

	// update GroupPtrs
	bool updateGroups(std::list<RsWirePulseSPtr> &pulsePtrs);

	// sub utility functions used by updateGroups.
	bool extractGroupIds(RsWirePulseConstSPtr pPulse, std::set<RsGxsGroupId> &groupIds);

	bool updateGroupPtrs(RsWirePulseSPtr pPulse,
			const std::map<RsGxsGroupId, RsWireGroupSPtr> &groups);

	bool trimToAvailGroupIds(const std::set<RsGxsGroupId> &pulseGroupIds,
		std::list<RsGxsGroupId> &availGroupIds);

	bool fetchGroupPtrs(const std::list<RsGxsGroupId> &groupIds,
			std::map<RsGxsGroupId, RsWireGroupSPtr> &groups);


	virtual void generateDummyData();
	std::string genRandomId();

	RsMutex mWireMtx;
};

#endif 
