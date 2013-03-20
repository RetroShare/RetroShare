/*
 * libretroshare/src/gxs: rsgxsutil.cc
 *
 * RetroShare C++ Interface. Generic routines that are useful in GXS
 *
 * Copyright 2013-2013 by Christopher Evi-Parker
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

#include "rsgxsutil.h"


RsGxsMessageCleanUp::RsGxsMessageCleanUp(RsGeneralDataService* const dataService, uint32_t messageStorePeriod, uint32_t chunkSize)
: mDs(dataService), MESSAGE_STORE_PERIOD(messageStorePeriod), CHUNK_SIZE(chunkSize)
{

	std::map<RsGxsGroupId, RsGxsGrpMetaData*> grpMeta;
	mDs->retrieveGxsGrpMetaData(grpMeta);

	std::map<RsGxsGroupId, RsGxsGrpMetaData*>::iterator cit = grpMeta.begin();

	for(;cit != grpMeta.end(); cit++)
	{
		mGrpIds.push_back(cit->first);
		delete cit->second;
	}
}


bool RsGxsMessageCleanUp::clean()
{
	int i = 0;

	time_t now = time(NULL);

	while(!mGrpIds.empty())
	{

		RsGxsGroupId grpId = mGrpIds.back();
		mGrpIds.pop_back();
		GxsMsgReq req;
		GxsMsgMetaResult result;

		result[grpId] = std::vector<RsGxsMsgMetaData*>();
		mDs->retrieveGxsMsgMetaData(req, result);

		GxsMsgMetaResult::iterator mit = result.begin();

		req.clear();

		for(; mit != result.end(); mit++)
		{
			std::vector<RsGxsMsgMetaData*>& metaV = mit->second;
			std::vector<RsGxsMsgMetaData*>::iterator vit = metaV.begin();

			for(; vit != metaV.end(); )
			{
				RsGxsMsgMetaData* meta = *vit;
				if(meta->mPublishTs + MESSAGE_STORE_PERIOD > now)
				{
					req[grpId].push_back(meta->mMsgId);
				}

				delete meta;
				vit = metaV.erase(vit);
			}
		}

		mDs->removeMsgs(req);

		i++;
		if(i > CHUNK_SIZE) break;
	}

	return mGrpIds.empty();
}
