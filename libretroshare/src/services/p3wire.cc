/*
 * libretroshare/src/services p3wire.cc
 *
 * Wire interface for RetroShare.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#include "services/p3wire.h"

#include "util/rsrandom.h"

/****
 * #define WIKI_DEBUG 1
 ****/

RsWire *rsWire = NULL;


/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3Wire::p3Wire(uint16_t type)
	:p3GxsDataService(type, new WireDataProxy()), mWireMtx("p3Wire"), mUpdated(true)
{
     	RsStackMutex stack(mWireMtx); /********** STACK LOCKED MTX ******/

	mWireProxy = (WireDataProxy *) mProxy;
	return;
}


int	p3Wire::tick()
{
	std::cerr << "p3Wire::tick()";
	std::cerr << std::endl;

	fakeprocessrequests();
	
	return 0;
}

bool p3Wire::updated()
{
	RsStackMutex stack(mWireMtx); /********** STACK LOCKED MTX ******/

	if (mUpdated)
	{
		mUpdated = false;
		return true;
	}
	return false;
}



       /* Data Requests */
bool p3Wire::requestGroupInfo(     uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds)
{
	generateToken(token);
	std::cerr << "p3Wire::requestGroupInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_GROUPS, groupIds);

	return true;
}

bool p3Wire::requestMsgInfo(       uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds)
{
	generateToken(token);
	std::cerr << "p3Wire::requestMsgInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGS, groupIds);

	return true;
}

bool p3Wire::requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &msgIds)
{
	generateToken(token);
	std::cerr << "p3Wire::requestMsgRelatedInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGRELATED, msgIds);

	return true;
}

        /* Generic Lists */
bool p3Wire::getGroupList(         const uint32_t &token, std::list<std::string> &groupIds)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_LIST)
	{
		std::cerr << "p3Wire::getGroupList() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3Wire::getGroupList() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3Wire::getGroupList() ERROR Status Incomplete" << std::endl;
		return false;
	}

	bool ans = loadRequestOutList(token, groupIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	return ans;
}




bool p3Wire::getMsgList(           const uint32_t &token, std::list<std::string> &msgIds)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_LIST)
	{
		std::cerr << "p3Wire::getMsgList() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3Wire::getMsgList() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3Wire::getMsgList() ERROR Status Incomplete" << std::endl;
		return false;
	}

	bool ans = loadRequestOutList(token, msgIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	return ans;
}


        /* Generic Summary */
bool p3Wire::getGroupSummary(      const uint32_t &token, std::list<RsGroupMetaData> &groupInfo)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_SUMMARY)
	{
		std::cerr << "p3Wire::getGroupSummary() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3Wire::getGroupSummary() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3Wire::getGroupSummary() ERROR Status Incomplete" << std::endl;
		return false;
	}

	std::list<std::string> groupIds;
	bool ans = loadRequestOutList(token, groupIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	/* convert to RsGroupMetaData */
	mProxy->getGroupSummary(groupIds, groupInfo);

	return ans;
}

bool p3Wire::getMsgSummary(        const uint32_t &token, std::list<RsMsgMetaData> &msgInfo)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_SUMMARY)
	{
		std::cerr << "p3Wire::getMsgSummary() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3Wire::getMsgSummary() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3Wire::getMsgSummary() ERROR Status Incomplete" << std::endl;
		return false;
	}

	std::list<std::string> msgIds;
	bool ans = loadRequestOutList(token, msgIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	/* convert to RsMsgMetaData */
	mProxy->getMsgSummary(msgIds, msgInfo);

	return ans;
}


        /* Specific Service Data */
bool p3Wire::getGroupData(const uint32_t &token, RsWireGroup &group)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);
	

	if (anstype != RS_TOKREQ_ANSTYPE_DATA)
	{
		std::cerr << "p3Wire::getGroupData() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3Wire::getGroupData() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3Wire::getGroupData() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestOutList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	/* convert to RsWireGroup */
	bool ans = mWireProxy->getGroup(id, group);
	return ans;
}


bool p3Wire::getMsgData(const uint32_t &token, RsWirePulse &pulse)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);
	

	if (anstype != RS_TOKREQ_ANSTYPE_DATA)
	{
		std::cerr << "p3Wire::getMsgData() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3Wire::getMsgData() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3Wire::getMsgData() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestOutList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	/* convert to RsWirePulse */
	bool ans = mWireProxy->getPulse(id, pulse);
	return ans;
}



        /* Poll */
uint32_t p3Wire::requestStatus(const uint32_t token)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	return status;
}


        /* Cancel Request */
bool p3Wire::cancelRequest(const uint32_t &token)
{
	return clearRequest(token);
}

        //////////////////////////////////////////////////////////////////////////////

bool p3Wire::setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask)
{
        return mWireProxy->setMessageStatus(msgId, status, statusMask);
}


bool p3Wire::setGroupStatus(const std::string &groupId, const uint32_t status, const uint32_t statusMask)
{
        return mWireProxy->setGroupStatus(groupId, status, statusMask);
}

bool p3Wire::setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask)
{
	return mWireProxy->setGroupSubscribeFlags(groupId, subscribeFlags, subscribeMask);
}

bool p3Wire::setMessageServiceString(const std::string &msgId, const std::string &str)
{
        return mWireProxy->setMessageServiceString(msgId, str);
}

bool p3Wire::setGroupServiceString(const std::string &grpId, const std::string &str)
{
        return mWireProxy->setGroupServiceString(grpId, str);
}


bool p3Wire::groupRestoreKeys(const std::string &groupId)
{
	return false;
}

bool p3Wire::groupShareKeys(const std::string &groupId, std::list<std::string>& peers)
{
	return false;
}


/********************************************************************************************/

	
std::string p3Wire::genRandomId()
{
	std::string randomId;
	for(int i = 0; i < 20; i++)
	{
		randomId += (char) ('a' + (RSRandom::random_u32() % 26));
	}
	
	return randomId;
}
	

bool p3Wire::createGroup(uint32_t &token, RsWireGroup &group, bool isNew)
{
	if (group.mMeta.mGroupId.empty())
	{
		/* new photo */

		/* generate a temp id */
		group.mMeta.mGroupId = genRandomId();
	}
	else
	{
		std::cerr << "p3Wire::createGroup() Group with existing Id... dropping";
		std::cerr << std::endl;
		return false;
	}

	{	
		RsStackMutex stack(mWireMtx); /********** STACK LOCKED MTX ******/

		mUpdated = true;
		mWireProxy->addGroup(group);
	}
	
	// Fake a request to return the GroupMetaData.
	generateToken(token);
	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptions opts; // NULL is good.
	std::list<std::string> groupIds;
	groupIds.push_back(group.mMeta.mGroupId); // It will just return this one.
	
	std::cerr << "p3Wiree::createGroup() Generating Request Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_GROUPS, groupIds);
	
	return true;
}




bool p3Wire::createPulse(uint32_t &token, RsWirePulse &pulse, bool isNew)
{
	if (pulse.mMeta.mGroupId.empty())
	{
		/* new photo */
		std::cerr << "p3Wire::createPulse() Missing PulseID";
		std::cerr << std::endl;
		return false;
	}
	
	/* check if its a mod or new pulse */
	if (pulse.mMeta.mOrigMsgId.empty())
	{
		std::cerr << "p3Wire::createPulse() New Pulse";
		std::cerr << std::endl;

		/* new pulse, generate a new OrigPulseId */
		pulse.mMeta.mOrigMsgId = genRandomId();
		pulse.mMeta.mMsgId = pulse.mMeta.mOrigMsgId;
	}
	else
	{
		std::cerr << "p3Wire::createPulse() Modified Pulse";
		std::cerr << std::endl;

		/* mod pulse, keep orig pulse id, generate a new PulseId */
		pulse.mMeta.mMsgId = genRandomId();
	}

	std::cerr << "p3Wire::createPulse() GroupId: " << pulse.mMeta.mGroupId;
	std::cerr << std::endl;
	std::cerr << "p3Wire::createPulse() PulseId: " << pulse.mMeta.mMsgId;
	std::cerr << std::endl;
	std::cerr << "p3Wire::createPulse() OrigPulseId: " << pulse.mMeta.mOrigMsgId;
	std::cerr << std::endl;

	{
		RsStackMutex stack(mWireMtx); /********** STACK LOCKED MTX ******/

		mUpdated = true;
		mWireProxy->addPulse(pulse);
	}
	
	// Fake a request to return the MsgMetaData.
	generateToken(token);
	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptions opts; // NULL is good.
	std::list<std::string> msgIds;
	msgIds.push_back(pulse.mMeta.mMsgId); // It will just return this one.
	
	std::cerr << "p3Wire::createPulse() Generating Request Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGRELATED, msgIds);

	return true;
}



/********************************************************************************************/


	
bool WireDataProxy::getGroup(const std::string &id, RsWireGroup &group)
{
	void *groupData = NULL;
	RsGroupMetaData meta;
	if (getGroupData(id, groupData) && getGroupSummary(id, meta))
	{
		RsWireGroup *pG = (RsWireGroup *) groupData;
		group = *pG;

		// update definitive version of the metadata.
		group.mMeta = meta;

		std::cerr << "WireDataProxy::getGroup() Id: " << id;
		std::cerr << " MetaData: " << meta << " DataPointer: " << groupData;
		std::cerr << std::endl;
		return true;
	}

	std::cerr << "WireDataProxy::getGroup() FAILED Id: " << id;
	std::cerr << std::endl;

	return false;
}

bool WireDataProxy::getPulse(const std::string &id, RsWirePulse &pulse)
{
	void *msgData = NULL;
	RsMsgMetaData meta;
	if (getMsgData(id, msgData) && getMsgSummary(id, meta))
	{
		RsWirePulse *pP = (RsWirePulse *) msgData;
		// Shallow copy of thumbnail.
		pulse = *pP;
	
		// update definitive version of the metadata.
		pulse.mMeta = meta;

		std::cerr << "WireDataProxy::getPulse() Id: " << id;
		std::cerr << " MetaData: " << meta << " DataPointer: " << msgData;
		std::cerr << std::endl;
		return true;
	}

	std::cerr << "WireDataProxy::getPulse() FAILED Id: " << id;
	std::cerr << std::endl;

	return false;
}

bool WireDataProxy::addGroup(const RsWireGroup &group)
{
	// Make duplicate.
	RsWireGroup *pG = new RsWireGroup();
	*pG = group;

	std::cerr << "WireDataProxy::addGroup()";
	std::cerr << " MetaData: " << pG->mMeta << " DataPointer: " << pG;
	std::cerr << std::endl;

	return createGroup(pG);
}


bool WireDataProxy::addPulse(const RsWirePulse &pulse)
{
	// Make duplicate.
	RsWirePulse *pP = new RsWirePulse();
	*pP = pulse;

	std::cerr << "WireDataProxy::addPulse()";
	std::cerr << " MetaData: " << pP->mMeta << " DataPointer: " << pP;
	std::cerr << std::endl;

	return createMsg(pP);
}



        /* These Functions must be overloaded to complete the service */
bool WireDataProxy::convertGroupToMetaData(void *groupData, RsGroupMetaData &meta)
{
	RsWireGroup *group = (RsWireGroup *) groupData;
	meta = group->mMeta;

	return true;
}

bool WireDataProxy::convertMsgToMetaData(void *msgData, RsMsgMetaData &meta)
{
	RsWirePulse *page = (RsWirePulse *) msgData;
	meta = page->mMeta;

	return true;
}
