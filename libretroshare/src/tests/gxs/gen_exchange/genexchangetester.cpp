#include "genexchangetester.h"
#include "support.h"
#include "gxs/rsdataservice.h"
#include "retroshare/rsgxsflags.h"


GenExchangeTest::GenExchangeTest(GenExchangeTestService* const mTestService, RsGeneralDataService* dataService, int pollingTO)
 : mDataService(dataService), mTestService(mTestService), mTokenService(mTestService->getTokenService()),
   mPollingTO(pollingTO)
{
}

GenExchangeTest::~GenExchangeTest()
{
}


void GenExchangeTest::pollForToken(uint32_t token, const RsTokReqOptions &opts, bool fill)
{
    double timeDelta = 0.2;
    rstime_t now = time(NULL);
    rstime_t stopw = now + mPollingTO;

    while(now < stopw)
    {
#ifndef WINDOWS_SYS
    usleep((int) (timeDelta * 1000000));
#else
    Sleep((int) (timeDelta * 1000));
#endif

        if((RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE == mTokenService->requestStatus(token)))
        {
            switch(opts.mReqType)
            {
            case GXS_REQUEST_TYPE_GROUP_DATA:
            	if(fill)
            		mTestService->getGroupDataTS(token, mGrpDataIn);
                break;
            case GXS_REQUEST_TYPE_GROUP_META:
            	if(fill)
            		mTestService->getGroupMetaTS(token, mGrpMetaDataIn);
                break;
            case GXS_REQUEST_TYPE_GROUP_IDS:
            	if(fill)
            		mTestService->getGroupListTS(token, mGrpIdsIn);
                break;
            case GXS_REQUEST_TYPE_MSG_DATA:
            	if(fill)
            		mTestService->getMsgDataTS(token, mMsgDataIn);
                break;
            case GXS_REQUEST_TYPE_MSG_META:
            	if(fill)
            		mTestService->getMsgMetaTS(token, mMsgMetaDataIn);
                break;
            case GXS_REQUEST_TYPE_MSG_IDS:
            	if(fill)
            		mTestService->getMsgListTS(token, mMsgIdsIn);
                break;
            case GXS_REQUEST_TYPE_MSG_RELATED_IDS:
            	if(fill)
            		mTestService->getMsgRelatedListTS(token, mMsgRelatedIdsIn);
                break;
            case GXS_REQUEST_TYPE_MSG_RELATED_DATA:
            	if(fill)
            		mTestService->getMsgRelatedDataTS(token, mMsgRelatedDataMapIn);
                break;
            }
            break;
        }
        else if(RsTokenService::GXS_REQUEST_V2_STATUS_FAILED == mTokenService->requestStatus(token))
        {
        	mTokenService->cancelRequest(token);
        	break;
        }
        now = time(NULL);
    }
}



bool GenExchangeTest::pollForMsgAcknowledgement(uint32_t token,
		RsGxsGrpMsgIdPair& msgId)
{
    double timeDelta = 0.2;

    rstime_t now = time(NULL);
	rstime_t stopw = now + mPollingTO;

    while(now < stopw)
    {
#ifndef WINDOWS_SYS
    usleep((int) (timeDelta * 1000000));
#else
    Sleep((int) (timeDelta * 1000));
#endif

		if((RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE == mTokenService->requestStatus(token)))
		{
			mTestService->acknowledgeTokenMsg(token, msgId);
			return true;
		}
		else if(RsTokenService::GXS_REQUEST_V2_STATUS_FAILED == mTokenService->requestStatus(token))
		{
			mTokenService->cancelRequest(token);
			return false;
		}
		now = time(NULL);
    }
    return false;
}

GenExchangeTestService* GenExchangeTest::getTestService()
{
	return mTestService;
}

RsTokenService*  GenExchangeTest::getTokenService()
{
	return mTokenService;
}
bool GenExchangeTest::pollForGrpAcknowledgement(uint32_t token,
		RsGxsGroupId& grpId)
{
    double timeDelta = 0.2;
    rstime_t now = time(NULL);
	rstime_t stopw = now + mPollingTO;
    while(now < stopw)
    {
#ifndef WINDOWS_SYS
    usleep((int) (timeDelta * 1000000));
#else
    Sleep((int) (timeDelta * 1000));
#endif

		if((RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE == mTokenService->requestStatus(token)))
		{
			mTestService->acknowledgeTokenGrp(token, grpId);
			return true;
		}
		else if(RsTokenService::GXS_REQUEST_V2_STATUS_FAILED == mTokenService->requestStatus(token))
		{
			mTokenService->cancelRequest(token);
			return false;
		}
		now = time(NULL);
    }
    return false;
}

void GenExchangeTest::setUp()
{
	mDataService->resetDataStore();

	// would be useful for genexchange services
	// to have a protected reset button
	mTestService->start();
}

void GenExchangeTest::breakDown()
{
    mDataService->resetDataStore();
    mTestService->join();
    clearAllData();
}

bool msgDataSort(const RsDummyMsg* m1, const RsDummyMsg* m2)
{
	return m1->meta.mMsgId < m2->meta.mMsgId;
}

bool GenExchangeTest::compareMsgDataMaps()
{
	DummyMsgMap::iterator mit = mMsgDataOut.begin();

	bool ok = true;
	for(; mit != mMsgDataOut.end(); mit++)
	{
		const RsGxsGroupId& grpId = mit->first;
		std::vector<RsDummyMsg*>& v1 = mit->second,
				&v2 = mMsgDataIn[grpId];

		if(v1.size() != v2.size())
			return false;

		std::sort(v1.begin(), v1.end(), msgDataSort);
		std::sort(v2.begin(), v2.end(), msgDataSort);

		ok &= Comparison<std::vector<RsDummyMsg*>, RsDummyMsg*>::comparison(v1, v2);
	}

	return ok;
}


bool GenExchangeTest::compareMsgIdMaps()
{
	GxsMsgIdResult::const_iterator mit = mMsgIdsOut.begin();
	bool ok = true;
	for(; mit != mMsgIdsOut.end(); mit++)
	{
		const RsGxsGroupId& grpId = mit->first;
		const std::vector<RsGxsMessageId>& v1 = mit->second,
				&v2 = mMsgIdsIn[grpId];

		ok &= Comparison<std::vector<RsGxsMessageId>, RsGxsMessageId>::comparison(v1, v2);
	}
	return ok;
}


bool GenExchangeTest::compareMsgMetaMaps()
{
	GxsMsgMetaMap::iterator mit = mMsgMetaDataOut.begin();
	bool ok = true;
	for(; mit != mMsgMetaDataOut.end(); mit++)
	{
		const RsGxsGroupId& grpId = mit->first;
		const std::vector<RsMsgMetaData>& v1 = mit->second,
				&v2 = mMsgMetaDataOut[grpId];
		ok &= Comparison<std::vector<RsMsgMetaData>, RsMsgMetaData>::comparison(v1, v2);
	}
	return ok;
}


bool GenExchangeTest::compareMsgRelateIdsMap()
{
	return false;
}


bool GenExchangeTest::compareMsgRelatedDataMap()
{
	return false;
}

bool grpDataSort(const RsDummyGrp* g1, const RsDummyGrp* g2)
{
	return g1->meta.mGroupId < g2->meta.mGroupId;
}

bool GenExchangeTest::compareGrpData()
{

	std::sort(mGrpDataIn.begin(), mGrpDataIn.end(), grpDataSort);
	std::sort(mGrpDataOut.begin(), mGrpDataOut.end(), grpDataSort);
	bool ok = Comparison<std::vector<RsDummyGrp*>, RsDummyGrp*>::comparison
			(mGrpDataIn, mGrpDataOut);
	return ok;
}

bool operator<(const RsGroupMetaData& l, const RsGroupMetaData& r)
{
	return l.mGroupId < r.mGroupId;
}

bool GenExchangeTest::compareGrpMeta()
{

	mGrpMetaDataIn.sort();
	mGrpMetaDataOut.sort();

	bool ok = Comparison<std::list<RsGroupMetaData>, RsGroupMetaData>::comparison
			(mGrpMetaDataIn, mGrpMetaDataOut);
	return ok;
}


bool GenExchangeTest::compareGrpIds()
{
	mGrpIdsIn.sort();
	mGrpIdsOut.sort();
	bool ok = Comparison<std::list<RsGxsGroupId>, RsGxsGroupId>::comparison
				(mGrpIdsIn, mGrpIdsOut);
	return ok;
}

void GenExchangeTest::createGrps(uint32_t nGrps,
		std::list<RsGxsGroupId>& groupId)
{
	// create n groups and publish all nGrps and collect id information
	for(uint32_t i=0; i < nGrps; i++)
	{
		RsDummyGrp* grp = new RsDummyGrp();
		init(*grp);
		uint32_t token;
		mTestService->publishDummyGrp(token, grp);
		RsGxsGroupId grpId;
		pollForGrpAcknowledgement(token, grpId);
		groupId.push_back(grpId);
	}
}

void GenExchangeTest::init(RsMsgMetaData& msgMetaData) const
{
    //randString(SHORT_STR, msgMeta.mAuthorId);
    randString(SHORT_STR, msgMetaData.mMsgName);
    randString(SHORT_STR, msgMetaData.mServiceString);
    randString(SHORT_STR, msgMetaData.mOrigMsgId);
    randString(SHORT_STR, msgMetaData.mParentId);
    randString(SHORT_STR, msgMetaData.mThreadId);
    randString(SHORT_STR, msgMetaData.mGroupId);

    msgMetaData.mChildTs = randNum();
    msgMetaData.mMsgStatus = randNum();
    msgMetaData.mMsgFlags = randNum();
    msgMetaData.mPublishTs = randNum();
}

uint32_t GenExchangeTest::randNum() const
{
    return rand()%23562424;
}

void GenExchangeTest::init(RsGroupMetaData& grpMetaData) const
{
    randString(SHORT_STR, grpMetaData.mGroupId);
    //randString(SHORT_STR, grpMetaData.mAuthorId);
    randString(SHORT_STR, grpMetaData.mGroupName);
    randString(SHORT_STR, grpMetaData.mServiceString);


    grpMetaData.mGroupFlags = randNum();
    grpMetaData.mLastPost = randNum();
    grpMetaData.mGroupStatus = randNum();
    grpMetaData.mMsgCount = randNum();
    grpMetaData.mPop = randNum();
    grpMetaData.mSignFlags = randNum();
    grpMetaData.mPublishTs = randNum();
    grpMetaData.mSubscribeFlags = GXS_SERV::GROUP_SUBSCRIBE_ADMIN;
}

void GenExchangeTest::init(RsDummyGrp& grpItem) const
{
    randString(SHORT_STR, grpItem.grpData);
    init(grpItem.meta);
}


void GenExchangeTest::init(RsDummyMsg& msgItem) const
{
    randString(SHORT_STR, msgItem.msgData);
    init(msgItem.meta);
}

void GenExchangeTest::storeToMsgDataOutMaps(const DummyMsgMap& msgDataOut)
{
	mMsgDataOut.insert(msgDataOut.begin(), msgDataOut.end());
}


void GenExchangeTest::storeToMsgIdsOutMaps(const GxsMsgIdResult& msgIdsOut)
{
	mMsgIdsOut.insert(msgIdsOut.begin(), msgIdsOut.end());
}


void GenExchangeTest::storeToMsgMetaOutMaps(const GxsMsgMetaMap& msgMetaOut)
{
	mMsgMetaDataOut.insert(msgMetaOut.begin(), msgMetaOut.end());
}


void GenExchangeTest::storeToMsgDataInMaps(const DummyMsgMap& msgDataIn)
{
	mMsgDataIn.insert(msgDataIn.begin(), msgDataIn.end());
}


void GenExchangeTest::storeToMsgIdsInMaps(const GxsMsgIdResult& msgIdsIn)
{
	mMsgIdsIn.insert(msgIdsIn.begin(), msgIdsIn.end());
}


void GenExchangeTest::storeToMsgMetaInMaps(const GxsMsgMetaMap& msgMetaIn)
{
	mMsgMetaDataIn.insert(msgMetaIn.begin(), msgMetaIn.end());
}


void GenExchangeTest::storeToGrpIdsOutList(
		const std::list<RsGxsGroupId>& grpIdOut)
{
	mGrpIdsOut.insert(mGrpIdsOut.end(), grpIdOut.begin(), grpIdOut.end());
}


void GenExchangeTest::storeToGrpMetaOutList(
		const std::list<RsGroupMetaData>& grpMetaOut)
{
	mGrpMetaDataOut.insert(mGrpMetaDataOut.end(), grpMetaOut.begin(), grpMetaOut.end());
}


void GenExchangeTest::storeToGrpDataOutList(
		const std::vector<RsDummyGrp*>& grpDataOut)
{
	mGrpDataOut.insert(mGrpDataOut.end(), grpDataOut.begin(), grpDataOut.end());
}


void GenExchangeTest::storeToGrpIdsInList(
		const std::list<RsGxsGroupId>& grpIdIn)
{
	mGrpIdsIn.insert(mGrpIdsIn.end(), grpIdIn.begin(), grpIdIn.end());
}


void GenExchangeTest::storeToGrpMetaInList(
		const std::list<RsGroupMetaData>& grpMetaIn)
{
	mGrpMetaDataIn.insert(mGrpMetaDataIn.end(), grpMetaIn.begin(), grpMetaIn.end());
}


void GenExchangeTest::storeToGrpDataInList(
		const std::vector<RsDummyGrp*>& grpDataIn)
{
	mGrpDataIn.insert(mGrpDataIn.begin(), grpDataIn.begin(), grpDataIn.end());
}
void GenExchangeTest::clearAllData()
{
	clearMsgDataInMap();
	clearMsgDataOutMap();
	clearMsgIdInMap();
	clearMsgIdOutMap();
	clearMsgMetaInMap();
	clearMsgMetaOutMap();
	clearGrpDataInList();
	clearGrpDataOutList();
	clearGrpMetaInList();
	clearGrpMetaOutList();
	clearGrpIdInList();
	clearGrpIdOutList();
}
void GenExchangeTest::clearMsgDataInMap()
{
	mMsgDataIn.clear();
}


void GenExchangeTest::clearMsgDataOutMap()
{

	clearMsgDataMap(mMsgDataOut);
}

void GenExchangeTest::clearMsgDataMap(DummyMsgMap& msgDataMap) const
{
	DummyMsgMap::iterator it = msgDataMap.begin();

	for(; it != msgDataMap.end(); it++)
	{
		deleteResVector<RsDummyMsg>(it->second);
	}
}

void GenExchangeTest::clearMsgMetaInMap()
{
	mMsgMetaDataIn.clear();
}


void GenExchangeTest::clearMsgMetaOutMap()
{
	mMsgMetaDataOut.clear();
}


void GenExchangeTest::clearMsgIdInMap()
{
	mMsgIdsIn.clear();
}


void GenExchangeTest::clearMsgIdOutMap()
{
	mMsgIdsOut.clear();
}


void GenExchangeTest::clearMsgRelatedIdInMap()
{
	mMsgRelatedIdsIn.clear();
}


void GenExchangeTest::clearGrpDataInList()
{
	clearGrpDataList(mGrpDataIn);
}

void GenExchangeTest::clearGrpDataList(std::vector<RsDummyGrp*>& grpData) const
{
	deleteResVector<RsDummyGrp>(grpData);
}


void GenExchangeTest::clearGrpDataOutList()
{
	clearGrpDataList(mGrpDataOut);
}


void GenExchangeTest::clearGrpMetaInList()
{
	mGrpMetaDataIn.clear();
}


void GenExchangeTest::clearGrpMetaOutList()
{
	mGrpMetaDataOut.clear();
}


void GenExchangeTest::clearGrpIdInList()
{
	mGrpIdsIn.clear();
}


void GenExchangeTest::clearGrpIdOutList()
{
	mGrpIdsOut.clear();
}


bool operator ==(const RsMsgMetaData& lMeta, const RsMsgMetaData& rMeta)
{

    if(lMeta.mAuthorId != rMeta.mAuthorId) return false;
    if(lMeta.mChildTs != rMeta.mChildTs) return false;
    if(lMeta.mGroupId != rMeta.mGroupId) return false;
    if(lMeta.mMsgFlags != rMeta.mMsgFlags) return false;
    if(lMeta.mMsgId != rMeta.mMsgId) return false;
    if(lMeta.mMsgName != rMeta.mMsgName) return false;
    //if(lMeta.mMsgStatus != rMeta.mMsgStatus) return false;
    if(lMeta.mOrigMsgId != rMeta.mOrigMsgId) return false;
    if(lMeta.mParentId != rMeta.mParentId) return false;
    //if(lMeta.mPublishTs != rMeta.mPublishTs) return false; // don't compare this as internally set in gxs
    if(lMeta.mThreadId != rMeta.mThreadId) return false;
    if(lMeta.mServiceString != rMeta.mServiceString) return false;

    return true;
}

bool operator ==(const RsGroupMetaData& lMeta, const RsGroupMetaData& rMeta)
{
    if(lMeta.mAuthorId != rMeta.mAuthorId) return false;
    if(lMeta.mGroupFlags != rMeta.mGroupFlags) return false;
    if(lMeta.mGroupId != rMeta.mGroupId) return false;
    if(lMeta.mGroupName != rMeta.mGroupName) return false;
    if(lMeta.mGroupStatus != rMeta.mGroupStatus) return false;
    if(lMeta.mLastPost != rMeta.mLastPost) return false;
    if(lMeta.mMsgCount != rMeta.mMsgCount) return false;
    if(lMeta.mPop != rMeta.mPop) return false;
   // if(lMeta.mPublishTs != rMeta.mPublishTs) return false; set in gxs
    if(lMeta.mServiceString != rMeta.mServiceString) return false;
    if(lMeta.mSignFlags != rMeta.mSignFlags) return false;
   // if(lMeta.mSubscribeFlags != rMeta.mSubscribeFlags) return false;

    return true;
}

bool operator ==(const RsDummyGrp& lGrp, const RsDummyGrp& rGrp)
{

    if(lGrp.grpData != rGrp.grpData) return false;
    if(! (lGrp.meta == rGrp.meta)) return false;

    return true;
}

bool operator ==(const RsDummyMsg& lMsg, const RsDummyMsg& rMsg)
{
    if(lMsg.msgData != rMsg.msgData) return false;
    if(!(lMsg.meta == rMsg.meta)) return false;

    return true;
}

bool operator ==(const RsGxsGrpItem& lGrp, const RsGxsGrpItem& rGrp)
{
	return false;
}

