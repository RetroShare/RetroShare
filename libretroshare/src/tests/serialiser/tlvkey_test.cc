

#include "support.h"
#include "serialiser/rstlvkeys.h"

INITTEST();

bool test_RsTlvKeySignatureSet();

int main()
{
    test_RsTlvKeySignatureSet(); REPORT("test_RsTlvKeySignatureSet()");

    FINALREPORT("RsTlvKey Test");
}



bool test_RsTlvKeySignatureSet()
{
    RsTlvKeySignatureSet set;

    init_item(set);

    char data[set.TlvSize()];
    uint32_t offset = 0;
    set.SetTlv(data, set.TlvSize(), &offset);

    RsTlvKeySignatureSet setConfirm;

    offset = 0;
    setConfirm.GetTlv(data, set.TlvSize(), &offset);

    CHECK(setConfirm == set);

}
