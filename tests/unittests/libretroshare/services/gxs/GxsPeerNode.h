/*******************************************************************************
 * unittests/libretroshare/services/gxs/GxsPeerNode.h                          *
 *                                                                             *
 * Copyright 2014      by Robert Fernie    <retroshare.project@gmail.com>      *
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

// from retroshare
#include "retroshare/rsids.h"
#include "retroshare/rstypes.h"
#include "retroshare/rsgxscircles.h"
#include "retroshare/rsidentity.h"

// from librssimulator
#include "peer/PeerNode.h"

#include "gxstestservice.h"

struct  RsGxsIdExchange;
class	RsGxsCircleExchange;
class	GxsTestService;
class 	RsGeneralDataService;
class   RsGxsNetService;
class   p3IdService;
class   p3GxsCircles;
class   FakePgpAuxUtils;

class GxsPeerNode: public PeerNode
{
public:

	GxsPeerNode(const RsPeerId &ownId, const std::list<RsPeerId> &peers, int testMode, bool useIdentityService);
	~GxsPeerNode();

bool checkTestServiceAllowedGroups(const RsPeerId &peerId);
bool checkCircleServiceAllowedGroups(const RsPeerId &peerId);

bool createIdentity(const std::string &name,
                bool pgpLinked,
                uint32_t circleType,
                const RsGxsCircleId &circleId,
                RsGxsId &gxsId);

bool createCircle(const std::string &name,
                uint32_t circleType,
                const RsGxsCircleId &circleId,
                const RsGxsId &authorId,
                std::set<RsPgpId> localMembers,
                std::set<RsGxsId> externalMembers,
                RsGxsGroupId &groupId);

bool createGroup(const std::string &name,
                uint32_t circleType,
                const RsGxsCircleId &circleId,
                const RsGxsId &authorId,
                RsGxsGroupId &groupId);

bool createMsg(const std::string &msgstr,
                const RsGxsGroupId &groupId,
                const RsGxsId &authorId,
                RsGxsMessageId &msgId);

bool subscribeToGroup(const RsGxsGroupId &groupId, bool subscribe);

bool getGroups(std::vector<RsTestGroup> &groups);
bool getGroupList(std::list<RsGxsGroupId> &groups);
bool getMsgList(const RsGxsGroupId &id, std::list<RsGxsMessageId> &msgIds);

bool getIdentities(std::vector<RsGxsIdGroup> &groups);
bool getIdentitiesList(std::list<RsGxsGroupId> &groups);

bool getCircles(std::vector<RsGxsCircleGroup> &groups);
bool getCirclesList(std::list<RsGxsGroupId> &groups);

	uint32_t mUseIdentityService;
	uint32_t mTestMode;
	std::string mGxsDir;

        FakePgpAuxUtils *mPgpAuxUtils;

	p3IdService *mGxsIdService;
	p3GxsCircles *mGxsCircles;

        RsGeneralDataService* mGxsIdDs;
        RsGxsNetService* mGxsIdNs;

        RsGeneralDataService* mGxsCirclesDs;
        RsGxsNetService* mGxsCirclesNs;

	GxsTestService *mTestService;
        RsGeneralDataService* mTestDs;
        RsGxsNetService* mTestNs;
};


