#include "support.h"
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


void init_item(RsNxsGrp& nxg)
{

    nxg.clear();

    randString(SHORT_STR, nxg.grpId);
    nxg.transactionNumber = rand()%23;
    init_item(nxg.grp);
    init_item(nxg.meta);
    return;
}





void init_item(RsNxsMsg& nxm)
{
    nxm.clear();

    randString(SHORT_STR, nxm.msgId);
    randString(SHORT_STR, nxm.grpId);
    init_item(nxm.msg);
    init_item(nxm.meta);
    nxm.transactionNumber = rand()%23;

    return;
}

void init_item(RsGxsGrpMetaData* metaGrp)
{

    randString(SHORT_STR, metaGrp->mGroupId);
    randString(SHORT_STR, metaGrp->mOrigGrpId);
    randString(SHORT_STR, metaGrp->mAuthorId);
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

    randString(SHORT_STR, metaMsg->mGroupId);
    randString(SHORT_STR, metaMsg->mMsgId);
    randString(SHORT_STR, metaMsg->mThreadId);
    randString(SHORT_STR, metaMsg->mParentId);
    randString(SHORT_STR, metaMsg->mAuthorId);
    randString(SHORT_STR, metaMsg->mOrigMsgId);
    randString(SHORT_STR, metaMsg->mMsgName);

    init_item(metaMsg->signSet);

    metaMsg->mPublishTs = rand()%313;
    metaMsg->mMsgFlags = rand()%224;
    metaMsg->mMsgStatus = rand()%4242;
    metaMsg->mChildTs = rand()%221;
}
