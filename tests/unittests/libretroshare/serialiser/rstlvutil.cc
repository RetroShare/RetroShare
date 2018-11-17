/*******************************************************************************
 * unittests/libretroshare/serialiser/rstlvutil.cc                             *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie <retroshare.project@gmail.com>         *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/

#include <gtest/gtest.h>

#include "rstlvutil.h"

#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvitem.h"
#include "util/rsstring.h"

#include <iostream>
#include <stdlib.h>

#if 0
/* print out a packet */
#include <iomanip>
#include <vector>


#endif

void displayRawPacket(std::ostream &out, void *data, uint32_t size)
{
	uint32_t i;
	std::string sout;
	rs_sprintf(sout, "DisplayRawPacket: Size: %ld", size);

	for(i = 0; i < size; i++)
	{
		if (i % 16 == 0)
		{
			sout += "\n";
		}
		rs_sprintf_append(sout, "%02x:", (int) (((unsigned char *) data)[i]));
	}

	out << sout << std::endl;
}


#define WHOLE_64K_SIZE 65536

int test_SerialiseTlvItem(std::ostream &str, RsTlvItem *in, RsTlvItem *out)
{
	uint16_t initsize = in->TlvSize();
	uint32_t serialOffset = 0;
	uint32_t deserialOffset  = 0;

	str << "test_SerialiseTlvItem() Testing ... Print/Serialise/Deserialise";
	str << std::endl;


	/* some space to serialise into */
	unsigned char serbuffer[WHOLE_64K_SIZE];

	EXPECT_TRUE(in->SetTlv(serbuffer, WHOLE_64K_SIZE, &serialOffset));

	EXPECT_TRUE(serialOffset == initsize); 		/* check that the offset matches the size */
	EXPECT_TRUE(in->TlvSize() == initsize);	/* check size hasn't changed */

	/* now we try to read it back in! */
	EXPECT_TRUE(out->GetTlv(serbuffer, serialOffset, &deserialOffset));

	/* again check sizes */
	EXPECT_TRUE(serialOffset == deserialOffset); 
	EXPECT_TRUE(deserialOffset == initsize); 
	EXPECT_TRUE(out->TlvSize() == initsize);

	str << "Class In/Serialised/Out!" << std::endl;
	in->print(str, 0);
	displayRawPacket(str, serbuffer, serialOffset);
	out->print(str, 0);
	str << std::endl;
	return 1;
}

/* This function checks the TLV header, and steps on to the next one
 */

bool test_StepThroughTlvStack(std::ostream &str, void *data, int size)
{
	uint32_t offset = 0;
	uint32_t index = 0;
	while (offset + 4 <= (uint32_t)size)
	{
		uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[offset]) );
		uint16_t tlvsize = GetTlvSize( &(((uint8_t *) data)[offset]) );
		str << "Tlv Entry[" << index << "] => Offset: " << offset;
		str << " Type: " << tlvtype;
		str << " Size: " << tlvsize;
		str << std::endl;

		offset += tlvsize ;
	}
	EXPECT_TRUE(offset == (uint32_t)size); /* we match up exactly */
	return 1;
}


int test_CreateTlvStack(std::ostream &str, 
		std::vector<RsTlvItem *> items, void *data, uint32_t *totalsize)
{
	/* (1) select a random item 
	 * (2) check size -> if okay serialise onto the end
	 * (3) loop!.
	 */
	uint32_t offset = 0;
	uint32_t count = 0;

	while(1)
	{
		int idx = (int) (items.size() * (rand() / (RAND_MAX + 1.0)));
		uint32_t tlvsize = items[idx] -> TlvSize();

		if (offset + tlvsize > *totalsize)
		{
			*totalsize = offset;
			return count;
		}

		str << "Stack[" << count << "]";
		str << " Offset: " << offset;
		str << " TlvSize: " << tlvsize;
		str << std::endl;
			
		/* serialise it */
		items[idx] -> SetTlv(data, *totalsize, &offset);
		items[idx] -> print(str, 10);
		count++;
	}
	*totalsize = offset;
	return 0;
}

int test_TlvSet(std::vector<RsTlvItem *> items, int maxsize)
{
	int totalsize = maxsize;
	void *data = malloc(totalsize);
	uint32_t size = totalsize;

	test_CreateTlvStack(std::cerr, items, data, &size);
	test_StepThroughTlvStack(std::cerr, data, size);

	free(data) ;

	return 1;
}


