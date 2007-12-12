
/*
 * libretroshare/src/serialiser: rsbaseitem_test.cc
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

#include <iostream>
#include <sstream>
#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvutil.h"
#include "util/utest.h"

INITTEST();

static int test_RsFileItem();
static int test_RsFileData();

int main()
{
	std::cerr << "RsFile[Item/Data/...] Tests" << std::endl;

	test_RsFileItem(); 
	test_RsFileData(); 
	
	FINALREPORT("RsTlvFile[Item/Data/...] Tests");

	return TESTRESULT();
}

int test_RsTlvFileItem()
{




	RsTlvFileItem i1;
	RsTlvFileItem i2;

	/* initialise */
	i1.filesize = 101010;
	i1.hash = "ABCDEFEGHE";
	i1.name = "TestFile.txt";
	i1.pop  = 12;
	i1.age  = 456;

	CHECK(test_SerialiseTlvItem(std::cerr, &i1, &i2));

	/* check the data is the same */
	CHECK(i1.filesize == i2.filesize);
	CHECK(i1.hash == i2.hash);
	CHECK(i1.name == i2.name);
	CHECK(i1.path == i2.path);
	CHECK(i1.pop  == i2.pop);
	CHECK(i1.age  == i2.age);

	/* do it again without optional data */
	i1.filesize = 123;
	i1.name = "";
	i1.pop  = 0;
	i1.age  = 0;

	CHECK(test_SerialiseTlvItem(std::cerr, &i1, &i2));

	/* check the data is the same */
	CHECK(i1.filesize == i2.filesize);
	CHECK(i1.hash == i2.hash);
	CHECK(i1.name == i2.name);
	CHECK(i1.path == i2.path);
	CHECK(i1.pop  == i2.pop);
	CHECK(i1.age  == i2.age);

	/* one more time - long file name, some optional data */
	i1.filesize = 123;
	i1.name = "A Very Long File name that should fit in easily ??? with som $&%&^%* strange char (**$^%#&^$#*^%(&^ in there too!!!! ~~~!!$#(^$)$)(&%^)&\"  oiyu thend";
	i1.pop  = 666;
	i1.age  = 0;

	CHECK(test_SerialiseTlvItem(std::cerr, &i1, &i2));

	/* check the data is the same */
	CHECK(i1.filesize == i2.filesize);
	CHECK(i1.hash == i2.hash);
	CHECK(i1.name == i2.name);
	CHECK(i1.path == i2.path);
	CHECK(i1.pop  == i2.pop);
	CHECK(i1.age  == i2.age);

	REPORT("Serialise/Deserialise RsTlvFileItem");

	return 1;
}

int test_RsTlvFileSet()
{
	RsTlvFileSet  s1;
	RsTlvFileSet  s2;

	int i = 0;
	for(i = 0; i < 15; i++)
	{
		RsTlvFileItem fi;
		fi.filesize = 16 + i * i;
		fi.hash = "ABCDEF";
		std::ostringstream out;
		out << "File" << i << "_inSet.txt";
		fi.name = out.str();
		if (i % 2 == 0)
		{
			fi.age = 10 * i;
		}
		else
		{
			fi.age = 0;
		}
		fi.pop = 0;

		s1.items.push_back(fi);
	}

	CHECK(test_SerialiseTlvItem(std::cerr, &s1, &s2));

	/* check the data is the same - TODO */

	REPORT("Serialise/Deserialise RsTlvFileSet");

	return 1;
}


int test_RsTlvFileData()
{
	RsTlvFileData d1;
	RsTlvFileData d2;

	/* initialise */
	d1.file.filesize = 101010;
	d1.file.hash = "ABCDEFEGHE";
	d1.file.name = "";
	d1.file.age = 0;
	d1.file.pop = 0;

	char data[15];
	d1.binData.setBinData(data, 15);

	d1.file_offset = 222;

	CHECK(test_SerialiseTlvItem(std::cerr, &d1, &d2));

	/* check the data is the same */
	CHECK(d1.file.filesize == d2.file.filesize);
	CHECK(d1.file.hash == d2.file.hash);
	CHECK(d1.file.name == d2.file.name);
	CHECK(d1.file.path == d2.file.path);
	CHECK(d1.file.pop  == d2.file.pop);
	CHECK(d1.file.age  == d2.file.age);

	CHECK(d1.file_offset  == d2.file_offset);
	CHECK(d1.binData.bin_len == d2.binData.bin_len);

	REPORT("Serialise/Deserialise RsTlvFileData");

	return 1;
}


