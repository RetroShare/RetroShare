/*******************************************************************************
 * bitdht/bdstddht.cc                                                          *
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

#include "bitdht/bdstddht.h"
#include "bitdht/bdpeer.h"
#include "util/bdrandom.h"
#include "util/bdstring.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <iostream>
#include <iomanip>

/**
 * #define BITDHT_DEBUG 1
**/

void bdStdRandomId(bdId *id)
{
	bdStdRandomNodeId(&(id->id));
	id->addr.sin_addr.s_addr = bdRandom::random_u32();
	id->addr.sin_port = (bdRandom::random_u32() % USHRT_MAX);

	return;
}

void bdStdRandomNodeId(bdNodeId *id)
{
	uint32_t *a_data = (uint32_t *) id->data;
	for(int i = 0; i < BITDHT_KEY_INTLEN; i++)
	{
		a_data[i] = bdRandom::random_u32();
	}
	return;
}

void bdStdZeroNodeId(bdNodeId *id)
{
	uint32_t *a_data = (uint32_t *) id->data;
	for(int i = 0; i < BITDHT_KEY_INTLEN; i++)
	{
		a_data[i] = 0;
	}
	return;
}

// Ignore differences in port....
// must be careful which one we accept after this.
// can could end-up with the wrong port.
// However this only matters with firewalled peers anyway.
// So not too serious.
bool bdStdSimilarId(const bdId *n1, const bdId *n2)
{
	if (n1->id == n2->id)
	{
		if (n1->addr.sin_addr.s_addr == n2->addr.sin_addr.s_addr)
		{
			return true;
		}
	}
	return false;
}

bool bdStdUpdateSimilarId(bdId *dest, const bdId *src)
{
	/* only difference that's currently allowed */
	if (dest->addr.sin_port == src->addr.sin_port)
	{
		/* no update required */
		return false;
	}

	dest->addr.sin_port = src->addr.sin_port;
	return true;
}


/* fills in bdNodeId r, with XOR of a and b */
int bdStdDistance(const bdNodeId *a, const bdNodeId *b, bdMetric *r)
{
	uint8_t *a_data = (uint8_t *) a->data;
	uint8_t *b_data = (uint8_t *) b->data;
	uint8_t *ans = (uint8_t *) r->data;
	for(int i = 0; i < BITDHT_KEY_LEN; i++)	
	{
		*(ans++) = *(a_data++) ^ *(b_data++);
	}
	return 1;
}

void bdStdRandomMidId(const bdNodeId *target, const bdNodeId *other, bdNodeId *midId)
{
	bdMetric dist;
	
	/* get distance between a & c */
	bdStdDistance(target, other, &dist);

	/* generate Random Id */
	bdStdRandomNodeId(midId);

	/* zero bits of Random Id until under 1/2 of distance 
	 * done in bytes for ease... matches one extra byte than distance = 0 
	 * -> hence wierd order of operations
	 */
	//bool done = false;
	for(int i = 0; i < BITDHT_KEY_LEN; i++)
	{
		midId->data[i] = target->data[i];

		if (dist.data[i] != 0)
			break;
	}
}

int  bdStdLoadNodeId(bdNodeId *id, std::string input)
{
	uint8_t *a_data = (uint8_t *) id->data;
	uint32_t reqlen = BITDHT_KEY_LEN * 2;
	if (input.size() < reqlen)
	{
		return 0;
	}

	for(int i = 0; i < BITDHT_KEY_LEN; i++)
	{
		char ch1 = input[2 * i];
		char ch2 = input[2 * i + 1];
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

		a_data[i] = (value1 << 4) + value2;
	}
	return 1;
}

std::string bdStdConvertToPrintable(std::string input)
{
    std::string out;
    for(uint32_t i = 0; i < input.length(); i++)
    {
        /* sensible chars */
        if ((input[i] > 31) && (input[i] < 127))
        {
                out += input[i];
        }
        else
        {
            bd_sprintf_append(out, "[0x%x]", (uint32_t) input[i]);
        }
    }
    return out;
}

void bdStdPrintNodeId(std::ostream &out, const bdNodeId *a)
{
	std::string s;
	bdStdPrintNodeId(s, a, true);
	out << s;
}

void bdStdPrintNodeId(std::string &out, const bdNodeId *a, bool append)
{
	if (!append)
	{
		out.clear();
	}

	for(int i = 0; i < BITDHT_KEY_LEN; i++)
	{
		bd_sprintf_append(out, "%02x", (uint32_t) (a->data)[i]);
	}
}

void bdStdPrintId(std::ostream &out, const bdId *a)
{
	std::string s;
	bdStdPrintId(s, a, false);
	out << s;
}

void bdStdPrintId(std::string &out, const bdId *a, bool append)
{
	bdStdPrintNodeId(out, &(a->id), append);
	bd_sprintf_append(out, " ip:%s:%u", bdnet_inet_ntoa(a->addr.sin_addr).c_str(), ntohs(a->addr.sin_port));
}

/* returns 0-160 depending on bucket */
int bdStdBucketDistance(const bdNodeId *a, const bdNodeId *b)
{
	bdMetric m;
	bdStdDistance(a, b, &m);
	return bdStdBucketDistance(&m);
}

/* returns 0-160 depending on bucket */
int bdStdBucketDistance(const bdMetric *m)
{
	for(int i = 0; i < BITDHT_KEY_BITLEN; i++)
	{
		int bit = BITDHT_KEY_BITLEN - i - 1;
		int byte = i / 8;
		int bbit = 7 - (i % 8);
		unsigned char comp = (1 << bbit);

#ifdef BITDHT_DEBUG
		fprintf(stderr, "bdStdBucketDistance: bit:%d  byte:%d bbit:%d comp:%x, data:%x\n", bit, byte, bbit, comp, m->data[byte]);
#endif

		if (comp & m->data[byte])
		{
			return bit;
		}
	}
	return 0;
}


bdStdDht::bdStdDht()
{
	return;
}
        /* setup variables */
uint16_t bdStdDht::bdNumBuckets()
{

	return BITDHT_STANDARD_N_BUCKETS;
}

uint16_t bdStdDht::bdNodesPerBucket() /* used for bdspace */
{
	return BITDHT_STANDARD_BUCKET_SIZE;
}

uint16_t bdStdDht::bdNumQueryNodes() /* used for queries */
{
	return BITDHT_STANDARD_BUCKET_SIZE;
}

uint16_t bdStdDht::bdBucketBitSize()
{
	return BITDHT_STANDARD_BUCKET_SIZE_BITS;
}

int bdStdDht::bdDistance(const bdNodeId *n1, const bdNodeId *n2, class bdMetric *metric)
{
	return bdStdDistance(n1, n2, metric);
}
	
int bdStdDht::bdBucketDistance(const bdNodeId *n1, const bdNodeId *n2)
{
	return bdStdBucketDistance(n1, n2);
}
	
int bdStdDht::bdBucketDistance(const bdMetric *metric)
{
	return bdStdBucketDistance(metric);
}
	

bool bdStdDht::bdSimilarId(const bdId *id1, const bdId *id2)
{
	return bdStdSimilarId(id1, id2);
}
	

bool bdStdDht::bdUpdateSimilarId(bdId *dest, const bdId *src)
{
	return bdStdUpdateSimilarId(dest, src);
}
	

void bdStdDht::bdRandomMidId(const bdNodeId *target, const bdNodeId *other, bdNodeId *mid)
{
	return bdStdRandomMidId(target, other, mid);
}
	

void bdStdDht::bdPrintId(std::ostream &out, const bdId *a)
{
	return bdStdPrintId(out, a);
}
	
void bdStdDht::bdPrintNodeId(std::ostream &out, const bdNodeId *a)
{
	return bdStdPrintNodeId(out, a);
}

	
/**************************/

bdModDht::bdModDht()
	:mNodesPerBucket(BITDHT_STANDARD_BUCKET_SIZE)
{
	return;
}

void bdModDht::setNodesPerBucket(uint16_t nodesPerBucket)
{
	mNodesPerBucket = nodesPerBucket;
	return;
}


uint16_t bdModDht::bdNodesPerBucket() /* used for bdspace */
{
	return mNodesPerBucket;
}


