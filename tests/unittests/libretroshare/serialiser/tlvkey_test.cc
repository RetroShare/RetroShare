
#include <gtest/gtest.h>

#include "support.h"
#include "serialiser/rstlvkeys.h"

TEST(libretroshare_serialiser, test_RsTlvKeySignatureSet)
{
    RsTlvKeySignatureSet set;

    init_item(set);

    char data[set.TlvSize()];
    uint32_t offset = 0;
    set.SetTlv(data, set.TlvSize(), &offset);

    RsTlvKeySignatureSet setConfirm;

    offset = 0;
    setConfirm.GetTlv(data, set.TlvSize(), &offset);

    EXPECT_TRUE(setConfirm == set);

}
