#ifndef GENEXCHANGETESTER_H
#define GENEXCHANGETESTER_H

#include "genexchangetestservice.h"
#include "gxs/rsgds.h"
#include "gxs/rsnxs.h"
#include "gxs/gxscoreserver.h"

bool operator ==(const RsMsgMetaData& lMeta, const RsMsgMetaData& rMeta);
bool operator ==(const RsDummyMsg& lMsg, const RsDummyMsg& rMsg);

bool operator ==(const RsGroupMetaData& lMeta, const RsGroupMetaData& rMeta);
bool operator ==(const RsDummyGrp& lMsg, const RsDummyGrp& rMsg);

/*!
 * The job of the service tester is to send dummy msg items to the GenExchange service
 * and then retrieve them (ignoring ackowledge message)
 * Also it modifies local meta items and check if it worked out
 */
class GenExchangeTester
{
public:

    void pollForToken(uint32_t, const RsTokReqOptions& opts);

    GenExchangeTester();

    // message tests
    bool testMsgSubmissionRetrieval();
    bool testMsgIdRetrieval();
    bool testMsgIdRetrieval_OptParents();
    bool testMsgIdRetrieval_OptOrigMsgId();
    bool testMsgIdRetrieval_OptLatest();
    bool testSpecificMsgMetaRetrieval();

    // request msg related tests
    bool testMsgRelatedChildIdRetrieval();
    bool testMsgRelatedChildDataRetrieval();
    bool testMsgAllVersions();


    // group tests
    bool testGrpSubmissionRetrieval();
    bool testSpecificGrpRetrieval();
    bool testGrpIdRetrieval();
    bool testGrpMetaRetrieval();

    bool testGrpMetaModRequest();
    bool testMsgMetaModRequest();

private:

    // to be called at start
    void setUp();

    // and at end of test routines, resets db and clears out maps
    void breakDown();


    void setUpGrps(uint32_t grpFlag=0); // to be called at start of msg tests

    /*!
     * Can be called at start grpId or grpMeta test
     * to help out
     * ids are store is mGrpIdsOut
     */
    void setUpLargeGrps(uint32_t nGrps);

    void storeMsgData(GxsMsgDataMap& msgData);
    void storeMsgMeta(GxsMsgMetaMap& msgMetaData);
    void storeMsgIds(GxsMsgIdResult& msgIds);

    void storeGrpData(std::vector<RsGxsGrpItem*>& grpData);
    void storeGrpMeta(std::list<RsGroupMetaData>& grpMetaData);
    void storeGrpId(std::list<RsGxsGroupId>& grpIds);

    void init(RsDummyGrp* grpItem) const;
    void init(RsGroupMetaData&) const;
    void init(RsDummyMsg* msgItem) const;
    void init(RsMsgMetaData&) const;

    uint32_t randNum() const;
private:

    void createMsgs(std::vector<RsDummyMsg*>& msgs, int nMsgs) const;

private:


    RsMutex mGenTestMutex;

    std::vector<RsGxsGrpItem*> mGrpDataOut, mGrpDataIn;
    std::list<RsGroupMetaData> mGrpMetaDataOut, mGrpMetaDataIn;
    std::list<RsGxsGroupId> mGrpIdsOut, mGrpIdsIn;

    GxsMsgDataMap mMsgDataOut, mMsgDataIn;
    GxsMsgMetaMap mMsgMetaDataOut, mMsgMetaDataIn;
    GxsMsgIdResult mMsgIdsOut, mMsgIdsIn;

    MsgRelatedIdResult mMsgRelatedIdsOut, mMsgRelatedIdsIn;
    GxsMsgRelatedDataMap mMsgRelatedDataMapOut, mMsgRelatedDataMapIn;

    std::vector<RsGxsGroupId> mRandGrpIds; // ids that exist to help group testing

private:

    GenExchangeTestService* mTestService;
    RsTokenService* mTokenService;

    RsNetworkExchangeService* mNxs;
    RsGeneralDataService* mDataStore;


};

#endif // GENEXCHANGETESTER_H
