
/*
 * libretroshare/src/serialiser: tlvbase_test.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Horatio.
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




#include <string.h>
#include <string>
#include <iostream>

#include "serialiser/rstlvbase.h"
#include "util/utest.h"
#include "util/rsnet.h"

INITTEST();

static  int test_RsTlvBase();
int main()
{
	std::cerr << " RsTlvBase Tests" <<std::endl;
	
	test_RsTlvBase();
	FINALREPORT("RsTlvBase Tests");
	return TESTRESULT();
}



int test_RsTlvBase()
{
		//uint32_t array[] = {0x122, 0x234};
	char data[20];
	memset((void*)data, 65, sizeof(data)); // In ASCII 'A' =65
	
	
	std::string out;
	
	data[0]=0;
	data[1]=0;
	data[2]=0;
	data[3]=8;
	uint32_t off =0;
	GetTlvString((void*)data, 20, &off, 0, out);
	
	CHECK(out == "AAAA");
	
	std::cout << "Output is : " << out << std::endl;
	
	uint16_t data2[] = {0, 0x0300};
	
	uint16_t  t = GetTlvSize((void*) data2);
	
	
	CHECK( t == ntohs(0x0300));
	
	//std::cout << "GetTlvSize = " <<t <<std::endl;
	
	
	std::string line;
	
	
	//*************Test SetTlvBase***********
	{
		uint16_t data3 [2];
		off =0;
		uint32_t *offset = &off;
		uint32_t const SIZE_SetTlvBase=4;
		uint32_t val_before = *offset;
		SetTlvBase((void *)data3, SIZE_SetTlvBase,  offset,  0x0011, 0x0001);
		
		CHECK(*offset - val_before  == SIZE_SetTlvBase);
		CHECK(0x0011 == ntohs(data3[0]));
		CHECK(0x0001 == ntohs(data3[1]));
		
		
	}

	REPORT("Test SetTlvBase");
	
	
	/**
	*   test GetTlvUInt32 & SetTlvGetUInt32
	*/
	{
		uint16_t data4[4];
		bool ok = true;
		uint32_t off =0;
		uint32_t pre_set_off = off;
		uint32_t* offset = &off;
		uint32_t out = 3324;
		*offset =0;
		ok = SetTlvUInt32((void*)data4, 8, offset, 0x0011, out);
		CHECK(*offset - pre_set_off == 8);
		
		uint32_t readPos = 0;
		offset = &readPos;
		uint32_t in =0;
		ok &= GetTlvUInt32((void*)data4, 8, offset, 0x0011, &in);
		CHECK(*offset - pre_set_off == 8);
		CHECK(in == out);
		
		std::cerr<<"in = " <<in <<std::endl;
		std::cout << "*offset = " <<*offset <<std::endl;
		std::cout <<std::hex << data4[2]<< "  "  <<data4[3] <<std::endl;
	}
	
	
	
	{
		uint16_t data4[4];
		data4[0] = 0x0500; /* type (little-endian?) */
		data4[1] = 0x0800; /* size (little-endian?) */
		data4[3]=0x0000;
		data4[2] = 0xFFFF;
		uint32_t  got;
		uint32_t off =0;
		uint32_t*offset = &off;
		GetTlvUInt32((void*)data4, 8, offset, 5,  &got);
		CHECK(got ==0xFFFF0000);
		std::cout << " got = " << std::hex << got <<std::endl;
		
	}

	REPORT("Test TlvUInt32");
	
	
	/**
	*   Test GetTlvString()
	*/
	{
		std::string teststring = "Hello RS!";
		uint16_t data5[4 + 20];
		uint32_t pos =0;
		uint32_t* offset = &pos;
		uint32_t pre_pos = pos;
		SetTlvString((void*)data5, sizeof(data5), offset, TLV_TYPE_STR_NAME, teststring);
		uint16_t tlvsize = GetTlvStringSize(teststring);
		CHECK(tlvsize == *offset);
		CHECK(data5[0] == htons(TLV_TYPE_STR_NAME));
		CHECK(data5[1] == htons(tlvsize));

		std::string str((char*) ((char*)data5 +4) ,strlen("Hello RS!"));
		CHECK(str == "Hello RS!");
		
//		std::cout <<str <<std::endl;
		

		std::string out_str;
		
		pos =0;
		GetTlvString((void*)data5, sizeof(data5), offset, TLV_TYPE_STR_NAME, out_str);
		CHECK(out_str == "Hello RS!");
		CHECK(*offset == sizeof(uint16_t)*2 + out_str.size());
		uint16_t data6[2];
		*offset =0;
		SetTlvSize((void*)data6, sizeof(data6),    0x00000022);
		std::cout << std::hex << data6[1] <<std::endl;
	
	}
	REPORT("Test TlvString");
	return 0;
}
