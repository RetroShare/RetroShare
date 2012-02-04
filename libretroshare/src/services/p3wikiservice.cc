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


/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3WikiService::p3WikiService(uint16_t type)
	:p3Service(type), mWikiMtx("p3WikiService"), mUpdated(true)
{
     	RsStackMutex stack(mWikiMtx); /********** STACK LOCKED MTX ******/
	return;
}


int	p3WikiService::tick()
{
	std::cerr << "p3WikiService::tick()";
	std::cerr << std::endl;
	
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

bool p3WikiService::getGroupList(std::list<std::string> &groups)
{
	RsStackMutex stack(mWikiMtx); /********** STACK LOCKED MTX ******/

        std::map<std::string, RsWikiGroup>::iterator it;
	for(it = mGroups.begin(); it != mGroups.end(); it++)
	{
		groups.push_back(it->second.mGroupId);
	}
	
	return false;
}

bool p3WikiService::getGroup(const std::string &groupid, RsWikiGroup &group)
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



bool p3WikiService::getPage(const std::string &pageid, RsWikiPage &page)
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


bool p3WikiService::getLatestPage(const std::string &origPageId, std::string &pageId)
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


bool p3WikiService::getPageVersions(const std::string &origPageId, std::list<std::string> &pageIds)
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


bool p3WikiService::getOrigPageList(const std::string &groupid, std::list<std::string> &pageIds)
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
	











