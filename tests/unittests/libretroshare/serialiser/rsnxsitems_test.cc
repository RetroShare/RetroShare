
#include "support.h"
#include "libretroshare/gxs/common/data_support.h"
#include "rsitems/rsnxsitems.h"


#define NUM_BIN_OBJECTS 5
#define NUM_SYNC_MSGS 8
#define NUM_SYNC_GRPS 5

TEST(libretroshare_serialiser, RsNxsItem)
{
    test_RsItem<RsNxsGrp>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsNxsMsg>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsNxsSyncGrpItem>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsNxsSyncMsgItem>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsNxsSyncGrpItem>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsNxsSyncMsgItem>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsNxsTransacItem>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}
