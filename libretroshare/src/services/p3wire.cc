/*******************************************************************************
 * libretroshare/src/services: p3wire.cc                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2020 by Robert Fernie <retroshare@lunamutt.com>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
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
 *******************************************************************************/
#include "services/p3wire.h"
#include "rsitems/rswireitems.h"

#include "util/rsrandom.h"

/****
 * #define WIRE_DEBUG 1
 ****/

RsWire *rsWire = NULL;


p3Wire::p3Wire(RsGeneralDataService* gds, RsNetworkExchangeService* nes, RsGixs *gixs)
	:RsGenExchange(gds, nes, new RsGxsWireSerialiser(), RS_SERVICE_GXS_TYPE_WIRE, gixs, wireAuthenPolicy()),
	RsWire(static_cast<RsGxsIface&>(*this)), mWireMtx("WireMtx")
{

}


const std::string WIRE_APP_NAME = "gxswire";
const uint16_t WIRE_APP_MAJOR_VERSION  =       1;
const uint16_t WIRE_APP_MINOR_VERSION  =       0;
const uint16_t WIRE_MIN_MAJOR_VERSION  =       1;
const uint16_t WIRE_MIN_MINOR_VERSION  =       0;

RsServiceInfo p3Wire::getServiceInfo()
{
        return RsServiceInfo(RS_SERVICE_GXS_TYPE_WIRE,
                WIRE_APP_NAME,
                WIRE_APP_MAJOR_VERSION,
                WIRE_APP_MINOR_VERSION,
                WIRE_MIN_MAJOR_VERSION,
                WIRE_MIN_MINOR_VERSION);
}



uint32_t p3Wire::wireAuthenPolicy()
{
	uint32_t policy = 0;
	uint8_t flag = 0;

	// Edits generally need an authors signature.

	// Wire requires all TopLevel (Orig/Reply) msgs to be signed with both PUBLISH & AUTHOR.
	// Reply References need to be signed by Author.
	flag = GXS_SERV::MSG_AUTHEN_ROOT_PUBLISH_SIGN | GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN;
	flag |= GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PUBLIC_GRP_BITS);

	// expect the requirements to be the same for RESTRICTED / PRIVATE groups too.
	// This needs to be worked through / fully evaluated.
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::RESTRICTED_GRP_BITS);
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PRIVATE_GRP_BITS);

	flag = 0;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::GRP_OPTION_BITS);

	return policy;
}

void p3Wire::service_tick()
{
	return;
}

RsTokenService* p3Wire::getTokenService() {

	return RsGenExchange::getTokenService();
}

void p3Wire::notifyChanges(std::vector<RsGxsNotify*>& changes)
{
	std::cerr << "p3Wire::notifyChanges() New stuff";
	std::cerr << std::endl;
}

        /* Specific Service Data */
bool p3Wire::getGroupData(const uint32_t &token, std::vector<RsWireGroup> &groups)
{
	std::cerr << "p3Wire::getGroupData()";
	std::cerr << std::endl;

	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);
	
	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();
		
		for(; vit != grpData.end(); ++vit)
		{
			RsGxsWireGroupItem* item = dynamic_cast<RsGxsWireGroupItem*>(*vit);

			if (item)
			{
				RsWireGroup group = item->group;
				group.mMeta = item->meta;
				delete item;
				groups.push_back(group);

				std::cerr << "p3Wire::getGroupData() Adding WireGroup to Vector: ";
				std::cerr << std::endl;
				std::cerr << group;
				std::cerr << std::endl;
			}
			else
			{
				std::cerr << "Not a WireGroupItem, deleting!" << std::endl;
				delete *vit;
			}

		}
	}
	return ok;
}


bool p3Wire::getPulseData(const uint32_t &token, std::vector<RsWirePulse> &pulses)
{
	GxsMsgDataMap msgData;
	bool ok = RsGenExchange::getMsgData(token, msgData);
	
	if(ok)
	{
		GxsMsgDataMap::iterator mit = msgData.begin();
		
		for(; mit != msgData.end(); ++mit)
		{
			RsGxsGroupId grpId = mit->first;
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();
			
			for(; vit != msgItems.end(); ++vit)
			{
				RsGxsWirePulseItem* item = dynamic_cast<RsGxsWirePulseItem*>(*vit);
				
				if(item)
				{
					RsWirePulse pulse = item->pulse;
					pulse.mMeta = item->meta;
					pulses.push_back(pulse);
					delete item;
				}
				else
				{
					std::cerr << "Not a WikiPulse Item, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}
	return ok;
}


bool p3Wire::createGroup(uint32_t &token, RsWireGroup &group)
{
	RsGxsWireGroupItem* groupItem = new RsGxsWireGroupItem();
	groupItem->group = group;
	groupItem->meta = group.mMeta;

	std::cerr << "p3Wire::createGroup(): ";
	std::cerr << std::endl;
	std::cerr << group;
	std::cerr << std::endl;

	std::cerr << "p3Wire::createGroup() pushing to RsGenExchange";
	std::cerr << std::endl;

	RsGenExchange::publishGroup(token, groupItem);
	return true;
}


bool p3Wire::createPulse(uint32_t &token, RsWirePulse &pulse)
{
	std::cerr << "p3Wire::createPulse(): " << pulse;
	std::cerr << std::endl;

	RsGxsWirePulseItem* pulseItem = new RsGxsWirePulseItem();
	pulseItem->pulse = pulse;
	pulseItem->meta = pulse.mMeta;

	RsGenExchange::publishMsg(token, pulseItem);
	return true;
}


std::ostream &operator<<(std::ostream &out, const RsWireGroup &group)
{
        out << "RsWireGroup [ ";
        out << " Name: " << group.mMeta.mGroupName;
        out << " Desc: " << group.mDescription;
        //out << " Category: " << group.mCategory;
        out << " ]";
        return out;
}

std::ostream &operator<<(std::ostream &out, const RsWirePulse &pulse)
{
        out << "RsWirePulse [ ";
        out << "Title: " << pulse.mMeta.mMsgName;
        out << "PulseText: " << pulse.mPulseText;
        out << "]";
        return out;
}

/***** FOR TESTING *****/

std::string p3Wire::genRandomId()
{
        std::string randomId;
        for(int i = 0; i < 20; i++)
        {
                randomId += (char) ('a' + (RSRandom::random_u32() % 26));
        }

        return randomId;
}

void p3Wire::generateDummyData()
{

}


