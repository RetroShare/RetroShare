/*******************************************************************************
 * unittests/libretroshare/gxs/data_service/rsdataservice_test.h               *
 *                                                                             *
 * Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/

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
