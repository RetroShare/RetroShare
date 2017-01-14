/*
 * GXS Mailing Service
 * Copyright (C) 2016-2017  Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "p3gxsmails.h"


bool p3GxsMails::sendEmail(const RsGxsId& own_gxsid, const RsGxsId& recipient, const std::string& body)
{
	std::cout << "p3GxsMails::sendEmail(...)" << std::endl;

	if(preferredGroupId.isNull())
	{
		requestGroupsList();
		return false;
	}

	RsGxsMailItem* m = new RsGxsMailItem;
	m->meta.mAuthorId = own_gxsid;
	m->meta.mGroupId = preferredGroupId;
	m->recipients.ids.insert(recipient);
	m->body = body;

	uint32_t token;
	publishMsg(token, m);

	return true;
}

void p3GxsMails::handleResponse(uint32_t token, uint32_t req_type)
{
	//std::cout << "p3GxsMails::handleResponse(" << token << ", " << req_type << ")" << std::endl;
	switch (req_type)
	{
	case GROUPS_LIST:
	{
		std::vector<RsGxsGrpItem*> groups;
		getGroupData(token, groups);

		for( std::vector<RsGxsGrpItem *>::iterator it = groups.begin();
		     it != groups.end(); ++it )
		{
			RsGxsGrpItem* grp = *it;
			if(!IS_GROUP_SUBSCRIBED(grp->meta.mSubscribeFlags))
			{
				std::cout << "p3GxsMails::handleResponse(...) subscribing to group: " << grp->meta.mGroupId << std::endl;
				uint32_t token;
				subscribeToGroup(token, grp->meta.mGroupId, true);
			}

			supersedePreferredGroup(grp->meta.mGroupId);
		}

		if(preferredGroupId.isNull())
		{
			std::cout << "p3GxsMails::handleResponse(...) preferredGroupId.isNull()" << std::endl;
			// TODO: Should check if we have friends before of doing this?
			uint32_t token;
			publishGroup(token, new RsGxsMailGroupItem());
			queueRequest(token, GROUP_CREATE);
		}

		break;
	}
	case GROUP_CREATE:
	{
		std::cout << "p3GxsMails::handleResponse(...) GROUP_CREATE" << std::endl;
		RsGxsGroupId grpId;
		acknowledgeTokenGrp(token, grpId);
		supersedePreferredGroup(grpId);
		break;
	}
	default:
		std::cout << "p3GxsMails::handleResponse(...) Unknown req_type: " << req_type << std::endl;
		break;
	}
/*
	GxsMsgDataMap msgItems;
	if(!RsGenExchange::getMsgData(token, msgItems))
	{
		std::cerr << "p3GxsMails::handleResponse(...) Cannot get msg data. "
				  << "Something's weird." << std::endl;
	}
*/
}

void p3GxsMails::service_tick()
{
	static int tc = 0;
	++tc;
	if((tc % 100) == 0)
	{
//		std::cout << "p3GxsMails::service_tick() " << tc << " "
//		          << preferredGroupId << std::endl;
		requestGroupsList();
	}

#if 0
	if(tc == 500)
	{
		RsGxsId own_gxsid("d0df7474bdde0464679e6ef787890287");
		RsGxsId recipient("d060bea09dfa14883b5e6e517eb580cd");
		sendEmail(own_gxsid, recipient, "Ciao!");
	}
#endif

	GxsTokenQueue::checkRequests();
}

RsGenExchange::ServiceCreate_Return p3GxsMails::service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& /*keySet*/)
{
	std::cout << "p3GxsMails::service_CreateGroup(...) " << grpItem->meta.mGroupId << std::endl;
	return SERVICE_CREATE_SUCCESS;
}

void p3GxsMails::notifyChanges(std::vector<RsGxsNotify*>& changes)
{
	std::cout << "p3GxsMails::notifyChanges(...)" << std::endl;
	for( std::vector<RsGxsNotify*>::const_iterator it = changes.begin();
	     it != changes.end(); ++it )
	{
		RsGxsGroupChange* grpChange = dynamic_cast<RsGxsGroupChange *>(*it);
		RsGxsMsgChange* msgChange = dynamic_cast<RsGxsMsgChange *>(*it);

		if (grpChange)
		{
			typedef std::list<RsGxsGroupId>::const_iterator itT;
			for( itT it = grpChange->mGrpIdList.begin();
			     it != grpChange->mGrpIdList.end(); ++it )
			{
				const RsGxsGroupId& grpId = *it;
				std::cout << "p3GxsMails::notifyChanges(...) got group "
				          << grpId << std::endl;
				supersedePreferredGroup(grpId);
			}
		}
		else if(msgChange)
		{
			typedef std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > mT;
			for( mT::const_iterator it = msgChange->msgChangeMap.begin();
			     it != msgChange->msgChangeMap.end(); ++it )
			{
				const RsGxsGroupId& grpId = it->first;
				const std::vector<RsGxsMessageId>& msgsIds = it->second;
				typedef std::vector<RsGxsMessageId>::const_iterator itT;
				for(itT vit = msgsIds.begin(); vit != msgsIds.end(); ++vit)
				{
					const RsGxsMessageId& msgId = *vit;
					std::cout << "p3GxsMails::notifyChanges(...) got "
					          << "new message " << msgId << " in group "
					          << grpId << std::endl;
				}
			}
		}
	}
}

bool p3GxsMails::requestGroupsList()
{
	//	std::cout << "p3GxsMails::requestGroupsList() GXS_REQUEST_TYPE_GROUP_META" << std::endl;
	uint32_t token;
	RsTokReqOptions opts; opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	RsGenExchange::getTokenService()->requestGroupInfo(token, 0xcaca, opts);
	GxsTokenQueue::queueRequest(token, GROUPS_LIST);
	return true;
}
