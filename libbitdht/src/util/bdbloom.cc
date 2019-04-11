/*******************************************************************************
 * util/bdbloom.cc                                                             *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright 2011 by Robert Fernie <bitdht@lunamutt.com>                       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "util/bdbloom.h"
#include "util/bdstring.h"

#include <stdlib.h>
#include <iomanip>

#if defined(_WIN32) || defined(__MINGW32__)
#include <malloc.h>
#endif

/* Bloom Filter implementation */


bloomFilter::bloomFilter(int m, int k)
{
	mBits.resize(m);
	mHashFns.resize(k);

	mFilterBits = m;
	mNoHashs = k;
	mNoElements = 0;

	int i;
	for(i = 0; i < m; i++)
	{
		mBits[i] = 0;
	}

	for(i = 0; i < k; i++)
	{
		mHashFns[i] = NULL;
	}

}

uint8_t convertCharToUint8(char ch1, char ch2)
{
	uint8_t value1 = 0;
	uint8_t value2 = 0;
	
	/* do char1 */
	if (ch1 >= '0' && ch1 <= '9')
	value1 = (ch1 - '0');
	else if (ch1 >= 'A' && ch1 <= 'F')
	value1 = (ch1 - 'A' + 10);
	else if (ch1 >= 'a' && ch1 <= 'f')
	value1 = (ch1 - 'a' + 10);
	
	/* do char2 */
	if (ch2 >= '0' && ch2 <= '9')
	value2 = (ch2 - '0');
	else if (ch2 >= 'A' && ch2 <= 'F')
	value2 = (ch2 - 'A' + 10);
	else if (ch2 >= 'a' && ch2 <= 'f')
	value2 = (ch2 - 'a' + 10);
	
	uint8_t output = (value1 << 4) + value2;
	return output;
}

#define BITS_PER_BYTE (8)

int bloomFilter::setFilterBits(const std::string &hex)
{
	uint32_t bytes = (mFilterBits / BITS_PER_BYTE);
	if (mFilterBits % BITS_PER_BYTE)
	{
		bytes++;
	}

	if (hex.size() < bytes * 2)
	{
		return 0;
	}

	// convert to binary array.
	uint8_t *tmparray = (uint8_t *) malloc(bytes);
    
    	if(tmparray == NULL)
        {
            std::cerr << "(EE) Error. Cannot allocate memory for " << bytes << " bytes in " << __PRETTY_FUNCTION__ << std::endl;
            return 0;
        }
        
	uint32_t i = 0;

	for(i = 0; i < bytes; i++)
	{
		tmparray[i] = convertCharToUint8(hex[2 * i], hex[2 * i + 1]);
	}


	for(i = 0; i < mFilterBits; i++)
	{
		int byte = i / BITS_PER_BYTE;
		int bit = i % BITS_PER_BYTE;
		uint8_t value = (tmparray[byte] & (1 << bit));

		if (value)
		{
			mBits[i] = 1;
		}
		else
		{
			mBits[i] = 0;
		}
	}

	free(tmparray);
	return 1;
}

std::string bloomFilter::getFilter()
{
	/* extract filter as a hex string */
	int bytes = (mFilterBits / BITS_PER_BYTE);
	if (mFilterBits % BITS_PER_BYTE)
	{
		bytes++;
	}

	if (bytes==0)
	{
		std::cerr << "(EE) Error. Cannot allocate memory for 0 byte in " << __PRETTY_FUNCTION__ << std::endl;
		return std::string();
	}
	// convert to binary array.
	uint8_t *tmparray = (uint8_t *) malloc(bytes);
    
    	if(tmparray == NULL)
        {
            std::cerr << "(EE) Error. Cannot allocate memory for " << bytes << " bytes in " << __PRETTY_FUNCTION__ << std::endl;
            return std::string();
        }
        
	int i,j;
	
	for(i = 0; i < bytes; i++)
	{
		tmparray[i] = 0;
		for(j = 0; j < BITS_PER_BYTE; j++)
		{
			int bit = i * BITS_PER_BYTE + j;
			if (mBits[bit])
			{
				tmparray[i] |= (1 << j);
			}
		}
	}

	std::string out;
	for(int i = 0; i < bytes; i++)
	{
		bd_sprintf_append(out, "%02lx",  (uint32_t) (tmparray)[i]);
	}

	free(tmparray);

	return out;
}

void bloomFilter::setBit(int bit)
{
	mBits[bit] = 1;

}


bool bloomFilter::isBitSet(int bit)
{
	return (mBits[bit] == 1);
}

uint32_t bloomFilter::filterBits()
{
	return mFilterBits;
}

uint32_t bloomFilter::countBits()
{
	int count = 0;
	uint32_t i;
	for(i = 0; i < mFilterBits; i++)
	{
		if (mBits[i])
		{
			count++;
		}
	}
	return count;
}


void bloomFilter::printFilter(std::ostream &out)
{
	out << "bloomFilter: m = " << mFilterBits;
	out << " k = " << mNoHashs;
	out << " n = " << mNoElements;
	out << std::endl;

	out << "BITS: ";
	uint32_t i;
	for(i = 0; i < mFilterBits; i++)
	{
		if ((i > 0) && (i % 32 == 0))
		{
			out << std::endl;
			out << "BITS: ";
		}
		if (mBits[i])
		{
			out << "1";
		}
		else
		{
			out << "0";
		}
	}
	out << std::endl;
	out << "STR: " << getFilter();
	out << std::endl;
}

void bloomFilter::setHashFunction(int idx,  uint32_t (*hashfn)(const std::string &))
{
	mHashFns[idx] = hashfn;
}

void bloomFilter::add(const std::string &hex)
{
	uint32_t (*hashfn)(const std::string &);
	uint32_t i;
	for(i = 0; i < mNoHashs; i++)
	{
		hashfn = mHashFns[i];

		int bit = hashfn(hex);

		setBit(bit);
	}

	mNoElements++;

}

bool bloomFilter::test(const std::string &hex)
{
	uint32_t (*hashfn)(const std::string &);
	uint32_t i;
	for(i = 0; i < mNoHashs; i++)
	{
		hashfn = mHashFns[i];

		int bit = hashfn(hex);

		if (!isBitSet(bit))
		{
			return false;
		}
	}
	return true;
}

uint32_t getFirst10BitsAsNumber(const std::string &input)
{
	if (input.size() < 8)
	{
		std::cerr << "getFirst10BitsAsNumber() ERROR Size too small!";
		std::cerr << std::endl;
		return 0;
	}

	uint8_t data[4];

	data[0] = convertCharToUint8(input[0], input[1]);
	data[1] = convertCharToUint8(input[2], input[3]);
	data[2] = convertCharToUint8(input[4], input[5]);
	data[3] = convertCharToUint8(input[6], input[7]);

	uint32_t val = ((data[0] & 0xff) << 2) + ((data[1] & 0xc0) >> 6);

#ifdef DEBUG_BLOOM
	std::cerr << "getFirst10BitsAsNumber() input: " << input;
	std::cerr << std::endl;
	std::cerr << "getFirst10BitsAsNumber() ";
	std::cerr << " data[0]: " << std::hex << (uint32_t) data[0];
	std::cerr << " data[1]: " << (uint32_t) data[1];
	std::cerr << " data[2]: " << (uint32_t) data[2];
	std::cerr << " data[3]: " << (uint32_t) data[3];
	std::cerr << " val: " << std::dec << (uint32_t) val;
	std::cerr << std::endl;
#endif

	return val;
}

uint32_t getSecond10BitsAsNumber(const std::string &input)
{
	if (input.size() < 8)
	{
		std::cerr << "getSecond10BitsAsNumber() ERROR Size too small!";
		std::cerr << std::endl;
		return 0;
	}

	uint8_t data[4];

	data[0] = convertCharToUint8(input[0], input[1]);
	data[1] = convertCharToUint8(input[2], input[3]);
	data[2] = convertCharToUint8(input[4], input[5]);
	data[3] = convertCharToUint8(input[6], input[7]);

	uint32_t val = ((data[1] & 0x3f) << 4) + ((data[2] & 0xf0) >> 4);

#ifdef DEBUG_BLOOM
	std::cerr << "getSecond10BitsAsNumber() input: " << input;
	std::cerr << std::endl;
	std::cerr << "getSecond10BitsAsNumber() ";
	std::cerr << " data[0]: " << std::hex << (uint32_t) data[0];
	std::cerr << " data[1]: " << (uint32_t) data[1];
	std::cerr << " data[2]: " << (uint32_t) data[2];
	std::cerr << " data[3]: " << (uint32_t) data[3];
	std::cerr << " val: " << std::dec << (uint32_t) val;
	std::cerr << std::endl;
#endif

	return val;
}


uint32_t getMid10BitsAsNumber(const std::string &input)
{
	if (input.size() < 8)
	{
		std::cerr << "getMid10BitsAsNumber() ERROR Size too small!";
		std::cerr << std::endl;
		return 0;
	}

	uint8_t data[4];

	data[0] = convertCharToUint8(input[0], input[1]);
	data[1] = convertCharToUint8(input[2], input[3]);
	data[2] = convertCharToUint8(input[4], input[5]);
	data[3] = convertCharToUint8(input[6], input[7]);

	uint32_t val = ((data[0] & 0x07) << 7) + ((data[1] & 0x7f) >> 1);

#ifdef DEBUG_BLOOM
	std::cerr << "getMid10BitsAsNumber() input: " << input;
	std::cerr << std::endl;
	std::cerr << "getMid10BitsAsNumber() ";
	std::cerr << " data[0]: " << std::hex << (uint32_t) data[0];
	std::cerr << " data[1]: " << (uint32_t) data[1];
	std::cerr << " data[2]: " << (uint32_t) data[2];
	std::cerr << " data[3]: " << (uint32_t) data[3];
	std::cerr << " val: " << std::dec << (uint32_t) val;
	std::cerr << std::endl;
#endif

	return val;
}


#define BDFILTER_M 1024
#define BDFILTER_K 3

bdBloom::bdBloom()
	:bloomFilter(BDFILTER_M, BDFILTER_K)
{
	/* set the fns. */
	setHashFunction(0, getFirst10BitsAsNumber);
	setHashFunction(1, getSecond10BitsAsNumber);
	setHashFunction(2, getMid10BitsAsNumber);
}





