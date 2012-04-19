/*
 * bitdht/bdbloom_test.cc
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2010 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 3 as published by the Free Software Foundation.
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
 * Please report all bugs and problems to "bitdht@lunamutt.com".
 *
 */


#include "util/bdbloom.h"
#include "bitdht/bdstddht.h"
#include <iostream>

#define N_TESTS 100

int main(int argc, char **argv)
{

	/* Do this multiple times */
	int i;


	bdBloom filter;

	std::list<bdNodeId> testIds;
	std::list<bdNodeId>::iterator it;

	for(i = 0; i < N_TESTS; i++)
	{
		bdNodeId targetId;
		bdStdRandomNodeId(&targetId);
		testIds.push_back(targetId);
	}

	std::cerr << "Test bdBloom Filter...." << std::endl;
	for(it = testIds.begin(); it != testIds.end(); it++)
	{
		bdNodeId targetId = *it;
	
		std::cerr << "-------------------------------------------------" << std::endl;
		std::cerr << "Inserting : ";
		std::ostringstream str;
		bdStdPrintNodeId(str, &targetId);
		std::cerr << str.str();
		std::cerr << std::endl;

		filter.add(str.str());

		filter.printFilter(std::cerr);
	}

	std::string fs1 = filter.getFilter();
	/* now extract, and reinsert filter */
	bdBloom filter2;
	filter2.setFilterBits(fs1);

	std::string fs2 = filter2.getFilter();
	filter2.printFilter(std::cerr);

	if (fs1 == fs2)
	{
		std::cerr << "SUCCESS: Filter Correctly Transferred!";
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "FAILURE: Filter Not Transferred!";
		std::cerr << std::endl;
	}


	for(it = testIds.begin(); it != testIds.end(); it++)
	{
		bdNodeId targetId = *it;
	
		std::cerr << "-------------------------------------------------" << std::endl;
		std::cerr << "Testing : ";
		std::cerr << std::endl;

		std::ostringstream str;
		bdStdPrintNodeId(str, &targetId);
		std::cerr << str.str() << std::endl;

		if (filter2.test(str.str()))
		{
			std::cerr << "SUCCESS: Filter Found Entry";
			std::cerr << std::endl;
		}
		else
		{
			std::cerr << "FAILURE: Filter Didn't Found Entry";
			std::cerr << std::endl;
		}
	}

	int bits = filter.countBits();
	int filterBits = filter.filterBits();
	std::cerr << "Filter Bits Set: " << bits;
	std::cerr << std::endl;

	double possible = ((double) bits) * bits;
	double max = ((double) filterBits) * filterBits;
	std::cerr << "Therefore Possible Finds: " << possible << "/" << max << " = %" << 100 * possible / max;
	std::cerr << std::endl;
	std::cerr << "With Insertions: " << N_TESTS;
	std::cerr << std::endl;

#define FINAL_TESTS	(1000000)
	int found = 0;
	int didnt = 0;
	for(i = 0; i < FINAL_TESTS; i++)
	{
		if ((i != 0) && (i % 100000 == 0))
		{
			std::cerr << "Run " << i << " Checks" << std::endl;
		}

		bdNodeId targetId;
		bdStdRandomNodeId(&targetId);
		std::ostringstream str;
		bdStdPrintNodeId(str, &targetId);

		if (filter2.test(str.str()))
		{
			found++;
		}
		else
		{
			didnt++;
		}
	}

	std::cerr << "Final Stats: " << FINAL_TESTS << " random checks done";
	std::cerr << std::endl;
	std::cerr << "\t" << found << "   " << 100.0 * ((double) found) / FINAL_TESTS << "% found";
	std::cerr << std::endl;
	std::cerr << "\t" << didnt << "   " << 100.0 * ((double) didnt) / FINAL_TESTS << "% didnt";
	std::cerr << std::endl;


	
		

	return 1;
}


