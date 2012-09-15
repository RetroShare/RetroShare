/*
 * libretroshare/src/services p3wikiservice.cc
 *
 * Wiki interface for RetroShare.
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

#include "services/p3wikiserviceVEG.h"

#include "util/rsrandom.h"

/****
 * #define WIKI_DEBUG 1
 ****/

RsWikiVEG *rsWikiVEG = NULL;


/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3WikiServiceVEG::p3WikiServiceVEG(uint16_t type)
	:p3GxsDataServiceVEG(type, new WikiDataProxy()), mWikiMtx("p3WikiService"), mUpdated(true)
{
     	RsStackMutex stack(mWikiMtx); /********** STACK LOCKED MTX ******/

	mWikiProxy = (WikiDataProxy *) mProxy;
	return;
}


int	p3WikiServiceVEG::tick()
{
	//std::cerr << "p3WikiServiceVEG::tick()";
	//std::cerr << std::endl;

	fakeprocessrequests();
	
	return 0;
}

bool p3WikiServiceVEG::updated()
{
	RsStackMutex stack(mWikiMtx); /********** STACK LOCKED MTX ******/

	if (mUpdated)
	{
		mUpdated = false;
		return true;
	}
	return false;
}



       /* Data Requests */
bool p3WikiServiceVEG::requestGroupInfo(     uint32_t &token, uint32_t ansType, const RsTokReqOptionsVEG &opts, const std::list<std::string> &groupIds)
{
	generateToken(token);
	std::cerr << "p3WikiServiceVEG::requestGroupInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_GROUPS, groupIds);

	return true;
}

bool p3WikiServiceVEG::requestMsgInfo(       uint32_t &token, uint32_t ansType, const RsTokReqOptionsVEG &opts, const std::list<std::string> &groupIds)
{
	generateToken(token);
	std::cerr << "p3WikiServiceVEG::requestMsgInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGS, groupIds);

	return true;
}

bool p3WikiServiceVEG::requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptionsVEG &opts, const std::list<std::string> &msgIds)
{
	generateToken(token);
	std::cerr << "p3WikiServiceVEG::requestMsgRelatedInfo() gets Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGRELATED, msgIds);

	return true;
}

        /* Generic Lists */
bool p3WikiServiceVEG::getGroupList(         const uint32_t &token, std::list<std::string> &groupIds)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_LIST)
	{
		std::cerr << "p3WikiServiceVEG::getGroupList() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3WikiServiceVEG::getGroupList() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3WikiServiceVEG::getGroupList() ERROR Status Incomplete" << std::endl;
		return false;
	}

	bool ans = loadRequestOutList(token, groupIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	return ans;
}




bool p3WikiServiceVEG::getMsgList(           const uint32_t &token, std::list<std::string> &msgIds)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_LIST)
	{
		std::cerr << "p3WikiServiceVEG::getMsgList() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3WikiServiceVEG::getMsgList() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3WikiServiceVEG::getMsgList() ERROR Status Incomplete" << std::endl;
		return false;
	}

	bool ans = loadRequestOutList(token, msgIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	return ans;
}


        /* Generic Summary */
bool p3WikiServiceVEG::getGroupSummary(      const uint32_t &token, std::list<RsGroupMetaData> &groupInfo)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_SUMMARY)
	{
		std::cerr << "p3WikiServiceVEG::getGroupSummary() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3WikiServiceVEG::getGroupSummary() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3WikiServiceVEG::getGroupSummary() ERROR Status Incomplete" << std::endl;
		return false;
	}

	std::list<std::string> groupIds;
	bool ans = loadRequestOutList(token, groupIds);	
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	/* convert to RsGroupMetaData */
	mProxy->getGroupSummary(groupIds, groupInfo);

	return ans;
}

bool p3WikiServiceVEG::getMsgSummary(        const uint32_t &token, std::list<RsMsgMetaData> &msgInfo)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	if (anstype != RS_TOKREQ_ANSTYPE_SUMMARY)
	{
		std::cerr << "p3WikiServiceVEG::getMsgSummary() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3WikiServiceVEG::getMsgSummary() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3WikiServiceVEG::getMsgSummary() ERROR Status Incomplete" << std::endl;
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
bool p3WikiServiceVEG::getGroupData(const uint32_t &token, RsWikiGroup &group)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);
	

	if (anstype != RS_TOKREQ_ANSTYPE_DATA)
	{
		std::cerr << "p3WikiServiceVEG::getGroupData() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if (reqtype != GXS_REQUEST_TYPE_GROUPS)
	{
		std::cerr << "p3WikiServiceVEG::getGroupData() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3WikiServiceVEG::getGroupData() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestOutList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	/* convert to RsWikiAlbum */
	bool ans = mWikiProxy->getGroup(id, group);
	return ans;
}


bool p3WikiServiceVEG::getMsgData(const uint32_t &token, RsWikiPage &page)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);
	

	if (anstype != RS_TOKREQ_ANSTYPE_DATA)
	{
		std::cerr << "p3WikiServiceVEG::getMsgData() ERROR AnsType Wrong" << std::endl;
		return false;
	}
	
	if ((reqtype != GXS_REQUEST_TYPE_MSGS) && (reqtype != GXS_REQUEST_TYPE_MSGRELATED))
	{
		std::cerr << "p3WikiServiceVEG::getMsgData() ERROR ReqType Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3WikiServiceVEG::getMsgData() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestOutList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	/* convert to RsWikiAlbum */
	bool ans = mWikiProxy->getPage(id, page);
	return ans;
}



        /* Poll */
uint32_t p3WikiServiceVEG::requestStatus(const uint32_t token)
{
	uint32_t status;
	uint32_t reqtype;
	uint32_t anstype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, anstype, ts);

	return status;
}


        /* Cancel Request */
bool p3WikiServiceVEG::cancelRequest(const uint32_t &token)
{
	return clearRequest(token);
}


        //////////////////////////////////////////////////////////////////////////////
bool p3WikiServiceVEG::setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask)
{
	return mWikiProxy->setMessageStatus(msgId, status, statusMask);
}

bool p3WikiServiceVEG::setGroupStatus(const std::string &groupId, const uint32_t status, const uint32_t statusMask)
{
	return mWikiProxy->setGroupStatus(groupId, status, statusMask);
}

bool p3WikiServiceVEG::setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask)
{
	return mWikiProxy->setGroupSubscribeFlags(groupId, subscribeFlags, subscribeMask);
}

bool p3WikiServiceVEG::setMessageServiceString(const std::string &msgId, const std::string &str)
{
	return mWikiProxy->setMessageServiceString(msgId, str);
}

bool p3WikiServiceVEG::setGroupServiceString(const std::string &grpId, const std::string &str)
{
	return mWikiProxy->setGroupServiceString(grpId, str);
}


bool p3WikiServiceVEG::groupRestoreKeys(const std::string &groupId)
{
	return false;
}

bool p3WikiServiceVEG::groupShareKeys(const std::string &groupId, std::list<std::string>& peers)
{
	return false;
}


/********************************************************************************************/

	
std::string p3WikiServiceVEG::genRandomId()
{
	std::string randomId;
	for(int i = 0; i < 20; i++)
	{
		randomId += (char) ('a' + (RSRandom::random_u32() % 26));
	}
	
	return randomId;
}
	
bool p3WikiServiceVEG::createGroup(uint32_t &token, RsWikiGroup &group, bool isNew)
{
	if (group.mMeta.mGroupId.empty())
	{
		/* new photo */

		/* generate a temp id */
		group.mMeta.mGroupId = genRandomId();
	}
	else
	{
		std::cerr << "p3WikiServiceVEG::createGroup() Group with existing Id... dropping";
		std::cerr << std::endl;
		return false;
	}

	{	
		RsStackMutex stack(mWikiMtx); /********** STACK LOCKED MTX ******/

		mUpdated = true;
		mWikiProxy->addGroup(group);
	}
	
	// Fake a request to return the GroupMetaData.
	generateToken(token);
	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptionsVEG opts; // NULL is good.
	std::list<std::string> groupIds;
	groupIds.push_back(group.mMeta.mGroupId); // It will just return this one.
	
	std::cerr << "p3WikiServiceVEG::createGroup() Generating Request Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_GROUPS, groupIds);

	return true;
}



bool p3WikiServiceVEG::createPage(uint32_t &token, RsWikiPage &page, bool isNew)
{
	if (page.mMeta.mGroupId.empty())
	{
		/* new photo */
		std::cerr << "p3WikiServiceVEG::createPage() Missing PageID";
		std::cerr << std::endl;
		return false;
	}
	
	/* check if its a mod or new page */
	if (page.mMeta.mOrigMsgId.empty())
	{
		std::cerr << "p3WikiServiceVEG::createPage() New Page";
		std::cerr << std::endl;

		/* new page, generate a new OrigPageId */
		page.mMeta.mOrigMsgId = genRandomId();
		page.mMeta.mMsgId = page.mMeta.mOrigMsgId;
	}
	else
	{
		std::cerr << "p3WikiServiceVEG::createPage() Modified Page";
		std::cerr << std::endl;

		/* mod page, keep orig page id, generate a new PageId */
		page.mMeta.mMsgId = genRandomId();
	}

	std::cerr << "p3WikiServiceVEG::createPage() GroupId: " << page.mMeta.mGroupId;
	std::cerr << std::endl;
	std::cerr << "p3WikiServiceVEG::createPage() PageId: " << page.mMeta.mMsgId;
	std::cerr << std::endl;
	std::cerr << "p3WikiServiceVEG::createPage() OrigPageId: " << page.mMeta.mOrigMsgId;
	std::cerr << std::endl;

	{
		RsStackMutex stack(mWikiMtx); /********** STACK LOCKED MTX ******/

		mUpdated = true;
		mWikiProxy->addPage(page);
	}

	// Fake a request to return the MsgMetaData.
	generateToken(token);
	uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
	RsTokReqOptionsVEG opts; // NULL is good.
	std::list<std::string> msgIds;
	msgIds.push_back(page.mMeta.mMsgId); // It will just return this one.
	
	std::cerr << "p3WikiServiceVEG::createPage() Generating Request Token: " << token << std::endl;
	storeRequest(token, ansType, opts, GXS_REQUEST_TYPE_MSGRELATED, msgIds);
	
	return true;
}



/********************************************************************************************/


	
bool WikiDataProxy::getGroup(const std::string &id, RsWikiGroup &group)
{
	void *groupData = NULL;
	RsGroupMetaData meta;
	if (getGroupData(id, groupData) && getGroupSummary(id, meta))
	{
		RsWikiGroup *pG = (RsWikiGroup *) groupData;
		group = *pG;

		// update definitive version of the metadata.
		group.mMeta = meta;

		std::cerr << "WikiDataProxy::getGroup() Id: " << id;
		std::cerr << " MetaData: " << meta << " DataPointer: " << groupData;
		std::cerr << std::endl;
		return true;
	}

	std::cerr << "WikiDataProxy::getGroup() FAILED Id: " << id;
	std::cerr << std::endl;

	return false;
}

bool WikiDataProxy::getPage(const std::string &id, RsWikiPage &page)
{
	void *msgData = NULL;
	RsMsgMetaData meta;
	if (getMsgData(id, msgData) && getMsgSummary(id, meta))
	{
		RsWikiPage *pP = (RsWikiPage *) msgData;
		// Shallow copy of thumbnail.
		page = *pP;
	
		// update definitive version of the metadata.
		page.mMeta = meta;

		std::cerr << "WikiDataProxy::getPage() Id: " << id;
		std::cerr << " MetaData: " << meta << " DataPointer: " << msgData;
		std::cerr << std::endl;
		return true;
	}

	std::cerr << "WikiDataProxy::getPage() FAILED Id: " << id;
	std::cerr << std::endl;

	return false;
}

bool WikiDataProxy::addGroup(const RsWikiGroup &group)
{
	// Make duplicate.
	RsWikiGroup *pG = new RsWikiGroup();
	*pG = group;

	std::cerr << "WikiDataProxy::addGroup()";
	std::cerr << " MetaData: " << pG->mMeta << " DataPointer: " << pG;
	std::cerr << std::endl;

	return createGroup(pG);
}


bool WikiDataProxy::addPage(const RsWikiPage &page)
{
	// Make duplicate.
	RsWikiPage *pP = new RsWikiPage();
	*pP = page;

	std::cerr << "WikiDataProxy::addPage()";
	std::cerr << " MetaData: " << pP->mMeta << " DataPointer: " << pP;
	std::cerr << std::endl;

	return createMsg(pP);
}



        /* These Functions must be overloaded to complete the service */
bool WikiDataProxy::convertGroupToMetaData(void *groupData, RsGroupMetaData &meta)
{
	RsWikiGroup *group = (RsWikiGroup *) groupData;
	meta = group->mMeta;

	return true;
}

bool WikiDataProxy::convertMsgToMetaData(void *msgData, RsMsgMetaData &meta)
{
	RsWikiPage *page = (RsWikiPage *) msgData;
	meta = page->mMeta;

	return true;
}
