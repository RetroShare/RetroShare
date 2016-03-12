
#include <gtest/gtest.h>

#include "libretroshare/serialiser/support.h"
#include "libretroshare/gxs/common/data_support.h"
#include "gxs/rsgxsdata.h"

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

    EXPECT_TRUE(grpMeta1 == grpMeta2);

    pktsize = msgMeta1.serial_size();
    char msg_data[pktsize];

    ok &= msgMeta1.serialise(msg_data, &pktsize);
    msgMeta2.clear();
    ok &= msgMeta2.deserialise(msg_data, &pktsize);

    EXPECT_TRUE(msgMeta1 == msgMeta2);
}


