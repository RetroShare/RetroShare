/*******************************************************************************
 * unittests/libretroshare/services/gxs/GxsPeerNode.cc                         *
 *                                                                             *
 * Copyright 2014      by Robert Fernie    <retroshare.project@gmail.com>      *
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


// from libretroshare
//#include "serialiser/rsstatusitems.h"
#include "services/p3idservice.h"
#include "services/p3gxscircles.h"
#include "gxs/rsgixs.h"
#include "gxs/rsdataservice.h"
#include "gxs/rsgxsnetservice.h"
#include "util/rsdir.h"

// local
#include "GxsPeerNode.h"
#include "FakePgpAuxUtils.h"
#include "gxstestservice.h"


#ifdef USER_NETSERVICE_WRAPPER
	#include "RsGxsNetServiceTester.h"
#endif

#include <unistd.h>


GxsPeerNode::GxsPeerNode(const RsPeerId &ownId, const std::list<RsPeerId> &friends, int testMode, bool useIdentityService)
	:PeerNode(ownId, friends, false),
	mUseIdentityService(useIdentityService),
	mTestMode(testMode),
	mGxsDir("./gxs_unittest/" + ownId.toStdString() + "/"),
	mGxsIdService(NULL),
	mGxsCircles(NULL),
	mTestService(NULL), 
	mTestDs(NULL), 
	mTestNs(NULL)
{ 
	// extract bits we need.
	//p3PeerMgr *peerMgr = getPeerMgr();
	//p3LinkMgr *linkMgr = getLinkMgr();
	//p3NetMgr  *netMgr = getNetMgr();
	RsNxsNetMgr *nxsMgr = getNxsNetMgr();
	//p3ServiceControl *serviceCtrl = getServiceControl();

	// Create Service for Testing.
	// Specific Testing service here.
	RsDirUtil::checkCreateDirectory(mGxsDir);

	std::set<std::string> filesToKeep;
	RsDirUtil::cleanupDirectory(mGxsDir, filesToKeep);

	std::string gxs_passwd = "testpassword";

        mPgpAuxUtils = new FakePgpAuxUtils(ownId);
        mPgpAuxUtils->addPeerListToPgpList(friends);

	if (mUseIdentityService)
	{
		/**** Identity service ****/
		mGxsIdDs = new RsDataService(mGxsDir, "gxsid_db",
				RS_SERVICE_GXS_TYPE_GXSID, NULL, gxs_passwd);
		mGxsIdService = new p3IdService(mGxsIdDs, NULL, mPgpAuxUtils);
	
		// circles created here, as needed by Ids.
		mGxsCirclesDs = new RsDataService(mGxsDir, "gxscircles_db",
				RS_SERVICE_GXS_TYPE_GXSCIRCLE, NULL, gxs_passwd);
		mGxsCircles = new p3GxsCircles(mGxsCirclesDs, NULL, mGxsIdService, mPgpAuxUtils);
	
		// create GXS ID service
		mGxsIdNs = new RsGxsNetService(
				RS_SERVICE_GXS_TYPE_GXSID, mGxsIdDs, nxsMgr,
				mGxsIdService, mGxsIdService->getServiceInfo(),
				NULL, mGxsCircles,mGxsIdService,
				mPgpAuxUtils,
				false); // don't synchronise group automatic (need explicit group request)

		mGxsIdService->setNes(mGxsIdNs);
		/**** GxsCircle service ****/
	
#ifdef USER_NETSERVICE_WRAPPER
		mGxsCirclesNs = new RsGxsNetServiceTester
#else
		mGxsCirclesNs = new RsGxsNetService
#endif
				(RS_SERVICE_GXS_TYPE_GXSCIRCLE, mGxsCirclesDs, nxsMgr,
				mGxsCircles, mGxsCircles->getServiceInfo(),
				NULL, mGxsCircles,NULL,
				mPgpAuxUtils);
	}
	else
	{
		mGxsIdDs = NULL;
		mGxsIdService = NULL;
		mGxsCirclesDs = NULL;
		mGxsCircles = NULL;
		mGxsIdNs = NULL;
		mGxsCirclesNs = NULL;
	}

	mTestDs = new RsDataService(mGxsDir, "test_db",
			RS_SERVICE_GXS_TYPE_TEST,
			NULL, "testPasswd");

	mTestService = new GxsTestService(mTestDs, NULL, mGxsIdService, testMode);

#ifdef USER_NETSERVICE_WRAPPER
	mTestNs = new RsGxsNetServiceTester
#else
	mTestNs = new RsGxsNetService
#endif
			(RS_SERVICE_GXS_TYPE_TEST, mTestDs, nxsMgr,
			mTestService, mTestService->getServiceInfo(),
			NULL, mGxsCircles,mGxsIdService,
			mPgpAuxUtils);

	if (mUseIdentityService)
	{
		AddService(mGxsIdNs);
		AddService(mGxsCirclesNs);
	}
	AddService(mTestNs);

	//mConfigMgr->addConfiguration("testservice.cfg", mTestService);

	if (mUseIdentityService)
	{
		//mConfigMgr->addConfiguration("gxsid.cfg", mGxsIdNs);
		//mConfigMgr->addConfiguration("gxscircles.cfg", mGxsCircleNs);

        mGxsIdService->start();
        mGxsIdNs->start();
        mGxsCircles->start();
        mGxsCirclesNs->start();
	}

    mTestService->start();
    mTestNs->start();

	//node->AddPqiServiceMonitor(status);
}


GxsPeerNode::~GxsPeerNode()
{
	mTestService->join();
	mTestNs->join();

	if (mUseIdentityService)
	{
		mGxsIdService->join();
		mGxsIdNs->join();
		mGxsCircles->join();
		mGxsCirclesNs->join();

		delete mGxsIdNs;
		delete mGxsIdService;

		delete mGxsCirclesNs;
		delete mGxsCircles;
	}

	delete mTestNs;
	delete mTestService;
	// this is deleted somewhere else?
	//delete mTestDs;

	std::set<std::string> filesToKeep;
	RsDirUtil::cleanupDirectory(mGxsDir, filesToKeep);
}
	
bool GxsPeerNode::checkTestServiceAllowedGroups(const RsPeerId &/*peerId*/)
{
#ifdef USER_NETSERVICE_WRAPPER
	std::vector<RsGxsGroupId> groups;
	return ((RsGxsNetServiceTester *) mTestNs)->fetchAllowedGroups(peerId, groups);
#else
	std::cerr << "GxsPeerNode::checkTestServiceAllowedGroups() not available";
	std::cerr << std::endl;
	return true;
#endif
}

	
bool GxsPeerNode::checkCircleServiceAllowedGroups(const RsPeerId &/*peerId*/)
{
#ifdef USER_NETSERVICE_WRAPPER
	std::vector<RsGxsGroupId> groups;
	return ((RsGxsNetServiceTester *) mGxsCirclesNs)->fetchAllowedGroups(peerId, groups);
#else
	std::cerr << "GxsPeerNode::checkCircleServiceAllowedGroups() not available";
	std::cerr << std::endl;
	return true;
#endif
}

bool GxsPeerNode::createIdentity(const std::string &name, 
		bool pgpLinked,
		uint32_t circleType, 
		const RsGxsCircleId &circleId, 
		RsGxsId &gxsId)
{
	/* create a couple of groups */
	RsTokenService *tokenService = mGxsIdService->RsGenExchange::getTokenService();


        RsGxsIdGroup id;
	id.mMeta.mGroupName = name;
        if (pgpLinked)
        {
                id.mMeta.mGroupFlags = RSGXSID_GROUPFLAG_REALID_kept_for_compatibility;
        }
        else
        {
                id.mMeta.mGroupFlags = 0;
        }
	id.mMeta.mCircleType = circleType;

	switch(circleType)
	{
                case GXS_CIRCLE_TYPE_YOUR_FRIENDS_ONLY:
			id.mMeta.mInternalCircle = circleId;
                        break;
                case GXS_CIRCLE_TYPE_LOCAL:
			// no CircleId,
                        break;
                case GXS_CIRCLE_TYPE_PUBLIC:
			// no CircleId,
                        break;
                case GXS_CIRCLE_TYPE_EXTERNAL:
			id.mMeta.mCircleId = circleId;
			break;
		default:
		case GXS_CIRCLE_TYPE_EXT_SELF:
			std::cerr << "GxsPeerNode::createIdentity() Invalid circleType";
			std::cerr << std::endl;
			return false;
			break;
	}

	uint32_t token;
	if (!mGxsIdService->createGroup(token, id))
	{
		std::cerr << "GxsPeerNode::createIdentity() failed";
		std::cerr << std::endl;
		return false;
	}

	while(tokenService->requestStatus(token) != RsTokenService::COMPLETE)
	{
		tick();
		sleep(1);
	}

	RsGxsGroupId groupId;
        if (mGxsIdService->acknowledgeTokenGrp(token, groupId))
	{
		RsGxsId tmpGxsId(groupId.toStdString());
		gxsId = tmpGxsId;
		return true;
	}
	return false;
		
}


bool GxsPeerNode::createCircle(const std::string &name, 
		uint32_t circleType, 
		const RsGxsCircleId &circleId, 
		const RsGxsId &authorId,
        std::set<RsPgpId> localMembers,
        std::set<RsGxsId> externalMembers,
		RsGxsGroupId &groupId)
{
	/* create a couple of groups */
	RsTokenService *tokenService = mGxsCircles->RsGenExchange::getTokenService();

	RsGxsCircleGroup grp1;
	grp1.mMeta.mGroupName = name;
	grp1.mMeta.mAuthorId = authorId;
	grp1.mMeta.mCircleType = circleType;

	switch(circleType)
	{
                case GXS_CIRCLE_TYPE_LOCAL:
			// no CircleId,
			// THIS is for LOCAL Storage.... 
			grp1.mLocalFriends = localMembers;
                        break;
                case GXS_CIRCLE_TYPE_YOUR_FRIENDS_ONLY:
			// Circle shouldn't use this.
			// but could potentially.
			grp1.mMeta.mInternalCircle = circleId;
			grp1.mInvitedMembers = externalMembers;
                        break;
                case GXS_CIRCLE_TYPE_PUBLIC:
			// no CircleId,
			grp1.mInvitedMembers = externalMembers;
                        break;
                case GXS_CIRCLE_TYPE_EXTERNAL:
			grp1.mMeta.mCircleId = circleId;
			grp1.mInvitedMembers = externalMembers;
			break;
		case GXS_CIRCLE_TYPE_EXT_SELF:
			// no CircleId.
			grp1.mInvitedMembers = externalMembers;
			break;
		default:
			std::cerr << "GxsPeerNode::createCircle() Invalid circleType";
			std::cerr << std::endl;
			return false;
			break;
	}

	std::cerr << "GxsPeerNode::createCircle()";
        std::cerr << " CircleType: " << (uint32_t) grp1.mMeta.mCircleType;
        std::cerr << " CircleId: " << grp1.mMeta.mCircleId.toStdString();
        std::cerr << std::endl;

	uint32_t token;
	mGxsCircles->createGroup(token, grp1) ;

	while(tokenService->requestStatus(token) != RsTokenService::COMPLETE)
	{
		tick();
		sleep(1);
	}
        return mGxsCircles->acknowledgeTokenGrp(token, groupId);
}



bool GxsPeerNode::createGroup(const std::string &name, 
		uint32_t circleType, 
		const RsGxsCircleId &circleId, 
		const RsGxsId &authorId,
		RsGxsGroupId &groupId)
{
	/* create a couple of groups */
	RsTokenService *tokenService = mTestService->RsGenExchange::getTokenService();

	RsTestGroup grp1;
	grp1.mMeta.mGroupName = name;
	grp1.mMeta.mAuthorId = authorId;
	grp1.mMeta.mCircleType = circleType;
	grp1.mTestString = "testString";

	switch(circleType)
	{
                case GXS_CIRCLE_TYPE_YOUR_FRIENDS_ONLY:
			grp1.mMeta.mInternalCircle = circleId;
                        break;
                case GXS_CIRCLE_TYPE_LOCAL:
			// no CircleId,
                        break;
                case GXS_CIRCLE_TYPE_PUBLIC:
			// no CircleId,
                        break;
                case GXS_CIRCLE_TYPE_EXTERNAL:
			grp1.mMeta.mCircleId = circleId;
			break;
		default:
		case GXS_CIRCLE_TYPE_EXT_SELF:
			std::cerr << "GxsPeerNode::createGroup() Invalid circleType";
			std::cerr << std::endl;
			return false;
			break;
	}

	uint32_t token;
	if (!mTestService->submitTestGroup(token, grp1))
	{
		std::cerr << "GxsPeerNode::createGroup() failed";
		std::cerr << std::endl;
		return false;
	}

	while(tokenService->requestStatus(token) != RsTokenService::COMPLETE)
	{
		tick();
		sleep(1);
	}
        return mTestService->acknowledgeTokenGrp(token, groupId);
}


bool GxsPeerNode::createMsg(const std::string &msgstr, 
		const RsGxsGroupId &groupId, 
		const RsGxsId &authorId,
		RsGxsMessageId &msgId)
{
	/* create a couple of groups */
	RsTokenService *tokenService = mTestService->RsGenExchange::getTokenService();

	RsTestMsg msg;
	msg.mMeta.mGroupId = groupId;
	msg.mMeta.mAuthorId = authorId;
	msg.mTestString = msgstr;


	uint32_t token;
	if (!mTestService->submitTestMsg(token, msg))
	{
		std::cerr << "GxsPeerNode::createMsg() failed";
		std::cerr << std::endl;
		return false;
	}

	while(tokenService->requestStatus(token) != RsTokenService::COMPLETE)
	{
		tick();
		sleep(1);
	}
	RsGxsGrpMsgIdPair pairId;
	bool retval = mTestService->acknowledgeTokenMsg(token, pairId);
	if (retval)
	{
		msgId = pairId.second;
	}
	return retval;
}


bool GxsPeerNode::subscribeToGroup(const RsGxsGroupId &groupId, bool subscribe)
{
	/* create a couple of groups */
	RsTokenService *tokenService = mTestService->RsGenExchange::getTokenService();

	uint32_t token;
	if (!mTestService->RsGenExchange::subscribeToGroup(token, groupId, subscribe))
	{
		std::cerr << "GxsPeerNode::subscribeToGroup() failed";
		std::cerr << std::endl;
		return false;
	}

	while(tokenService->requestStatus(token) != RsTokenService::COMPLETE)
	{
		tick();
		sleep(1);
	}
	RsGxsGroupId ackGroupId;
        return mTestService->acknowledgeTokenGrp(token, ackGroupId);
}



bool GxsPeerNode::getGroups(std::vector<RsTestGroup> &groups)
{
        std::cerr << "GxsPeerNode::getGroups()";
	std::cerr << std::endl;

	RsTokenService *tokenService = mTestService->RsGenExchange::getTokenService();

        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

        uint32_t token;
	tokenService->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts);
	while(tokenService->requestStatus(token) != RsTokenService::COMPLETE)
	{
		tick();
		sleep(1);
	}
        return mTestService->getTestGroups(token, groups);
}



bool GxsPeerNode::getGroupList(std::list<RsGxsGroupId> &groups)
{
        std::cerr << "GxsPeerNode::getGroupList()";
	std::cerr << std::endl;

	RsTokenService *tokenService = mTestService->RsGenExchange::getTokenService();

        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_IDS;

        uint32_t token;
	tokenService->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_LIST, opts);
	while(tokenService->requestStatus(token) != RsTokenService::COMPLETE)
	{
		tick();
		sleep(1);
	}

    	return mTestService->RsGenExchange::getGroupList(token, groups);
}


bool GxsPeerNode::getMsgList(const RsGxsGroupId &id, std::list<RsGxsMessageId> &msgIds)
{
        std::cerr << "GxsPeerNode::getMsgList()";
	std::cerr << std::endl;

	RsTokenService *tokenService = mTestService->RsGenExchange::getTokenService();

        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_MSG_IDS;

        uint32_t token;
 	std::list<RsGxsGroupId> grpIds;
	grpIds.push_back(id);

	tokenService->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_LIST, opts, grpIds);
	while(tokenService->requestStatus(token) != RsTokenService::COMPLETE)
	{
		tick();
		sleep(1);
	}

	std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > msgResult;

	if (mTestService->RsGenExchange::getMsgList(token, msgResult))
	{
		std::vector<RsGxsMessageId>::iterator vit;
		for(vit = msgResult[id].begin(); vit != msgResult[id].end(); vit++)
		{
			msgIds.push_back(*vit);
		}
		return true;
	}
	return false;
}


bool GxsPeerNode::getIdentities(std::vector<RsGxsIdGroup> &groups)
{
        std::cerr << "GxsPeerNode::getIdentities()";
	std::cerr << std::endl;

	RsTokenService *tokenService = mGxsIdService->RsGenExchange::getTokenService();

        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

        uint32_t token;
	tokenService->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts);
	while(tokenService->requestStatus(token) != RsTokenService::COMPLETE)
	{
		tick();
		sleep(1);
	}
        return mGxsIdService->getGroupData(token, groups);
}

bool GxsPeerNode::getIdentitiesList(std::list<RsGxsGroupId> &groups)
{
        std::cerr << "GxsPeerNode::getIdentitiesList()";
	std::cerr << std::endl;

	RsTokenService *tokenService = mGxsIdService->RsGenExchange::getTokenService();

        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_IDS;

        uint32_t token;
	tokenService->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_LIST, opts);
	while(tokenService->requestStatus(token) != RsTokenService::COMPLETE)
	{
		tick();
		sleep(1);
	}

    	return mGxsIdService->RsGenExchange::getGroupList(token, groups);
}


bool GxsPeerNode::getCircles(std::vector<RsGxsCircleGroup> &groups)
{
        std::cerr << "GxsPeerNode::getCircles()";
	std::cerr << std::endl;

	RsTokenService *tokenService = mGxsCircles->RsGenExchange::getTokenService();

        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

        uint32_t token;
	tokenService->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts);
	while(tokenService->requestStatus(token) != RsTokenService::COMPLETE)
	{
		tick();
		sleep(1);
	}
        return mGxsCircles->getGroupData(token, groups);
}

bool GxsPeerNode::getCirclesList(std::list<RsGxsGroupId> &groups)
{
        std::cerr << "GxsPeerNode::getCirclesList()";
	std::cerr << std::endl;

	RsTokenService *tokenService = mGxsCircles->RsGenExchange::getTokenService();

        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_IDS;

        uint32_t token;
	tokenService->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_LIST, opts);
	while(tokenService->requestStatus(token) != RsTokenService::COMPLETE)
	{
		tick();
		sleep(1);
	}

    	return mGxsCircles->RsGenExchange::getGroupList(token, groups);
}








