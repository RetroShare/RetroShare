#include "libretroshare/serialiser/support.h"
#include "data_support.h"


bool operator==(const RsNxsGrp& l, const RsNxsGrp& r){

    if(l.grpId != r.grpId) return false;
    if(!(l.grp == r.grp) ) return false;
    if(!(l.meta == r.meta) ) return false;
    if(l.transactionNumber != r.transactionNumber) return false;

    return true;
}

bool operator==(const RsNxsMsg& l, const RsNxsMsg& r){


    if(l.msgId != r.msgId) return false;
    if(l.grpId != r.grpId) return false;
    if(! (l.msg == r.msg) ) return false;
    if(! (l.meta == r.meta) ) return false;
    if(l.transactionNumber != r.transactionNumber) return false;

    return true;
}


bool operator ==(const RsGxsGrpMetaData& l, const RsGxsGrpMetaData& r)
{
    if(!(l.signSet == r.signSet)) return false;
    if(!(l.keys == r.keys)) return false;
    if(l.mGroupFlags != r.mGroupFlags) return false;
    if(l.mPublishTs != r.mPublishTs) return false;
    if(l.mAuthorId != r.mAuthorId) return false;
    if(l.mGroupName != r.mGroupName) return false;
    if(l.mGroupId != r.mGroupId) return false;

    return true;
}

bool operator ==(const RsGxsMsgMetaData& l, const RsGxsMsgMetaData& r)
{

    if(!(l.signSet == r.signSet)) return false;
    if(l.mGroupId != r.mGroupId) return false;
    if(l.mAuthorId != r.mAuthorId) return false;
    if(l.mParentId != r.mParentId) return false;
    if(l.mOrigMsgId != r.mOrigMsgId) return false;
    if(l.mThreadId != r.mThreadId) return false;
    if(l.mMsgId != r.mMsgId) return false;
    if(l.mMsgName != r.mMsgName) return false;
    if(l.mPublishTs != r.mPublishTs) return false;
    if(l.mMsgFlags != r.mMsgFlags) return false;

    return true;
}



#if 0
void init_item(RsNxsGrp& nxg)
{

    nxg.clear();

    nxg.grpId.random();
    nxg.transactionNumber = rand()%23;
    init_item(nxg.grp);
    init_item(nxg.meta);
    return;
}

void init_item(RsNxsMsg& nxm)
{
    nxm.clear();

    nxm.msgId.random();
    nxm.grpId.random();
    init_item(nxm.msg);
    init_item(nxm.meta);
    nxm.transactionNumber = rand()%23;

    return;
}
#endif


void init_item(RsGxsGrpMetaData* metaGrp)
{

    metaGrp->mGroupId.random();
    metaGrp->mOrigGrpId.random();
    metaGrp->mAuthorId.random();
    randString(SHORT_STR, metaGrp->mGroupName);

    init_item(metaGrp->signSet);
    init_item(metaGrp->keys);

    metaGrp->mPublishTs = rand()%3452;
    metaGrp->mGroupFlags = rand()%43;

    metaGrp->mGroupStatus = rand()%313;
    metaGrp->mSubscribeFlags = rand()%2251;
    metaGrp->mMsgCount = rand()%2421;
    metaGrp->mLastPost = rand()%2211;
    metaGrp->mPop = rand()%5262;
}

void init_item(RsGxsMsgMetaData* metaMsg)
{

    metaMsg->mGroupId.random();
    metaMsg->mMsgId.random();
    metaMsg->mThreadId.random();
    metaMsg->mParentId.random();
    metaMsg->mAuthorId.random();
    metaMsg->mOrigMsgId.random();
    randString(SHORT_STR, metaMsg->mMsgName);

    init_item(metaMsg->signSet);

    metaMsg->mPublishTs = rand()%313;
    metaMsg->mMsgFlags = rand()%224;
    metaMsg->mMsgStatus = rand()%4242;
    metaMsg->mChildTs = rand()%221;
}




RsSerialType* init_item(RsNxsGrp& nxg)
{
    nxg.clear();

    nxg.grpId.random();
    nxg.transactionNumber = rand()%23;
    init_item(nxg.grp);
    init_item(nxg.meta);

    return new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}


RsSerialType* init_item(RsNxsMsg& nxm)
{
    nxm.clear();

    nxm.msgId.random();
    nxm.grpId.random();
    init_item(nxm.msg);
    init_item(nxm.meta);
    nxm.transactionNumber = rand()%23;

    return new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

RsSerialType* init_item(RsNxsSyncGrp& rsg)
{
    rsg.clear();
    rsg.flag = RsNxsSyncGrp::FLAG_USE_SYNC_HASH;
    rsg.createdSince = rand()%2423;
    randString(3124,rsg.syncHash);

    return new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

RsSerialType* init_item(RsNxsSyncMsg& rsgm)
{
    rsgm.clear();

    rsgm.flag = RsNxsSyncMsg::FLAG_USE_SYNC_HASH;
    rsgm.createdSince = rand()%24232;
    rsgm.transactionNumber = rand()%23;
    rsgm.grpId.random();
    randString(SHORT_STR, rsgm.syncHash);

    return new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

RsSerialType* init_item(RsNxsSyncGrpItem& rsgl)
{
    rsgl.clear();

    rsgl.flag = RsNxsSyncGrpItem::FLAG_RESPONSE;
    rsgl.transactionNumber = rand()%23;
    rsgl.publishTs = rand()%23;
    rsgl.grpId.random();

    return new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

RsSerialType* init_item(RsNxsSyncMsgItem& rsgml)
{
    rsgml.clear();

    rsgml.flag = RsNxsSyncGrpItem::FLAG_RESPONSE;
    rsgml.transactionNumber = rand()%23;
    rsgml.grpId.random();
    rsgml.msgId.random();

    return new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

RsSerialType* init_item(RsNxsTransac& rstx){

    rstx.clear();

    rstx.timestamp = rand()%14141;
    rstx.transactFlag = rand()%2424;
    rstx.nItems = rand()%33132;
    rstx.transactionNumber = rand()%242112;

    return new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}


bool operator==(const RsNxsSyncGrp& l, const RsNxsSyncGrp& r)
{

    if(l.syncHash != r.syncHash) return false;
    if(l.flag != r.flag) return false;
    if(l.createdSince != r.createdSince) return false;
    if(l.transactionNumber != r.transactionNumber) return false;

    return true;
}

bool operator==(const RsNxsSyncMsg& l, const RsNxsSyncMsg& r)
{

    if(l.flag != r.flag) return false;
    if(l.createdSince != r.createdSince) return false;
    if(l.syncHash != r.syncHash) return false;
    if(l.grpId != r.grpId) return false;
    if(l.transactionNumber != r.transactionNumber) return false;

    return true;
}

bool operator==(const RsNxsSyncGrpItem& l, const RsNxsSyncGrpItem& r)
{
    if(l.flag != r.flag) return false;
    if(l.publishTs != r.publishTs) return false;
    if(l.grpId != r.grpId) return false;
    if(l.transactionNumber != r.transactionNumber) return false;

    return true;
}

bool operator==(const RsNxsSyncMsgItem& l, const RsNxsSyncMsgItem& r)
{
    if(l.flag != r.flag) return false;
    if(l.grpId != r.grpId) return false;
    if(l.msgId != r.msgId) return false;
    if(l.transactionNumber != r.transactionNumber) return false;

    return true;
}

bool operator==(const RsNxsTransac& l, const RsNxsTransac& r){

    if(l.transactFlag != r.transactFlag) return false;
    if(l.transactionNumber != r.transactionNumber) return false;
    if(l.timestamp != r.timestamp) return false;
    if(l.nItems != r.nItems) return false;


    return true;
}

