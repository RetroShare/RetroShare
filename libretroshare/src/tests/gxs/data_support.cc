#include "support.h"
#include "data_support.h"


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


void init_item(RsNxsGrp* nxg)
{

    randString(SHORT_STR, nxg->identity);
    randString(SHORT_STR, nxg->grpId);
    nxg->timeStamp = rand()%23;
    nxg->grpFlag = rand()%242;
    init_item(nxg->grp);

    init_item(nxg->adminSign);
    init_item(nxg->idSign);

    int nKey = rand()%12;
    nxg->keys.groupId  = nxg->grpId;
    for(int i=0; i < nKey; i++){
        RsTlvSecurityKey k;
        init_item(k);
        nxg->keys.keys[k.keyId] = k;
    }

    return;
}


void init_item(RsNxsMsg* nxm)
{
    randString(SHORT_STR, nxm->msgId);
    randString(SHORT_STR, nxm->grpId);
    randString(SHORT_STR, nxm->identity);

    init_item(nxm->publishSign);
    init_item(nxm->idSign);
    init_item(nxm->msg);
    nxm->msgFlag = rand()%4252;
    nxm->timeStamp = rand()%246;

    return;
}
