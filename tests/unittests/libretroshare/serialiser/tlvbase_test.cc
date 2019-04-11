/*******************************************************************************
 * unittests/libretroshare/serialiser/tlvbase_test.cc                          *
 *                                                                             *
 * Copyright 2007-2008 by Horatio  <retroshare.project@gmail.com>              *
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

#include <string.h>
#include <string>
#include <iostream>
#include <limits.h>

#include "serialiser/rstlvbase.h"
#include "util/rsnet.h"

TEST(libretroshare_serialiser, test_RsTlvBase)
{
		//uint32_t array[] = {0x122, 0x234};
	char data[20];
	memset((void*)data, 65, sizeof(data)); // In ASCII 'A' =65
	
	
	std::string out;

	// First two bytes are type.	
	data[0]=0;
	data[1]=0;
	// Next 4 bytes is size.
	data[2]=0;
	data[3]=0;
	data[4]=0;
	data[5]=10;
	uint32_t off =0;
	GetTlvString((void*)data, 20, &off, 0, out);
	
	EXPECT_TRUE(out == "AAAA");
	
	std::cout << "Output is : " << out << std::endl;
	
	uint16_t data2[] = {0, 0, 0x0300};
	
	uint16_t  t = GetTlvSize((void*) data2);
	
	
	EXPECT_TRUE( t == ntohs(0x0300));
	
	//std::cout << "GetTlvSize = " <<t <<std::endl;
	
	
	std::string line;
	
	
	//*************Test SetTlvBase***********
	{
		uint16_t data3 [3];
		off =0;
		uint32_t *offset = &off;
		uint32_t const SIZE_SetTlvBase=6;
		uint32_t val_before = *offset;
		uint32_t base_set_size = 0x1234567;
		SetTlvBase((void *)data3, SIZE_SetTlvBase,  offset,  0x0011, base_set_size);
		
		EXPECT_TRUE(*offset - val_before  == SIZE_SetTlvBase);
		EXPECT_TRUE(0x0011 == ntohs(data3[0]));
		EXPECT_TRUE(base_set_size == ntohl(*((uint32_t *) &(data3[1]))));
		
		
	}

	/**
	*   test GetTlvUInt32 & SetTlvGetUInt32
	*/
	{
		uint16_t data4[5];
		bool ok = true;
		uint32_t off =0;
		uint32_t pre_set_off = off;
		uint32_t* offset = &off;
		uint32_t out = 3324;
		*offset =0;
		ok = SetTlvUInt32((void*)data4, 10, offset, 0x0011, out);
		EXPECT_TRUE(*offset - pre_set_off == 10);
		
		uint32_t readPos = 0;
		offset = &readPos;
		uint32_t in =0;
		ok &= GetTlvUInt32((void*)data4, 10, offset, 0x0011, &in);
		EXPECT_TRUE(*offset - pre_set_off == 10);
		EXPECT_TRUE(in == out);
		
		std::cerr<<"in = " <<in <<std::endl;
		std::cout << "*offset = " <<*offset <<std::endl;
		std::cout <<std::hex << data4[3]<< "  "  <<data4[4] <<std::endl;
	}
	
	
	uint32_t i;
	for(i = 0; i < UINT_MAX / 2; i *= 2, i += 111)	
	{
		uint16_t data4[5];
		data4[0] = htons(5); /* type */
		*((uint32_t *) &(data4[1])) = htonl(10); /* length */
		uint32_t int_val = i;
		*((uint32_t *) &(data4[3])) = htonl(int_val); /* value */
		uint32_t  got;
		uint32_t off = 0;
		uint32_t *offset = &off;
		GetTlvUInt32((void*)data4, 10, offset, 5,  &got);
		EXPECT_TRUE(got == int_val);
		std::cout << " got = " << std::hex << got <<std::endl;
		
	}

	
	/**
	*   Test GetTlvString()
	*/
	{
		std::string teststring = "Hello RS!";
		uint16_t data5[6 + 20];
		uint32_t pos =0;
		uint32_t* offset = &pos;
		//uint32_t pre_pos = pos;
		SetTlvString((void*)data5, sizeof(data5), offset, TLV_TYPE_STR_NAME, teststring);
		uint32_t tlvsize = GetTlvStringSize(teststring);
		EXPECT_TRUE(tlvsize == *offset);
		EXPECT_TRUE(data5[0] == htons(TLV_TYPE_STR_NAME));
		uint32_t encoded_size = ntohl(*((uint32_t *) &(data5[1])));
		EXPECT_TRUE(tlvsize ==  encoded_size);
		std::cerr << "tlvsize: " << tlvsize << " encoded_size: " << encoded_size;
		std::cerr << std::endl;

		std::string str((char*) ((char*)data5 +6) ,strlen("Hello RS!"));
		EXPECT_TRUE(str == "Hello RS!");
		
//		std::cout <<str <<std::endl;
		

		std::string out_str;
		
		pos =0;
		GetTlvString((void*)data5, sizeof(data5), offset, TLV_TYPE_STR_NAME, out_str);
		EXPECT_TRUE(out_str == "Hello RS!");
		EXPECT_TRUE(*offset == sizeof(uint16_t)*3 + out_str.size());
		uint16_t data6[3];
		*offset =0;
		EXPECT_TRUE(SetTlvSize((void*)data6, sizeof(data6),    0x00000022));
		std::cout << std::hex << data6[1] <<std::endl;
	
	}
}
