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

#include "services/p3wikiservice.h"

#include "util/rsrandom.h"

/****
 * #define WIKI_DEBUG 1
 ****/

RsWiki *rsWiki = NULL;

#define WIKI_REQUEST_GROUPLIST		0x0001
#define WIKI_REQUEST_MSGLIST 		0x0002
#define WIKI_REQUEST_GROUPS    		0x0004
#define WIKI_REQUEST_MSGS          	0x0008

#define WIKI_REQUEST_PAGEVERSIONS	0x0010
#define WIKI_REQUEST_ORIGPAGE		0x0020
#define WIKI_REQUEST_LATESTPAGE		0x0040


/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3WikiService::p3WikiService(uint16_t type)
	:p3GxsService(type), mWikiMtx("p3WikiService"), mUpdated(true)
{
     	RsStackMutex stack(mWikiMtx); /********** STACK LOCKED MTX ******/
	return;
}


int	p3WikiService::tick()
{
	std::cerr << "p3WikiService::tick()";
	std::cerr << std::endl;
	
	fakeprocessrequests();

	return 0;
}

bool p3WikiService::updated()
{
	RsStackMutex stack(mWikiMtx); /********** STACK LOCKED MTX ******/

	if (mUpdated)
	{
		mUpdated = false;
		return true;
	}
	return false;
}



/***********************************************************************************************/

       /* Data Requests */
bool p3WikiService::requestGroupList(     uint32_t &token, const RsTokReqOptions &opts)
{
        generateToken(token);
        std::cerr << "p3WikiService::requestGroupList() gets Token: " << token << std::endl;

        std::list<std::string> ids;
        storeRequest(token, GXS_REQUEST_TYPE_LIST | GXS_REQUEST_TYPE_GROUPS | WIKI_REQUEST_GROUPLIST, ids);

        return true;
}


bool p3WikiService::requestMsgList(       uint32_t &token, const RsTokReqOptions &opts, const std::list<std::string> &groupIds)
{
        generateToken(token);
        std::cerr << "p3WikiService::requestMsgList() gets Token: " << token << std::endl;
        storeRequest(token, GXS_REQUEST_TYPE_LIST | GXS_REQUEST_TYPE_MSGS | WIKI_REQUEST_ORIGPAGE, groupIds);

        return true;
}

bool p3WikiService::requestMsgRelatedList(uint32_t &token, const RsTokReqOptions &opts, const std::list<std::string> &msgIds)
{
        generateToken(token);
        std::cerr << "p3WikiService::requestMsgRelatedList() gets Token: " << token << std::endl;

	// Look at opts to set the flags.
	uint32_t optFlags = 0;
	if (opts.mOptions == RS_TOKREQOPT_MSG_VERSIONS)
	{
		optFlags = WIKI_REQUEST_PAGEVERSIONS;
	}
	else if (opts.mOptions == RS_TOKREQOPT_MSG_LATEST)
	{
		optFlags = WIKI_REQUEST_LATESTPAGE;
	}

        storeRequest(token, GXS_REQUEST_TYPE_LIST | GXS_REQUEST_TYPE_MSGS | optFlags, msgIds);

        return true;
}


bool p3WikiService::requestGroupData(     uint32_t &token, const std::list<std::string> &groupIds)
{
        generateToken(token);
        std::cerr << "p3WikiService::requestGroupData() gets Token: " << token << std::endl;
        storeRequest(token, GXS_REQUEST_TYPE_DATA | GXS_REQUEST_TYPE_GROUPS | WIKI_REQUEST_GROUPS, groupIds);

        return true;
}

bool p3WikiService::requestMsgData(       uint32_t &token, const std::list<std::string> &msgIds)
{
        generateToken(token);
        std::cerr << "p3WikiService::requestMsgData() gets Token: " << token << std::endl;
        storeRequest(token, GXS_REQUEST_TYPE_DATA | GXS_REQUEST_TYPE_MSGS | WIKI_REQUEST_MSGS, msgIds);

        return true;
}

/**************** Return Data *************/

bool p3WikiService::getGroupList(const uint32_t &token, std::list<std::string> &groupIds)
{
	uint32_t status;
	uint32_t reqtype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, ts);
	
	if (reqtype != (GXS_REQUEST_TYPE_LIST | GXS_REQUEST_TYPE_GROUPS | WIKI_REQUEST_GROUPLIST))
	{
		std::cerr << "p3WikiService::getGroupList() ERROR Type Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3WikiService::getGroupList() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	bool ans = InternalgetGroupList(groupIds);

	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);

	return ans;
}
	
bool p3WikiService::getMsgList(const uint32_t &token, std::list<std::string> &msgIds)
{
	uint32_t status;
	uint32_t reqtype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, ts);

	// MULTIPLY TYPES MATCH HERE....
	if (!((reqtype & GXS_REQUEST_TYPE_LIST) && (reqtype & GXS_REQUEST_TYPE_MSGS)))
	{
		std::cerr << "p3WikiService::getMsgList() ERROR Type Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3WikiService::getMsgList() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	bool ans = false;
	while (popRequestList(token, id))
	{
		std::cerr << "p3WikiService::getMsgList() Processing Id: " << id << std::endl;
		if (reqtype & WIKI_REQUEST_PAGEVERSIONS)
		{
			std::cerr << "p3WikiService::getMsgList() get PAGEVERSIONS" << std::endl;
			if (InternalgetPageVersions(id, msgIds))
			{
				ans = true;
			}
		}
		else if (reqtype & WIKI_REQUEST_ORIGPAGE)
		{
			std::cerr << "p3WikiService::getMsgList() get ORIGPAGE" << std::endl;
			if (InternalgetOrigPageList(id, msgIds))
			{
				ans = true;
			}
		}
		else if (reqtype & WIKI_REQUEST_LATESTPAGE)
		{
			std::cerr << "p3WikiService::getMsgList() get LATESTPAGE" << std::endl;
			std::string latestpage;
			if (InternalgetLatestPage(id, latestpage))
			{
				msgIds.push_back(latestpage);
				ans = true;
			}
		}
		else
		{
			std::cerr << "p3WikiService::getMsgList() ERROR Invalid Request Type" << std::endl;
			return false;
		}
	
	}
	updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
	return ans;
}
	
	
bool p3WikiService::getGroupData(const uint32_t &token, RsWikiGroup &group)
{
	uint32_t status;
	uint32_t reqtype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, ts);
	
	if (reqtype != (GXS_REQUEST_TYPE_DATA | GXS_REQUEST_TYPE_GROUPS | WIKI_REQUEST_GROUPS))
	{
		std::cerr << "p3WikiService::getGroupData() ERROR Type Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3WikiService::getGroupData() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	bool ans = InternalgetGroup(id, group);
	return ans;
}


bool p3WikiService::getMsgData(const uint32_t &token, RsWikiPage &page)
{
	uint32_t status;
	uint32_t reqtype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, ts);
	
	if (reqtype != (GXS_REQUEST_TYPE_DATA | GXS_REQUEST_TYPE_MSGS | WIKI_REQUEST_MSGS))
	{
		std::cerr << "p3WikiService::getMsgData() ERROR Type Wrong" << std::endl;
		return false;
	}
	
	if (status != GXS_REQUEST_STATUS_COMPLETE)
	{
		std::cerr << "p3WikiService::getMsgData() ERROR Status Incomplete" << std::endl;
		return false;
	}
	
	std::string id;
	if (!popRequestList(token, id))
	{
		/* finished */
		updateRequestStatus(token, GXS_REQUEST_STATUS_DONE);
		return false;
	}
	
	bool ans = InternalgetPage(id, page);
	return ans;
}

        /* Poll */
uint32_t p3WikiService::requestStatus(const uint32_t token)
{
	uint32_t status;
	uint32_t reqtype;
	time_t ts;
	checkRequestStatus(token, status, reqtype, ts);

	return status;
}





/****************** INTERNALS ***********************/


bool p3WikiService::InternalgetGroupList(std::list<std::string> &groups)
{
	RsStackMutex stack(mWikiMtx); /********** STACK LOCKED MTX ******/

        std::map<std::string, RsWikiGroup>::iterator it;
	for(it = mGroups.begin(); it != mGroups.end(); it++)
	{
		groups.push_back(it->second.mGroupId);
	}
	
	return false;
}

bool p3WikiService::InternalgetGroup(const std::string &groupid, RsWikiGroup &group)
{
	RsStackMutex stack(mWikiMtx); /********** STACK LOCKED MTX ******/
	
        std::map<std::string, RsWikiGroup>::iterator it;
	it = mGroups.find(groupid);
	if (it == mGroups.end())
	{
		return false;
	}
	
	group = it->second;
	return true;
}



bool p3WikiService::InternalgetPage(const std::string &pageid, RsWikiPage &page)
{
	RsStackMutex stack(mWikiMtx); /********** STACK LOCKED MTX ******/
	
        std::map<std::string, RsWikiPage>::iterator it;
	it = mPages.find(pageid);
	if (it == mPages.end())
	{
		return false;
	}
	
	page = it->second;
	return true;
}


bool p3WikiService::InternalgetLatestPage(const std::string &origPageId, std::string &pageId)
{
	RsStackMutex stack(mWikiMtx); /********** STACK LOCKED MTX ******/
	
	std::map<std::string, std::string>::iterator it;
	it = mOrigPageToLatestPage.find(origPageId);
	if (it == mOrigPageToLatestPage.end())
	{
		return false;
	}
	
	pageId = it->second;
	return true;
}


bool p3WikiService::InternalgetPageVersions(const std::string &origPageId, std::list<std::string> &pageIds)
{
	RsStackMutex stack(mWikiMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string, std::list<std::string> >::iterator it;
	it = mOrigToPageVersions.find(origPageId);
	if (it == mOrigToPageVersions.end())
	{
		return false;
	}
	
	std::list<std::string>::iterator lit;
	for(lit = it->second.begin(); lit != it->second.end(); lit++)
	{
		pageIds.push_back(*lit);
	}	
	return true;
}


bool p3WikiService::InternalgetOrigPageList(const std::string &groupid, std::list<std::string> &pageIds)
{
	RsStackMutex stack(mWikiMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string, std::list<std::string> >::iterator it;
	it = mGroupToOrigPages.find(groupid);
	if (it == mGroupToOrigPages.end())
	{
		return false;
	}
	
	std::list<std::string>::iterator lit;
	for(lit = it->second.begin(); lit != it->second.end(); lit++)
	{
		pageIds.push_back(*lit);
	}	
	return true;
}


/* details are updated in album - to choose Album ID, and storage path */
bool p3WikiService::createGroup(RsWikiGroup &group)
{
	if (group.mGroupId.empty())
	{
		/* new photo */

		/* generate a temp id */
		group.mGroupId = genRandomId();
	}
	else
	{
		std::cerr << "p3WikiService::createGroup() Group with existing Id... dropping";
		std::cerr << std::endl;
		return false;
	}
	
	RsStackMutex stack(mWikiMtx); /********** STACK LOCKED MTX ******/

	mUpdated = true;

	/* add / modify */
	mGroups[group.mGroupId] = group;

	return true;
}




bool p3WikiService::createPage(RsWikiPage &page)
{
	if (page.mGroupId.empty())
	{
		/* new photo */
		std::cerr << "p3WikiService::createPage() Missing PageID";
		std::cerr << std::endl;
		return false;
	}
	
	/* check if its a mod or new page */
	if (page.mOrigPageId.empty())
	{
		std::cerr << "p3WikiService::createPage() New Page";
		std::cerr << std::endl;

		/* new page, generate a new OrigPageId */
		page.mOrigPageId = genRandomId();
		page.mPageId = page.mOrigPageId;
	}
	else
	{
		std::cerr << "p3WikiService::createPage() Modified Page";
		std::cerr << std::endl;

		/* mod page, keep orig page id, generate a new PageId */
		page.mPageId = genRandomId();
	}

	std::cerr << "p3WikiService::createPage() GroupId: " << page.mGroupId;
	std::cerr << std::endl;
	std::cerr << "p3WikiService::createPage() PageId: " << page.mPageId;
	std::cerr << std::endl;
	std::cerr << "p3WikiService::createPage() OrigPageId: " << page.mOrigPageId;
	std::cerr << std::endl;

	RsStackMutex stack(mWikiMtx); /********** STACK LOCKED MTX ******/

	mUpdated = true;

	std::map<std::string, std::list<std::string> >::iterator it;
	if (page.mPageId == page.mOrigPageId)
	{
		it = mGroupToOrigPages.find(page.mGroupId);
		if (it == mGroupToOrigPages.end())
		{
			std::cerr << "p3WikiService::createPage() First Page in Group";
			std::cerr << std::endl;

			std::list<std::string> emptyList;
			mGroupToOrigPages[page.mGroupId] = emptyList;

			it = mGroupToOrigPages.find(page.mGroupId);
		}
		it->second.push_back(page.mPageId);
	}


	it = mOrigToPageVersions.find(page.mOrigPageId);
	if (it == mOrigToPageVersions.end())
	{
		std::cerr << "p3WikiService::createPage() Adding OrigPage";
		std::cerr << std::endl;

		std::list<std::string> emptyList;
		mOrigToPageVersions[page.mOrigPageId] = emptyList;

		it = mOrigToPageVersions.find(page.mOrigPageId);
	}
	/* push back both Orig and all Mods */
	it->second.push_back(page.mPageId);
		
	/* add / modify */
	mPages[page.mPageId] = page;

	/* Total HACK - but will work for demo */
	mOrigPageToLatestPage[page.mOrigPageId] = page.mPageId;

	return true;
}
	
std::string p3WikiService::genRandomId()
{
	std::string randomId;
	for(int i = 0; i < 20; i++)
	{
		randomId += (char) ('a' + (RSRandom::random_u32() % 26));
	}
	
	return randomId;
}
	











