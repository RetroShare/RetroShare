/*
 * bitdht/bdstddht.cc
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


#include "bitdht/bdstddht.h"
#include "bitdht/bdpeer.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <sstream>
#include <iomanip>

/**
 * #define BITDHT_DEBUG 1
**/

void bdStdRandomId(bdId *id)
{
	bdStdRandomNodeId(&(id->id));

	id->addr.sin_addr.s_addr = rand();
	id->addr.sin_port = rand();

	return;
}

void bdStdRandomNodeId(bdNodeId *id)
{
	uint32_t *a_data = (uint32_t *) id->data;
	for(int i = 0; i < BITDHT_KEY_INTLEN; i++)
	{
		a_data[i] = rand();
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

uint32_t bdStdLikelySameNode(const bdId *n1, const bdId *n2)
{
	if (*n1 == *n2)
	{
		return 1;
	}
	return 0;
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

std::string bdStdConvertToPrintable(std::string input)
{
	std::ostringstream out;
        for(uint32_t i = 0; i < input.length(); i++)
        {
                /* sensible chars */
                if ((input[i] > 31) && (input[i] < 127))
                {
                        out << input[i];
                }
                else
                {
			out << "[0x" << std::hex << (uint32_t) input[i] << "]";
			out << std::dec;
                }
        }
	return out.str();
}

void bdStdPrintNodeId(std::ostream &out, const bdNodeId *a)
{
	for(int i = 0; i < BITDHT_KEY_LEN; i++)	
	{
		out << std::setw(2) << std::setfill('0') << std::hex << (uint32_t) (a->data)[i];
	}
	out << std::dec;

	return;
}


void bdStdPrintId(std::ostream &out, const bdId *a)
{
	bdStdPrintNodeId(out, &(a->id));
	out << " ip:" << inet_ntoa(a->addr.sin_addr);
	out << ":" << ntohs(a->addr.sin_port);
	return;
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

uint16_t bdStdDht::bdNodesPerBucket() /* used for query + bdspace */
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
	

uint32_t bdStdDht::bdLikelySameNode(const bdId *id1, const bdId *id2)
{
	return bdStdLikelySameNode(id1, id2);
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
	


