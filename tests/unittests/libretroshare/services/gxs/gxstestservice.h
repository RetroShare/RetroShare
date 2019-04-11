/*******************************************************************************
 * unittests/libretroshare/services/gxs/gxstestservice.h                       *
 *                                                                             *
 * Copyright 2012      by Robert Fernie    <retroshare.project@gmail.com>      *
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

#pragma once

#include "gxs/rsgenexchange.h"
#include "util/rstickevent.h"

#include "retroshare/rstokenservice.h"
#include "retroshare/rsgxsifacehelper.h"


#include <map>
#include <string>

/* 
 * Test Service.
 *
 *
 */

#define RS_SERVICE_GXS_TYPE_TEST	0x123

class RsTestGroup
{
	public:
	RsGroupMetaData mMeta;
	std::string mTestString;
};

class RsTestMsg
{
	public:
	RsMsgMetaData mMeta;
	std::string mTestString;
};

std::ostream &operator<<(std::ostream &out, const RsTestGroup &group);
std::ostream &operator<<(std::ostream &out, const RsTestMsg &msg);


class GxsTestService: public RsGenExchange, public RsTickEvent, public RsGxsIfaceHelper
{
public:
    GxsTestService(RsGeneralDataService* gds, RsNetworkExchangeService* nes, RsGixs *gixs, uint32_t testMode);
virtual RsServiceInfo getServiceInfo();
static uint32_t testAuthenPolicy(uint32_t testMode);

protected:

virtual void notifyChanges(std::vector<RsGxsNotify*>& changes) ;

        // Overloaded from RsTickEvent.
virtual void handle_event(uint32_t event_type, const std::string &elabel);

	// override for examination of data.
virtual RsGenExchange::ServiceCreate_Return service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& /*keySet*/);

public:

virtual void service_tick();

        /* Specific Service Data */
virtual bool getTestGroups(const uint32_t &token, std::vector<RsTestGroup> &groups);
virtual bool getTestMsgs(const uint32_t &token, std::vector<RsTestMsg> &msgs);
virtual bool getRelatedMsgs(const uint32_t &token, std::vector<RsTestMsg> &msgs);

virtual bool submitTestGroup(uint32_t &token, RsTestGroup &group);
virtual bool submitTestMsg(uint32_t &token, RsTestMsg &msg);


virtual bool updateTestGroup(uint32_t &token, RsTestGroup &group);

	private:

	uint32_t mTestMode;
};

