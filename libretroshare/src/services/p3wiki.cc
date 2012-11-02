/*
 * libretroshare/src/services p3wiki.cc
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

#include "services/p3wiki.h"
#include "serialiser/rswikiitems.h"

/****
 * #define WIKI_DEBUG 1
 ****/

RsWiki *rsWiki = NULL;


p3Wiki::p3Wiki(RsGeneralDataService* gds, RsNetworkExchangeService* nes)
	:RsGenExchange(gds, nes, new RsGxsWikiSerialiser(), RS_SERVICE_GXSV1_TYPE_WIKI), RsWiki(this)
{


}

void p3Wiki::service_tick()
{
	return;
}


void p3Wiki::notifyChanges(std::vector<RsGxsNotify*>& changes)
{
	receiveChanges(changes);
}

        /* Specific Service Data */
bool p3Wiki::getCollections(const uint32_t &token, std::vector<RsWikiCollection> &collections)
{
	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);
	
	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();
		
		for(; vit != grpData.end(); vit++)
		{
			RsGxsWikiCollectionItem* item = dynamic_cast<RsGxsWikiCollectionItem*>(*vit);
			RsWikiCollection collection = item->collection;
			collection.mMeta = item->collection.mMeta;
			delete item;
			collections.push_back(collection);
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
	RsGenExchange::publishGroup(token, collectionItem);
	return true;
}


bool p3Wiki::submitSnapshot(uint32_t &token, RsWikiSnapshot &snapshot)
{
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


