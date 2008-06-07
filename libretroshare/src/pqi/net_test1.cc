/*
 * libretroshare/src/pqi net_test.cc
 *
 * 3P/PQI network interface for RetroShare.
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

/******
 * NETWORKING Test to check Big/Little Endian behaviour
 * as well as socket behaviour
 *
 */

#include "pqi/pqinetwork.h"
#include "util/rsnet.h"
#include <iostream>
#include <sstream>
#include "util/utest.h"

const char * loopback_addrstr = "127.0.0.1";

const char * localnet1_addrstr = "10.0.0.1";
const char * localnet2_addrstr = "169.254.0.1";
const char * localnet3_addrstr = "172.16.0.1";
const char * localnet4_addrstr = "192.168.1.1";

const char * localnet5_addrstr = "10.4.28.34";
const char * localnet6_addrstr = "169.254.1.81";
const char * localnet7_addrstr = "172.20.9.201";
const char * localnet8_addrstr = "192.168.1.254";

const char * external_addrstr = "74.125.19.99"; /* google */
const char * invalid_addrstr = "AAA.BBB.256.256";

int test_isExternalNet();
int test_isPrivateNet();
int test_isLoopbackNet();
int test_sameNet();
int test_isValidNet();
int test_isSameSubnet();
int test_pqi_inet_netof();

INITTEST();

int main(int argc, char **argv)
{
	std::cerr << "net_test1" << std::endl;

	test_isExternalNet();
	test_isPrivateNet();
	test_isLoopbackNet();
	test_sameNet();
	test_isValidNet();
	test_isSameSubnet();
	test_pqi_inet_netof();

	FINALREPORT("net_test1");

	return TESTRESULT();
}

int test_isExternalNet()
{
	struct in_addr loopback_addr;
	struct in_addr localnet1_addr;
	struct in_addr localnet2_addr;
	struct in_addr localnet3_addr;
	struct in_addr localnet4_addr;
	struct in_addr external_addr;
	struct in_addr invalid_addr;
	struct in_addr invalid_addr2;

	inet_aton(loopback_addrstr, &loopback_addr);
	inet_aton(localnet1_addrstr, &localnet1_addr);
	inet_aton(localnet2_addrstr, &localnet2_addr);
	inet_aton(localnet3_addrstr, &localnet3_addr);
	inet_aton(localnet4_addrstr, &localnet4_addr);
	inet_aton(external_addrstr, &external_addr);
	invalid_addr.s_addr = 0;
	invalid_addr2.s_addr = -1;

	CHECK(isExternalNet(&loopback_addr)==false);
	CHECK(isExternalNet(&localnet1_addr)==false);
	CHECK(isExternalNet(&localnet2_addr)==false);
	CHECK(isExternalNet(&localnet3_addr)==false);
	CHECK(isExternalNet(&localnet4_addr)==false);
	CHECK(isExternalNet(&external_addr)==true);
	CHECK(isExternalNet(&invalid_addr)==false);
	CHECK(isExternalNet(&invalid_addr2)==false);

	REPORT("isExternalNet()");

	return 1;
}

int test_isPrivateNet()
{
	struct in_addr loopback_addr;
	struct in_addr localnet1_addr;
	struct in_addr localnet2_addr;
	struct in_addr localnet3_addr;
	struct in_addr localnet4_addr;
	struct in_addr external_addr;

	inet_aton(loopback_addrstr, &loopback_addr);
	inet_aton(localnet1_addrstr, &localnet1_addr);
	inet_aton(localnet2_addrstr, &localnet2_addr);
	inet_aton(localnet3_addrstr, &localnet3_addr);
	inet_aton(localnet4_addrstr, &localnet4_addr);
	inet_aton(external_addrstr, &external_addr);

	CHECK(isPrivateNet(&loopback_addr)==false); //loopback not considered a "private network"
	CHECK(isPrivateNet(&localnet1_addr)==true);
	CHECK(isPrivateNet(&localnet2_addr)==true);
	CHECK(isPrivateNet(&localnet3_addr)==true);
	CHECK(isPrivateNet(&localnet4_addr)==true);
	CHECK(isPrivateNet(&external_addr)==false);

	REPORT("isPrivateNet()");

	return 1;
}

int test_isLoopbackNet()
{
	struct in_addr loopback_addr;
	struct in_addr localnet1_addr;
	struct in_addr external_addr;

	inet_aton(loopback_addrstr, &loopback_addr);
	inet_aton(localnet1_addrstr, &localnet1_addr);
	inet_aton(external_addrstr, &external_addr);

	CHECK(isLoopbackNet(&loopback_addr)==true);
	CHECK(isLoopbackNet(&localnet1_addr)==false);
	CHECK(isLoopbackNet(&external_addr)==false);

	REPORT("isLoopbackNet()");

	return 1;
}

int test_sameNet()
{
	struct in_addr localnet1_addr;
	struct in_addr localnet2_addr;
	struct in_addr localnet3_addr;
	struct in_addr localnet4_addr;
	struct in_addr localnet5_addr;
	struct in_addr localnet6_addr;
	struct in_addr localnet7_addr;
	struct in_addr localnet8_addr;
	struct in_addr external_addr;

	inet_aton(localnet1_addrstr, &localnet1_addr);
	inet_aton(localnet2_addrstr, &localnet2_addr);
	inet_aton(localnet3_addrstr, &localnet3_addr);
	inet_aton(localnet4_addrstr, &localnet4_addr);
	inet_aton(localnet5_addrstr, &localnet5_addr);
	inet_aton(localnet6_addrstr, &localnet6_addr);
	inet_aton(localnet7_addrstr, &localnet7_addr);
	inet_aton(localnet8_addrstr, &localnet8_addr);
	inet_aton(external_addrstr, &external_addr);

	CHECK(sameNet(&localnet1_addr, &localnet5_addr)==true);
	CHECK(sameNet(&localnet2_addr, &localnet6_addr)==true);
	CHECK(sameNet(&localnet3_addr, &localnet7_addr)==true);
	CHECK(sameNet(&localnet4_addr, &localnet8_addr)==true);
	CHECK(sameNet(&localnet1_addr, &external_addr)==false);
	CHECK(sameNet(&localnet2_addr, &external_addr)==false);
	CHECK(sameNet(&localnet3_addr, &external_addr)==false);
	CHECK(sameNet(&localnet4_addr, &external_addr)==false);

	REPORT("sameNet()");

	return 1;
}

int test_isValidNet()
{
	struct in_addr localnet1_addr;
	struct in_addr invalid_addr;

	inet_aton(localnet1_addrstr, &localnet1_addr);
	CHECK(isValidNet(&localnet1_addr)==true);

	CHECK(inet_aton(invalid_addrstr, &invalid_addr)==0);
	std::cerr << inet_ntoa(invalid_addr) << std::endl;
	//CHECK(isValidNet(&invalid_addr)==false);

	REPORT("isValidNet()");

	return 1;
}

int test_isSameSubnet()
{
	struct in_addr localnet1_addr;
	struct in_addr classc1_addr;
	struct in_addr classc2_addr;

	inet_aton(localnet1_addrstr, &localnet1_addr);
	//random class C addresses
	inet_aton("197.67.28.93", &classc1_addr);
	inet_aton("197.67.28.3", &classc2_addr);

	CHECK(isSameSubnet(&localnet1_addr, &classc1_addr)==false);
	CHECK(isSameSubnet(&classc1_addr, &classc2_addr)==true);

	REPORT("isSameSubnet()");

	return 1;
}

int test_pqi_inet_netof()
{
	struct in_addr localnet1_addr;
	struct in_addr localnet2_addr;
	struct in_addr localnet3_addr;
	struct in_addr localnet4_addr;
	struct in_addr localnet5_addr;
	struct in_addr localnet6_addr;
	struct in_addr localnet7_addr;
	struct in_addr localnet8_addr;
	struct in_addr external_addr;

	inet_aton(localnet1_addrstr, &localnet1_addr);
	inet_aton(localnet2_addrstr, &localnet2_addr);
	inet_aton(localnet3_addrstr, &localnet3_addr);
	inet_aton(localnet4_addrstr, &localnet4_addr);
	inet_aton(localnet5_addrstr, &localnet5_addr);
	inet_aton(localnet6_addrstr, &localnet6_addr);
	inet_aton(localnet7_addrstr, &localnet7_addr);
	inet_aton(localnet8_addrstr, &localnet8_addr);
	inet_aton(external_addrstr, &external_addr);

	CHECK(pqi_inet_netof(localnet1_addr)==htonl(10<<24));
	CHECK(pqi_inet_netof(localnet2_addr)==htonl(169<<24 | 254<<16));
	CHECK(pqi_inet_netof(localnet3_addr)==htonl(172<<24 | 16<<16));
	CHECK(pqi_inet_netof(localnet4_addr)==htonl(192<<24 | 168<<16 | 1<<8));
	CHECK(pqi_inet_netof(localnet5_addr)==htonl(10<<24));
	CHECK(pqi_inet_netof(localnet6_addr)==htonl(169<<24 | 254<<16));
	CHECK(pqi_inet_netof(localnet7_addr)==htonl(172<<24 | 20<<16));
	CHECK(pqi_inet_netof(localnet8_addr)==htonl(192<<24 | 168<<16 | 1<<8));
	CHECK(pqi_inet_netof(external_addr)==htonl(74<<24));

	REPORT("pqi_inet_netof()");

	return 1;
}
