
/*
 * libretroshare/src/serialiser: tlvitems_test.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie.
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
 * tlvfileitem test.
 *
 *
 */

#include <iostream>
#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvutil.h"
#include "util/utest.h"

INITTEST();

static int test_RsTlvBinData();
static int test_RsTlvStepping();

int main()
{
	std::cerr << "RsTlvItems Tests" << std::endl;

	test_RsTlvBinData(); 
	
	FINALREPORT("RsTlvItems Tests");

	return TESTRESULT();
}

#define BIN_LEN 523456  /* bigger than 64k */

int test_RsTlvBinData()
{
	RsTlvBinaryData  d1(1023);
	RsTlvBinaryData  d2(1023);

	char data[BIN_LEN] = {0};
	int i, j;
	for(i = 0; i < BIN_LEN; i++)
	{
		data[i] = i%13;
	}

	for(j = 1; j < BIN_LEN; j *= 2)
	{
		d1.setBinData(data, j);
		CHECK(test_SerialiseTlvItem(std::cerr, &d1, &d2));

		CHECK(d1.bin_len == d2.bin_len);
		CHECK(0 == memcmp(d1.bin_data, d2.bin_data, d1.bin_len));
	}

	REPORT("Serialise/Deserialise RsTlvBinData");

	return 1;
}

int test_RsTlvStepping()
{


	return 1;
}

