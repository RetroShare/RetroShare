#ifndef RSDATASERVICE_TEST_H
#define RSDATASERVICE_TEST_H

#include "util/rsthreads.h"
#include "serialiser/rsnxsitems.h"
#include "gxs/rsgds.h"

void test_messageStoresAndRetrieve();
void test_messageVersionRetrieve();

void test_groupStoreAndRetrieve();
void test_groupVersionRetrieve();

void test_storeAndDeleteGroup();
void test_storeAndDeleteMessage();

void test_searchMsg();
void test_searchGrp();

void test_multiThreaded();

class DataReadWrite : RsThread
{



};

void test_cacheSize();

void init_item(RsNxsGrp*);
void init_item(RsNxsMsg*);

#endif // RSDATASERVICE_TEST_H
