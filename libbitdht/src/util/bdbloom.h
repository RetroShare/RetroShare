#ifndef BITDHT_BLOOM_H
#define BITDHT_BLOOM_H

/*
 * bitdht/bdbloom.h
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2011 by Robert Fernie
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


#include <vector>
#include <string>

#include <iostream>
#include <inttypes.h>


class bloomFilter
{
	public:

	bloomFilter(int m, int k);

int 	setFilterBits(const std::string &hex);
std::string getFilter();

void 	printFilter(std::ostream &out);

bool 	test(const std::string &hex);  // takes first m bits.
void 	add(const std::string &hex);
uint32_t countBits();
uint32_t filterBits();

	protected:
void 	setHashFunction(int idx,  uint32_t (*hashfn)(const std::string &));

	private:
void 	setBit(int bit);
bool 	isBitSet(int bit);

	std::vector<uint8_t> mBits;
	std::vector<uint32_t (*)(const std::string &)> mHashFns;

	uint32_t mFilterBits;
	uint32_t mNoHashs;
	uint32_t mNoElements;
};


/* our specific implementation */
class bdBloom: public bloomFilter
{
	public:

	bdBloom();
};


#endif

