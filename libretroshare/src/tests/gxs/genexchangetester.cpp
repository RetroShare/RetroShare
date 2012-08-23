#include "genexchangetester.h"
#include "support.h"


GenExchangeTester::GenExchangeTester(GenExchangeTestService* testService)
    : mTestService(testService)
{
    mTokenService = mTestService->getTokenService();
}

bool GenExchangeTester::testMsgSubmissionRetrieval()
{
    RsDummyMsg* msg = new RsDummyMsg();
    init(msg);
    uint32_t token;
    mTestService->publishDummyMsg(token, msg);

    // poll will block until found
    pollForToken(token);

    if(mMsgIdsIn.empty())
        return false;

    RsGxsMessageId& msgId = mMsgIdsIn.begin()->second;
    RsGxsGroupId& grpId = mMsgIdsIn.begin()->first;

    GxsMsgReq req;
    req.insert(std::make_pair(msgId, grpId));

    RsTokReqOptionsV2 opts;
    opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
    mTokenService->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_LIST, opts, req);

    // poll again
    pollForToken(token);

    mTestService->getMsgDataTS(token, mMsgDataIn);

    bool ok = true;

    if(mMsgDataIn.size() != mMsgDataOut.size()) return false;

    GxsMsgDataMap::iterator mit = mMsgDataOut.begin();

    for(; mit != mMsgDataOut.end(); mit++)
    {
        const RsGxsGroupId& gId = mit->first;

        std::vector<RsGxsMsgItem*>& msgV_1 = mit->second,
        msgV_2 = mMsgDataIn[grpId];
        std::vector<RsGxsMsgItem*>::iterator vit1, vit2;

        for(vit1 = msgV_1.begin(); vit1 = msgV_1.end();
        vit1++)
        {
            RsDummyMsg* lMsg = dynamic_cast<RsDummyMsg*>(*vit1);
            for(vit2 = msgV_2.begin(); vit2 = msgV_2.end();
            vit2++)
            {
                RsDummyMsg* rMsg = dynamic_cast<RsDummyMsg*>(*vit2);

                if(rMsg->meta.mMsgId == lMsg->meta.mMsgId)
                    ok &= *rMsg == *lMsg;

            }
        }

    }

    return ok;
}

bool GenExchangeTester::testMsgIdRetrieval()
{
    return false;
}

bool GenExchangeTester::testSpecificMsgRetrieval()
{
    return false;
}

bool GenExchangeTester::testGrpIdRetrieval()
{
    return false;
}

bool GenExchangeTester::testGrpStatusRequest()
{
    return false;
}


// helper functions

void GenExchangeTester::storeMsgData(GxsMsgDataMap &msgData)
{

}

void GenExchangeTester::storeGrpData(GxsMsgDataMap &grpData)
{

}

void GenExchangeTester::storeGrpId(GxsMsgIdResult &grpIds)
{

}

void GenExchangeTester::storeMsgData(GxsMsgDataMap &msgData)
{

}


void GenExchangeTester::storeMsgMeta(GxsMsgMetaMap &msgMetaData)
{

}

void GenExchangeTester::storeMsgIds(GxsMsgIdResult &msgIds)
{

}


void GenExchangeTester::init(RsGroupMetaData &grpMeta)
{

}

void GenExchangeTester::init(RsMsgMetaData &msgMeta)
{
    randString(LARGE_STR, msgMeta.mAuthorId);
    randString(LARGE_STR, msgMeta.mMsgName);
    randString(LARGE_STR, msgMeta.mServiceString);
    randString(LARGE_STR, msgMeta.mOrigMsgId);
    randString(LARGE_STR, msgMeta.mParentId);
    randString(LARGE_STR, msgMeta.mThreadId);
    randString(LARGE_STR, msgMeta.mGroupId);

    randNum(msgMeta.mChildTs);
    randNum(msgMeta.mMsgStatus);
    randNum(msgMeta.mMsgFlags);
    randNum(msgMeta.mPublishTs);
}

void randNum(uint32_t& num)
{
    num = rand()%23562424;
}


bool GenExchangeTester::operator ==(const RsMsgMetaData& lMeta, const RsMsgMetaData& rMeta)
{

    if(lMeta.mAuthorId != rMeta.mAuthorId) return false;
    if(lMeta.mChildTs != rMeta.mChildTs) return false;
    if(lMeta.mGroupId != rMeta.mGroupId) return false;
    if(lMeta.mMsgFlags != rMeta.mMsgFlags) return false;
    if(lMeta.mMsgId != rMeta.mMsgId) return false;
    if(lMeta.mMsgName != rMeta.mMsgName) return false;
    if(lMeta.mMsgStatus != rMeta.mMsgStatus) return false;
    if(lMeta.mOrigMsgId != rMeta.mOrigMsgId) return false;
    if(lMeta.mParentId != rMeta.mParentId) return false;
    if(lMeta.mPublishTs != rMeta.mPublishTs) return false;
    if(lMeta.mThreadId != rMeta.mThreadId) return false;

    return true;
}

bool operator ==(const RsDummyMsg& lMsg, const RsDummyMsg& rMsg)
{
    if(lMsg.msgData != rMsg.msgData) return false;
    if(!(lMsg.meta == rMsg.meta)) return false;

    return true;
}

void GenExchangeTester::init(RsGxsGrpItem *grpItem)
{

}

void GenExchangeTester::init(RsDummyMsg *msgItem)
{
    randString(LARGE_STR, msgItem->msgData);
    init(msgItem->meta);
}

void GenExchangeTester::pollForToken(uint32_t token)
{
    double timeDelta = 0.2;

    while(true)
    {
#ifndef WINDOWS_SYS
    usleep((int) (timeDelta * 1000000));
#else
    Sleep((int) (timeDelta * 1000));
#endif

        if(RsTokenServiceV2::GXS_REQUEST_STATUS_COMPLETE ==
           mTokenService->requestStatus(token))
        {
            break;
        }
    }
}

void GenExchangeTester::setUp()
{

}

void GenExchangeTester::setUpGrps()
{

}

void GenExchangeTester::breakDown()
{

}









