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


uint32_t RsWirePulse::ImageCount()
{
	if (!mImage4.empty()) {
		return 4;
	}
	if (!mImage3.empty()) {
		return 3;
	}
	if (!mImage2.empty()) {
		return 2;
	}
	if (!mImage1.empty()) {
		return 1;
	}
	return 0;
}

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

void p3Wire::notifyChanges(std::vector<RsGxsNotify*>& /*changes*/)
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

bool p3Wire::getGroupPtrData(const uint32_t &token, std::map<RsGxsGroupId, RsWireGroupSPtr> &groups)
{
	std::cerr << "p3Wire::getGroupPtrData()";
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
				RsWireGroupSPtr pGroup = std::make_shared<RsWireGroup>(item->group);
				pGroup->mMeta = item->meta;
				delete item;

				groups[pGroup->mMeta.mGroupId] = pGroup;

				std::cerr << "p3Wire::getGroupPtrData() Adding WireGroup to Vector: ";
				std::cerr << pGroup->mMeta.mGroupId.toStdString();
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
					std::cerr << "Not a WirePulse Item, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}
	return ok;
}

bool p3Wire::getPulsePtrData(const uint32_t &token, std::list<RsWirePulseSPtr> &pulses)
{
	GxsMsgDataMap msgData;
	bool ok = RsGenExchange::getMsgData(token, msgData);

	if(ok)
	{
		GxsMsgDataMap::iterator mit = msgData.begin();

		for(; mit != msgData.end(); ++mit)
		{
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();

			for(; vit != msgItems.end(); ++vit)
			{
				RsGxsWirePulseItem* item = dynamic_cast<RsGxsWirePulseItem*>(*vit);

				if(item)
				{
					RsWirePulseSPtr pPulse = std::make_shared<RsWirePulse>(item->pulse);
					pPulse->mMeta = item->meta;
					pulses.push_back(pPulse);
					delete item;
				}
				else
				{
					std::cerr << "Not a WirePulse Item, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}
	return ok;
}

bool p3Wire::getRelatedPulseData(const uint32_t &token, std::vector<RsWirePulse> &pulses)
{
	GxsMsgRelatedDataMap msgData;
	std::cerr << "p3Wire::getRelatedPulseData()";
	std::cerr << std::endl;
	bool ok = RsGenExchange::getMsgRelatedData(token, msgData);

	if (ok)
	{
		std::cerr << "p3Wire::getRelatedPulseData() is OK";
		std::cerr << std::endl;
		GxsMsgRelatedDataMap::iterator mit = msgData.begin();

		for(; mit != msgData.end(); ++mit)
		{
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
					std::cerr << "Not a WirePulse Item, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}
	else
	{
		std::cerr << "p3Wire::getRelatedPulseData() is NOT OK";
		std::cerr << std::endl;
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

// Blocking Interfaces.
bool p3Wire::createGroup(RsWireGroup &group)
{
	uint32_t token;
	return createGroup(token, group) && waitToken(token) == RsTokenService::COMPLETE;
}

bool p3Wire::updateGroup(const RsWireGroup & /*group*/)
{
	// TODO
	return false;
}

bool p3Wire::getGroups(const std::list<RsGxsGroupId> groupIds, std::vector<RsWireGroup> &groups)
{
	uint32_t token;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	if (groupIds.empty())
	{
		if (!requestGroupInfo(token, opts) || waitToken(token) != RsTokenService::COMPLETE )
		{
			return false;
		}
	}
	else
	{
		if (!requestGroupInfo(token, opts, groupIds) || waitToken(token) != RsTokenService::COMPLETE )
		{
			return false;
		}
	}
	return getGroupData(token, groups) && !groups.empty();
}


std::ostream &operator<<(std::ostream &out, const RsWireGroup &group)
{
	out << "RsWireGroup [ ";
	out << " Name: " << group.mMeta.mGroupName;
	out << " Tagline: " << group.mTagline;
	out << " Location: " << group.mLocation;
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


// New Interfaces.
bool p3Wire::fetchPulse(RsGxsGroupId grpId, RsGxsMessageId msgId, RsWirePulseSPtr &pPulse)
{
	std::cerr << "p3Wire::fetchPulse(" << grpId << ", " << msgId << ") waiting for token";
	std::cerr << std::endl;

	uint32_t token;
	{
		RsTokReqOptions opts;
		opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
		opts.mOptions = RS_TOKREQOPT_MSG_LATEST;

		GxsMsgReq ids;
		std::set<RsGxsMessageId> &set_msgIds = ids[grpId];
		set_msgIds.insert(msgId);

		getTokenService()->requestMsgInfo(
			token, RS_TOKREQ_ANSTYPE_DATA, opts, ids);
	}

	// wait for pulse request to completed.
	std::cerr << "p3Wire::fetchPulse() waiting for token";
	std::cerr << std::endl;

	int result = waitToken(token);
	if (result != RsTokenService::COMPLETE)
	{
		std::cerr << "p3Wire::fetchPulse() token FAILED, result: " << result;
		std::cerr << std::endl;
		return false;
	}

	// retrieve Pulse.
	std::cerr << "p3Wire::fetchPulse() retrieving token";
	std::cerr << std::endl;
	{
		bool okay = true;
		std::vector<RsWirePulse> pulses;
		if (getPulseData(token, pulses)) {
			if (pulses.size() == 1) {
				// save to output pulse.
				pPulse = std::make_shared<RsWirePulse>(pulses[0]);
				std::cerr << "p3Wire::fetchPulse() retrieved token: " << *pPulse;
				std::cerr << std::endl;
				std::cerr << "p3Wire::fetchPulse() ANS GrpId: " << pPulse->mMeta.mGroupId;
				std::cerr << " MsgId: " << pPulse->mMeta.mMsgId;
				std::cerr << " OrigMsgId: " << pPulse->mMeta.mOrigMsgId;
				std::cerr << std::endl;
			} else {
				std::cerr << "p3Wire::fetchPulse() ERROR multiple pulses";
				std::cerr << std::endl;
				okay = false;
			}
		} else {
			std::cerr << "p3Wire::fetchPulse() ERROR failed to retrieve token";
			std::cerr << std::endl;
			okay = false;
		}

		if (!okay) {
			std::cerr << "p3Wire::fetchPulse() tokenPulse ERROR";
			std::cerr << std::endl;
			// TODO cancel other request.
			return false;
		}
	}
	return true;
}

// New Interfaces.
bool p3Wire::createOriginalPulse(const RsGxsGroupId &grpId, RsWirePulseSPtr pPulse)
{
	// request Group.
	std::list<RsGxsGroupId> groupIds = { grpId };
	std::vector<RsWireGroup> groups;
	bool groupOkay = getGroups(groupIds, groups);
	if (!groupOkay) {
		std::cerr << "p3Wire::createOriginalPulse() getGroups failed";
		std::cerr << std::endl;
		return false;
	}

	if (groups.size() != 1) {
		std::cerr << "p3Wire::createOriginalPulse() getGroups invalid size";
		std::cerr << std::endl;
		return false;
	}

	// ensure Group is suitable.
	RsWireGroup group = groups[0];
	if ((group.mMeta.mGroupId != grpId) ||
		(!(group.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_PUBLISH)))
	{
		std::cerr << "p3Wire::createOriginalPulse() Group unsuitable";
		std::cerr << std::endl;
		return false;
	}

	// Create Msg.
	// Start fresh, just to be sure nothing dodgy happens in UX world.
	RsWirePulse pulse;

	pulse.mMeta.mGroupId  = group.mMeta.mGroupId;
	pulse.mMeta.mAuthorId = group.mMeta.mAuthorId;
	pulse.mMeta.mThreadId.clear();
	pulse.mMeta.mParentId.clear();
	pulse.mMeta.mOrigMsgId.clear();

	// copy info over
	pulse.mPulseType = WIRE_PULSE_TYPE_ORIGINAL;
	pulse.mSentiment = pPulse->mSentiment;
	pulse.mPulseText = pPulse->mPulseText;
	pulse.mImage1 = pPulse->mImage1;
	pulse.mImage2 = pPulse->mImage2;
	pulse.mImage3 = pPulse->mImage3;
	pulse.mImage4 = pPulse->mImage4;

	// all mRefs should empty.

	uint32_t token;
	createPulse(token, pulse);

	int result = waitToken(token);
	if (result != RsTokenService::COMPLETE)
	{
		std::cerr << "p3Wire::createOriginalPulse() Failed to create Pulse";
		std::cerr << std::endl;
		return false;
	}

	return true;
}

bool p3Wire::createReplyPulse(RsGxsGroupId grpId, RsGxsMessageId msgId, RsGxsGroupId replyWith, uint32_t reply_type, RsWirePulseSPtr pPulse)
{
	// check reply_type. can only be ONE.
	if (!((reply_type == WIRE_PULSE_TYPE_REPLY) ||
		(reply_type == WIRE_PULSE_TYPE_REPUBLISH) ||
		(reply_type == WIRE_PULSE_TYPE_LIKE)))
	{
		std::cerr << "p3Wire::createReplyPulse() reply_type is invalid";
		std::cerr << std::endl;
		return false;
	}

	// request both groups.
	std::list<RsGxsGroupId> groupIds = { grpId, replyWith };
	std::vector<RsWireGroup> groups;
	bool groupOkay = getGroups(groupIds, groups);
	if (!groupOkay) {
		std::cerr << "p3Wire::createReplyPulse() getGroups failed";
		std::cerr << std::endl;
		return false;
	}

	// extract group info.
	RsWireGroup replyToGroup;
	RsWireGroup replyWithGroup;

	if (grpId == replyWith)
	{
		if (groups.size() != 1) {
			std::cerr << "p3Wire::createReplyPulse() getGroups != 1";
			std::cerr << std::endl;
			return false;
		}

		replyToGroup = groups[0];
		replyWithGroup = groups[0];
	}
	else
	{
		if (groups.size() != 2) {
			std::cerr << "p3Wire::createReplyPulse() getGroups != 2";
			std::cerr << std::endl;
			return false;
		}

		if (groups[0].mMeta.mGroupId == grpId) {
			replyToGroup = groups[0];
			replyWithGroup = groups[1];
		} else {
			replyToGroup = groups[1];
			replyWithGroup = groups[0];
		}
	}

	// check groupIds match
	if ((replyToGroup.mMeta.mGroupId != grpId) ||
		(replyWithGroup.mMeta.mGroupId != replyWith))
	{
		std::cerr << "p3Wire::createReplyPulse() groupid mismatch";
		std::cerr << std::endl;
		return false;
	}

	// ensure Group is suitable.
	if ((!(replyToGroup.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)) ||
		(!(replyWithGroup.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_PUBLISH)))
	{
		std::cerr << "p3Wire::createReplyPulse() Group unsuitable";
		std::cerr << std::endl;
		return false;
	}

	// **********************************************************
	RsWirePulseSPtr replyToPulse;
	if (!fetchPulse(grpId, msgId, replyToPulse))
	{
		std::cerr << "p3Wire::createReplyPulse() fetchPulse FAILED";
		std::cerr << std::endl;
		return false;
	}

	// create Reply Msg.
	RsWirePulse responsePulse;

	responsePulse.mMeta.mGroupId  = replyWithGroup.mMeta.mGroupId;
	responsePulse.mMeta.mAuthorId = replyWithGroup.mMeta.mAuthorId;
	responsePulse.mMeta.mThreadId.clear();
	responsePulse.mMeta.mParentId.clear();
	responsePulse.mMeta.mOrigMsgId.clear();

	responsePulse.mPulseType = WIRE_PULSE_TYPE_RESPONSE | reply_type;
	responsePulse.mSentiment = pPulse->mSentiment;
	responsePulse.mPulseText = pPulse->mPulseText;
	responsePulse.mImage1 = pPulse->mImage1;
	responsePulse.mImage2 = pPulse->mImage2;
	responsePulse.mImage3 = pPulse->mImage3;
	responsePulse.mImage4 = pPulse->mImage4;

	// mRefs refer to parent post.
	responsePulse.mRefGroupId   = replyToPulse->mMeta.mGroupId;
	responsePulse.mRefGroupName = replyToGroup.mMeta.mGroupName;
	responsePulse.mRefOrigMsgId = replyToPulse->mMeta.mOrigMsgId;
	responsePulse.mRefAuthorId  = replyToPulse->mMeta.mAuthorId;
	responsePulse.mRefPublishTs = replyToPulse->mMeta.mPublishTs;
	responsePulse.mRefPulseText = replyToPulse->mPulseText;
	responsePulse.mRefImageCount = replyToPulse->ImageCount();

	std::cerr << "p3Wire::createReplyPulse() create Response Pulse";
	std::cerr << std::endl;

	uint32_t token;
	if (!createPulse(token, responsePulse))
	{
		std::cerr << "p3Wire::createReplyPulse() FAILED to create Response Pulse";
		std::cerr << std::endl;
		return false;
	}

	int result = waitToken(token);
	if (result != RsTokenService::COMPLETE)
	{
		std::cerr << "p3Wire::createReplyPulse() FAILED(2) to create Response Pulse";
		std::cerr << std::endl;
		return false;
	}

	// get MsgId.
	std::pair<RsGxsGroupId, RsGxsMessageId> responsePair;
	if (!acknowledgeMsg(token, responsePair))
	{
		std::cerr << "p3Wire::createReplyPulse() FAILED acknowledgeMsg for Response Pulse";
		std::cerr << std::endl;
		return false;
	}

	std::cerr << "p3Wire::createReplyPulse() Response Pulse ID: (";
	std::cerr << responsePair.first.toStdString() << ", ";
	std::cerr << responsePair.second.toStdString() << ")";
	std::cerr << std::endl;

	// retrieve newly generated message.
	// **********************************************************
	RsWirePulseSPtr createdResponsePulse;
	if (!fetchPulse(responsePair.first, responsePair.second, createdResponsePulse))
	{
		std::cerr << "p3Wire::createReplyPulse() fetch createdReponsePulse FAILED";
		std::cerr << std::endl;
		return false;
	}

	/* Check that pulses is created properly */
	if ((createdResponsePulse->mMeta.mGroupId != responsePulse.mMeta.mGroupId) ||
	    (createdResponsePulse->mPulseText != responsePulse.mPulseText) ||
	    (createdResponsePulse->mRefGroupId != responsePulse.mRefGroupId) ||
	    (createdResponsePulse->mRefOrigMsgId != responsePulse.mRefOrigMsgId))
	{
		std::cerr << "p3Wire::createReplyPulse() fetch createdReponsePulse FAILED";
		std::cerr << std::endl;
		return false;
	}

	// create ReplyTo Ref Msg.
	std::cerr << "PulseAddDialog::postRefPulse() create Reference!";
	std::cerr << std::endl;

	// Reference Pulse. posted on Parent's Group.
	RsWirePulse refPulse;

	refPulse.mMeta.mGroupId  = replyToPulse->mMeta.mGroupId;
	refPulse.mMeta.mAuthorId = replyWithGroup.mMeta.mAuthorId; // own author Id.
	refPulse.mMeta.mThreadId = replyToPulse->mMeta.mOrigMsgId;
	refPulse.mMeta.mParentId = replyToPulse->mMeta.mOrigMsgId;
	refPulse.mMeta.mOrigMsgId.clear();

	refPulse.mPulseType = WIRE_PULSE_TYPE_REFERENCE | reply_type;
	refPulse.mSentiment = 0; // should this be =? createdResponsePulse->mSentiment;

	// Dont put parent PulseText into refPulse - it is available on Thread Msg.
	// otherwise gives impression it is correctly setup Parent / Reply...
	// when in fact the parent PublishTS, and AuthorId are wrong.
	refPulse.mPulseText = "";

	// refs refer back to own Post.
	refPulse.mRefGroupId   = replyWithGroup.mMeta.mGroupId;
	refPulse.mRefGroupName = replyWithGroup.mMeta.mGroupName;
	refPulse.mRefOrigMsgId = createdResponsePulse->mMeta.mOrigMsgId;
	refPulse.mRefAuthorId  = replyWithGroup.mMeta.mAuthorId;
	refPulse.mRefPublishTs = createdResponsePulse->mMeta.mPublishTs;
	refPulse.mRefPulseText = createdResponsePulse->mPulseText;
	refPulse.mRefImageCount = createdResponsePulse->ImageCount();

	// publish Ref Msg.
	if (!createPulse(token, refPulse))
	{
		std::cerr << "p3Wire::createReplyPulse() FAILED to create Ref Pulse";
		std::cerr << std::endl;
		return false;
	}

	result = waitToken(token);
	if (result != RsTokenService::COMPLETE)
	{
		std::cerr << "p3Wire::createReplyPulse() FAILED(2) to create Ref Pulse";
		std::cerr << std::endl;
		return false;
	}

	// get MsgId.
	std::pair<RsGxsGroupId, RsGxsMessageId> refPair;
	if (!acknowledgeMsg(token, refPair))
	{
		std::cerr << "p3Wire::createReplyPulse() FAILED acknowledgeMsg for Ref Pulse";
		std::cerr << std::endl;
		return false;
	}

	std::cerr << "p3Wire::createReplyPulse() Success: Ref Pulse ID: (";
	std::cerr << refPair.first.toStdString() << ", ";
	std::cerr << refPair.second.toStdString() << ")";
	std::cerr << std::endl;

	return true;
}


	// Blocking, request structures for display.
#if 0
bool p3Wire::createReplyPulse(uint32_t &token, RsWirePulse &pulse)
{

	return true;
}

bool p3Wire::createRepublishPulse(uint32_t &token, RsWirePulse &pulse)
{

	return true;
}

bool p3Wire::createLikePulse(uint32_t &token, RsWirePulse &pulse)
{

	return true;
}
#endif

	// WireGroup Details.
bool p3Wire::getWireGroup(const RsGxsGroupId &groupId, RsWireGroupSPtr &grp)
{
	std::set<RsGxsGroupId> groupIds = { groupId };
	std::map<RsGxsGroupId, RsWireGroupSPtr> groups;
	if (!fetchGroupPtrs(groupIds, groups))
	{
		std::cerr << "p3Wire::getWireGroup() failed to fetchGroupPtrs";
		std::cerr << std::endl;
		return false;
	}

	if (groups.size() != 1)
	{
		std::cerr << "p3Wire::getWireGroup() invalid group size";
		std::cerr << std::endl;
		return false;
	}

	grp = groups.begin()->second;

	// TODO Should fill in Counters of pulses/likes/republishes/replies
	return true;
}

// TODO Remove duplicate ...
bool p3Wire::getWirePulse(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId, RsWirePulseSPtr &pPulse)
{
	return fetchPulse(groupId, msgId, pPulse);
}




bool compare_time(const RsWirePulseSPtr& first, const RsWirePulseSPtr &second)
{
	return first->mMeta.mPublishTs > second->mMeta.mPublishTs;
}

	// should this filter them in some way?
	// date, or count would be more likely.
bool p3Wire::getPulsesForGroups(const std::list<RsGxsGroupId> &groupIds, std::list<RsWirePulseSPtr> &pulsePtrs)
{
	// request all the pulses (Top-Level Thread Msgs).
	std::cerr << "p3Wire::getPulsesForGroups()";
	std::cerr << std::endl;

	uint32_t token;
	{
		RsTokReqOptions opts;
		opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
		opts.mOptions = RS_TOKREQOPT_MSG_LATEST | RS_TOKREQOPT_MSG_THREAD;

		getTokenService()->requestMsgInfo(
			token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds);
	}

	// wait for pulse request to completed.
	std::cerr << "p3Wire::getPulsesForGroups() waiting for token";
	std::cerr << std::endl;

	int result = waitToken(token);
	if (result != RsTokenService::COMPLETE)
	{
		std::cerr << "p3Wire::getPulsesForGroups() token FAILED, result: " << result;
		std::cerr << std::endl;
		return false;
	}

	// retrieve Pulses.
	std::cerr << "p3Wire::getPulsesForGroups() retrieving token";
	std::cerr << std::endl;
	if (!getPulsePtrData(token, pulsePtrs))
	{
		std::cerr << "p3Wire::getPulsesForGroups() tokenPulse ERROR";
		std::cerr << std::endl;
		return false;
	}

	std::cerr << "p3Wire::getPulsesForGroups() size = " << pulsePtrs.size();
	std::cerr << " sorting and trimming";
	std::cerr << std::endl;

	// sort and filter list.
	pulsePtrs.sort(compare_time);

	// trim to N max.
	uint32_t N = 10;
	if (pulsePtrs.size() > N) {
		pulsePtrs.resize(N);
	}

	// set to collect groupIds...
	// this is only important if updatePulse Level > 1.
	// but this is more general.
	std::set<RsGxsGroupId> allGroupIds;

	// for each fill in details.
	std::list<RsWirePulseSPtr>::iterator it;
	for (it = pulsePtrs.begin(); it != pulsePtrs.end(); it++)
	{
		if (!updatePulse(*it, 1))
		{
			std::cerr << "p3Wire::getPulsesForGroups() Failed to updatePulse";
			std::cerr << std::endl;
			return false;
		}

		if (!extractGroupIds(*it, allGroupIds))
		{
			std::cerr << "p3Wire::getPulsesForGroups() failed to extractGroupIds";
			std::cerr << std::endl;
			return false;
		}
	}

	// fetch GroupPtrs for allGroupIds.
	std::map<RsGxsGroupId, RsWireGroupSPtr> groups;
	if (!fetchGroupPtrs(allGroupIds, groups))
	{
		std::cerr << "p3Wire::getPulsesForGroups() failed to fetchGroupPtrs";
		std::cerr << std::endl;
		return false;
	}

	// update GroupPtrs for all pulsePtrs.
	for (it = pulsePtrs.begin(); it != pulsePtrs.end(); it++)
	{
		if (!updateGroupPtrs(*it, groups))
		{
			std::cerr << "p3Wire::getPulsesForGroups() failed to updateGroupPtrs";
			std::cerr << std::endl;
			return false;
		}
	}

	return true;
}


bool p3Wire::getPulseFocus(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId, int /* type */, RsWirePulseSPtr &pPulse)
{
	std::cerr << "p3Wire::getPulseFocus(";
	std::cerr << "grpId: " << groupId << " msgId: " << msgId;
	std::cerr << " )";
	std::cerr << std::endl;

	if (!fetchPulse(groupId, msgId, pPulse))
	{
		std::cerr << "p3Wire::getPulseFocus() failed to fetch Pulse";
		std::cerr << std::endl;
		return false;
	}

	if (!updatePulse(pPulse, 3))
	{
		std::cerr << "p3Wire::getPulseFocus() failed to update Pulse";
		std::cerr << std::endl;
		return false;
	}

	/* final stage is to fetch associated groups and reference them from pulses
	 * this could be done as part of updates, but probably more efficient to do once
	 * -- Future improvement.
	 *	-- Fetch RefGroups as well, these are not necessarily available,
	 *	   so need to add dataRequest FlAG to return okay even if not all groups there.
	 */

	std::set<RsGxsGroupId> groupIds;
	if (!extractGroupIds(pPulse, groupIds))
	{
		std::cerr << "p3Wire::getPulseFocus() failed to extractGroupIds";
		std::cerr << std::endl;
		return false;
	}

	std::map<RsGxsGroupId, RsWireGroupSPtr> groups;
	if (!fetchGroupPtrs(groupIds, groups))
	{
		std::cerr << "p3Wire::getPulseFocus() failed to fetchGroupPtrs";
		std::cerr << std::endl;
		return false;
	}

	if (!updateGroupPtrs(pPulse, groups))
	{
		std::cerr << "p3Wire::getPulseFocus() failed to updateGroupPtrs";
		std::cerr << std::endl;
		return false;
	}

	return true;
}


// function to update a pulse with the (Ref) child with actual data.
bool p3Wire::updatePulse(RsWirePulseSPtr pPulse, int levels)
{
	bool okay = true;

	// setup logging label.
	std::ostringstream out;
	out << "pulse[" << (void *) pPulse.get() << "], " << levels;
	std::string label = out.str();

	std::cerr << "p3Wire::updatePulse(" << label << ") starting";
	std::cerr << std::endl;

	// is pPulse is a REF, then request the original.
	// if no original available the done.
	if (pPulse->mPulseType & WIRE_PULSE_TYPE_REFERENCE)
	{
		RsWirePulseSPtr fullPulse;
		std::cerr << "p3Wire::updatePulse(" << label << ") fetching REF (";
		std::cerr << "grpId: " << pPulse->mRefGroupId << " msgId: " << pPulse->mRefOrigMsgId;
		std::cerr << " )";
		std::cerr << std::endl;
		if (!fetchPulse(pPulse->mRefGroupId, pPulse->mRefOrigMsgId, fullPulse))
		{
			std::cerr << "p3Wire::updatePulse(" << label << ") failed to fetch REF";
			std::cerr << std::endl;
			return false;
		}
		std::cerr << "p3Wire::updatePulse(" << label << ") replacing REF";
		std::cerr << std::endl;

		*pPulse = *fullPulse;
	}

	// Request children: (Likes / Retweets / Replies)
	std::cerr << "p3Wire::updatePulse(" << label << ") requesting children";
	std::cerr << std::endl;

	uint32_t token;
	{
		RsTokReqOptions opts;
		opts.mReqType = GXS_REQUEST_TYPE_MSG_RELATED_DATA;
		// OR opts.mOptions = RS_TOKREQOPT_MSG_LATEST | RS_TOKREQOPT_MSG_PARENT;
		opts.mOptions = RS_TOKREQOPT_MSG_LATEST | RS_TOKREQOPT_MSG_THREAD;

		std::vector<RsGxsGrpMsgIdPair> msgIds = { 
			std::make_pair(pPulse->mMeta.mGroupId, pPulse->mMeta.mOrigMsgId)
		};

		getTokenService()->requestMsgRelatedInfo(
			token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds);
	}

	// wait for request to complete
	std::cerr << "p3Wire::updatePulse(" << label << ") waiting for token";
	std::cerr << std::endl;

	int result = waitToken(token);
	if (result != RsTokenService::COMPLETE)
	{
		std::cerr << "p3Wire::updatePulse(" << label << ") token FAILED, result: " << result;
		std::cerr << std::endl;
		return false;
	}

	/* load children */
	okay = updatePulseChildren(pPulse, token);
	if (!okay)
	{
		std::cerr << "p3Wire::updatePulse(" << label << ") FAILED to update Children";
		std::cerr << std::endl;
		return false;
	}

	/* if down to last level, no need to updateChildren */
	if (levels <= 1)
	{
		std::cerr << "p3Wire::updatePulse(" << label << ") Level <= 1 finished";
		std::cerr << std::endl;
		return okay;
	}

	/* recursively update children */
	std::cerr << "p3Wire::updatePulse(" << label << ") updating children recursively";
	std::cerr << std::endl;
	std::list<RsWirePulseSPtr>::iterator it;
	for (it = pPulse->mReplies.begin(); it != pPulse->mReplies.end(); it++)
	{
		bool childOkay = updatePulse(*it, levels - 1);
		if (!childOkay) {
			std::cerr << "p3Wire::updatePulse(" << label << ") update children (reply) failed";
			std::cerr << std::endl;
		}
	}

	for (it = pPulse->mRepublishes.begin(); it != pPulse->mRepublishes.end(); it++)
	{
		bool childOkay = updatePulse(*it, levels - 1);
		if (!childOkay) {
			std::cerr << "p3Wire::updatePulse(" << label << ") update children (repub) failed";
			std::cerr << std::endl;
		}
	}

	return okay;
}


// function to update the (Ref) child with actual data.
bool p3Wire::updatePulseChildren(RsWirePulseSPtr pParent,  uint32_t token)
{
	{
		bool okay = true;
		std::vector<RsWirePulse> pulses;
		if (getRelatedPulseData(token, pulses)) {
			std::vector<RsWirePulse>::iterator it;
			for (it = pulses.begin(); it != pulses.end(); it++)
			{
				std::cerr << "p3Wire::updatePulseChildren() retrieved child: " << *it;
				std::cerr << std::endl;

				RsWirePulseSPtr pPulse = std::make_shared<RsWirePulse>(*it);
				// switch on type.
				if (it->mPulseType & WIRE_PULSE_TYPE_LIKE) {
					pParent->mLikes.push_back(pPulse);
					std::cerr << "p3Wire::updatePulseChildren() adding Like";
					std::cerr << std::endl;
				}
				else if (it->mPulseType & WIRE_PULSE_TYPE_REPUBLISH) {
					pParent->mRepublishes.push_back(pPulse);
					std::cerr << "p3Wire::updatePulseChildren() adding Republish";
					std::cerr << std::endl;
				}
				else if (it->mPulseType & WIRE_PULSE_TYPE_REPLY) {
					pParent->mReplies.push_back(pPulse);
					std::cerr << "p3Wire::updatePulseChildren() adding Reply";
					std::cerr << std::endl;
				}
				else {
					std::cerr << "p3Wire::updatePulseChildren() unknown child type: " << it->mPulseType;
					std::cerr << std::endl;
				}
			}
		} else {
			std::cerr << "p3Wire::updatePulseChildren() ERROR failed to retrieve token";
			std::cerr << std::endl;
			okay = false;
		}

		if (!okay) {
			std::cerr << "p3Wire::updatePulseChildren() token ERROR";
			std::cerr << std::endl;
		}
		return okay;
	}
}


// this function doesn't depend on p3Wire, could make static.
bool p3Wire::extractGroupIds(RsWirePulseConstSPtr pPulse, std::set<RsGxsGroupId> &groupIds)
{
	std::cerr << "p3Wire::extractGroupIds()";
	std::cerr << std::endl;

	if (!pPulse) {
		std::cerr << "p3Wire::extractGroupIds() INVALID pPulse";
		std::cerr << std::endl;
		return false;
	}

	/* do this recursively */
	if (pPulse->mPulseType & WIRE_PULSE_TYPE_REFERENCE) {
		/* skipping */
		std::cerr << "p3Wire::extractGroupIds() skipping ref type";
		std::cerr << std::endl;
		return true;
	}

	// install own groupId.
	groupIds.insert(pPulse->mMeta.mGroupId);

	/* iterate through children, recursively */
	std::list<RsWirePulseSPtr>::const_iterator it;
	for (it = pPulse->mReplies.begin(); it != pPulse->mReplies.end(); it++)
	{
		bool childOkay = extractGroupIds(*it, groupIds);
		if (!childOkay) {
			std::cerr << "p3Wire::extractGroupIds() update children (reply) failed";
			std::cerr << std::endl;
			return false;
		}
	}

	for (it = pPulse->mRepublishes.begin(); it != pPulse->mRepublishes.end(); it++)
	{
		bool childOkay = extractGroupIds(*it, groupIds);
		if (!childOkay) {
			std::cerr << "p3Wire::extractGroupIds() update children (repub) failed";
			std::cerr << std::endl;
			return false;
		}
	}

	// not bothering with LIKEs at the moment. TODO.

	return true;
}

bool p3Wire::updateGroupPtrs(RsWirePulseSPtr pPulse, const std::map<RsGxsGroupId, RsWireGroupSPtr> &groups)
{
	std::map<RsGxsGroupId, RsWireGroupSPtr>::const_iterator git;
	git = groups.find(pPulse->mMeta.mGroupId);
	if (git == groups.end()) {
		// error
		return false;
	}

	pPulse->mGroupPtr = git->second;

	/* if Refs, GroupId refers to parent, so GroupPtr is parent's group
	 * It should already be in groups lists - if its not... */
	if (pPulse->mPulseType & WIRE_PULSE_TYPE_REFERENCE) {
		// if REF is in list, fill in (unlikely but try anyway)
		// unlikely, as we are not adding RefGroupId, as can potentially fail to look up.
		// need additional flag OKAY_IF_NONEXISTENT or similar.
		// no error if its not there.
		std::map<RsGxsGroupId, RsWireGroupSPtr>::const_iterator rgit;
		rgit = groups.find(pPulse->mRefGroupId);
		if (rgit != groups.end()) {
			pPulse->mRefGroupPtr = rgit->second;
		}

		// no children for REF pulse, so can return now.
		return true;
	}

	/* recursively apply to children */
	std::list<RsWirePulseSPtr>::iterator it;
	for (it = pPulse->mReplies.begin(); it != pPulse->mReplies.end(); it++)
	{
		bool childOkay = updateGroupPtrs(*it, groups);
		if (!childOkay) {
			std::cerr << "p3Wire::updateGroupPtrs() update children (reply) failed";
			std::cerr << std::endl;
			return false;
		}
	}

	for (it = pPulse->mRepublishes.begin(); it != pPulse->mRepublishes.end(); it++)
	{
		bool childOkay = updateGroupPtrs(*it, groups);
		if (!childOkay) {
			std::cerr << "p3Wire::updateGroupPtrs() update children (repub) failed";
			std::cerr << std::endl;
			return false;
		}
	}

	// not bothering with LIKEs at the moment. TODO.
	return true;
}

bool p3Wire::fetchGroupPtrs(const std::set<RsGxsGroupId> &groupIds,
	std::map<RsGxsGroupId, RsWireGroupSPtr> &groups)
{
	std::cerr << "p3Wire::fetchGroupPtrs()";
	std::cerr << std::endl;
	std::list<RsGxsGroupId> groupIdList(groupIds.begin(), groupIds.end());

	uint32_t token;
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	if (!requestGroupInfo(token, opts, groupIdList) || waitToken(token) != RsTokenService::COMPLETE )
	{
		std::cerr << "p3Wire::fetchGroupPtrs() failed to fetch groups";
		std::cerr << std::endl;
		return false;
	}
	return getGroupPtrData(token, groups);
}


