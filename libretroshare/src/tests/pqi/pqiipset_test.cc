/*
 * libretroshare/src/test/pqi pqiipset_test.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2007-2010 by Robert Fernie.
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

#include "util/utest.h"

#include "pqi/pqiipset.h"
#include "pqi/pqinetwork.h"
#include "util/rsnet.h"
#include <iostream>
#include <sstream>


bool test_addr_list();


INITTEST();

int main(int argc, char **argv)
{
	std::cerr << "libretroshare:pqi " << argv[0] << std::endl;

	test_addr_list();

	FINALREPORT("libretroshare pqiipset Tests");
	return TESTRESULT();
}

	/* test 1: byte manipulation */
bool test_addr_list()
{
	pqiIpAddress addr;
	pqiIpAddrList list;

	for(int i = 100; i < 150; i++)
	{
		std::ostringstream out;
		out << "192.168.2." << i;
		inet_aton(out.str().c_str(), &(addr.mAddr.sin_addr));
		addr.mAddr.sin_port = htons(7812);
		addr.mSeenTime = time(NULL) - i;

		list.updateIpAddressList(addr);

		if (i < 100 + 4)
		{
			/* check that was added to the back */
			CHECK(list.mAddrs.back().mSeenTime == addr.mSeenTime);
			CHECK(list.mAddrs.back().mAddr.sin_addr.s_addr == addr.mAddr.sin_addr.s_addr);
			CHECK(list.mAddrs.back().mAddr.sin_port == addr.mAddr.sin_port);
		}
		else
		{
			/* check that wasn't added to the back */
			CHECK(list.mAddrs.back().mSeenTime != addr.mSeenTime);
			CHECK(list.mAddrs.back().mAddr.sin_addr.s_addr != addr.mAddr.sin_addr.s_addr);
		}
	}

	/* print out the list */
	std::cerr << "IpAddressList (expect variation: 192.168.2.[100-103]:7812)";
	std::cerr << std::endl;
	std::string out ;
	list.printIpAddressList(out) ;
	std::cerr << out << std::endl;

	const uint32_t expectedListSize = 4;
	CHECK(list.mAddrs.size() == expectedListSize);

	rstime_t min_time = time(NULL) - expectedListSize + 100;

	/* expect the most recent ones to appear */
	std::list<pqiIpAddress>::iterator it;
	for(it = list.mAddrs.begin(); it != list.mAddrs.end(); it++)
	{
		CHECK(it->mSeenTime < min_time);
	}

	/* now add some with same address + port */
	{
		std::ostringstream out;
		out << "192.168.2.200";
		inet_aton(out.str().c_str(), &(addr.mAddr.sin_addr));
		addr.mAddr.sin_port = htons(8812);
	}

	/* make sure it more recent than the previous ones */
	for(int i = 99; i > 89; i--)
	{
		addr.mSeenTime = time(NULL) - i;
		list.updateIpAddressList(addr);

		/* check that was added to the front */
		CHECK(list.mAddrs.front().mSeenTime == addr.mSeenTime);
		CHECK(list.mAddrs.front().mAddr.sin_addr.s_addr == addr.mAddr.sin_addr.s_addr);
		CHECK(list.mAddrs.front().mAddr.sin_port == addr.mAddr.sin_port);
	}

	/* print out the list */
	std::cerr << "IpAddressList (first item to be 192.168.2.200:8812)";
	std::cerr << std::endl;
	list.printIpAddressList(out) ;
	std::cerr << out<< std::endl;
	
	/* now add with the different ports */

	for(int i = 70; i > 50; i--)
	{
		addr.mAddr.sin_port = htons(8000 + i);
		addr.mSeenTime = time(NULL) - i;

		list.updateIpAddressList(addr);

		/* check that was added to the back */
		CHECK(list.mAddrs.front().mSeenTime == addr.mSeenTime);
		CHECK(list.mAddrs.front().mAddr.sin_addr.s_addr == addr.mAddr.sin_addr.s_addr);
		CHECK(list.mAddrs.front().mAddr.sin_port == addr.mAddr.sin_port);

	}

	std::cerr << "IpAddressList (expect same Ip, but variations in port)";
	std::cerr << std::endl;
	list.printIpAddressList(out) ;
	std::cerr << out <<std::endl;

        REPORT("pqiIpAddrList Test");

	return 1;
}

