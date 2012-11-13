#include "genexchangetester.h"
#include "support.h"
#include "gxs/rsdataservice.h"
#include "gxs/rsgxsflags.h"

GenExchangeTester::GenExchangeTester()
    : mGenTestMutex("genTest")
{
    remove("testServiceDb");
}


void GenExchangeTester::setUp()
{
    mDataStore = new RsDataService("./", "testServiceDb", RS_SERVICE_TYPE_DUMMY, NULL);
    mNxs = new RsDummyNetService();

    RsGixsDummy* gixsDummy = new RsGixsDummy("incoming", "outgoing");

    mTestService = new GenExchangeTestService(mDataStore, mNxs, gixsDummy, 0);
    mTokenService = mTestService->getTokenService();
    mTestService->start();
}

void GenExchangeTester::setUpGrps(uint32_t grpFlags)
{
 // create some random grps to allow msg testing

    RsDummyGrp* dgrp1 = new RsDummyGrp();
    RsDummyGrp* dgrp2 = new RsDummyGrp();
    RsDummyGrp* dgrp3 = new RsDummyGrp();

    init(dgrp1);
    dgrp1->meta.mGroupFlags = grpFlags;
    uint32_t token;
    mTestService->publishDummyGrp(token, dgrp1);

    RsTokReqOptions opts;
    opts.mReqType = 45000;
    pollForToken(token, opts);

    RsGxsGroupId grpId;
    mTestService->acknowledgeTokenGrp(token, grpId);
    mRandGrpIds.push_back(grpId);

    init(dgrp2);
    dgrp2->meta.mGroupFlags = grpFlags;
    mTestService->publishDummyGrp(token, dgrp2);
    pollForToken(token, opts);
    mTestService->acknowledgeTokenGrp(token, grpId);
    mRandGrpIds.push_back(grpId);

    init(dgrp3);
    dgrp3->meta.mGroupFlags = grpFlags;
    mTestService->publishDummyGrp(token, dgrp3);
    pollForToken(token, opts);
    mTestService->acknowledgeTokenGrp(token, grpId);
    mRandGrpIds.push_back(grpId);
}


void GenExchangeTester::breakDown()
{
    mTestService->join();

    delete mTestService;

    // a bit protracted, but start a new db and use to clear up junk
    mDataStore = new RsDataService("./", "testServiceDb", RS_SERVICE_TYPE_DUMMY, NULL);
    mDataStore->resetDataStore();
    delete mDataStore;
    mTokenService = NULL;
    // remove data base file
    remove("testServiceDb");

    // clear up all latent data
    GxsMsgDataMap::iterator gmd_mit = mMsgDataIn.begin();

    for(; gmd_mit != mMsgDataIn.end(); gmd_mit++)
    {
        std::vector<RsGxsMsgItem*>& msgV = gmd_mit->second;
        std::vector<RsGxsMsgItem*>::iterator vit = msgV.begin();

        for(; vit != msgV.end(); vit++)
        {
            delete *vit;
        }
    }
    mMsgDataIn.clear();

    gmd_mit = mMsgDataOut.begin();

    for(; gmd_mit != mMsgDataOut.end(); gmd_mit++)
    {
        std::vector<RsGxsMsgItem*>& msgV = gmd_mit->second;
        std::vector<RsGxsMsgItem*>::iterator vit = msgV.begin();

        for(; vit != msgV.end(); vit++)
        {
            delete *vit;
        }
    }

    mMsgDataOut.clear();

    std::vector<RsGxsGrpItem*>::iterator g_mit = mGrpDataIn.begin();

    for(; g_mit != mGrpDataIn.end(); g_mit++)
    {
        delete *g_mit;
    }

    mGrpDataIn.clear();
    g_mit = mGrpDataOut.begin();

    for(; g_mit != mGrpDataOut.end(); g_mit++)
    {
        delete *g_mit;
    }
    mGrpDataOut.clear();


    // these don't hold any dynamic memory
    mGrpIdsIn.clear();
    mGrpIdsOut.clear();
    mMsgIdsIn.clear();
    mMsgIdsOut.clear();
    mRandGrpIds.clear();
    mMsgMetaDataIn.clear();
    mMsgMetaDataOut.clear();
    mGrpMetaDataIn.clear();
    mGrpMetaDataOut.clear();
}

bool GenExchangeTester::testGrpSubmissionRetrieval()
{


    setUp();

    // create some random grps to allow msg testing

   RsDummyGrp* dgrp1 = new RsDummyGrp();
   RsDummyGrp* dgrp2 = new RsDummyGrp();
   RsDummyGrp* dgrp3 = new RsDummyGrp();

   RsDummyGrp* dgrp1_copy = new RsDummyGrp();
   RsDummyGrp* dgrp2_copy = new RsDummyGrp();
   RsDummyGrp* dgrp3_copy = new RsDummyGrp();

   init(dgrp1);
   init(dgrp2);
   init(dgrp3);

   RsTokReqOptions opts;
   opts.mReqType = 45000;
   uint32_t token;
   RsGxsGroupId grpId;

   *dgrp1_copy = *dgrp1;
   mTestService->publishDummyGrp(token, dgrp1);
   pollForToken(token, opts);
   mTestService->acknowledgeTokenGrp(token, grpId);
   dgrp1_copy->meta.mGroupId = grpId;
   mGrpDataOut.push_back(dgrp1_copy);

   *dgrp2_copy = *dgrp2;
   mTestService->publishDummyGrp(token, dgrp2);
   pollForToken(token, opts);
   mTestService->acknowledgeTokenGrp(token, grpId);
   dgrp2_copy->meta.mGroupId = grpId;
   mGrpDataOut.push_back(dgrp2_copy);

   *dgrp3_copy = *dgrp3;
   mTestService->publishDummyGrp(token, dgrp3);
   pollForToken(token, opts);
   mTestService->acknowledgeTokenGrp(token, grpId);
   dgrp3_copy->meta.mGroupId = grpId;
   mGrpDataOut.push_back(dgrp3_copy);

   opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

   std::list<RsGxsGroupId> grpIds;
   mTokenService->requestGroupInfo(token, 0, opts);

   pollForToken(token, opts);

   std::vector<RsGxsGrpItem*>::iterator lit_out = mGrpDataOut.begin();

   bool ok = true;

   for(; lit_out != mGrpDataOut.end(); lit_out++)
   {
       RsDummyGrp* grpOut = dynamic_cast<RsDummyGrp*>(*lit_out);
       if(!grpOut)
       {
           ok = false;
           break;
       }



       std::vector<RsGxsGrpItem*>::iterator lit_in = mGrpDataIn.begin();

       bool found = true;
       for(; lit_in != mGrpDataIn.end(); lit_in++)
       {
            RsDummyGrp* grpIn = dynamic_cast<RsDummyGrp*>(*lit_in);

            if(!grpIn)
            {
                ok = false;
                break;
            }

            if(grpIn->meta.mGroupId == grpOut->meta.mGroupId)
            {
                found = true;
                ok &= *grpIn == *grpOut;
            }
       }

       if(!found)
       {
           ok = false;
           break;
       }
   }

   breakDown();
   return ok;
}

bool GenExchangeTester::testGrpMetaRetrieval()
{
    setUp();

    // create some random grps to allow msg testing

   RsDummyGrp* dgrp1 = new RsDummyGrp();
   RsDummyGrp* dgrp2 = new RsDummyGrp();
   RsDummyGrp* dgrp3 = new RsDummyGrp();

   init(dgrp1);
   init(dgrp2);
   init(dgrp3);

   RsTokReqOptions opts;
   opts.mReqType = 45000;
   uint32_t token;
   RsGxsGroupId grpId;

   RsGroupMetaData tempMeta;

   tempMeta = dgrp1->meta;
   mTestService->publishDummyGrp(token, dgrp1);
   pollForToken(token, opts);
   mTestService->acknowledgeTokenGrp(token, grpId);
   tempMeta.mGroupId = grpId;
   mGrpMetaDataOut.push_back(tempMeta);


   tempMeta = dgrp2->meta;
   mTestService->publishDummyGrp(token, dgrp2);
   pollForToken(token, opts);
   mTestService->acknowledgeTokenGrp(token, grpId);
   tempMeta.mGroupId = grpId;
   mGrpMetaDataOut.push_back(tempMeta);

   tempMeta = dgrp3->meta;
   mTestService->publishDummyGrp(token, dgrp3);
   pollForToken(token, opts);
   mTestService->acknowledgeTokenGrp(token, grpId);
   tempMeta.mGroupId = grpId;
   mGrpMetaDataOut.push_back(tempMeta);


   opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

   std::list<RsGxsGroupId> grpIds;
   mTokenService->requestGroupInfo(token, 0, opts);

   pollForToken(token, opts);

   std::list<RsGroupMetaData>::iterator lit_out = mGrpMetaDataOut.begin();

   bool ok = true;

   for(; lit_out != mGrpMetaDataOut.end(); lit_out++)
   {
       const RsGroupMetaData& grpMetaOut = *lit_out;

       std::list<RsGroupMetaData>::iterator lit_in = mGrpMetaDataIn.begin();

       bool found = true;
       for(; lit_in != mGrpMetaDataIn.end(); lit_in++)
       {
            const RsGroupMetaData& grpMetaIn = *lit_in;

            if(grpMetaOut.mGroupId == grpMetaIn.mGroupId)
            {
                found = true;
                ok &= grpMetaIn == grpMetaOut;
            }
       }

       if(!found)
       {
           ok = false;
           break;
       }
   }

   breakDown();
   return ok;
}

bool GenExchangeTester::testGrpIdRetrieval()
{
    setUp();
    setUpLargeGrps(30); // create a large amount of grps


    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_GROUP_IDS;
    uint32_t token;
    std::list<RsGxsGroupId> grpIds;
    mTokenService->requestGroupInfo(token, 0, opts);

    pollForToken(token, opts);

    std::list<RsGxsGroupId>::iterator lit_out = mGrpIdsOut.begin();

    bool ok = true;

    for(; lit_out != mGrpIdsOut.end(); lit_out++)
    {
        std::list<RsGxsGroupId>::iterator lit_in = mGrpIdsIn.begin();

        bool found = true;
        for(; lit_in != mGrpIdsIn.end(); lit_in++)
        {

             if(*lit_out == *lit_in)
             {
                 found = true;
             }
        }

        if(!found)
        {
            ok = false;
            break;
        }
    }

    breakDown();
    return ok;
}


bool GenExchangeTester::testGrpMetaModRequest()
{

    setUp();

    // create some random grps to allow msg testing

   RsDummyGrp* dgrp1 = new RsDummyGrp();
   RsDummyGrp* dgrp2 = new RsDummyGrp();
   RsDummyGrp* dgrp3 = new RsDummyGrp();
   init(dgrp1);
   init(dgrp2);
   init(dgrp3);

   uint32_t token;
   RsTokReqOptions opts;
   opts.mReqType = 45000;
   std::vector<RsGxsGroupId> grpIds;
   RsGxsGroupId grpId;

   mTestService->publishDummyGrp(token, dgrp1);
   pollForToken(token, opts);
   mTestService->acknowledgeTokenGrp(token, grpId);
   grpIds.push_back(grpId);

   mTestService->publishDummyGrp(token, dgrp2);
   pollForToken(token, opts);
   mTestService->acknowledgeTokenGrp(token, grpId);
   grpIds.push_back(grpId);

   mTestService->publishDummyGrp(token, dgrp3);
   pollForToken(token, opts);
   mTestService->acknowledgeTokenGrp(token, grpId);
   grpIds.push_back(grpId);

   bool ok = true;

   std::string newServiceString;
   uint32_t newGrpStatus = randNum();
   uint32_t newSubscribeGrpFlag = randNum();
   randString(SHORT_STR, newServiceString);

   // mod service flag for first grp
    mTestService->setGroupServiceStringTS(token, grpIds[0], newServiceString);
    pollForToken(token, opts);
    ok = mTestService->acknowledgeTokenGrp(token, grpId);

    mTestService->setGroupStatusFlagTS(token, grpIds[0], newGrpStatus);
    pollForToken(token, opts);
    ok = mTestService->acknowledgeTokenGrp(token, grpId);

    mTestService->setGroupSubscribeFlagTS(token, grpIds[0], newSubscribeGrpFlag);
    pollForToken(token, opts);
    ok = mTestService->acknowledgeTokenGrp(token, grpId);

    std::list<RsGxsGroupId> reqGrpIds;
    reqGrpIds.push_back(grpIds[0]);
    opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;
    mTokenService->requestGroupInfo(token, 0, opts, reqGrpIds);
    pollForToken(token, opts);



   if(mGrpMetaDataIn.empty())
       ok = false;

   if(ok)
   {
       RsGroupMetaData meta = *(mGrpMetaDataIn.begin());

       if(meta.mServiceString != newServiceString)
           ok = false;

       if(meta.mSubscribeFlags != newSubscribeGrpFlag)
           ok = false;

       if(meta.mGroupStatus != newGrpStatus)
           ok = false;


   }

       breakDown();
       return ok;
}


void GenExchangeTester::setUpLargeGrps(uint32_t nGrps)
{
    for(int i=0; i < nGrps; i++)
    {
        RsDummyGrp* dgrp = new RsDummyGrp();
        RsGxsGroupId grpId;
        uint32_t token;
        init(dgrp);
        RsTokReqOptions opts;
        opts.mReqType = 4000;
        mTestService->publishDummyGrp(token, dgrp);
        pollForToken(token, opts);
        mTestService->acknowledgeTokenGrp(token, grpId);
        mGrpIdsOut.push_back(grpId);
    }
}

bool GenExchangeTester::testMsgMetaModRequest()
{
    setUp();
    setUpGrps();

    RsDummyMsg* msg = new RsDummyMsg();
    init(msg);

    msg->meta.mGroupId = mRandGrpIds[(rand()%3)];
    uint32_t token;
    RsDummyMsg* msgOut = new RsDummyMsg();
    *msgOut = *msg;
    mTestService->publishDummyMsg(token, msg);

    // poll will block until found
    RsTokReqOptions opts;
    opts.mReqType = 4200;
    pollForToken(token, opts);
    RsGxsGrpMsgIdPair msgId;

    mTestService->acknowledgeTokenMsg(token, msgId);

    if(msgId.first.empty() || msgId.second.empty()){
        breakDown();
        return false;
    }

    std::string newServiceString;
    randString(SHORT_STR, newServiceString);

    // first modify service string
    mTestService->setMsgServiceStringTS(token, msgId, newServiceString);
    pollForToken(token, opts);

    mTestService->acknowledgeTokenMsg(token, msgId);

    uint32_t newStatus = 2;
    // first modify service string
    mTestService->setMsgStatusFlagTS(token, msgId, newStatus);
    pollForToken(token, opts);

    mTestService->acknowledgeTokenMsg(token, msgId);

    // now request msg
    GxsMsgReq req;
    std::vector<RsGxsMessageId> msgV;
    msgV.push_back(msgId.second);
    req.insert(std::make_pair(msgId.first, msgV));

    opts.mReqType = GXS_REQUEST_TYPE_MSG_META;
    mTokenService->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, req);

    // poll again
    pollForToken(token, opts);

    bool ok = true;

    if(mMsgMetaDataIn.empty())
        ok = false;

    if(ok){

        GxsMsgMetaMap::iterator mit = mMsgMetaDataIn.begin();
        std::vector<RsMsgMetaData>& metaV = mit->second;

        RsMsgMetaData meta;
        if(metaV.empty()){
            ok = false;
        }else{
            meta = *(metaV.begin());
        }


        if(meta.mServiceString != newServiceString)
            ok &= false;

        if(meta.mMsgStatus != newStatus)
            ok &= false;


    }

    /********************/

    // complete
    breakDown();

    return ok;

}

bool GenExchangeTester::testMsgSubmissionRetrieval()
{

    // start up
    setUp();
    setUpGrps(GXS_SERV::FLAG_PRIVACY_PUBLIC);

    /********************/

    RsDummyMsg* msg = new RsDummyMsg();
    init(msg);

    msg->meta.mGroupId = mRandGrpIds[(rand()%3)];
    uint32_t token;
    RsDummyMsg* msgOut = new RsDummyMsg();
    *msgOut = *msg;
    mTestService->publishDummyMsg(token, msg);

    // poll will block until found
    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
    pollForToken(token, opts);
    RsGxsGrpMsgIdPair msgId;

    mTestService->acknowledgeTokenMsg(token, msgId);

    // add msg sent to msg out
    msgOut->meta.mMsgId = msgId.second;
    std::vector<RsGxsMsgItem*> msgDataV;
    msgDataV.push_back(msgOut);
    mMsgDataOut[msgId.first] = msgDataV;

    if(msgId.first.empty() || msgId.second.empty()){
        breakDown();
        return false;
    }

    GxsMsgReq req;
    std::vector<RsGxsMessageId> msgV;
    msgV.push_back(msgId.second);
    req.insert(std::make_pair(msgId.first, msgV));

    opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
    mTokenService->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, req);

    // poll again
    pollForToken(token, opts);

    bool ok = true;

    if(mMsgDataIn.size() != mMsgDataOut.size())
    {
        breakDown();
        return false;
    }

    GxsMsgDataMap::iterator mit = mMsgDataOut.begin();

    for(; mit != mMsgDataOut.end(); mit++)
    {
        const RsGxsGroupId& grpId = mit->first;

        std::vector<RsGxsMsgItem*>& msgV_1 = mit->second,
        msgV_2 = mMsgDataIn[grpId];
        std::vector<RsGxsMsgItem*>::iterator vit1, vit2;

        for(vit1 = msgV_1.begin(); vit1 != msgV_1.end();
        vit1++)
        {
            RsDummyMsg* lMsg = dynamic_cast<RsDummyMsg*>(*vit1);
            for(vit2 = msgV_2.begin(); vit2 != msgV_2.end();
            vit2++)
            {
                RsDummyMsg* rMsg = dynamic_cast<RsDummyMsg*>(*vit2);

                if(rMsg->meta.mMsgId == lMsg->meta.mMsgId)
                    ok &= *rMsg == *lMsg;

            }
        }
    }

    /********************/

    // complete
    breakDown();

    return ok;
}

bool GenExchangeTester::testMsgIdRetrieval()
{

    // start up
    setUp();
    setUpGrps(GXS_SERV::FLAG_PRIVACY_PUBLIC);

    /********************/

    // first create several msgs for 3 different groups (the "this" rand groups)
    // and store them

    // then make all requests immediately then poll afterwards for each and run outbound test
    // we want only latest for now
    int nMsgs = (rand()%121)+2; // test a large number of msgs
    std::vector<RsDummyMsg*> msgs;
    createMsgs(msgs, nMsgs);
    RsTokReqOptions opts;
    opts.mReqType = 4000;
    uint32_t token;

    std::vector<uint32_t> tokenV;
    for(int i=0; i < nMsgs; i++)
    {
        mTestService->publishDummyMsg(token, msgs[i]);
        tokenV.push_back(token);
    }

    for(int i=0; i < nMsgs; i++)
    {
        pollForToken(tokenV[i], opts);
        RsGxsGrpMsgIdPair msgId;
        mTestService->acknowledgeTokenMsg(tokenV[i], msgId);
        mMsgIdsOut[msgId.first].push_back(msgId.second);
    }

    // now do ask of all msg ids

    std::list<RsGxsGroupId> req;
    opts.mReqType = GXS_REQUEST_TYPE_MSG_IDS;

    // use empty grp ids request types, non specific msgs ids
    for(int i=0; i < mRandGrpIds.size(); i++)
    {
        req.push_back(mRandGrpIds[i]);
    }

    mTokenService->requestMsgInfo(token, 0, opts, req);

    pollForToken(token, opts);

    GxsMsgIdResult::iterator mit = mMsgIdsOut.begin();
    for(; mit != mMsgIdsOut.end(); mit++)
    {
        std::vector<RsGxsMessageId> msgIdsOut, msgIdsIn;
        msgIdsOut = mit->second;

        std::vector<RsGxsMessageId>::iterator vit_out = msgIdsOut.begin(), vit_in;

        for(; vit_out != msgIdsOut.end(); vit_out++)
        {
            bool found = false;
            msgIdsIn = mMsgIdsIn[mit->first];
            vit_in = msgIdsIn.begin();

            for(; vit_in != msgIdsIn.end(); vit_in++)
            {
                if(*vit_in == *vit_out)
                    found = true;
            }

            if(!found){
                breakDown();
                return false;
            }

        }
    }

    /********************/

    // complete
    breakDown();

    return true;
}
bool GenExchangeTester::testMsgAllVersions()
{
    // start up
    setUp();
    setUpGrps(GXS_SERV::FLAG_PRIVACY_PUBLIC);

    // want to create several msgs of the same version (i.e. same origMsgId)


    // create msgs
    // then make all requests immediately then poll afterwards for each and run outbound test
    // we want only latest for now
    int nMsgs = (rand()%50)+2; // test a large number of msgs
    std::vector<RsDummyMsg*> msgs;
    createMsgs(msgs, nMsgs);
    RsTokReqOptions opts;
    opts.mReqType = 4000;
    uint32_t token;

   bool first = true;
   RsGxsGrpMsgIdPair firstMsgId;

   // everyone is a version of first msg
    for(int i=0; i < nMsgs; i++)
    {
        RsDummyMsg* msg = msgs[i];

        if(first){
            msg->meta.mParentId = "";
            msg->meta.mOrigMsgId = ""; // don't worry GXS sets origid to first
        }
        else
        {
            msg->meta.mParentId = "";
            msg->meta.mGroupId = firstMsgId.first;
            msg->meta.mOrigMsgId = firstMsgId.second;
        }

        mTestService->publishDummyMsg(token, msg);
        pollForToken(token, opts);
        RsGxsGrpMsgIdPair msgId;
        mTestService->acknowledgeTokenMsg(token, msgId);

        if(msgId.first.empty() || msgId.second.empty())
        {
            breakDown();
            std::cerr << "serious error: Acknowledgement failed! " << std::endl;
            return false;
        }



        // less than half have no parents
        if(first){
            firstMsgId.second = msgId.second;
            firstMsgId.first = msgId.first;
            first = false;

        }

         mMsgRelatedIdsOut[firstMsgId].push_back(msgId.second);
    }


    opts.mReqType = GXS_REQUEST_TYPE_MSG_RELATED_IDS;
    opts.mOptions = RS_TOKREQOPT_MSG_VERSIONS;
    std::vector<RsGxsGrpMsgIdPair> msgIdList;
    msgIdList.push_back(firstMsgId);
    mTokenService->requestMsgRelatedInfo(token, 0, opts, msgIdList);

    pollForToken(token, opts);

    MsgRelatedIdResult::iterator mit = mMsgRelatedIdsOut.begin();
    for(; mit != mMsgRelatedIdsOut.end(); mit++)
    {
        std::vector<RsGxsMessageId> msgIdsOut, msgIdsIn;
        msgIdsOut = mit->second;

        std::vector<RsGxsMessageId>::iterator vit_out = msgIdsOut.begin(), vit_in;

        for(; vit_out != msgIdsOut.end(); vit_out++)
        {
            bool found = false;
            msgIdsIn = mMsgRelatedIdsIn[mit->first];
            vit_in = msgIdsIn.begin();

            for(; vit_in != msgIdsIn.end(); vit_in++)
            {
                if(*vit_in == *vit_out)
                    found = true;
            }

            if(!found){
                breakDown();
                return false;
            }

        }
    }

    // complete
    breakDown();

    return true;
}

bool GenExchangeTester::testMsgChildRetrieval()
{
//    // start up
//    setUp();
//    setUpGrps(GXS_SERV::FLAG_PRIVACY_PUBLIC);

//    /********************/


//    // create msgs
//    // then make all requests immediately then poll afterwards for each and run outbound test
//    // we want only latest for now
//    int nMsgs = (rand()%50)+2; // test a large number of msgs
//    std::vector<RsDummyMsg*> msgs;
//    createMsgs(msgs, nMsgs);
//    RsTokReqOptions opts;
//    opts.mReqType = 4000;
//    uint32_t token;

//   bool first = true;
//   RsGxsGrpMsgIdPair firstMsgId;

//   // everyone is parent of first msg

//    for(int i=0; i < nMsgs; i++)
//    {
//        RsDummyMsg* msg = msgs[i];

//        if(first){
//            msg->meta.mParentId = "";
//            msg->meta.mOrigMsgId = "";
//        }
//        else
//        {
//            msg->meta.mParentId = firstMsgId.second;
//            msg->meta.mGroupId = firstMsgId.first;
//            msg->meta.mOrigMsgId = "";
//        }

//        mTestService->publishDummyMsg(token, msg);
//        pollForToken(token, opts);
//        RsGxsGrpMsgIdPair msgId;
//        mTestService->acknowledgeTokenMsg(token, msgId);


//        if(msgId.first.empty() || msgId.second.empty())
//        {
//            breakDown();
//            std::cerr << "serious error: Acknowledgement failed! " << std::endl;
//            return false;
//        }

//        if(!first)
//        {
//            mMsgIdsOut[msgId.first].push_back(msgId.second);
//        }

//        if(first){
//            firstMsgId.second = msgId.second;
//            firstMsgId.first = msgId.first;
//            first = false;
//        }

//    }


//    opts.mReqType = GXS_REQUEST_TYPE_MSG_IDS;
//    opts.mOptions = RS_TOKREQOPT_MSG_PARENT | RS_TOKREQOPT_MSG_LATEST;
//    std::vector<RsGxsGrpMsgIdPair> msgIdList;
//    msgIdList.push_back(firstMsgId);
//    mTokenService->requestMsgRelatedInfo(token, 0, opts, msgIdList);

//    pollForToken(token, opts);

//    GxsMsgIdResult::iterator mit = mMsgIdsOut.begin();
//    for(; mit != mMsgIdsOut.end(); mit++)
//    {
//        std::vector<RsGxsMessageId> msgIdsOut, msgIdsIn;
//        msgIdsOut = mit->second;

//        std::vector<RsGxsMessageId>::iterator vit_out = msgIdsOut.begin(), vit_in;

//        for(; vit_out != msgIdsOut.end(); vit_out++)
//        {
//            bool found = false;
//            msgIdsIn = mMsgIdsIn[mit->first];
//            vit_in = msgIdsIn.begin();

//            for(; vit_in != msgIdsIn.end(); vit_in++)
//            {
//                if(*vit_in == *vit_out)
//                    found = true;
//            }

//            if(!found){
//                breakDown();
//                return false;
//            }

//        }
//    }

//    /********************/

//    // complete
//    breakDown();

//    return true;
}

bool GenExchangeTester::testSpecificMsgMetaRetrieval()
{


    // start up
    setUp();
    setUpGrps(GXS_SERV::FLAG_PRIVACY_PUBLIC);

    /********************/

    RsDummyMsg* msg = new RsDummyMsg();
    init(msg);

    msg->meta.mGroupId = mRandGrpIds[(rand()%3)];
    uint32_t token;
    RsMsgMetaData msgMetaOut;
    msgMetaOut = msg->meta;
    mTestService->publishDummyMsg(token, msg);

    // poll will block until found
    RsTokReqOptions opts;
    opts.mReqType = 4200;
    pollForToken(token, opts);
    RsGxsGrpMsgIdPair msgId;

    mTestService->acknowledgeTokenMsg(token, msgId);

    // add msg sent to msg out
    msgMetaOut.mMsgId = msgId.second;

    std::vector<RsMsgMetaData> msgMetaDataV;
    msgMetaDataV.push_back(msgMetaOut);
    mMsgMetaDataOut[msgId.first] = msgMetaDataV;

    if(msgId.first.empty() || msgId.second.empty()){
        breakDown();
        return false;
    }

    GxsMsgReq req;
    std::vector<RsGxsMessageId> msgV;
    msgV.push_back(msgId.second);
    req.insert(std::make_pair(msgId.first, msgV));

    opts.mReqType = GXS_REQUEST_TYPE_MSG_META;
    mTokenService->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, req);

    // poll again
    pollForToken(token, opts);

    bool ok = true;

    if(mMsgMetaDataIn.size() != mMsgMetaDataOut.size())
    {
        breakDown();
        return false;
    }

    GxsMsgMetaMap::iterator mit = mMsgMetaDataOut.begin();

    for(; mit != mMsgMetaDataOut.end(); mit++)
    {
        const RsGxsGroupId& grpId = mit->first;

        std::vector<RsMsgMetaData>& msgV_1 = mit->second,
        msgV_2 = mMsgMetaDataIn[grpId];
        std::vector<RsMsgMetaData>::iterator vit1, vit2;

        for(vit1 = msgV_1.begin(); vit1 != msgV_1.end();
        vit1++)
        {
            bool found = false;
            const RsMsgMetaData& lMsgMeta = *vit1;
            for(vit2 = msgV_2.begin(); vit2 != msgV_2.end();
            vit2++)
            {
                const RsMsgMetaData& rMsgMeta = *vit2;

                if(rMsgMeta.mMsgId == lMsgMeta.mMsgId){
                    found = true;
                    ok &= rMsgMeta == lMsgMeta;
                }

            }

            if(!found)
            {
                breakDown();
                return false;
            }

        }
    }

    /********************/

    // complete
    breakDown();

    return ok;
}

bool GenExchangeTester::testMsgIdRetrieval_OptParents()
{
    // start up
    setUp();
    setUpGrps();

    /********************/


    // create msgs
    // then make all requests immediately then poll afterwards for each and run outbound test
    // we want only latest for now
    int nMsgs = (rand()%50)+2; // test a large number of msgs
    std::vector<RsDummyMsg*> msgs;
    createMsgs(msgs, nMsgs);
    RsTokReqOptions opts;
    opts.mReqType = 4000;
    uint32_t token;


    for(int i=0; i < nMsgs; i++)
    {
        RsDummyMsg* msg = msgs[i];

        int j = rand()%5;


        if(j<3)
            msg->meta.mParentId = "";

        mTestService->publishDummyMsg(token, msg);
        pollForToken(token, opts);
        RsGxsGrpMsgIdPair msgId;
        mTestService->acknowledgeTokenMsg(token, msgId);

        if(msgId.first.empty() || msgId.second.empty())
        {
            breakDown();
            std::cerr << "serious error: Acknowledgement failed! " << std::endl;
            return false;
        }

        if(j<3)
            mMsgIdsOut[msgId.first].push_back(msgId.second);

    }

    std::list<RsGxsGroupId> req;

    // use empty grp ids request types, non specific msgs ids
    for(int i=0; i < mRandGrpIds.size(); i++)
    {
        req.push_back(mRandGrpIds[i]);
    }

    opts.mReqType = GXS_REQUEST_TYPE_MSG_IDS;
    opts.mOptions = RS_TOKREQOPT_MSG_THREAD;
    mTokenService->requestMsgInfo(token, 0, opts, req);

    pollForToken(token, opts);

    GxsMsgIdResult::iterator mit = mMsgIdsOut.begin();
    for(; mit != mMsgIdsOut.end(); mit++)
    {
        std::vector<RsGxsMessageId> msgIdsOut, msgIdsIn;
        msgIdsOut = mit->second;

        std::vector<RsGxsMessageId>::iterator vit_out = msgIdsOut.begin(), vit_in;

        for(; vit_out != msgIdsOut.end(); vit_out++)
        {
            bool found = false;
            msgIdsIn = mMsgIdsIn[mit->first];
            vit_in = msgIdsIn.begin();

            for(; vit_in != msgIdsIn.end(); vit_in++)
            {
                if(*vit_in == *vit_out)
                    found = true;
            }

            if(!found){
                breakDown();
                return false;
            }

        }
    }

    /********************/

    // complete
    breakDown();

    return true;

}

bool GenExchangeTester::testMsgIdRetrieval_OptOrigMsgId()
{
    // start up
    setUp();
    setUpGrps();

    /********************/


    // create msgs
    // then make all requests immediately then poll afterwards for each and run outbound test
    // we want only latest for now
    int nMsgs = (rand()%50)+2; // test a large number of msgs
    std::vector<RsDummyMsg*> msgs;
    createMsgs(msgs, nMsgs);
    RsTokReqOptions opts;
    opts.mReqType = 4000;
    uint32_t token;


    for(int i=0; i < nMsgs; i++)
    {
        RsDummyMsg* msg = msgs[i];

        int j = rand()%5;

        if(j<3)
            msg->meta.mOrigMsgId = "";

        mTestService->publishDummyMsg(token, msg);
        pollForToken(token, opts);
        RsGxsGrpMsgIdPair msgId;
        mTestService->acknowledgeTokenMsg(token, msgId);

        if(msgId.first.empty() || msgId.second.empty())
        {
            breakDown();
            std::cerr << "serious error: Acknowledgement failed! " << std::endl;
            return false;
        }

        if(j<3)
            mMsgIdsOut[msgId.first].push_back(msgId.second);

    }

    std::list<RsGxsGroupId> req;

    // use empty grp ids request types, non specific msgs ids
    for(int i=0; i < mRandGrpIds.size(); i++)
    {
        req.push_back(mRandGrpIds[i]);
    }

    opts.mReqType = GXS_REQUEST_TYPE_MSG_IDS;
    opts.mOptions = RS_TOKREQOPT_MSG_ORIGMSG;
    mTokenService->requestMsgInfo(token, 0, opts, req);

    pollForToken(token, opts);

    GxsMsgIdResult::iterator mit = mMsgIdsOut.begin();
    for(; mit != mMsgIdsOut.end(); mit++)
    {
        std::vector<RsGxsMessageId> msgIdsOut, msgIdsIn;
        msgIdsOut = mit->second;

        std::vector<RsGxsMessageId>::iterator vit_out = msgIdsOut.begin(), vit_in;

        for(; vit_out != msgIdsOut.end(); vit_out++)
        {
            bool found = false;
            msgIdsIn = mMsgIdsIn[mit->first];
            vit_in = msgIdsIn.begin();

            for(; vit_in != msgIdsIn.end(); vit_in++)
            {
                if(*vit_in == *vit_out)
                    found = true;
            }

            if(!found){
                breakDown();
                return false;
            }

        }
    }

    /********************/

    // complete
    breakDown();

    return true;
}


bool GenExchangeTester::testMsgIdRetrieval_OptLatest()
{

    // testing for latest, create msg which are origMsgIds then
    // create another batch and select random msgs again to have
    // id of the other, so selecting latest msg should only come up
    // with this batch of msgs for the grp plus the others not selected
    // start up
    setUp();
    setUpGrps();

    /********************/


    // create msgs which will be used later to set orig msg ids of latest msgs
    int nMsgs = (rand()%50)+2; // test a large number of msgs
    std::vector<RsDummyMsg*> msgs;
    createMsgs(msgs, nMsgs);
    RsTokReqOptions opts;
    opts.mReqType = 4000;
    uint32_t token;


    for(int i=0; i < nMsgs; i++)
    {
        RsDummyMsg* msg = msgs[i];

        int j = rand()%5;

        if(j<3)
            msg->meta.mOrigMsgId = "";

        mTestService->publishDummyMsg(token, msg);
        pollForToken(token, opts);
        RsGxsGrpMsgIdPair msgId;
        mTestService->acknowledgeTokenMsg(token, msgId);

        if(msgId.first.empty() || msgId.second.empty())
        {
            breakDown();
            std::cerr << "serious error: Acknowledgement failed! " << std::endl;
            return false;
        }

        if(j<3)
            mMsgIdsOut[msgId.first].push_back(msgId.second);

    }


    /** just in case put it to sleep so publish time is sufficiently different **/

    double timeDelta = 2.;

#ifndef WINDOWS_SYS
        usleep((int) (timeDelta * 1000000));
#else
        Sleep((int) (timeDelta * 1000));
#endif


    /* now to create the later msgs */

    nMsgs = (rand()%50)+2; // test a large number of msgs
    msgs.clear();
    createMsgs(msgs, nMsgs);
    opts.mReqType = 4000;

    // fist transfer to grpID->msgId(Set) pair in
    // order to remove used msgs, that is
    // msgs who ids have been used for orig id so not expected in result set
    GxsMsgIdResult::iterator mit = mMsgIdsOut.begin();
    std::map<RsGxsGroupId, std::set<RsGxsMessageId> > msgIdOutTemp;

    for(; mit != mMsgIdsOut.end(); mit++)
    {
        std::vector<RsGxsMessageId> msgIdsOut;
        msgIdsOut = mit->second;
        std::vector<RsGxsMessageId>::iterator vit = msgIdsOut.begin();

        for(; vit != msgIdsOut.end(); vit++)
            msgIdOutTemp[mit->first].insert(*vit);
    }

    mMsgIdsOut.clear(); // to be repopulated later with expected result set

    std::vector<RsDummyMsg*>::iterator vit = msgs.begin();

    // loop over newly create msgs and assign a msg ids
    // to origmsg id field from previously published msgs
    for(; vit != msgs.end();)
    {
        RsDummyMsg* msg = *vit;

        // first find grp
        const RsGxsGroupId& grpId = msg->meta.mGroupId;

        // if grp does not exist then don't pub msg
        if(msgIdOutTemp.find(grpId) == msgIdOutTemp.end()){
            delete msg;
            vit = msgs.erase(vit);
            continue;
        }else{

            // now assign msg a rand msgId
            std::set<RsGxsMessageId>& msgIdS = msgIdOutTemp[grpId];

            std::set<RsGxsMessageId>::iterator sit = msgIdS.begin();
            std::vector<RsGxsMessageId> tempMsgV;

            // store in vect for convenience
            for(; sit != msgIdS.end(); sit++)
                tempMsgV.push_back(*sit);

            if(tempMsgV.size() == 0)
            {
                delete msg;
                vit = msgs.erase(vit);
                continue;
            }
            int j = rand()%tempMsgV.size();

            msg->meta.mOrigMsgId = tempMsgV[j];
            msgIdS.erase(tempMsgV[j]); // remove msg as it has been used
        }

        // go on and publish msg
        mTestService->publishDummyMsg(token, msg);
        pollForToken(token, opts);
        RsGxsGrpMsgIdPair msgId;
        mTestService->acknowledgeTokenMsg(token, msgId);

        if(msgId.first.empty() || msgId.second.empty())
        {
            breakDown();
            std::cerr << "serious error: Acknowledgement failed! " << std::endl;
            return false;
        }

        mMsgIdsOut[msgId.first].push_back(msgId.second);
        vit++;
    }

    // now add back unused msgs
    std::map<RsGxsGroupId, std::set<RsGxsMessageId> >::iterator mit_id_temp = msgIdOutTemp.begin();

    for(; mit_id_temp != msgIdOutTemp.end(); mit_id_temp++){

        std::set<RsGxsMessageId>& idSet = mit_id_temp->second;
        std::set<RsGxsMessageId>::iterator sit = idSet.begin();

        for(; sit != idSet.end(); sit++)
        {
            mMsgIdsOut[mit_id_temp->first].push_back(*sit);
        }
    }


    // use empty grp ids request types, non specific msgs ids
    std::list<RsGxsGroupId> req;
    for(int i=0; i < mRandGrpIds.size(); i++)
    {
        req.push_back(mRandGrpIds[i]);
    }

    opts.mReqType = GXS_REQUEST_TYPE_MSG_IDS;
    opts.mOptions = RS_TOKREQOPT_MSG_LATEST;
    mTokenService->requestMsgInfo(token, 0, opts, req);

    pollForToken(token, opts);

    mit = mMsgIdsOut.begin();
    for(; mit != mMsgIdsOut.end(); mit++)
    {
        std::vector<RsGxsMessageId> msgIdsOut, msgIdsIn;
        msgIdsOut = mit->second;

        std::vector<RsGxsMessageId>::iterator vit_out = msgIdsOut.begin(), vit_in;

        for(; vit_out != msgIdsOut.end(); vit_out++)
        {
            bool found = false;
            msgIdsIn = mMsgIdsIn[mit->first];
            vit_in = msgIdsIn.begin();

            for(; vit_in != msgIdsIn.end(); vit_in++)
            {
                if(*vit_in == *vit_out)
                    found = true;
            }

            if(!found){
                breakDown();
                return false;
            }

        }
    }

    /********************/

    // complete
    breakDown();

    return true;
}

void GenExchangeTester::createMsgs(std::vector<RsDummyMsg *> &msgs, int nMsgs) const
{
    for(int i = 0; i < nMsgs; i++)
    {
        RsDummyMsg* msg = new RsDummyMsg();
        init(msg);
        msgs.push_back(msg);
        int j = (rand()%3);
        msg->meta.mGroupId = mRandGrpIds[j];
    }
}


// helper functions

void GenExchangeTester::storeGrpMeta(std::list<RsGroupMetaData> &grpMetaData){
    mGrpMetaDataIn = grpMetaData;
}

void GenExchangeTester::storeMsgData(GxsMsgDataMap &msgData)
{

    mMsgDataIn = msgData;

}

void GenExchangeTester::storeGrpData(std::vector<RsGxsGrpItem *> &grpData)
{
    mGrpDataIn = grpData;
}

void GenExchangeTester::storeGrpId(std::list<RsGxsGroupId> &grpIds)
{
    mGrpIdsIn = grpIds;
}


void GenExchangeTester::storeMsgMeta(GxsMsgMetaMap &msgMetaData)
{
    mMsgMetaDataIn = msgMetaData;
}

void GenExchangeTester::storeMsgIds(GxsMsgIdResult &msgIds)
{
    mMsgIdsIn = msgIds;
}


void GenExchangeTester::init(RsGroupMetaData &grpMeta) const
{
    randString(SHORT_STR, grpMeta.mGroupId);
    randString(SHORT_STR, grpMeta.mAuthorId);
    randString(SHORT_STR, grpMeta.mGroupName);
    randString(SHORT_STR, grpMeta.mServiceString);


    grpMeta.mGroupFlags = randNum();
    grpMeta.mLastPost = randNum();
    grpMeta.mGroupStatus = randNum();
    grpMeta.mMsgCount = randNum();
    grpMeta.mPop = randNum();
    grpMeta.mSignFlags = randNum();
    grpMeta.mPublishTs = randNum();
    grpMeta.mSubscribeFlags = GXS_SERV::GROUP_SUBSCRIBE_ADMIN;

}

void GenExchangeTester::init(RsMsgMetaData &msgMeta) const
{
    randString(SHORT_STR, msgMeta.mAuthorId);
    randString(SHORT_STR, msgMeta.mMsgName);
    randString(SHORT_STR, msgMeta.mServiceString);
    randString(SHORT_STR, msgMeta.mOrigMsgId);
    randString(SHORT_STR, msgMeta.mParentId);
    randString(SHORT_STR, msgMeta.mThreadId);
    randString(SHORT_STR, msgMeta.mGroupId);

    msgMeta.mChildTs = randNum();
    msgMeta.mMsgStatus = randNum();
    msgMeta.mMsgFlags = randNum();
    msgMeta.mPublishTs = randNum();
}

uint32_t GenExchangeTester::randNum() const
{
    return rand()%23562424;
}


bool operator ==(const RsMsgMetaData& lMeta, const RsMsgMetaData& rMeta)
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
    if(lMeta.mSubscribeFlags != rMeta.mSubscribeFlags) return false;

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

void GenExchangeTester::init(RsDummyGrp *grpItem) const
{
    randString(SHORT_STR, grpItem->grpData);
    init(grpItem->meta);
}

void GenExchangeTester::init(RsDummyMsg *msgItem) const
{
    randString(SHORT_STR, msgItem->msgData);
    init(msgItem->meta);
}

void GenExchangeTester::pollForToken(uint32_t token, const RsTokReqOptions &opts)
{
    double timeDelta = 0.2;

    while(true)
    {
#ifndef WINDOWS_SYS
    usleep((int) (timeDelta * 1000000));
#else
    Sleep((int) (timeDelta * 1000));
#endif

        if((RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE == mTokenService->requestStatus(token))
            || (RsTokenService::GXS_REQUEST_V2_STATUS_FAILED == mTokenService->requestStatus(token)))
        {
            switch(opts.mReqType)
            {
            case GXS_REQUEST_TYPE_GROUP_DATA:
                mTestService->getGroupDataTS(token, mGrpDataIn);
                break;
            case GXS_REQUEST_TYPE_GROUP_META:
                mTestService->getGroupMetaTS(token, mGrpMetaDataIn);
                break;
            case GXS_REQUEST_TYPE_GROUP_IDS:
                mTestService->getGroupListTS(token, mGrpIdsIn);
                break;
            case GXS_REQUEST_TYPE_MSG_DATA:
                mTestService->getMsgDataTS(token, mMsgDataIn);
                break;
            case GXS_REQUEST_TYPE_MSG_META:
                mTestService->getMsgMetaTS(token, mMsgMetaDataIn);
                break;
            case GXS_REQUEST_TYPE_MSG_IDS:
                mTestService->getMsgListTS(token, mMsgIdsIn);
                break;
            case GXS_REQUEST_TYPE_MSG_RELATED_IDS:
                mTestService->getMsgRelatedListTS(token, mMsgRelatedIdsIn);
                break;
            }
            break;
        }

    }
}
