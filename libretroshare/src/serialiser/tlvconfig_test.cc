
/******************************************************************
 * tlvfileitem test.
 *
 *
 */

#include <iostream>
#include <sstream>
#include "rstlvtypes.h"
#include "rstlvutil.h"
#include "utest.h"

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


