/*******************************************************************************
 * unittests/libretroshare/serialiser/tlvrandom_test.cc                        *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie <retroshare.project@gmail.com>         *
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

/******************************************************************
 * tlvrandom_test.
 *
 * This test is designed to attempt to break the TLV serialiser.
 * 
 * To do this we throw random data at the serialisers and try to decode it.
 * As the serialiser will only attempt to deserialise if the tlvtype matches
 * we cheat a little, and make this match - to increase to actual deserialise
 * attempts.
 *
 * This test runs for 30 seconds and attempts to do as 
 * many deserialisation as possible.
 */

#include <gtest/gtest.h>

#include <time.h>
#include <string.h>
#include <iostream>
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvkeys.h"
#include "serialiser/rstlvbinary.h"
#include "serialiser/rstlvidset.h"
#include "serialiser/rstlvfileitem.h"
#include "serialiser/rstlvkeyvalue.h"
#include "serialiser/rstlvimage.h"
#include "rstlvutil.h"

#define TEST_LENGTH 10
// more time for valgrind
//#define TEST_LENGTH 500


#define BIN_LEN 523456  /* bigger than 64k */

bool test_TlvItem(RsTlvItem *item, void *data, uint32_t size, uint32_t offset);
bool test_SetTlvItem(RsTlvItem *item, uint16_t type, void *data, uint32_t size, uint32_t offset);


int test_TlvRandom(void *data, uint32_t len, uint32_t offset)
{
	//uint32_t tmpoffset = 0;

	/* List of all the TLV types it could be! */
	RsTlvPublicRSAKey skey;
	RsTlvSecurityKeySet skeyset;
	RsTlvKeySignature   keysign;

	RsTlvBinaryData	    bindata(TLV_TYPE_IMAGE);

	RsTlvFileItem	    fileitem;
	RsTlvFileSet	    fileset;
	RsTlvFileData	    filedata;

	RsTlvPeerIdSet	    peerset;
	RsTlvServiceIdSet   servset;

	RsTlvKeyValue       kv;
	RsTlvKeyValueSet    kvset;

	RsTlvImage   	    image;
   
	/* try to decode - with all types first */
	std::cerr << "test_TlvRandom:: Testing Files " << std::endl;
	EXPECT_TRUE(test_TlvItem(&bindata, data, len, offset));
	EXPECT_TRUE(test_TlvItem(&fileitem, data, len, offset));
	EXPECT_TRUE(test_TlvItem(&fileset, data, len, offset));
	EXPECT_TRUE(test_TlvItem(&filedata, data, len, offset));
	std::cerr << "test_TlvRandom:: Testing Sets " << std::endl;
	EXPECT_TRUE(test_TlvItem(&peerset, data, len, offset));
	EXPECT_TRUE(test_TlvItem(&servset, data, len, offset));
	EXPECT_TRUE(test_TlvItem(&kv, data, len, offset));
	EXPECT_TRUE(test_TlvItem(&kvset, data, len, offset));
	std::cerr << "test_TlvRandom:: Testing Keys " << std::endl;
	EXPECT_TRUE(test_TlvItem(&skey, data, len, offset));
	EXPECT_TRUE(test_TlvItem(&skeyset, data, len, offset));
	EXPECT_TRUE(test_TlvItem(&keysign, data, len, offset));

	/* now set the type correctly before decoding */
	std::cerr << "test_TlvRandom:: Testing Files (TYPESET)" << std::endl;
	EXPECT_TRUE(test_SetTlvItem(&bindata, TLV_TYPE_IMAGE, data, len, offset));
	EXPECT_TRUE(test_SetTlvItem(&fileitem,TLV_TYPE_FILEITEM, data, len, offset));
	EXPECT_TRUE(test_SetTlvItem(&fileset, TLV_TYPE_FILESET, data, len, offset));
	EXPECT_TRUE(test_SetTlvItem(&filedata, TLV_TYPE_FILEDATA, data, len, offset));
	std::cerr << "test_TlvRandom:: Testing Sets (TYPESET)" << std::endl;
	EXPECT_TRUE(test_SetTlvItem(&peerset, TLV_TYPE_PEERSET, data, len, offset));
	EXPECT_TRUE(test_SetTlvItem(&servset, TLV_TYPE_SERVICESET, data, len, offset));
	EXPECT_TRUE(test_SetTlvItem(&kv, TLV_TYPE_KEYVALUE, data, len, offset));
	EXPECT_TRUE(test_SetTlvItem(&kvset, TLV_TYPE_KEYVALUESET, data, len, offset));
	std::cerr << "test_TlvRandom:: Testing Keys (TYPESET)" << std::endl;
	EXPECT_TRUE(test_SetTlvItem(&skey, TLV_TYPE_SECURITY_KEY, data, len, offset));
	EXPECT_TRUE(test_SetTlvItem(&skeyset, TLV_TYPE_SECURITYKEYSET, data, len, offset));
	EXPECT_TRUE(test_SetTlvItem(&keysign, TLV_TYPE_KEYSIGNATURE, data, len, offset));

	return 26; /* number of tests */
}

bool test_TlvItem(RsTlvItem *item, void *data, uint32_t size, uint32_t offset)
{
	uint32_t tmp_offset = offset;
	if (item->GetTlv(data, size, &tmp_offset))
	{
		std::cerr << "TLV decoded Random!";
		std::cerr << std::endl;
		item->print(std::cerr, 20);
		return false;
	}
	else
	{
		std::cerr << "TLV failed to decode";
		std::cerr << std::endl;
		return true;
	}
}

bool test_SetTlvItem(RsTlvItem *item, uint16_t type, void *data, uint32_t size, uint32_t offset)
{
	/* set TLV type first! */
	void *typedata = (((uint8_t *) data) + offset);
	SetTlvType(typedata, size - offset, type);

	return test_TlvItem(item, data, size, offset);
}


TEST(libretroshare_serialiser, DISABLED_test_RsTlvRandom)
{
	/* random data array to work through */
	uint32_t dsize = 100000;
	uint32_t i;
	uint8_t *data = (uint8_t *) malloc(dsize);

	if (!data)
	{
		std::cerr << "Failed to allocate array";
		std::cerr << std::endl;
		exit(1);
	}

	time_t startTs = time(NULL);
	time_t endTs = startTs + TEST_LENGTH;

	srand(startTs);
	for(i = 0; i < dsize; i++)
	{
		data[i] = rand() % 256;
	}

	std::cerr << "TlvRandom Tests: setup data." << std::endl;

	int count = 0;
	for(i = 0; endTs > time(NULL); i += 2)
	{
        uint32_t len = dsize - 2*i; // two times i, because we also use it as offset

        // no point in testing smaller than header size,
        // because items currently don't check if they can read the header
        if(len < TLV_HEADER_SIZE)
        {
            std::cerr << "reached the end of our datablock!";
            std::cerr << std::endl;
            return;
        }
		count += test_TlvRandom(&(data[i]), len, i);

		std::cerr << "Run: " << count << " tests";
		std::cerr << std::endl;
	}
}
