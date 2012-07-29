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
	// Disable for now.
	// background_tick();

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








/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/


/* here we are running a background process that calculates the reputation scores
 * for each of the IDs....
 * 
 * As this class will be extensively used by many other threads... it is best
 * that we don't block at all. This should be in a background thread.
 * Perhaps a generic method to handle this will be advisable.... but we do that later.
 *
 * To start with we will work from the Posted service.
 *
 * 
 *
 * So Reputation....
 *   Three components:
 *     1) Your Opinion: Should override everything else.
 *     2) Implicit Factors: Know the associated GPG Key.
 *     3) Your Friends Opinions: 
 *     4) Your Friends Calculated Reputation Scores.
 *
 * Must make sure that there is no Feedback loop in the Reputation calculation.
 *
 * So: Our Score + Friends Scores => Local Reputation.
 *  Local Reputation + Friends Reputations => Final Reputation?
 *
 * Do we need to 'ignore' Non-scores?
 *   ---> This becomes like the "Best Comment" algorithm from Reddit...
 *   Use a statistical mechanism to work out a lower bound on Reputation.
 *
 * But what if your opinion is wrong?.... well likely your friends will
 * get their messages and reply... you'll see the missing message - request it - check reputation etc.
 *
 * So we are going to have three different scores (Own, Peers, (the neighbour) Hood)...
 *
 * So next question, when do we need to incrementally calculate the score?
 *  .... how often do we need to recalculate everything -> this could lead to a flux of messages. 
 *
 *
 * 
 * MORE NOTES:
 *
 *   The Opinion Messages will have to be signed by PGP or SSL Keys, to guarantee that we don't 
 * multiple votes per person... As the message system doesn't handle uniqueness in this respect, 
 * we might have to do FULL_CALC for everything - This bit TODO.
 *
 * This will make IdService quite different to the other GXS services.
 */

/************************************************************************************/
/*
 * Processing Algorithm:
 *  - Grab all Groups which have received messages. 
 *  (opt 1)-> grab latest msgs for each of these and process => score.
 *  (opt 2)-> try incremental system (people probably won't change opinions often -> just set them once)
 *      --> if not possible, fallback to full calculation.
 *
 * 
 */


#define ID_BACKGROUND_PERIOD	60

int	p3IdService::background_tick()
{
	std::cerr << "p3IdService::background_tick()";
	std::cerr << std::endl;

	// Run Background Stuff.	
	background_checkTokenRequest();

	/* every minute - run a background check */
	time_t now = time(NULL);
	bool doCheck = false;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		if (now -  mLastBgCheck > ID_BACKGROUND_PERIOD)
		{
			doCheck = true;
			mLastBgCheck = now;
		}
	}

	if (doCheck)
	{
		//addExtraDummyData();
		background_requestGroups();
	}



	// Add in new votes + comments.
	return 0;
}




/***** Background Processing ****
 *
 * Process Each Message - as it arrives.
 *
 * Update 
 *
 */
#define ID_BG_IDLE						0
#define ID_BG_REQUEST_GROUPS			1
#define ID_BG_REQUEST_UNPROCESSED		2
#define ID_BG_REQUEST_FULLCALC			3

bool p3IdService::background_checkTokenRequest()
{
	uint32_t token = 0;
	uint32_t phase = 0;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		if (!mBgProcessing)
		{
			return false;
		}

		token = mBgToken;
		phase = mBgPhase;
	}


	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);
	
	if (status == GXS_REQUEST_STATUS_COMPLETE)
	{
		switch(phase)
		{
			case ID_BG_REQUEST_GROUPS:
				background_requestNewMessages();
				break;
			case ID_BG_REQUEST_UNPROCESSED:
				background_processNewMessages();
				break;
			case ID_BG_REQUEST_FULLCALC:
				background_processFullCalc();
				break;
			default:
				break;
		}
	}
	return true;
}


bool p3IdService::background_requestGroups()
{
	std::cerr << "p3IdService::background_requestGroups()";
	std::cerr << std::endl;

	// grab all the subscribed groups.
	uint32_t token = 0;

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

		if (mBgProcessing)
		{
			std::cerr << "p3IdService::background_requestGroups() ERROR Already processing, Skip this cycle";
			std::cerr << std::endl;
			return false;
		}

		mBgProcessing = true;
		mBgPhase = ID_BG_REQUEST_GROUPS;
		mBgToken = 0;
	}

	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptions opts; 
	std::list<std::string> groupIds;

	opts.mStatusFilter = RSGXS_GROUP_STATUS_NEWMSG;
	opts.mStatusMask = RSGXS_GROUP_STATUS_NEWMSG;

	requestGroupInfo(token, ansType, opts, groupIds);
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		mBgToken = token;
	}

	return true;
}



bool p3IdService::background_requestNewMessages()
{
	std::cerr << "p3IdService::background_requestNewMessages()";
	std::cerr << std::endl;

	std::list<RsGroupMetaData> modGroupList;
	std::list<RsGroupMetaData>::iterator it;

	std::list<std::string> groupIds;
	uint32_t token = 0;

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		token = mBgToken;
	}

	if (!getGroupSummary(token, modGroupList))
	{
		std::cerr << "p3IdService::background_requestNewMessages() ERROR No Group List";
		std::cerr << std::endl;
		background_cleanup();
		return false;
	}

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		mBgPhase = ID_BG_REQUEST_UNPROCESSED;
		mBgToken = 0;

		/* now we process the modGroupList -> a map so we can use it easily later, and create id list too */
		for(it = modGroupList.begin(); it != modGroupList.end(); it++)
		{
			setGroupStatus(it->mGroupId, 0, RSGXS_GROUP_STATUS_NEWMSG);

			mBgGroupMap[it->mGroupId] = *it;
			groupIds.push_back(it->mGroupId);
		}
	}

	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY; 
	RsTokReqOptions opts; 
	token = 0;

	opts.mStatusFilter = RSGXS_MSG_STATUS_UNPROCESSED;
	opts.mStatusMask = RSGXS_MSG_STATUS_UNPROCESSED;

	requestMsgInfo(token, ansType, opts, groupIds);

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		mBgToken = token;
	}
	return true;
}


bool p3IdService::background_processNewMessages()
{
	std::cerr << "p3IdService::background_processNewMessages()";
	std::cerr << std::endl;

	std::list<RsMsgMetaData> newMsgList;
	std::list<RsMsgMetaData>::iterator it;
	uint32_t token = 0;

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		token = mBgToken;
	}

	if (!getMsgSummary(token, newMsgList))
	{
		std::cerr << "p3IdService::background_processNewMessages() ERROR No New Msgs";
		std::cerr << std::endl;
		background_cleanup();
		return false;
	}


	/* iterate through the msgs.. update the mBgGroupMap with new data, 
	 * and flag these items as modified - so we rewrite them to the db later.
	 *
	 * If a message is not an original -> store groupId for requiring full analysis later.
         */

	std::map<std::string, RsGroupMetaData>::iterator mit;
	for(it = newMsgList.begin(); it != newMsgList.end(); it++)
	{
		std::cerr << "p3IdService::background_processNewMessages() new MsgId: " << it->mMsgId;
		std::cerr << std::endl;

		/* flag each new vote as processed */
		setMessageStatus(it->mMsgId, 0, RSGXS_MSG_STATUS_UNPROCESSED);

		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

		mit = mBgGroupMap.find(it->mGroupId);
		if (mit == mBgGroupMap.end())
		{
			std::cerr << "p3IdService::background_processNewMessages() ERROR missing GroupId: ";
			std::cerr << it->mGroupId;
			std::cerr << std::endl;

			/* error */
			continue;
		}

		if (mit->second.mGroupStatus & ID_LOCAL_STATUS_FULL_CALC_FLAG)
		{
			std::cerr << "p3IdService::background_processNewMessages() Group Already marked FULL_CALC";
			std::cerr << std::endl;

			/* already marked */
			continue;
		}

		if (it->mMsgId != it->mOrigMsgId)
		{
			/*
			 *  not original -> hard, redo calc (alt: could substract previous score)
			 */

			std::cerr << "p3IdService::background_processNewMessages() Update, mark for FULL_CALC";
			std::cerr << std::endl;

			mit->second.mGroupStatus |= ID_LOCAL_STATUS_FULL_CALC_FLAG;
		}
		else
		{
			/*
			 * Try incremental calculation.
			 * - extract parameters from group.
			 * - increment, & save back.
			 * - flag group as modified.
			 */

			std::cerr << "p3IdService::background_processNewMessages() NewOpt, Try Inc Calc";
			std::cerr << std::endl;

			mit->second.mGroupStatus |= ID_LOCAL_STATUS_INC_CALC_FLAG;

			std::string serviceString;
			IdGroupServiceStrData ssData;
			
			if (!extractIdGroupCache(serviceString, ssData))
			{
				/* error */
				std::cerr << "p3IdService::background_processNewMessages() ERROR Extracting";
				std::cerr << std::endl;
			}

			/* do calcs */
			std::cerr << "p3IdService::background_processNewMessages() Extracted: ";
			std::cerr << std::endl;

			/* store it back in */
			std::cerr << "p3IdService::background_processNewMessages() Stored: ";
			std::cerr << std::endl;

			if (!encodeIdGroupCache(serviceString, ssData))
			{
				/* error */
				std::cerr << "p3IdService::background_processNewMessages() ERROR Storing";
				std::cerr << std::endl;
			}
		}
	}


	/* now iterate through groups again 
	 *  -> update status as we go
	 *  -> record one requiring a full analyssis
	 */

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

		std::cerr << "p3IdService::background_processNewMessages() Checking Groups for Calc Type";
		std::cerr << std::endl;

		for(mit = mBgGroupMap.begin(); mit != mBgGroupMap.end(); mit++)
		{
			if (mit->second.mGroupStatus & ID_LOCAL_STATUS_FULL_CALC_FLAG)
			{
				std::cerr << "p3IdService::background_processNewMessages() FullCalc for: ";
				std::cerr << mit->second.mGroupId;
				std::cerr << std::endl;

				mBgFullCalcGroups.push_back(mit->second.mGroupId);
			}
			else if (mit->second.mGroupStatus & ID_LOCAL_STATUS_INC_CALC_FLAG)
			{
				std::cerr << "p3IdService::background_processNewMessages() IncCalc done for: ";
				std::cerr << mit->second.mGroupId;
				std::cerr << std::endl;

				/* set Cache */
				setGroupServiceString(mit->second.mGroupId, mit->second.mServiceString);
			}
			else
			{
				/* why is it here? error. */
				std::cerr << "p3IdService::background_processNewMessages() ERROR for: ";
				std::cerr << mit->second.mGroupId;
				std::cerr << std::endl;
			}
		}
	}

	return background_FullCalcRequest();
}


bool p3IdService::encodeIdGroupCache(std::string &str, const IdGroupServiceStrData &data)
{
	char line[RSGXS_MAX_SERVICE_STRING];

	snprintf(line, RSGXS_MAX_SERVICE_STRING, "v1 {%s} {Y:%d O:%d %d %f %f R:%d %d %f %f}", 
			 data.pgpId.c_str(), data.ownScore, 
		data.opinion.count, data.opinion.nullcount, data.opinion.sum, data.opinion.sumsq,
		data.reputation.count, data.reputation.nullcount, data.reputation.sum, data.reputation.sumsq);

	str = line;
	return true;
}


bool p3IdService::extractIdGroupCache(std::string &str, IdGroupServiceStrData &data)
{
	char pgpline[RSGXS_MAX_SERVICE_STRING];
	char scoreline[RSGXS_MAX_SERVICE_STRING];
	
	uint32_t iOwnScore;
	IdRepCumulScore iOpin;
	IdRepCumulScore iRep;

	// split into two parts.
	if (2 != sscanf(str.c_str(), "v1 {%[^}]} {%[^}]", pgpline, scoreline))
	{
		std::cerr << "p3IdService::extractIdGroupCache() Failed to extract Two Parts";
		std::cerr << std::endl;
		return false;
	}

	std::cerr << "p3IdService::extractIdGroupCache() pgpline: " << pgpline;
	std::cerr << std::endl;
	std::cerr << "p3IdService::extractIdGroupCache() scoreline: " << scoreline;
	std::cerr << std::endl;
	
	std::string pgptmp = pgpline;
	if (pgptmp.length() > 5)
	{
		std::cerr << "p3IdService::extractIdGroupCache() Believe to have pgpId: " << pgptmp;
		std::cerr << std::endl;
		data.pgpIdKnown = true;
		data.pgpId = pgptmp;
	}
	else
	{
		std::cerr << "p3IdService::extractIdGroupCache() Think pgpId Invalid";
		std::cerr << std::endl;
		data.pgpIdKnown = false;
	}
	
	
	if (9 == sscanf(scoreline, " Y:%d O:%d %d %lf %lf R:%d %d %lf %lf", &iOwnScore, 
		&(iOpin.count), &(iOpin.nullcount), &(iOpin.sum), &(iOpin.sumsq),
		&(iRep.count), &(iRep.nullcount), &(iRep.sum), &(iRep.sumsq)))
	{
		data.ownScore = iOwnScore;
		data.opinion = iOpin;
		data.reputation = iRep;
		return true;
	}

	std::cerr << "p3IdService::extractIdGroupCache() Failed to extract scores";
	std::cerr << std::endl;

	return false;
}



bool p3IdService::background_FullCalcRequest()
{
	/* 
	 * grab an GroupId from List.
	 *  - If empty, we are finished.
	 *  - request all latest mesgs
	 */
	
	std::list<std::string> groupIds;
	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		mBgPhase = ID_BG_REQUEST_FULLCALC;
		mBgToken = 0;
		mBgGroupMap.clear();

		if (mBgFullCalcGroups.empty())
		{
			/* finished! */
			background_cleanup();
			return true;
	
		}
	
		groupIds.push_back(mBgFullCalcGroups.front());
		mBgFullCalcGroups.pop_front();
		
	}

	/* request the summary info from the parents */
	uint32_t ansType = RS_TOKREQ_ANSTYPE_DATA; 
	uint32_t token = 0;
	RsTokReqOptions opts; 
	opts.mOptions = RS_TOKREQOPT_MSG_LATEST;

	requestMsgInfo(token, ansType, opts, groupIds);

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		mBgToken = token;
	}
	return true;
}




bool p3IdService::background_processFullCalc()
{
	std::cerr << "p3IdService::background_processFullCalc()";
	std::cerr << std::endl;

	std::list<RsMsgMetaData> msgList;
	std::list<RsMsgMetaData>::iterator it;

	RsIdMsg msg;

	bool validmsgs = false;

	/* calc variables */
	uint32_t opinion_count = 0;
	uint32_t opinion_nullcount = 0;
	double   opinion_sum = 0;
	double   opinion_sumsq = 0;

	uint32_t rep_count = 0;
	uint32_t rep_nullcount = 0;
	double   rep_sum = 0;
	double   rep_sumsq = 0;

	while(getMsgData(mBgToken, msg))
	{
		std::cerr << "p3IdService::background_processFullCalc() Msg:";
		std::cerr << msg;
		std::cerr << std::endl;

		validmsgs = true;

		/* for each msg ... extract score, and reputation */
		if (msg.mOpinion != 0)
		{
			opinion_count++;
			opinion_sum += msg.mOpinion;
			opinion_sum += (msg.mOpinion * msg.mOpinion);
		}
		else
		{
			opinion_nullcount++;
		}
		

		/* for each msg ... extract score, and reputation */
		if (msg.mReputation != 0)
		{
			rep_nullcount++;
			rep_sum += msg.mReputation;
			rep_sum += (msg.mReputation * msg.mReputation);
		}
		else
		{
			rep_nullcount++;
		}
	}

	double opinion_avg = 0;
	double opinion_var = 0;
	double opinion_frac = 0;

	double rep_avg = 0;
	double rep_var = 0;
	double rep_frac = 0;


	if (opinion_count)
	{
		opinion_avg = opinion_sum / opinion_count;
		opinion_var = (opinion_sumsq  - opinion_count * opinion_avg * opinion_avg) / opinion_count;
		opinion_frac = opinion_count / ((float) (opinion_count + opinion_nullcount));
	}

	if (rep_count)
	{
		rep_avg = rep_sum / rep_count;
		rep_var = (rep_sumsq  - rep_count * rep_avg * rep_avg) / rep_count;
		rep_frac = rep_count / ((float) (rep_count + rep_nullcount));
	}


	if (validmsgs)
	{
		std::string groupId = msg.mMeta.mGroupId;

		std::string serviceString;
		IdGroupServiceStrData ssData;
		
		
		if (!encodeIdGroupCache(serviceString, ssData))
		{
			std::cerr << "p3IdService::background_updateVoteCounts() Failed to encode Votes";
			std::cerr << std::endl;
		}
		else
		{
			std::cerr << "p3IdService::background_updateVoteCounts() Encoded String: " << serviceString;
			std::cerr << std::endl;
			/* store new result */
			setGroupServiceString(it->mMsgId, serviceString);
		}
	}

	{
		RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/
		mBgPhase = ID_BG_IDLE;
		mBgToken = 0;
	}

	return background_FullCalcRequest();
}


bool p3IdService::background_cleanup()
{
	std::cerr << "p3IdService::background_cleanup()";
	std::cerr << std::endl;

	RsStackMutex stack(mIdMtx); /********** STACK LOCKED MTX ******/

	// Cleanup.
	mBgProcessing = false;
	mBgPhase = ID_BG_IDLE;
	mBgToken = 0;
	mBgGroupMap.clear();
	mBgFullCalcGroups.clear();
	
	return true;
}


std::ostream &operator<<(std::ostream &out, const RsIdGroup &grp)
{
	out << "RsIdGroup: Meta: " << grp.mMeta;
	out << " IdType: " << grp.mIdType << " GpgIdHash: " << grp.mGpgIdHash;
	out << "(((Unusable: ( GpgIdKnown: " << grp.mGpgIdKnown << " GpgId: " << grp.mGpgId;
	out << " GpgName: " << grp.mGpgName << " GpgEmail: " << grp.mGpgEmail << ") )))";
	out << std::endl;
	
	return out;
}

std::ostream &operator<<(std::ostream &out, const RsIdMsg &msg)
{
	out << "RsIdMsg: Meta: " << msg.mMeta;
	//out << " IdType: " << grp.mIdType << " GpgIdHash: " << grp.mGpgIdHash;
	out << std::endl;
	
	return out;
}

