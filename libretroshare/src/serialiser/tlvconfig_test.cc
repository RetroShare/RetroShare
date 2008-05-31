
/*
 * libretroshare/src/serialiser: tlvconfig_test.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Chris Parker.
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

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvutil.h"
#include "util/utest.h"

INITTEST();

static int testRsTlvPeerIdSet();
static int testRsTlvServiceIdSet();
static int testRsTlvKeyValue();
static int testRsTlvKeyValueSet();


int main()
{
	std::cerr << "RsTlvConfig[Item/Data/...] Tests" << std::endl;

	testRsTlvPeerIdSet();
	testRsTlvServiceIdSet();//tbd
	testRsTlvKeyValue();//tbd
	testRsTlvKeyValueSet();//tbd

	
	FINALREPORT("RsTlvConfig[Item/Data/...] Tests");

	return TESTRESULT();
}

int testRsTlvPeerIdSet()
{

	RsTlvPeerIdSet i1, i2; // one to set and other to get

	std::string testString;

	std::string randString[5];
	randString[0] = "e$424!£!£";
	randString[1] = "e~:@L{L{KHKG";
	randString[2] = "e{@O**/*/*";
	randString[3] = "e?<<BNMB>HG£!£%$";
	randString[4] = "e><?<NVBCEE£$$%*^";

	/* store a number of random ids */

	for(int i = 0; i < 15 ; i++)
	{
		testString = randString[(rand() % 4)] + randString[(rand() % 4)];

		i1.ids.push_back(testString);
	}
		
	CHECK(test_SerialiseTlvItem(std::cerr, &i1, &i2));

	/*check that the data is the same*/

	REPORT("Serialize/Deserialize RsTlvPeerIdSet");

	return 1;
}

int testRsTlvServiceIdSet()
{		
	RsTlvServiceIdSet i1, i2; // one to set and other to get

	
	/* store random numbers */
	for(int i = 0; i < 15 ; i++)
	{
		i1.ids.push_back(1 + rand() % 12564);
	}
	std::cout << "error here!!!?";	
	CHECK(test_SerialiseTlvItem(std::cerr, &i1, &i2));

	/*check that the data is the same*/
	REPORT("Serialize/Deserialize RsTlvServiceIdSet");
	return 1;
}


int testRsTlvKeyValue()
{
	RsTlvKeyValue i1, i2; // one to set and other to get

	i1.key = "whatever";
	i1.value = "better work";

	CHECK(test_SerialiseTlvItem(std::cerr, &i1, &i2));

	/*check that the data is the same*/
	REPORT("Serialize/Deserialize RsTlvKeyValue");

	return 1;
}
int testRsTlvKeyValueSet()
{	
	RsTlvKeyValueSet i1, i2; // one to set and other to get

	/* instantiate the objects values */

	std::string randString[5];
	randString[0] = "e$424!£!£";
	randString[1] = "e~:@L{L{KHKG";
	randString[2] = "e{@O**/*/*";
	randString[3] = "e?<<BNMB>HG£!£%$";
	randString[4] = "e><?<NVBCEE£$$%*^";
  
	for(int i = 0; i < 15; i++)
	{
		RsTlvKeyValue kv;

		kv.key = randString[(rand() % 4)] + randString[(rand() % 4)];
		kv.value = randString[(rand() % 4)] + randString[(rand() % 4)];

		i1.pairs.push_back(kv);

	}

	CHECK(test_SerialiseTlvItem(std::cerr, &i1, &i2));

	/*check that the data is the same*/
	REPORT("Serialize/Deserialize RsTlvKeyValueSet");
}


