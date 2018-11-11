/*******************************************************************************
 * unittests/libretroshare/serialiser/tlvtypes_test.cc                         *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie    <retroshare.project@gmail.com>      *
 * Copyright 2007-2008 by Chris Evi-Parker <retroshare.project@gmail.com>      *
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
#include "serialiser/rstlvbinary.h"
#include "serialiser/rstlvfileitem.h"
#include "serialiser/rstlvstring.h"
#include "serialiser/rstlvidset.h"
#include "serialiser/rstlvkeyvalue.h"
#include "serialiser/rstlvimage.h"
#include "serialiser/rstlvbase.h"

#include "rstlvutil.h"

#define RAND_SEED 1352534


TEST(libretroshare_serialiser, test_RsTlvFileItem)
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

TEST(libretroshare_serialiser, test_RsTlvFileSet)
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


TEST(libretroshare_serialiser, test_RsTlvFileData)
{
	RsTlvFileData d1;
	RsTlvFileData d2;

	/* initialise */
	d1.file.filesize = 101010;
	d1.file.hash = RsFileHash::random() ;
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

TEST(libretroshare_serialiser, test_RsTlvPeerIdSet)
{

	RsTlvPeerIdSet i1, i2; // one to set and other to get

	RsPeerId testId;

	std::string randString[5];
	randString[0] = "e$424!�!�";
	randString[1] = "e~:@L{L{KHKG";
	randString[2] = "e{@O**/*/*";
	randString[3] = "e?<<BNMB>HG�!�%$";
	randString[4] = "e><?<NVBCEE�$$%*^";

	/* store a number of random ids */

	for(int i = 0; i < 15 ; i++)
	{
        testId = RsPeerId::random();
        i1.ids.insert(testId);
	}

	EXPECT_TRUE(test_SerialiseTlvItem(std::cerr, &i1, &i2));
}

TEST(libretroshare_serialiser, test_RsTlvServiceIdSet)
{
	RsTlvServiceIdSet i1, i2; // one to set and other to get
	srand(RAND_SEED);


	/* store random numbers */
	for(int i = 0; i < 15 ; i++)
	{
		i1.ids.push_back(1 + rand() % 12564);
	}
	std::cout << "error here!!!?";
	std::cout << std::endl;
	EXPECT_TRUE(test_SerialiseTlvItem(std::cerr, &i1, &i2));
}

TEST(libretroshare_serialiser, test_RsTlvKeyValue)
{
	RsTlvKeyValue i1, i2; // one to set and other to get

	i1.key = "whatever";
	i1.value = "better work";

	EXPECT_TRUE(test_SerialiseTlvItem(std::cerr, &i1, &i2));
}

TEST(libretroshare_serialiser, test_RsTlvKeyValueSet)
{
	RsTlvKeyValueSet i1, i2; // one to set and other to get
	srand(RAND_SEED);

	/* instantiate the objects values */

	std::string randString[5];
	randString[0] = "e$424!�!�";
	randString[1] = "e~:@L{L{KHKG";
	randString[2] = "e{@O**/*/*";
	randString[3] = "e?<<BNMB>HG�!�%$";
	randString[4] = "e><?<NVBCEE�$$%*^";

	for(int i = 0; i < 15; i++)
	{
		RsTlvKeyValue kv;

		kv.key = randString[(rand() % 4)] + randString[(rand() % 4)];
		kv.value = randString[(rand() % 4)] + randString[(rand() % 4)];

		i1.pairs.push_back(kv);

	}

	EXPECT_TRUE(test_SerialiseTlvItem(std::cerr, &i1, &i2));
}

TEST(libretroshare_serialiser, test_RsTlvBinData)
{
	RsTlvBinaryData b1(TLV_TYPE_BIN_IMAGE), b2(TLV_TYPE_BIN_IMAGE);
	unsigned char* data = NULL;
	const uint32_t bin_size = 16000;
	char alpha  = 'a';

	data = new unsigned char[bin_size];
	srand(RAND_SEED);


	// initialise binary data with random values
	for(int i=0; i != bin_size; i++)
		data[i] = alpha + (rand() % 26);


	b1.setBinData(data, bin_size);
	delete[] data;

	//do check
	EXPECT_TRUE(test_SerialiseTlvItem(std::cerr, &b1, &b2));
}

TEST(libretroshare_serialiser, test_RsTlvImage)
{

	RsTlvImage image1, image2;
	unsigned char* image_data = NULL;
	const uint32_t bin_size = 16000;
	char alpha  = 'a';

	image_data = new unsigned char[bin_size];
	srand(RAND_SEED);


	// initialise binary data with random values
	for(int i=0; i != bin_size; i++)
		image_data[i] = alpha + (rand() % 26);

	image1.image_type = RSTLV_IMAGE_TYPE_PNG;
	image1.binData.setBinData(image_data, bin_size);

	delete[] image_data;

	EXPECT_TRUE(test_SerialiseTlvItem(std::cerr, &image1, &image2));
}


TEST(libretroshare_serialiser, test_RsTlvHashSet)
{
	RsTlvPeerIdSet i1, i2; // one to set and other to get
	srand(RAND_SEED);

	int numRandStrings = rand()%30;

	/* store a number of random ids */

	for(int i = 0; i < numRandStrings ; i++)
	{
		RsPeerId randId;
        randId = RsPeerId::random();
        i1.ids.insert(randId);
	}

	EXPECT_TRUE(test_SerialiseTlvItem(std::cerr, &i1, &i2));
}
