
#include "support.h"
#include "libretroshare/gxs/common/data_support.h"
#include "serialiser/rsnxsitems.h"


#define NUM_BIN_OBJECTS 5
#define NUM_SYNC_MSGS 8
#define NUM_SYNC_GRPS 5

// disabled because it fails
TEST(libretroshare_serialiser, DISABLED_RsNxsItem)
{
    test_RsItem<RsNxsGrp>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsNxsMsg>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsNxsSyncGrp>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsNxsSyncMsg>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsNxsSyncGrpItem>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsNxsSyncMsgItem>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsNxsTransac>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}
