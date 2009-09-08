
/*
 * libretroshare/src/serialiser: tlvrandom_test.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2009 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

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

#include <string.h>
#include <iostream>
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvkeys.h"
#include "serialiser/rstlvkvwide.h"
#include "serialiser/rstlvutil.h"

#include "util/utest.h"

INITTEST();

#define TEST_LENGTH 30

static int test_TlvRandom(void *data, uint32_t len, uint32_t offset);

int main()
{
	std::cerr << "TlvRandom Tests" << std::endl;

	/* random data array to work through */
	uint32_t dsize = 10000000;
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

	srandom(startTs);
	for(i = 0; i < dsize; i++)
	{
		data[i] = random() % 256;
	}

	std::cerr << "TlvRandom Tests: setup data." << std::endl;

	int count = 0;
	for(i = 0; endTs > time(NULL); i += 2)
	{
		uint32_t len = dsize - i;
		count += test_TlvRandom(&(data[i]), len, i);

		std::cerr << "Run: " << count << " tests";
		std::cerr << std::endl;
	}

	FINALREPORT("RsTlvItems Tests");

	return TESTRESULT();
}

#define BIN_LEN 523456  /* bigger than 64k */

int test_TlvItem(RsTlvItem *item, void *data, uint32_t size, uint32_t offset);
int test_SetTlvItem(RsTlvItem *item, uint16_t type, void *data, uint32_t size, uint32_t offset);


int test_TlvRandom(void *data, uint32_t len, uint32_t offset)
{
	uint32_t tmpoffset = 0;

	/* List of all the TLV types it could be! */
	RsTlvSecurityKey skey;
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
	RsTlvKeyValueWide   kvwide;
	RsTlvKeyValueWideSet   kvwideset;

	RsTlvImage   	    image;
   
	/* try to decode - with all types first */
	std::cerr << "test_TlvRandom:: Testing Files " << std::endl;
	test_TlvItem(&bindata, data, len, offset);
	test_TlvItem(&fileitem, data, len, offset);
	test_TlvItem(&fileset, data, len, offset);
	test_TlvItem(&filedata, data, len, offset);
	std::cerr << "test_TlvRandom:: Testing Sets " << std::endl;
	test_TlvItem(&peerset, data, len, offset);
	test_TlvItem(&servset, data, len, offset);
	test_TlvItem(&kv, data, len, offset);
	test_TlvItem(&kvset, data, len, offset);
	test_TlvItem(&kvwide, data, len, offset);
	test_TlvItem(&kvwideset, data, len, offset);
	std::cerr << "test_TlvRandom:: Testing Keys " << std::endl;
	test_TlvItem(&skey, data, len, offset);
	test_TlvItem(&skeyset, data, len, offset);
	test_TlvItem(&keysign, data, len, offset);

	/* now set the type correctly before decoding */
	std::cerr << "test_TlvRandom:: Testing Files (TYPESET)" << std::endl;
	test_SetTlvItem(&bindata, TLV_TYPE_IMAGE, data, len, offset);
	test_SetTlvItem(&fileitem,TLV_TYPE_FILEITEM, data, len, offset);
	test_SetTlvItem(&fileset, TLV_TYPE_FILESET, data, len, offset);
	test_SetTlvItem(&filedata, TLV_TYPE_FILEDATA, data, len, offset);
	std::cerr << "test_TlvRandom:: Testing Sets (TYPESET)" << std::endl;
	test_SetTlvItem(&peerset, TLV_TYPE_PEERSET, data, len, offset);
	test_SetTlvItem(&servset, TLV_TYPE_SERVICESET, data, len, offset);
	test_SetTlvItem(&kv, TLV_TYPE_KEYVALUE, data, len, offset);
	test_SetTlvItem(&kvset, TLV_TYPE_KEYVALUESET, data, len, offset);
	test_SetTlvItem(&kvwide, TLV_TYPE_WKEYVALUE, data, len, offset);
	test_SetTlvItem(&kvwideset, TLV_TYPE_WKEYVALUESET, data, len, offset);
	std::cerr << "test_TlvRandom:: Testing Keys (TYPESET)" << std::endl;
	test_SetTlvItem(&skey, TLV_TYPE_SECURITYKEY, data, len, offset);
	test_SetTlvItem(&skeyset, TLV_TYPE_SECURITYKEYSET, data, len, offset);
	test_SetTlvItem(&keysign, TLV_TYPE_KEYSIGNATURE, data, len, offset);

	return 26; /* number of tests */
}

int test_TlvItem(RsTlvItem *item, void *data, uint32_t size, uint32_t offset)
{
	uint32_t tmp_offset = offset;
	if (item->GetTlv(data, size, &tmp_offset))
	{
		std::cerr << "TLV decoded Random!";
		std::cerr << std::endl;
		item->print(std::cerr, 20);
	}
	else
	{
		std::cerr << "TLV failed to decode";
		std::cerr << std::endl;
	}
}

int test_SetTlvItem(RsTlvItem *item, uint16_t type, void *data, uint32_t size, uint32_t offset)
{
	/* set TLV type first! */
	void *typedata = (((uint8_t *) data) + offset);
	SetTlvType(typedata, size - offset, type);

	return test_TlvItem(item, data, size, offset);
}


