
#include "support.h"

#include "rsnxsitems_test.h"

INITTEST();

#define NUM_BIN_OBJECTS 5
#define NUM_SYNC_MSGS 8
#define NUM_SYNC_GRPS 5

RsSerialType* init_item(RsGrpResp& rgr)
{

    rgr.clear();

    for(int i=0; i < 5; i++){
        RsTlvBinaryData* b = new RsTlvBinaryData(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
        init_item(*b);
        rgr.grps.push_back(b);
    }

    return new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}


RsSerialType* init_item(RsGrpMsgResp& rgmr)
{
    rgmr.clear();

    for(int i=0; i < 5; i++){
        RsTlvBinaryData* b = new RsTlvBinaryData(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
        init_item(*b);
        rgmr.msgs.push_back(b);
    }

    return new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

RsSerialType* init_item(RsSyncGrp& rsg)
{
    rsg.clear();

    rsg.flag = RsSyncGrp::FLAG_USE_SYNC_HASH;
    rsg.syncAge = rand()%2423;
    randString(3124,rsg.syncHash);

    return new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

RsSerialType* init_item(RsSyncGrpMsg& rsgm)
{
    rsgm.clear();

    rsgm.flag = RsSyncGrpMsg::FLAG_USE_SYNC_HASH;
    rsgm.syncAge = rand()%24232;
    randString(SHORT_STR, rsgm.grpId);
    randString(SHORT_STR, rsgm.syncHash);

    return new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

RsSerialType* init_item(RsSyncGrpList& rsgl)
{
    rsgl.clear();

    rsgl.flag = RsSyncGrpList::FLAG_RESPONSE;

    for(int i=0; i < NUM_SYNC_GRPS; i++){

        int nVers = rand()%8;
        std::list<uint32_t> verL;
        for(int j=0; j < nVers; j++){
            verL.push_back(rand()%343);
        }
        std::string grpId;
        randString(SHORT_STR, grpId);

        std::pair<std::string, std::list<uint32_t> > p(grpId, verL);
        rsgl.grps.insert(p);
    }
    return new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

RsSerialType* init_item(RsSyncGrpMsgList& rsgml)
{
    rsgml.clear();

    rsgml.flag = RsSyncGrpList::FLAG_RESPONSE;
    randString(SHORT_STR, rsgml.grpId);

    for(int i=0; i < NUM_SYNC_GRPS; i++){

        int nVers = rand()%8;
        std::list<uint32_t> verL;
        for(int j=0; j < nVers; j++){
            verL.push_back(rand()%343);
        }
        std::string msgId;
        randString(SHORT_STR, msgId);

        std::pair<std::string, std::list<uint32_t> > p(msgId, verL);
        rsgml.msgs.insert(p);
    }
    return new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}


bool operator==(const RsGrpResp& l, const RsGrpResp& r){

    if(l.grps.size() != r.grps.size()) return false;

    std::list<RsTlvBinaryData*>::const_iterator lit
            = l.grps.begin(), rit  =
              r.grps.begin();

    for(; lit != l.grps.end(); lit++, rit++){
        if(!(*(*lit) == *(*rit))) return false;
    }

    return true;
}

bool operator==(const RsGrpMsgResp& l, const RsGrpMsgResp& r){

    if(l.msgs.size() != r.msgs.size()) return false;

    std::list<RsTlvBinaryData*>::const_iterator lit
            = l.msgs.begin(), rit  =
              r.msgs.begin();

    for(; lit != l.msgs.end(); lit++, rit++){
        if(!(*(*lit) == *(*rit))) return false;
    }

    return true;
}

bool operator==(const RsSyncGrp& l, const RsSyncGrp& r)
{

    if(l.syncHash != r.syncHash) return false;
    if(l.flag != r.flag) return false;
    if(l.syncAge != r.syncAge) return false;

    return true;
}

bool operator==(const RsSyncGrpMsg& l, const RsSyncGrpMsg& r)
{

    if(l.flag != r.flag) return false;
    if(l.syncAge != r.syncAge) return false;
    if(l.syncHash != r.syncHash) return false;
    if(l.grpId != r.grpId) return false;

    return true;
}

bool operator==(const RsSyncGrpList& l, const RsSyncGrpList& r)
{
    if(l.flag != r.flag) return false;

    SyncList::const_iterator lit = l.grps.begin(), rit= r.grps.begin();

    for(; lit != l.grps.end(); lit++, rit++){

        if(lit->first != rit->first) return false;
        const std::list<uint32_t>& lList = lit->second, &rList
                = rit->second;

        std::list<uint32_t>::const_iterator lit2 = lList.begin(), rit2
                                   = rList.begin();

        for(; lit2 != lList.end(); lit2++, rit2++){

            if(*lit2 != *rit2) return false;
        }
    }
    return true;
}

bool operator==(const RsSyncGrpMsgList& l, const RsSyncGrpMsgList& r)
{
    if(l.flag != r.flag) return false;
    if(l.grpId != r.grpId) return false;

    SyncList::const_iterator lit = l.msgs.begin(), rit= r.msgs.begin();

    for(; lit != l.msgs.end(); lit++, rit++){

        if(lit->first != rit->first) return false;
        const std::list<uint32_t>& lList = lit->second, &rList
                = rit->second;

        std::list<uint32_t>::const_iterator lit2 = lList.begin(), rit2
                                   = rList.begin();

        for(; lit2 != lList.end(); lit2++, rit2++){

            if(*lit2 != *rit2) return false;
        }
    }

    return true;
}

int main()
{
    std::cerr << "RsNxsItem Tests" << std::endl;

    test_RsItem<RsGrpResp>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM); REPORT("Serialise/Deserialise RsGrpResp");
    test_RsItem<RsGrpMsgResp>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM); REPORT("Serialise/Deserialise RsGrpMsgResp");
    test_RsItem<RsSyncGrp>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM); REPORT("Serialise/Deserialise RsSyncGrp");
    test_RsItem<RsSyncGrpMsg>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM); REPORT("Serialise/Deserialise RsSyncGrpMsg");
    test_RsItem<RsSyncGrpList>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM); REPORT("Serialise/Deserialise RsSyncGrpList");
    test_RsItem<RsSyncGrpMsgList>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM); REPORT("Serialise/Deserialise RsSyncGrpMsgList");

    FINALREPORT("RsNxsItem Tests");

    return TESTRESULT();
}
