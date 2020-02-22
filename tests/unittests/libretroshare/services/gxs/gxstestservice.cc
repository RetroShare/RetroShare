/*******************************************************************************
 * unittests/libretroshare/services/gxs/gxstestservice.cc                      *
 *                                                                             *
 * Copyright 2012      by Robert Fernie    <retroshare.project@gmail.com>      *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/

#include "gxstestservice.h"
#include "retroshare/rsgxsflags.h"
#include "rsgxstestitems.h"

#include "util/rsrandom.h"

GxsTestService::GxsTestService(RsGeneralDataService* gds, RsNetworkExchangeService* nes, RsGixs *gixs, uint32_t testMode)
	:RsGenExchange(gds, nes, new RsGxsTestSerialiser(), RS_SERVICE_GXS_TYPE_TEST, gixs, testAuthenPolicy(testMode)), 
	RsGxsIfaceHelper(this),
	mTestMode(testMode)
{

}

const std::string GXS_TEST_APP_NAME = "gxstest";
const uint16_t GXS_TEST_APP_MAJOR_VERSION  =       1;
const uint16_t GXS_TEST_APP_MINOR_VERSION  =       0;
const uint16_t GXS_TEST_MIN_MAJOR_VERSION  =       1;
const uint16_t GXS_TEST_MIN_MINOR_VERSION  =       0;

RsServiceInfo GxsTestService::getServiceInfo()
{
        return RsServiceInfo(RS_SERVICE_GXS_TYPE_TEST,
                GXS_TEST_APP_NAME,
                GXS_TEST_APP_MAJOR_VERSION,
                GXS_TEST_APP_MINOR_VERSION,
                GXS_TEST_MIN_MAJOR_VERSION,
                GXS_TEST_MIN_MINOR_VERSION);
}


uint32_t GxsTestService::testAuthenPolicy(uint32_t /*testMode*/)
{
	uint32_t policy = 0;
	uint8_t flag = 0;

	// Edits generally need an authors signature.

	//flag = GXS_SERV::MSG_AUTHEN_ROOT_PUBLISH_SIGN | GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN;
	flag = GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN | GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PUBLIC_GRP_BITS);

	flag |= GXS_SERV::MSG_AUTHEN_CHILD_PUBLISH_SIGN;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::RESTRICTED_GRP_BITS);
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PRIVATE_GRP_BITS);

	flag = 0;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::GRP_OPTION_BITS);

	return policy;
}


void GxsTestService::service_tick()
{
	RsTickEvent::tick_events();
	return;
}


void GxsTestService::notifyChanges(std::vector<RsGxsNotify*>& changes)
{
	std::cerr << "GxsTestService::notifyChanges() New stuff";
	std::cerr << std::endl;
}

        /* Specific Service Data */
bool GxsTestService::getTestGroups(const uint32_t &token, std::vector<RsTestGroup> &groups)
{
	std::cerr << "GxsTestService::getTestGroups()";
	std::cerr << std::endl;

	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);
	
	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();
		
		for(; vit != grpData.end(); vit++)
		{
			RsGxsTestGroupItem* item = dynamic_cast<RsGxsTestGroupItem*>(*vit);

			if (item)
			{
				RsTestGroup group = item->group;
				group.mMeta = item->meta;
				delete item;
				groups.push_back(group);

				std::cerr << "GxsTestService::getTestGroups() Adding Group to Vector: ";
				std::cerr << std::endl;
				std::cerr << group;
				std::cerr << std::endl;
			}
			else
			{
				std::cerr << "Not a TestGroupItem, deleting!" << std::endl;
				delete *vit;
			}

		}
	}
	return ok;
}


bool GxsTestService::getTestMsgs(const uint32_t &token, std::vector<RsTestMsg> &msgs)
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
				RsGxsTestMsgItem* item = dynamic_cast<RsGxsTestMsgItem*>(*vit);
				
				if(item)
				{
					RsTestMsg msg = item->msg;
					msg.mMeta = item->meta;
					msgs.push_back(msg);
					delete item;
				}
				else
				{
					std::cerr << "Not a TestMsg Item, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}

	return ok;
}


bool GxsTestService::getRelatedMsgs(const uint32_t &token, std::vector<RsTestMsg> &msgs)
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
				RsGxsTestMsgItem* item = dynamic_cast<RsGxsTestMsgItem*>(*vit);
				
				if(item)
				{
					RsTestMsg msg = item->msg;
					msg.mMeta = item->meta;
					msgs.push_back(msg);
					delete item;
				}
				else
				{
					std::cerr << "Not a TestMsg Item, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}

	return ok;
}


bool GxsTestService::submitTestGroup(uint32_t &token, RsTestGroup &group)
{
	RsGxsTestGroupItem* groupItem = new RsGxsTestGroupItem();
	groupItem->group = group;
	groupItem->meta = group.mMeta;

        std::cerr << "GxsTestService::submitTestGroup(): ";
	std::cerr << std::endl;
	std::cerr << group;
	std::cerr << std::endl;

        std::cerr << "GxsTestService::submitTestGroup() pushing to RsGenExchange";
	std::cerr << std::endl;

	RsGenExchange::publishGroup(token, groupItem);
	return true;
}

RsGenExchange::ServiceCreate_Return GxsTestService::service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& /*keySet*/)
{
        std::cerr << "GxsTestService::service_CreateGroup()";
        std::cerr << std::endl;

        RsGxsTestGroupItem *item = dynamic_cast<RsGxsTestGroupItem *>(grpItem);
        if (!item)
        {
                std::cerr << "p3GxsCircles::service_CreateGroup() ERROR invalid cast";
                std::cerr << std::endl;
                return SERVICE_CREATE_FAIL;
        }

        std::cerr << "GxsTestService::service_CreateGroup() Details:";
        std::cerr << std::endl;
	std::cerr << "GroupId: " << item->meta.mGroupId;
        std::cerr << std::endl;
	std::cerr << "AuthorId: " << item->meta.mAuthorId;
        std::cerr << std::endl;
	std::cerr << "CircleType: " << item->meta.mCircleType;
        std::cerr << std::endl;
	std::cerr << "CircleId: " << item->meta.mCircleId;
        std::cerr << std::endl;
	std::cerr << "InternalCircle: " << item->meta.mInternalCircle;
        std::cerr << std::endl;

        return SERVICE_CREATE_SUCCESS;
}



bool GxsTestService::submitTestMsg(uint32_t &token, RsTestMsg &msg)
{
	std::cerr << "GxsTestService::submitTestMsg(): " << msg;
	std::cerr << std::endl;

        RsGxsTestMsgItem* msgItem = new RsGxsTestMsgItem();
        msgItem->msg = msg;
        msgItem->meta = msg.mMeta;

        RsGenExchange::publishMsg(token, msgItem);
	return true;
}

bool GxsTestService::updateTestGroup(uint32_t &token, RsTestGroup &group)
{
        std::cerr << "GxsTestService::updateTestGroup()" << std::endl;

        RsGxsTestGroupItem* grpItem = new RsGxsTestGroupItem();
        grpItem->group = group;
        grpItem->meta = group.mMeta;

        RsGenExchange::updateGroup(token, grpItem);
        return true;
}


std::ostream &operator<<(std::ostream &out, const RsTestGroup &group)
{
        out << "RsTestGroup [ ";
        out << " Name: " << group.mMeta.mGroupName;
        out << " TestString: " << group.mTestString;
        out << " ]";
        return out;
}

std::ostream &operator<<(std::ostream &out, const RsTestMsg &msg)
{
        out << "RsTestMsg [ ";
        out << " Name: " << msg.mMeta.mMsgName;
        out << " TestString: " << msg.mTestString;
        out << "]";
        return out;
}


        // Overloaded from RsTickEvent for Event callbacks.
void GxsTestService::handle_event(uint32_t event_type, const std::string &/*elabel*/)
{
	std::cerr << "GxsTestService::handle_event(" << event_type << ")";
	std::cerr << std::endl;

	// stuff.
	switch(event_type)
	{
		default:
			/* error */
			std::cerr << "GxsTestService::handle_event() Unknown Event Type: " << event_type;
			std::cerr << std::endl;
			break;
	}
}


