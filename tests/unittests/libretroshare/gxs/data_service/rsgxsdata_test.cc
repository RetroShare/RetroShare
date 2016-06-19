
#include <gtest/gtest.h>

#include "libretroshare/serialiser/support.h"
#include "libretroshare/gxs/common/data_support.h"
#include "gxs/rsgxsdata.h"

bool testEqual(const RsGxsGrpMetaData& d1,const RsGxsGrpMetaData& d2)
{
    if(d1.mGroupId      !=  d2.mGroupId       ) return false ;   
    if(d1.mOrigGrpId    !=  d2.mOrigGrpId     ) return false ;
    if(d1.mGroupName    !=  d2.mGroupName     ) return false ;
    if(d1.mGroupFlags   !=  d2.mGroupFlags    ) return false ; 
    if(d1.mPublishTs    !=  d2.mPublishTs     ) return false ;
    if(d1.mSignFlags    !=  d2.mSignFlags     ) return false ;
    if(d1.mAuthorId     !=  d2.mAuthorId      ) return false ;
    if(d1.mCircleId     !=  d2.mCircleId      ) return false ;
    if(d1.mCircleType   !=  d2.mCircleType    ) return false ; 
    if(d1.mServiceString!=  d2.mServiceString ) return false ;    
    if(d1.mAuthenFlags  !=  d2.mAuthenFlags   ) return false ;  
    if(d1.mParentGrpId  !=  d2.mParentGrpId   ) return false ;  
    
    // if(d1.signSet       !=  d2.signSet        ) return false ;
    // if(d1.keys          !=  d2.keys           ) return false ;
    
    return true ;
}
bool testEqual(const RsGxsMsgMetaData& d1,const RsGxsMsgMetaData& d2)
{
    if(d1.mGroupId      !=  d2.mGroupId       ) return false ;   
    if(d1.mMsgId        !=  d2.mMsgId         ) return false ;
    if(d1.refcount      !=  d2.refcount       ) return false ;
    if(d1.mThreadId     !=  d2.mThreadId      ) return false ; 
    if(d1.mPublishTs    !=  d2.mPublishTs     ) return false ;
    if(d1.mParentId     !=  d2.mParentId      ) return false ;
    if(d1.mOrigMsgId    !=  d2.mOrigMsgId     ) return false ;
    if(d1.mAuthorId     !=  d2.mAuthorId      ) return false ;
    if(d1.mServiceString!=  d2.mServiceString ) return false ;    
    if(d1.mMsgName      !=  d2.mMsgName       ) return false ;  
    if(d1.mPublishTs    !=  d2.mPublishTs     ) return false ;  
    if(d1.mMsgFlags     !=  d2.mMsgFlags      ) return false ;  
    
    // if(d1.signSet       !=  d2.signSet        ) return false ;
    
    return true ;
}
TEST(libretroshare_gxs, RsGxsData)
{

    RsGxsGrpMetaData grpMeta1, grpMeta2;
    RsGxsMsgMetaData msgMeta1, msgMeta2;

    grpMeta1.clear();
    init_item(&grpMeta1);

    msgMeta1.clear();
    init_item(&msgMeta1);

    uint32_t pktsize = grpMeta1.serial_size(RS_GXS_GRP_META_DATA_CURRENT_API_VERSION);
    char grp_data[pktsize];

    bool ok = true;

    ok &= grpMeta1.serialise(grp_data, pktsize, RS_GXS_GRP_META_DATA_CURRENT_API_VERSION);
    grpMeta2.clear();
    ok &= grpMeta2.deserialise(grp_data, pktsize);

    EXPECT_TRUE(testEqual(grpMeta1 , grpMeta2));

    pktsize = msgMeta1.serial_size();
    char msg_data[pktsize];

    ok &= msgMeta1.serialise(msg_data, &pktsize);
    msgMeta2.clear();
    ok &= msgMeta2.deserialise(msg_data, &pktsize);

    EXPECT_TRUE(testEqual(msgMeta1 , msgMeta2));
}


