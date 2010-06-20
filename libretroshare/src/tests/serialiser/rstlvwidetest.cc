/*
 * libretroshare/src/serialiser: rstlvkvwidetest.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Chris Parker
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
 

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include "serialiser/rstlvkvwide.h"
#include "serialiser/rstlvutil.h"
#include "util/utest.h"

 
INITTEST();

 /* define utility functions to test out with */
 
 //static int testRstlvWide();
static int testRsTlvWideSet();

int main()
{
	std::cerr << "RsTlvWideTest[Item/Data/...] Tests" << std::endl;

	testRsTlvWideSet();

	FINALREPORT("RsTlvWideTest[Item/Data/...] Tests");

	return TESTRESULT();
}

int testRsTlvWideSet()
{
 	RsTlvKeyValueWideSet i1, i2; // one to set and other to get
 	RsTlvKeyValueWide i_pair; // input pair
 	RsTlvFileItem hello;

	std::string randString("it should work now.");
	int j, k;
	
	std::cerr << "entering loop now" << std::endl;
	/* store a 15 random pairs */

	for(int i = 0; i < 15 ; i++)
	{	
		j = rand() % 4;
		k = rand() % 4;
		std::cerr << "j: " << j << " k: " << k << std::endl;
		i_pair.wKey.assign(randString.begin(), randString.end()); 
		std::cerr << "loop count:" << i << std::endl;
		i_pair.wValue.assign(randString.begin(), randString.end()); 
		std::cerr << "loop count:" << i << std::endl;
		i1.wPairs.push_back(i_pair);
		
		i_pair.TlvClear();
	}
	
	std::cerr << "out of loop now" << std::endl;
		
	CHECK(test_SerialiseTlvItem(std::cerr, &i1, &i2));

	/*check that the data is the same*/

	REPORT("Serialize/Deserialize RsTlvKeyValueWideSet");

	return 1;
}
