
/*
 * libretroshare/src/serialiser: tlvstack_test.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#include <gtest/gtest.h>

#include <iostream>
#include "serialiser/rstlvfileitem.h"
#include "serialiser/rstlvbinary.h"

#include "rstlvutil.h"

#define BIN_LEN 1024

TEST(libretroshare_serialiser, test_RsTlvStack)
{

	/* now create a set of TLV items for the random generator */

	RsTlvBinaryData  bd1(123);
	RsTlvBinaryData  bd2(125);

	char data[BIN_LEN] = {0};
	int i;
	for(i = 0; i < BIN_LEN; i++)
	{
		data[i] = i%13;
	}

	bd1.setBinData(data, 5);
	bd2.setBinData(data, 21);

        RsTlvFileItem fi1;
        RsTlvFileItem fi2;

        /* initialise */
        fi1.filesize = 101010;
        fi1.hash = RsFileHash("123456789ABCDEF67890123456789ABCDEF67890");//SHA1_SIZE*2 = 40
        fi1.name = "TestFile.txt";
        fi1.pop  = 12;
        fi1.age  = 456;

        fi2.filesize = 101010;
        fi2.hash = RsFileHash("123456789ABCDEF67890123456789ABCDEF67890");//SHA1_SIZE*2 = 40
        fi2.name = "TestFile.txt";
        fi2.pop  = 0;
        fi2.age  = 0;;

	std::vector<RsTlvItem *> items;
	items.resize(4);
	items[0] = &bd1;
	items[1] = &bd2;
	items[2] = &fi1;
	items[3] = &fi2;

	test_TlvSet(items, 1024);
}


