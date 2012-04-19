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
	/* read from the file */
	if (argc < 2)
	{
		std::cerr << "Missing Hash File";
		std::cerr << std::endl;
	}

	FILE *fd = fopen(argv[1], "r");
	if (!fd)
	{
		std::cerr << "Failed to Open File: " << argv[1];
		std::cerr << std::endl;
		return 1;
	}

	char line[1000];

	bdBloom filter;

	int nHashes = 0;
	while(fgets(line, 1000-1, fd))
	{
		std::string hash = line;
		std::cerr << "Read Hash: " << hash;
		std::cerr << std::endl;

		filter.add(hash);
		nHashes++;
	}
	fclose(fd);

	filter.printFilter(std::cerr);

	int bits = filter.countBits();
	int filterBits = filter.filterBits();
	std::cerr << "Filter Bits Set: " << bits;
	std::cerr << std::endl;

	double possible = ((double) bits) * bits;
	double max = ((double) filterBits) * filterBits;
	std::cerr << "Therefore Possible Finds: " << possible << "/" << max << " = %" << 100 * possible / max;
	std::cerr << std::endl;
	std::cerr << "With Insertions: " << nHashes;
	std::cerr << std::endl;

	std::cerr << std::endl;
	std::cerr << std::endl;

	std::cerr << "Filter String: " << filter.getFilter() << std::endl;
	std::cerr << std::endl;
	std::cerr << std::endl;

	return 1;
}


