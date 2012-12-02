/*
 * libretroshare/src/services p3wiki.cc
 *
 * Wiki interface for RetroShare.
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

#include "services/p3wiki.h"
#include "serialiser/rswikiitems.h"

#include "util/rsrandom.h"

/****
 * #define WIKI_DEBUG 1
 ****/

RsWiki *rsWiki = NULL;


p3Wiki::p3Wiki(RsGeneralDataService* gds, RsNetworkExchangeService* nes)
	:RsGenExchange(gds, nes, new RsGxsWikiSerialiser(), RS_SERVICE_GXSV1_TYPE_WIKI), RsWiki(this)
{


	// Setup of dummy Pages.
	mAboutActive = false;
	mImprovActive = false;
}

void p3Wiki::service_tick()
{
	dummyTick();
	return;
}


void p3Wiki::notifyChanges(std::vector<RsGxsNotify*>& changes)
{
	std::cerr << "p3Wiki::notifyChanges() New stuff";
	std::cerr << std::endl;

	receiveChanges(changes);
}

        /* Specific Service Data */
bool p3Wiki::getCollections(const uint32_t &token, std::vector<RsWikiCollection> &collections)
{
	std::cerr << "p3Wiki::getCollections()";
	std::cerr << std::endl;

	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);
	
	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();
		
		for(; vit != grpData.end(); vit++)
		{
			RsGxsWikiCollectionItem* item = dynamic_cast<RsGxsWikiCollectionItem*>(*vit);

			if (item)
			{
				RsWikiCollection collection = item->collection;
				collection.mMeta = item->meta;
				delete item;
				collections.push_back(collection);

				std::cerr << "p3Wiki::getCollections() Adding Collection to Vector: ";
				std::cerr << std::endl;
				std::cerr << collection;
				std::cerr << std::endl;
			}
			else
			{
				std::cerr << "Not a WikiCollectionItem, deleting!" << std::endl;
				delete *vit;
			}

		}
	}
	return ok;
}


bool p3Wiki::getSnapshots(const uint32_t &token, std::vector<RsWikiSnapshot> &snapshots)
{
	GxsMsgDataMap msgData;
	bool ok = RsGenExchange::getMsgData(token, msgData);
	
	if(ok)
	{
		GxsMsgDataMap::iterator mit = msgData.begin();
		
		for(; mit != msgData.end();  mit++)
		{
			RsGxsGroupId grpId = mit->first;
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();
			
			for(; vit != msgItems.end(); vit++)
			{
				RsGxsWikiSnapshotItem* item = dynamic_cast<RsGxsWikiSnapshotItem*>(*vit);
				
				if(item)
				{
					RsWikiSnapshot snapshot = item->snapshot;
					snapshot.mMeta = item->meta;
					snapshots.push_back(snapshot);
					delete item;
				}
				else
				{
					std::cerr << "Not a WikiSnapshot Item, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}

	return ok;
}


bool p3Wiki::getRelatedSnapshots(const uint32_t &token, std::vector<RsWikiSnapshot> &snapshots)
{
        GxsMsgRelatedDataMap msgData;
	bool ok = RsGenExchange::getMsgRelatedData(token, msgData);
	
	if(ok)
	{
		GxsMsgRelatedDataMap::iterator mit = msgData.begin();
		
		for(; mit != msgData.end();  mit++)
		{
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();
			
			for(; vit != msgItems.end(); vit++)
			{
				RsGxsWikiSnapshotItem* item = dynamic_cast<RsGxsWikiSnapshotItem*>(*vit);
				
				if(item)
				{
					RsWikiSnapshot snapshot = item->snapshot;
					snapshot.mMeta = item->meta;
					snapshots.push_back(snapshot);
					delete item;
				}
				else
				{
					std::cerr << "Not a WikiSnapshot Item, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}

	return ok;
}


bool p3Wiki::getComments(const uint32_t &token, std::vector<RsWikiComment> &comments)
{
	GxsMsgDataMap msgData;
	bool ok = RsGenExchange::getMsgData(token, msgData);
	
	if(ok)
	{
		GxsMsgDataMap::iterator mit = msgData.begin();
		
		for(; mit != msgData.end();  mit++)
		{
			RsGxsGroupId grpId = mit->first;
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();
			
			for(; vit != msgItems.end(); vit++)
			{
				RsGxsWikiCommentItem* item = dynamic_cast<RsGxsWikiCommentItem*>(*vit);
				
				if(item)
				{
					RsWikiComment comment = item->comment;
					comment.mMeta = item->meta;
					comments.push_back(comment);
					delete item;
				}
				else
				{
					std::cerr << "Not a WikiComment Item, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}

	return ok;
	return false;
}



bool p3Wiki::submitCollection(uint32_t &token, RsWikiCollection &collection)
{
	RsGxsWikiCollectionItem* collectionItem = new RsGxsWikiCollectionItem();
	collectionItem->collection = collection;
	collectionItem->meta = collection.mMeta;

        std::cerr << "p3Wiki::submitCollection(): ";
	std::cerr << std::endl;
	std::cerr << collection;
	std::cerr << std::endl;

        std::cerr << "p3Wiki::submitCollection() pushing to RsGenExchange";
	std::cerr << std::endl;

	RsGenExchange::publishGroup(token, collectionItem);
	return true;
}


bool p3Wiki::submitSnapshot(uint32_t &token, RsWikiSnapshot &snapshot)
{
	std::cerr << "p3Wiki::submitSnapshot(): " << snapshot;
	std::cerr << std::endl;

        RsGxsWikiSnapshotItem* snapshotItem = new RsGxsWikiSnapshotItem();
        snapshotItem->snapshot = snapshot;
        snapshotItem->meta = snapshot.mMeta;
        snapshotItem->meta.mMsgFlags = FLAG_MSG_TYPE_WIKI_SNAPSHOT;

        RsGenExchange::publishMsg(token, snapshotItem);
	return true;
}


bool p3Wiki::submitComment(uint32_t &token, RsWikiComment &comment)
{
        RsGxsWikiCommentItem* commentItem = new RsGxsWikiCommentItem();
        commentItem->comment = comment;
        commentItem->meta = comment.mMeta;
        commentItem->meta.mMsgFlags = FLAG_MSG_TYPE_WIKI_COMMENT;

        RsGenExchange::publishMsg(token, commentItem);
	return true;
}



std::ostream &operator<<(std::ostream &out, const RsWikiCollection &group)
{
        out << "RsWikiCollection [ ";
        out << " Name: " << group.mMeta.mGroupName;
        out << " Desc: " << group.mDescription;
        out << " Category: " << group.mCategory;
        out << " ]";
        return out;
}

std::ostream &operator<<(std::ostream &out, const RsWikiSnapshot &shot)
{
        out << "RsWikiSnapshot [ ";
        out << "Title: " << shot.mMeta.mMsgName;
        out << "]";
        return out;
}

std::ostream &operator<<(std::ostream &out, const RsWikiComment &comment)
{
        out << "RsWikiComment [ ";
        out << "Title: " << comment.mMeta.mMsgName;
        out << "]";
        return out;
}


/***** FOR TESTING *****/

std::string p3Wiki::genRandomId()
{
        std::string randomId;
        for(int i = 0; i < 20; i++)
        {
                randomId += (char) ('a' + (RSRandom::random_u32() % 26));
        }

        return randomId;
}

const int about_len = 10;
const std::string about_txt[] = 
	{	"Welcome to RsWiki, a fully distributed Wiki system that anyone can edit.",
		"Please read through these dummy Wiki pages to learn how it should work\n",
		"Basic Functionality:",
		" - New Group: creates a collection of Wiki Pages, the creator of group",
		" and anyone they share the publish keys with) is the moderator\n", 
		" - New Page: Create a new Wiki Page, only Group Moderators can do this\n",
		" - Edit: Anyone Can Edit the Wiki Page, and the changes form a tree under the original\n",
		" - RePublish: This is used by the Moderators to accept and reject Edits.",
		" the republished page becomes a new version of the Root Page, allowing more edits to be done\n",
		"Please read the improvements section to see how we envision the wiki's working",
	};


const int improvements_len = 14;
const std::string improvements_txt[] = 
	{	"As you can see, the current Wiki is a basic framework waiting to be expanded.",
		"There are lots of potential improvements... some of my ideas are listed below\n",
		"Ideas:",
		" - Formatting: No HTML, lets use Markdown or something similar.\n",
		" - Diffs, lots of edits will lead to complex merges - a robust merge tool is essential\n",
		" - Read Mode... hide all the Edits, and only show the most recently published versions\n",
		" - Easy Duplication - to take over an Abandoned or badly moderated Wiki. Copies All base versions to a new group\n",
		" - WikiLinks. A generic Wiki Cross Linking system. This should be combined with Easy Duplication option,",
		" to allow easy replacement of groups if necessary... A good design here is critical to a successful Wiki ecosystem\n",
		" - work out how to include media (photos, audio, video, etc) without embedding in pages",
		" this would leverage the turtle transfer system somehow - maybe like channels.\n",
		" - Comments, reviews etc can be incorporated - ideas here are welcome.\n",
		" - Any other suggestion???",
		" - Come on more ideas!"
	};


void p3Wiki::generateDummyData()
{

#define GEN_COLLECTIONS		0

        int i;
        for(i = 0; i < GEN_COLLECTIONS; i++)
        {
		RsWikiCollection wiki;
		wiki.mMeta.mGroupId = genRandomId();
		wiki.mMeta.mGroupFlags = 0;
		wiki.mMeta.mGroupName = genRandomId();

		uint32_t dummyToken = 0;
		submitCollection(dummyToken, wiki);
	}


	RsWikiCollection wiki;
	wiki.mMeta.mGroupFlags = 0;
	wiki.mMeta.mGroupName = "About RsWiki";

	submitCollection(mAboutToken, wiki);

	wiki.mMeta.mGroupFlags = 0;
	wiki.mMeta.mGroupName = "RsWiki Improvements";

	submitCollection(mImprovToken, wiki);

	mAboutLines = 0;
	mImprovLines = 0;

	mAboutActive = true;
	mImprovActive = true;
}


bool generateNextDummyPage(const RsGxsMessageId &threadId, const int lines, const RsGxsGrpMsgIdPair &parentId, 
				const std::string *page_lines, const int num_pagelines, RsWikiSnapshot &snapshot)
{
	snapshot.mMeta.mGroupId = parentId.first;

	// Create an About Page.
#define NUM_SUB_PAGES 3

	if ((lines % NUM_SUB_PAGES == 0) || (lines == num_pagelines))
	{
		/* do a new baseline */
		snapshot.mMeta.mOrigMsgId = threadId;
	}
	else
	{
		snapshot.mMeta.mParentId = parentId.second;
		snapshot.mMeta.mThreadId = threadId;
	}

	std::string page;
	for(int i = 0; (i < lines) && (i < num_pagelines); i++)
	{
		snapshot.mPage += page_lines[i];
		snapshot.mPage += '\n';
	}

	return (lines <= num_pagelines);
}


#include <retroshare/rsidentity.h>

std::string chooseRandomAuthorId()
{
        /* chose a random Id to sign with */
        std::list<RsGxsId> ownIds;
        std::list<RsGxsId>::iterator it;

        rsIdentity->getOwnIds(ownIds);

        uint32_t idx = (uint32_t) (ownIds.size() * RSRandom::random_f32());
        int i = 0;
        for(it = ownIds.begin(); (it != ownIds.end()) && (i < idx); it++, i++);

	std::string answer;
        if (it != ownIds.end())
        {
		answer = *it;
        }
	return answer;
}



void p3Wiki::dummyTick()
{
	if (mAboutActive)
	{
		std::cerr << "p3Wiki::dummyTick() AboutActive";
		std::cerr << std::endl;

		uint32_t status = RsGenExchange::getTokenService()->requestStatus(mAboutToken);

		if (status == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
		{
			std::cerr << "p3Wiki::dummyTick() AboutActive, Lines: " << mAboutLines;
			std::cerr << std::endl;

			if (mAboutLines == 0)
			{
				/* get the group Id */
				RsGxsGroupId groupId;
				if (!acknowledgeTokenGrp(mAboutToken, groupId))
				{
					std::cerr << " ERROR ";
					std::cerr << std::endl;
					mAboutActive = false;
				}

				/* create baseline snapshot */
				RsWikiSnapshot page;				
				page.mMeta.mGroupId = groupId;
				page.mPage = "Baseline page... a placeholder for About Wiki";
				page.mMeta.mMsgName = "About RsWiki";
				page.mMeta.mAuthorId = chooseRandomAuthorId();

				submitSnapshot(mAboutToken, page);
				mAboutLines++;
			}
			else
			{
				/* get the msg Id, and generate next snapshot */
				RsGxsGrpMsgIdPair msgId;
				if (!acknowledgeTokenMsg(mAboutToken, msgId))
				{
					std::cerr << " ERROR ";
					std::cerr << std::endl;
					mAboutActive = false;
				}

				if (mAboutLines == 1)
				{
					mAboutThreadId = msgId.second;
				}

				RsWikiSnapshot page;				
				page.mMeta.mMsgName = "About RsWiki";
				page.mMeta.mAuthorId = chooseRandomAuthorId();
				if (!generateNextDummyPage(mAboutThreadId, mAboutLines, msgId, about_txt, about_len, page))
				{
					std::cerr << "About Pages Done";
					std::cerr << std::endl;
					mAboutActive = false;
				}
				else
				{
					mAboutLines++;
					submitSnapshot(mAboutToken, page);
				}
			}
		}
	}

	if (mImprovActive)
	{
		std::cerr << "p3Wiki::dummyTick() ImprovActive";
		std::cerr << std::endl;

		uint32_t status = RsGenExchange::getTokenService()->requestStatus(mImprovToken);

		if (status == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
		{
			std::cerr << "p3Wiki::dummyTick() ImprovActive, Lines: " << mImprovLines;
			std::cerr << std::endl;

			if (mImprovLines == 0)
			{
				/* get the group Id */
				RsGxsGroupId groupId;
				if (!acknowledgeTokenGrp(mImprovToken, groupId))
				{
					std::cerr << " ERROR ";
					std::cerr << std::endl;
					mImprovActive = false;
				}

				/* create baseline snapshot */
				RsWikiSnapshot page;				
				page.mMeta.mGroupId = groupId;
				page.mPage = "Baseline page... a placeholder for Improv Wiki";
				page.mMeta.mMsgName = "Improv RsWiki";
				page.mMeta.mAuthorId = chooseRandomAuthorId();

				submitSnapshot(mImprovToken, page);
				mImprovLines++;
			}
			else
			{
				/* get the msg Id, and generate next snapshot */
				RsGxsGrpMsgIdPair msgId;
				if (!acknowledgeTokenMsg(mImprovToken, msgId))
				{
					std::cerr << " ERROR ";
					std::cerr << std::endl;
					mImprovActive = false;
				}

				if (mImprovLines == 1)
				{
					mImprovThreadId = msgId.second;
				}

				RsWikiSnapshot page;				
				page.mMeta.mMsgName = "Improv RsWiki";
				page.mMeta.mAuthorId = chooseRandomAuthorId();
				if (!generateNextDummyPage(mImprovThreadId, mImprovLines, msgId, improvements_txt, improvements_len, page))
				{
					std::cerr << "Improv Pages Done";
					std::cerr << std::endl;
					mImprovActive = false;
				}
				else
				{
					mImprovLines++;
					submitSnapshot(mImprovToken, page);
				}
			}
		}
	}
}





