#ifndef RSDATASERVICE_TEST_H
#define RSDATASERVICE_TEST_H

#include "util/rsthreads.h"
#include "rsitems/rsnxsitems.h"
#include "gxs/rsgds.h"

void test_messageStoresAndRetrieve();

void test_groupStoreAndRetrieve();

void test_storeAndDeleteGroup();
void test_storeAndDeleteMessage();

void test_searchMsg();
void test_searchGrp();

bool operator ==(const RsGxsGrpMetaData& l, const RsGxsGrpMetaData& r);
bool operator ==(const RsGxsMsgMetaData& l, const RsGxsMsgMetaData& r);

void test_multiThreaded();

class DataReadWrite : RsThread
{



};

void test_cacheSize();


#endif // RSDATASERVICE_TEST_H
