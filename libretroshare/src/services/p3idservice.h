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

class IdDataProxy: public GxsDataProxy
{
        public:

        bool getGroup(const std::string &id, RsIdGroup &group);
        bool getMsg(const std::string &id, RsIdMsg &msg);

        bool addGroup(const RsIdGroup &group);
        bool addMsg(const RsIdMsg &msg);

        /* These Functions must be overloaded to complete the service */
virtual bool convertGroupToMetaData(void *groupData, RsGroupMetaData &meta);
virtual bool convertMsgToMetaData(void *msgData, RsMsgMetaData &meta);
};


class p3IdService: public p3GxsDataService, public RsIdentity
{
	public:

	p3IdService(uint16_t type);

virtual int	tick();

	public:


        /* changed? */
virtual bool updated();

	/* From RsTokenService */
        /* Data Requests */
virtual bool requestGroupInfo(     uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds);
virtual bool requestMsgInfo(       uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds);
virtual bool requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &msgIds);

        /* Generic Lists */
virtual bool getGroupList(         const uint32_t &token, std::list<std::string> &groupIds);
virtual bool getMsgList(           const uint32_t &token, std::list<std::string> &msgIds);

        /* Generic Summary */
virtual bool getGroupSummary(      const uint32_t &token, std::list<RsGroupMetaData> &groupInfo);
virtual bool getMsgSummary(        const uint32_t &token, std::list<RsMsgMetaData> &msgInfo);

        /* Actual Data -> specific to Interface */
virtual bool 	getGroupData(const uint32_t &token, RsIdGroup &group);
virtual bool 	getMsgData(const uint32_t &token, RsIdMsg &msg);

        /* Poll */
virtual uint32_t requestStatus(const uint32_t token);

        /* Cancel Request */
virtual bool cancelRequest(const uint32_t &token);

        //////////////////////////////////////////////////////////////////////////////
virtual bool setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask);
virtual bool setGroupStatus(const std::string &groupId, const uint32_t status, const uint32_t statusMask);
virtual bool setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask);
virtual bool setMessageServiceString(const std::string &msgId, const std::string &str);
virtual bool setGroupServiceString(const std::string &grpId, const std::string &str);

virtual bool groupRestoreKeys(const std::string &groupId);
virtual bool groupShareKeys(const std::string &groupId, std::list<std::string>& peers);

virtual bool 	createGroup(uint32_t &token, RsIdGroup &group, bool isNew);
virtual bool 	createMsg(uint32_t &token, RsIdMsg &msg, bool isNew);


	private:

virtual void generateDummyData();

std::string genRandomId();

        IdDataProxy *mIdProxy;

	RsMutex mIdMtx;

	/***** below here is locked *****/

	bool mUpdated;

#if 0
	std::map<std::string, RsIdData> mIds;
	std::map<std::string, std::map<std::string, RsIdOpinion> >  mOpinions;

	std::map<std::string, RsIdReputation> mReputations; // this is created locally.
#endif

};

#endif 
