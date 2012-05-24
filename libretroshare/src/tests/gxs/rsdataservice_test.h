#ifndef RSDATASERVICE_TEST_H
#define RSDATASERVICE_TEST_H

#include "util/rsthreads.h"

bool test_messageStoresAndRetrieve();
bool test_messageVersionRetrieve();

bool test_groupStoreAndRetrieve();
bool test_groupVersionRetrieve();

bool test_storeAndDeleteGroup();
bool test_storeAndDeleteMessage();

bool test_searchMsg();
bool test_searchGrp();

bool test_multiThreaded();

class DataReadWrite : RsThread
{



};

bool test_cacheSize();

#endif // RSDATASERVICE_TEST_H
