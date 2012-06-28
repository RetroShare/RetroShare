
#include "support.h"
#include "data_support.h"
#include "gxs/rsgxsdata.h"
#include "util/utest.h"

INITTEST();

bool operator ==(const RsGxsGrpMetaData& l, const RsGxsGrpMetaData& r);
bool operator ==(const RsGxsMsgMetaData& l, const RsGxsMsgMetaData& r);

int main()
{

    RsGxsGrpMetaData grpMeta1, grpMeta2;
    RsGxsMsgMetaData msgMeta1, msgMeta2;

    grpMeta1.clear();
    init_item(&grpMeta1);

    msgMeta1.clear();
    init_item(&msgMeta1);

    uint32_t pktsize = grpMeta1.serial_size();
    char grp_data[pktsize];

    bool ok = true;

    ok &= grpMeta1.serialise(grp_data, pktsize);
    grpMeta2.clear();
    ok &= grpMeta2.deserialise(grp_data, pktsize);

    CHECK(grpMeta1 == grpMeta2);

    pktsize = msgMeta1.serial_size();
    char msg_data[pktsize];

    ok &= msgMeta1.serialise(msg_data, &pktsize);
    msgMeta2.clear();
    ok &= msgMeta2.deserialise(msg_data, &pktsize);

    CHECK(msgMeta1 == msgMeta2);

    FINALREPORT("GxsMeta Data Test");

    return TESTRESULT();
}


bool operator ==(const RsGxsGrpMetaData& l, const RsGxsGrpMetaData& r)
{
    if(!(l.adminSign == r.adminSign)) return false;
    if(!(l.idSign == r.idSign)) return false;
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

    if(!(l.idSign == r.idSign)) return false;
    if(!(l.pubSign == r.pubSign)) return false;
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
