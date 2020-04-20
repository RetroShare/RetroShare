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
	virtual bool updateGroup(const RsWireGroup &group) override;
	virtual bool getGroups(const std::list<RsGxsGroupId> grpIds, std::vector<RsWireGroup> &groups) override;

private:
	virtual void generateDummyData();
	std::string genRandomId();

	RsMutex mWireMtx;
};

#endif 
