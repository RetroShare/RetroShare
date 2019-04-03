/*******************************************************************************
 * unittests/libretroshare/serialiser/rsbaseitem_test.cc                       *
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

#include <iostream>
#include <sstream>
#include "serialiser/rstlvfileitem.h"
#include "rstlvutil.h"


TEST(libretroshare_serialiser, RsTlvFileItem)
{
	RsTlvFileItem i1;
	RsTlvFileItem i2;

	/* initialise */
	i1.filesize = 101010;
	i1.hash = RsFileHash("123456789ABCDEF67890123456789ABCDEF67890");//SHA1_SIZE*2 = 40
	i1.name = "TestFile.txt";
	i1.pop  = 12;
	i1.age  = 456;

	EXPECT_TRUE(test_SerialiseTlvItem(std::cerr, &i1, &i2));

	/* check the data is the same */
	EXPECT_TRUE(i1.filesize == i2.filesize);
	EXPECT_TRUE(i1.hash == i2.hash);
	EXPECT_TRUE(i1.name == i2.name);
	EXPECT_TRUE(i1.path == i2.path);
	EXPECT_TRUE(i1.pop  == i2.pop);
	EXPECT_TRUE(i1.age  == i2.age);

	/* do it again without optional data */
	i1.filesize = 123;
	i1.name = "";
	i1.pop  = 0;
	i1.age  = 0;

	EXPECT_TRUE(test_SerialiseTlvItem(std::cerr, &i1, &i2));

	/* check the data is the same */
	EXPECT_TRUE(i1.filesize == i2.filesize);
	EXPECT_TRUE(i1.hash == i2.hash);
	EXPECT_TRUE(i1.name == i2.name);
	EXPECT_TRUE(i1.path == i2.path);
	EXPECT_TRUE(i1.pop  == i2.pop);
	EXPECT_TRUE(i1.age  == i2.age);

	/* one more time - long file name, some optional data */
	i1.filesize = 123;
	i1.name = "A Very Long File name that should fit in easily ??? with som $&%&^%* strange char (**$^%#&^$#*^%(&^ in there too!!!! ~~~!!$#(^$)$)(&%^)&\"  oiyu thend";
	i1.pop  = 666;
	i1.age  = 0;

	EXPECT_TRUE(test_SerialiseTlvItem(std::cerr, &i1, &i2));

	/* check the data is the same */
	EXPECT_TRUE(i1.filesize == i2.filesize);
	EXPECT_TRUE(i1.hash == i2.hash);
	EXPECT_TRUE(i1.name == i2.name);
	EXPECT_TRUE(i1.path == i2.path);
	EXPECT_TRUE(i1.pop  == i2.pop);
	EXPECT_TRUE(i1.age  == i2.age);
}

TEST(libretroshare_serialiser, RsTlvFileSet)
{
	RsTlvFileSet  s1;
	RsTlvFileSet  s2;

	int i = 0;
	for(i = 0; i < 15; i++)
	{
		RsTlvFileItem fi;
		fi.filesize = 16 + i * i;
		fi.hash = RsFileHash("123456789ABCDEF67890123456789ABCDEF67890");//SHA1_SIZE*2 = 40
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

	EXPECT_TRUE(test_SerialiseTlvItem(std::cerr, &s1, &s2));
}

TEST(libretroshare_serialiser, RsTlvFileData)
{
	RsTlvFileData d1;
	RsTlvFileData d2;

	/* initialise */
	d1.file.filesize = 101010;
	d1.file.hash = RsFileHash("123456789ABCDEF67890123456789ABCDEF67890");//SHA1_SIZE*2 = 40
	d1.file.name = "";
	d1.file.age = 0;
	d1.file.pop = 0;

	unsigned char data[15];
	RSRandom::random_bytes(data,15) ;
	d1.binData.setBinData(data, 15);

	d1.file_offset = 222;

	EXPECT_TRUE(test_SerialiseTlvItem(std::cerr, &d1, &d2));

	/* check the data is the same */
	EXPECT_TRUE(d1.file.filesize == d2.file.filesize);
	EXPECT_TRUE(d1.file.hash == d2.file.hash);
	EXPECT_TRUE(d1.file.name == d2.file.name);
	EXPECT_TRUE(d1.file.path == d2.file.path);
	EXPECT_TRUE(d1.file.pop  == d2.file.pop);
	EXPECT_TRUE(d1.file.age  == d2.file.age);

	EXPECT_TRUE(d1.file_offset  == d2.file_offset);
	EXPECT_TRUE(d1.binData.bin_len == d2.binData.bin_len);
}


