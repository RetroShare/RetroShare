#ifndef GENEXCHANGETESTER_H
#define GENEXCHANGETESTER_H

#include "genexchangetestservice.h"

/*!
 * The job of the service tester is to send dummy msg items to the GenExchange service
 * and then retrieve them (ignoring ackowledge message)
 * Also it modifies local meta items and check if it worked out
 */
class GenExchangeTester
{
public:

    void pollForToken(uint32_t);

    GenExchangeTester(GenExchangeTestService* testService);

    bool testMsgSubmissionRetrieval();
    bool testMsgIdRetrieval();
    bool testSpecificMsgRetrieval();

    bool testGrpSubmissionRetrieval();
    bool testSpecificGrpRetrieval();
    bool testGrpIdRetrieval();

    bool testGrpSubscribeRequest();
    bool testGrpStatusRequest();

private:

    // to be called at start
    void setUp();

    // and at end of test routines, resets db and clears out maps
    void breakDown();


    void setUpGrps(); // to be called at start of msg tests

    void storeMsgData(GxsMsgDataMap& msgData);
    void storeMsgMeta(GxsMsgMetaMap& msgMetaData);
    void storeMsgIds(GxsMsgIdResult& msgIds);

    void storeGrpData(GxsMsgDataMap& grpData);
    void storeGrpMeta(GxsMsgMetaMap& grpMetaData);
    void storeGrpId(GxsMsgIdResult& grpIds);

    void init(RsDummyGrp* grpItem);
    void init(RsGroupMetaData&);
    void init(RsDummyMsg* msgItem);
    void init(RsMsgMetaData&);

    bool operator ==(const RsMsgMetaData& lMeta, const RsMsgMetaData& rMeta);
    bool operator ==(const RsDummyMsg& lMsg, const RsDummyMsg& rMsg);

private:

    RsMutex mGenTestMutex;

    GxsMsgDataMap mGrpDataOut, mGrpDataIn;
    GxsMsgMetaMap mGrpMetaDataOut, mGrpMetaDataIn;
    GxsMsgIdResult mGrpIdsOut, mGrpIdsIn;

    GxsMsgDataMap mMsgDataOut, mMsgDataIn;
    GxsMsgMetaMap mMsgMetaDataOut, mMsgMetaDataIn;
    GxsMsgIdResult mMsgIdsOut, mMsgIdsIn;

private:

    GenExchangeTestService* mTestService;
    RsTokenServiceV2* mTokenService;
};

#endif // GENEXCHANGETESTER_H
