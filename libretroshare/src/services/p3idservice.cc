/*
 * libretroshare/src/services p3idservice.cc
 *
 * Id interface for RetroShare.
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

#include "services/p3idservice.h"

#include "util/rsrandom.h"
#include <retroshare/rspeers.h>
#include <sstream>

/****
 * #define ID_DEBUG 1
 ****/

#define ID_REQUEST_LIST		0x0001
#define ID_REQUEST_IDENTITY	0x0002
#define ID_REQUEST_REPUTATION	0x0003
#define ID_REQUEST_OPINION	0x0004

RsIdentity *rsIdentity = NULL;


/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3IdService::p3IdService(uint16_t type)
	:p3GxsDataService(type, new IdDataProxy()), mIdMtx("p3IdService"), mUpdated(true)
{
     	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

	mIdProxy = (IdDataProxy *) mProxy;
	return;
}


int	p3IdService::tick()
{
	std::cerr << "p3IdService::tick()";
	std::cerr << std::endl;

	fakeprocessrequests();
	
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



       /* Data Requests */
bool p3IdService::requestGroupInfo(     uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds)
{
	generateToken(token);
	std::cerr << "p3IdService::requestGroupInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_GROUPS, groupIds);

	return true;
}

bool p3IdService::requestMsgInfo(       uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds)
{
	generateToken(token);
	std::cerr << "p3IdService::requestMsgInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGS, groupIds);

	return true;
}

bool p3IdService::requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &msgIds)
{
	generateToken(token);
	std::cerr << "p3IdService::requestMsgRelatedInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGRELATED, msgIds);

	return true;
}

        /* Generic Lists */
bool p3IdService::getGroupList(         const uint32_t &token, std::list<std::string> &groupIds)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_LIST)
	{
		std::cerr << "p3IdService::getGroupList() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3IdService::getGroupList() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3IdService::getGroupList() ERROR Status Incomplete" << std::endl;
		return false;
	}

	bool ans = loadRequestOutList(token, groupIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	return ans;
}




bool p3IdService::getMsgList(           const uint32_t &token, std::list<std::string> &msgIds)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_LIST)
	{
		std::cerr << "p3IdService::getMsgList() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3IdService::getMsgList() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3IdService::getMsgList() ERROR Status Incomplete" << std::endl;
		return false;
	}

	bool ans = loadRequestOutList(token, msgIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	return ans;
}


        /* Generic Summary */
bool p3IdService::getGroupSummary(      const uint32_t &token, std::list<RsGroupMetaData> &groupInfo)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_SUMMARY)
	{
		std::cerr << "p3IdService::getGroupSummary() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3IdService::getGroupSummary() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3IdService::getGroupSummary() ERROR Status Incomplete" << std::endl;
		return false;
	}

	std::list<std::string> groupIds;
	bool ans = loadRequestOutList(token, groupIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	/* convert to RsGroupMetaData */
	mProxy->getGroupSummary(groupIds, groupInfo);

	return ans;
}

bool p3IdService::getMsgSummary(        const uint32_t &token, std::list<RsMsgMetaData> &msgInfo)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_SUMMARY)
	{
		std::cerr << "p3IdService::getMsgSummary() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3IdService::getMsgSummary() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3IdService::getMsgSummary() ERROR Status Incomplete" << std::endl;
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
bool p3IdService::getGroupData(const uint32_t &token, RsIdGroup &group)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);
	

	if (anstype != RS_TOKREQ_ANSTYPE_DATA)
	{
		std::cerr << "p3IdService::getGroupData() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3IdService::getGroupData() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3IdService::getGroupData() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestOutList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	/* convert to RsIdGroup */
	bool ans = mIdProxy->getGroup(id, group);
	return ans;
}


bool p3IdService::getMsgData(const uint32_t &token, RsIdMsg &msg)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);
	

	if (anstype != RS_TOKREQ_ANSTYPE_DATA)
	{
		std::cerr << "p3IdService::getMsgData() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3IdService::getMsgData() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3IdService::getMsgData() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestOutList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	/* convert to RsIdMsg */
	bool ans = mIdProxy->getMsg(id, msg);
	return ans;
}



        /* Poll */
uint32_t p3IdService::requestStatus(const uint32_t token)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	return status;
}


        /* Cancel Request */
bool p3IdService::cancelRequest(const uint32_t &token)
{
	return clearRequest(token);
}

        //////////////////////////////////////////////////////////////////////////////
bool p3IdService::setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask)
{
	return mIdProxy->setMessageStatus(msgId, status, statusMask);
}

bool p3IdService::setGroupStatus(const std::string &groupId, const uint32_t status, const uint32_t statusMask)
{
	return mIdProxy->setGroupStatus(groupId, status, statusMask);
}

bool p3IdService::setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask)
{
	return mIdProxy->setGroupSubscribeFlags(groupId, subscribeFlags, subscribeMask);
}

bool p3IdService::setMessageServiceString(const std::string &msgId, const std::string &str)
{
        return mIdProxy->setMessageServiceString(msgId, str);
}

bool p3IdService::setGroupServiceString(const std::string &grpId, const std::string &str)
{
        return mIdProxy->setGroupServiceString(grpId, str);
}


bool p3IdService::groupRestoreKeys(const std::string &groupId)
{
	return false;
}

bool p3IdService::groupShareKeys(const std::string &groupId, std::list<std::string>& peers)
{
	return false;
}


/********************************************************************************************/

	
std::string p3IdService::genRandomId()
{
	std::string randomId;
	for(int i = 0; i < 20; i++)
	{
		randomId += (char) ('a' + (RSRandom::random_u32() % 26));
	}
	
	return randomId;
}
	
bool    p3IdService::createGroup(uint32_t &token, RsIdGroup &group, bool isNew)
{
	if (group.mMeta.mGroupId.empty())
	{
		/* new photo */

		/* generate a temp id */
		group.mMeta.mGroupId = genRandomId();
	}
	else
	{
		std::cerr << "p3IdService::createGroup() Group with existing Id... dropping";
		std::cerr << std::endl;
		return false;
	}

	{	
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

		mUpdated = true;
		mIdProxy->addGroup(group);
	}

	// Fake a request to return the GroupMetaData.
	generateToken(token);
	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptions opts; // NULL is good.
	std::list<std::string> groupIds;
	groupIds.push_back(group.mMeta.mGroupId); // It will just return this one.
	
	std::cerr << "p3IdService::createGroup() Generating Request Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_GROUPS, groupIds);
	

	return true;
}




bool    p3IdService::createMsg(uint32_t &token, RsIdMsg &msg, bool isNew)
{
	if (msg.mMeta.mGroupId.empty())
	{
		/* new photo */
		std::cerr << "p3IdService::createMsg() Missing MsgID";
		std::cerr << std::endl;
		return false;
	}
	
	/* check if its a mod or new msg */
	if (msg.mMeta.mOrigMsgId.empty())
	{
		std::cerr << "p3IdService::createMsg() New Msg";
		std::cerr << std::endl;

		/* new msg, generate a new OrigMsgId */
		msg.mMeta.mOrigMsgId = genRandomId();
		msg.mMeta.mMsgId = msg.mMeta.mOrigMsgId;
	}
	else
	{
		std::cerr << "p3IdService::createMsg() Modified Msg";
		std::cerr << std::endl;

		/* mod msg, keep orig msg id, generate a new MsgId */
		msg.mMeta.mMsgId = genRandomId();
	}

	std::cerr << "p3IdService::createMsg() GroupId: " << msg.mMeta.mGroupId;
	std::cerr << std::endl;
	std::cerr << "p3IdService::createMsg() MsgId: " << msg.mMeta.mMsgId;
	std::cerr << std::endl;
	std::cerr << "p3IdService::createMsg() OrigMsgId: " << msg.mMeta.mOrigMsgId;
	std::cerr << std::endl;

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

		mUpdated = true;
		mIdProxy->addMsg(msg);
	}
	
	// Fake a request to return the MsgMetaData.
	generateToken(token);
	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptions opts; // NULL is good.
	std::list<std::string> msgIds;
	msgIds.push_back(msg.mMeta.mMsgId); // It will just return this one.
	
	std::cerr << "p3IdService::createMsg() Generating Request Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGRELATED, msgIds);

	return true;
}



/********************************************************************************************/


	
bool IdDataProxy::getGroup(const std::string &id, RsIdGroup &group)
{
	void *groupData = NULL;
	RsGroupMetaData meta;
	if (getGroupData(id, groupData) && getGroupSummary(id, meta))
	{
		RsIdGroup *pG = (RsIdGroup *) groupData;
		group = *pG;

		// update definitive version of the metadata.
		group.mMeta = meta;

		std::cerr << "IdDataProxy::getGroup() Id: " << id;
		std::cerr << " MetaData: " << meta << " DataPointer: " << groupData;
		std::cerr << std::endl;
		return true;
	}

	std::cerr << "IdDataProxy::getGroup() FAILED Id: " << id;
	std::cerr << std::endl;

	return false;
}

bool IdDataProxy::getMsg(const std::string &id, RsIdMsg &msg)
{
	void *msgData = NULL;
	RsMsgMetaData meta;
	if (getMsgData(id, msgData) && getMsgSummary(id, meta))
	{
		RsIdMsg *pM = (RsIdMsg *) msgData;
		// Shallow copy of thumbnail.
		msg = *pM;
	
		// update definitive version of the metadata.
		msg.mMeta = meta;

		std::cerr << "IdDataProxy::getMsg() Id: " << id;
		std::cerr << " MetaData: " << meta << " DataPointer: " << msgData;
		std::cerr << std::endl;
		return true;
	}

	std::cerr << "IdDataProxy::getMsg() FAILED Id: " << id;
	std::cerr << std::endl;

	return false;
}

bool IdDataProxy::addGroup(const RsIdGroup &group)
{
	// Make duplicate.
	RsIdGroup *pG = new RsIdGroup();
	*pG = group;

	std::cerr << "IdDataProxy::addGroup()";
	std::cerr << " MetaData: " << pG->mMeta << " DataPointer: " << pG;
	std::cerr << std::endl;

	return createGroup(pG);
}


bool IdDataProxy::addMsg(const RsIdMsg &msg)
{
	// Make duplicate.
	RsIdMsg *pM = new RsIdMsg();
	*pM = msg;

	std::cerr << "IdDataProxy::addMsg()";
	std::cerr << " MetaData: " << pM->mMeta << " DataPointer: " << pM;
	std::cerr << std::endl;

	return createMsg(pM);
}



        /* These Functions must be overloaded to complete the service */
bool IdDataProxy::convertGroupToMetaData(void *groupData, RsGroupMetaData &meta)
{
	RsIdGroup *group = (RsIdGroup *) groupData;
	meta = group->mMeta;

	return true;
}

bool IdDataProxy::convertMsgToMetaData(void *msgData, RsMsgMetaData &meta)
{
	RsIdMsg *page = (RsIdMsg *) msgData;
	meta = page->mMeta;

	return true;
}




/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

#if 0

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


#endif

	

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
			RsIdGroup id;

                	RsPeerDetails details;

			//id.mKeyId = genRandomId();
			id.mMeta.mGroupId = genRandomId();
			id.mIdType = RSID_TYPE_REALID;
			id.mGpgIdHash = genRandomId();

                	if (rsPeers->getPeerDetails(*it, details))
			{
				std::ostringstream out;
				out << details.name << "_" << i + 1;

				//id.mNickname = out.str();
				id.mMeta.mGroupName = out.str();
				
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
				//id.mNickname = genRandomId();
				id.mMeta.mGroupName = genRandomId();
				id.mGpgIdKnown = false;
			}

			//mIds[id.mKeyId] = id;
			mIdProxy->addGroup(id);
		}
	}

#define MAX_RANDOM_GPGIDS	10 //1000
#define MAX_RANDOM_PSEUDOIDS	50 //5000

	int nFakeGPGs = (RSRandom::random_u32() % MAX_RANDOM_GPGIDS);
	int nFakePseudoIds = (RSRandom::random_u32() % MAX_RANDOM_PSEUDOIDS);

	/* make some fake gpg ids */
	for(i = 0; i < nFakeGPGs; i++)
	{
		RsIdGroup id;

                RsPeerDetails details;

		//id.mKeyId = genRandomId();
		id.mMeta.mGroupId = genRandomId();
		id.mIdType = RSID_TYPE_REALID;
		id.mGpgIdHash = genRandomId();

		id.mIdType |= RSID_RELATION_OTHER;
		//id.mNickname = genRandomId();
		id.mMeta.mGroupName = genRandomId();
		id.mGpgIdKnown = false;
		id.mGpgId = "";
		id.mGpgName = "";
		id.mGpgEmail = "";

		//mIds[id.mKeyId] = id;
		mIdProxy->addGroup(id);
	}

	/* make lots of pseudo ids */
	for(i = 0; i < nFakePseudoIds; i++)
	{
		RsIdGroup id;

                RsPeerDetails details;

		//id.mKeyId = genRandomId();
		id.mMeta.mGroupId = genRandomId();
		id.mIdType = RSID_TYPE_PSEUDONYM;
		id.mGpgIdHash = "";

		//id.mNickname = genRandomId();
		id.mMeta.mGroupName = genRandomId();
		id.mGpgIdKnown = false;
		id.mGpgId = "";
		id.mGpgName = "";
		id.mGpgEmail = "";

		//mIds[id.mKeyId] = id;
		mIdProxy->addGroup(id);
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








