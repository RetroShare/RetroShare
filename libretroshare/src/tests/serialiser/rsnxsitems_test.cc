
#include "support.h"

#include "rsnxsitems_test.h"

INITTEST();

#define NUM_BIN_OBJECTS 5
#define NUM_SYNC_MSGS 8
#define NUM_SYNC_GRPS 5

RsSerialType* init_item(RsNxsGrp& nxg)
{

    randString(SHORT_STR, nxg.identity);
    randString(SHORT_STR, nxg.grpId);
    nxg.timeStamp = rand()%23;
    nxg.grpFlag = rand()%242;
    init_item(nxg.grp);

    init_item(nxg.adminSign);
    init_item(nxg.idSign);

    int nKey = rand()%12;

    for(int i=0; i < nKey; i++){
        nxg.keys.groupId  = nxg.grpId;
        std::string s;
        RsTlvSecurityKey k;
        init_item(k);
        nxg.keys.keys[k.keyId] = k;
    }

    return new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}


RsSerialType* init_item(RsNxsMsg& nxm)
{
    randString(SHORT_STR, nxm.msgId);
    randString(SHORT_STR, nxm.grpId);
    randString(SHORT_STR, nxm.identity);

    init_item(nxm.publishSign);
    init_item(nxm.idSign);
    init_item(nxm.msg);
    nxm.msgFlag = rand()%4252;
    nxm.timeStamp = rand()%246;

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
    randString(SHORT_STR, rsgl.grpId);
    init_item(rsgl.adminSign);


    return new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

RsSerialType* init_item(RsSyncGrpMsgList& rsgml)
{
    rsgml.clear();

    rsgml.flag = RsSyncGrpList::FLAG_RESPONSE;
    randString(SHORT_STR, rsgml.grpId);
    randString(SHORT_STR, rsgml.msgId);
    init_item(rsgml.idSign);

    return new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

RsSerialType* init_item(RsNxsTransac& rstx){

    rstx.clear();

    rstx.timeout = rand()%14141;
    rstx.transactFlag = rand()%2424;
    rstx.nItems = rand()%33132;
    rstx.transactionId = rand()%242112;

    return new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

bool operator==(const RsNxsGrp& l, const RsNxsGrp& r){

    if(!(l.adminSign == r.adminSign)) return false;
    if(!(l.idSign == r.idSign)) return false;
    if(l.timeStamp != r.timeStamp) return false;
    if(l.grpFlag != r.grpFlag) return false;
    if(l.identity != r.identity) return false;
    if(l.grpId != r.grpId) return false;
    if(l.keys.groupId != r.keys.groupId) return false;
    if(!(l.grp == r.grp) ) return false;

    std::map<std::string, RsTlvSecurityKey>::const_iterator mit =
            l.keys.keys.begin(), mit_end = l.keys.keys.end();

    for(; mit != mit_end; mit++){
        const RsTlvSecurityKey& lk = l.keys.keys.find(mit->first)->second;
        const RsTlvSecurityKey& rk = r.keys.keys.find(mit->first)->second;

        if(! ( lk == rk) ) return false;
    }

    return true;
}

bool operator==(const RsNxsMsg& l, const RsNxsMsg& r){

    if(l.msgId != r.msgId) return false;
    if(l.grpId != r.grpId) return false;
    if(l.identity != r.identity) return false;
    if(l.timeStamp != r.timeStamp) return false;
    if(l.msgFlag != r.msgFlag) return false;
    if(! (l.msg == r.msg) ) return false;
    if(! (l.publishSign == r.publishSign) ) return false;
    if(! (l.idSign == r.idSign) ) return false;

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
    if(! (l.adminSign == r.adminSign) ) return false;
    if(l.grpId != r.grpId) return false;

    return true;
}

bool operator==(const RsSyncGrpMsgList& l, const RsSyncGrpMsgList& r)
{
    if(l.flag != r.flag) return false;
    if(l.grpId != r.grpId) return false;
    if(l.msgId != r.msgId) return false;
    if(! (l.idSign == r.idSign) ) return false;

    return true;
}

bool operator==(const RsNxsTransac& l, const RsNxsTransac& r){

    if(l.transactFlag != r.transactFlag) return false;
    if(l.transactionId != r.transactionId) return false;
    if(l.timeout != r.timeout) return false;
    if(l.nItems != r.nItems) return false;


    return true;
}

int main()
{
    std::cerr << "RsNxsItem Tests" << std::endl;

    test_RsItem<RsNxsGrp>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM); REPORT("Serialise/Deserialise RsGrpResp");
    test_RsItem<RsNxsMsg>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM); REPORT("Serialise/Deserialise RsGrpMsgResp");
    test_RsItem<RsSyncGrp>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM); REPORT("Serialise/Deserialise RsSyncGrp");
    test_RsItem<RsSyncGrpMsg>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM); REPORT("Serialise/Deserialise RsSyncGrpMsg");
    test_RsItem<RsSyncGrpList>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM); REPORT("Serialise/Deserialise RsSyncGrpList");
    test_RsItem<RsSyncGrpMsgList>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM); REPORT("Serialise/Deserialise RsSyncGrpMsgList");
    test_RsItem<RsNxsTransac>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM); REPORT("Serialise/Deserialise RsNxsTransac");

    FINALREPORT("RsNxsItem Tests");

    return TESTRESULT();
}
