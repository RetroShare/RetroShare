
/*
 * libretroshare/src/serialiser: tlvbase_test2.cc
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
#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvutil.h"
#include "util/utest.h"
#include "util/rsnet.h"

INITTEST();

static int test_RsTlvString();
static int test_RsTlvUInt32();
static int test_RsTlvIPAddr();

int main()
{
	std::cerr << "RsTlvBase Tests" << std::endl;

	test_RsTlvString(); 
	test_RsTlvUInt32(); 
	test_RsTlvIPAddr();
	
	FINALREPORT("RsTlvBase Tests");

	return TESTRESULT();
}

int test_RsTlvUInt32()
{
	return 1;
}

int test_OneString(std::string input, uint16_t type);

int test_RsTlvString()
{
	std::string nullString;
	std::string oneString = "1";
	std::string shortString = "ab cd";
	std::string longString = "abcd efgh ijkl mnop qrst uvw";

	std::cerr << "test_RsTlvString() Testing" << std::endl;
	test_OneString(nullString, 1234);
	test_OneString(oneString, 12);
	test_OneString(shortString, 79);
	test_OneString(longString, 7654);

	REPORT("Serialise RsTlvFileItem");

	return 1;
}


int test_OneString(std::string input, uint16_t type)
{
	/* an array to work from */
	char tlvdata[2048];
	std::string OutString;

	std::cerr << "test_OneString() Testing ... Print/Serialise/Deserialise";
	std::cerr << std::endl;
	/* start with SetTlvString() */

	uint16_t initsize =  GetTlvStringSize(input);
	uint32_t outOffset = 0;
	uint32_t inOffset  = 0;

	std::cerr << "Serialising: " << input << std::endl;
	CHECK(SetTlvString((void*)tlvdata, 2048, &outOffset, type, input));
	std::cerr << "Init Size: " << initsize << std::endl;
	std::cerr << "Serialised Size: " << outOffset << std::endl;
	displayRawPacket(std::cerr, tlvdata, outOffset);

	CHECK(outOffset == initsize); 		/* check that the offset matches the size */

	std::cerr << "DeSerialising" << std::endl;

	/* fails if type is wrong! */
	CHECK(0 == GetTlvString((void*)tlvdata, outOffset, &inOffset, type-1, OutString));
	CHECK(GetTlvString((void*)tlvdata, outOffset, &inOffset, type, OutString));

	CHECK(initsize == inOffset); 		/* check that the offset matches the size */
	CHECK(input == OutString);	/* check that strings match */
	std::cerr << "Deserialised: Size: " << inOffset << std::endl;
	std::cerr << "Deserialised: String: " << OutString << std::endl;

	REPORT("Serialise OneString");

	return 1;
}

int test_IpAddr(struct sockaddr_in *addr, uint16_t type);

static int test_RsTlvIPAddr()
{
	struct sockaddr_in addr;

	inet_aton("10.0.0.111", &(addr.sin_addr));
	addr.sin_port = htons(1111);

	test_IpAddr(&addr, 1234);
	
	inet_aton("255.255.255.1", &(addr.sin_addr));
	addr.sin_port = htons(9999);

	test_IpAddr(&addr, 1234);
	
	inet_aton("128.255.255.1", &(addr.sin_addr));
	addr.sin_port = htons(0);

	test_IpAddr(&addr, 1234);

	return 1;
}

int test_IpAddr(struct sockaddr_in *addr, uint16_t type)
{
	/* an array to work from */
	char tlvdata[2048];
	struct sockaddr_in outaddr;

	std::cerr << "test_IpAddr() Testing ... Print/Serialise/Deserialise";
	std::cerr << std::endl;
	/* start with SetTlvString() */

	uint16_t initsize =  GetTlvIpAddrPortV4Size();
	uint32_t outOffset = 0;
	uint32_t inOffset  = 0;

	std::cerr << "Serialising IPAddr: " << inet_ntoa(addr->sin_addr) << std::endl;
	std::cerr << "            Port  : " << ntohs(addr->sin_port) << std::endl;

	CHECK(SetTlvIpAddrPortV4((void*)tlvdata, 2048, &outOffset, type, addr));
	std::cerr << "Init Size: " << initsize << std::endl;
	std::cerr << "Serialised Size: " << outOffset << std::endl;
	displayRawPacket(std::cerr, tlvdata, outOffset);

	CHECK(outOffset == initsize); 		/* check that the offset matches the size */

	std::cerr << "DeSerialising" << std::endl;

	/* fails if type is wrong! */
	CHECK(0 == GetTlvIpAddrPortV4((void*)tlvdata, outOffset, &inOffset, type-1, &outaddr));
	CHECK(GetTlvIpAddrPortV4((void*)tlvdata, outOffset, &inOffset, type, &outaddr));

	CHECK(initsize == inOffset); 		/* check that the offset matches the size */
	CHECK(addr->sin_addr.s_addr == outaddr.sin_addr.s_addr);	/* check that IP match */
	CHECK(addr->sin_port        == outaddr.sin_port); 		/* check that Port match */
	std::cerr << "Deserialised: Size: " << inOffset << std::endl;
	std::cerr << "Deserialised IPAddr: " << inet_ntoa(outaddr.sin_addr) << std::endl;
	std::cerr << "             Port  : " << ntohs(outaddr.sin_port) << std::endl;

	REPORT("Serialise OneIP/Port");

	return 1;
}



