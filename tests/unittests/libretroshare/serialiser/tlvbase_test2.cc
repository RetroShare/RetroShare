/*******************************************************************************
 * unittests/libretroshare/serialiser/tlvbase_test2.cc                         *
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
#include "serialiser/rstlvbase.h"
#include "rstlvutil.h"
#include "util/rsnet.h"


int test_OneString(std::string input, uint16_t type);

TEST(libretroshare_serialiser, test_RsTlvString)
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
	EXPECT_TRUE(SetTlvString((void*)tlvdata, 2048, &outOffset, type, input));
	std::cerr << "Init Size: " << initsize << std::endl;
	std::cerr << "Serialised Size: " << outOffset << std::endl;
	displayRawPacket(std::cerr, tlvdata, outOffset);

	EXPECT_TRUE(outOffset == initsize); 		/* check that the offset matches the size */

	std::cerr << "DeSerialising" << std::endl;

	/* fails if type is wrong! */
	std::cerr << "### These errors are expected." << std::endl;
	EXPECT_TRUE(0 == GetTlvString((void*)tlvdata, outOffset, &inOffset, type-1, OutString));
	EXPECT_TRUE(GetTlvString((void*)tlvdata, outOffset, &inOffset, type, OutString));

	EXPECT_TRUE(initsize == inOffset); 		/* check that the offset matches the size */
	EXPECT_TRUE(input == OutString);	/* check that strings match */
	std::cerr << "Deserialised: Size: " << inOffset << std::endl;
	std::cerr << "Deserialised: String: " << OutString << std::endl;

	return 1;
}

int test_IpAddr(struct sockaddr_in *addr, uint16_t type);

TEST(libretroshare_serialiser, test_RsTlvIPAddr)
{
	struct sockaddr_in addr;

	inet_aton("10.0.0.111", &(addr.sin_addr));
	addr.sin_port = htons(1111);

	test_IpAddr(&addr, 1234);
	
	inet_aton("255.255.255.1", &(addr.sin_addr));
	addr.sin_port = htons(9999);

	test_IpAddr(&addr, 1234);
	
	inet_aton("128.255.255.1", &(addr.sin_addr));
	addr.sin_port = htons(1);

	test_IpAddr(&addr, 1234);
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

	EXPECT_TRUE(SetTlvIpAddrPortV4((void*)tlvdata, 2048, &outOffset, type, addr));
	std::cerr << "Init Size: " << initsize << std::endl;
	std::cerr << "Serialised Size: " << outOffset << std::endl;
	displayRawPacket(std::cerr, tlvdata, outOffset);

	EXPECT_TRUE(outOffset == initsize); 		/* check that the offset matches the size */

	std::cerr << "DeSerialising" << std::endl;

	/* fails if type is wrong! */
	std::cerr << "### These errors are expected." << std::endl;
	EXPECT_TRUE(0 == GetTlvIpAddrPortV4((void*)tlvdata, outOffset, &inOffset, type-1, &outaddr));
	EXPECT_TRUE(GetTlvIpAddrPortV4((void*)tlvdata, outOffset, &inOffset, type, &outaddr));

	EXPECT_TRUE(initsize == inOffset); 		/* check that the offset matches the size */
	EXPECT_TRUE(addr->sin_addr.s_addr == outaddr.sin_addr.s_addr);	/* check that IP match */
	EXPECT_TRUE(addr->sin_port        == outaddr.sin_port); 		/* check that Port match */
	std::cerr << "Deserialised: Size: " << inOffset << std::endl;
	std::cerr << "Deserialised IPAddr: " << inet_ntoa(outaddr.sin_addr) << std::endl;
	std::cerr << "             Port  : " << ntohs(outaddr.sin_port) << std::endl;

	return 1;
}



