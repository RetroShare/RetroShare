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


bool test_iface();
bool test_iface_loop();

int main(int argc, char **argv)
{
	bool repeat = false;
	if (argc > 1)
	{
		repeat = true;
	}

	test_iface();

	if (repeat)
	{
		test_iface_loop();
	}

	return 1;
}

	/* test 1: single check if interfaces */
bool test_iface()
{
	struct in_addr pref_iface;
	std::list<struct in_addr> ifaces;
	std::list<struct in_addr>::iterator it;

	getPreferredInterface(pref_iface); 
	getLocalInterfaces(ifaces);

	std::cerr << "test_iface()" << std::endl;
	for(it = ifaces.begin(); it != ifaces.end(); it++)
	{
		std::cerr << "available iface: " << rs_inet_ntoa(*it) << std::endl;
	}
	std::cerr << "preferred " << rs_inet_ntoa(pref_iface) << std::endl;

	return true;
}

	/* test 2: catch changing interfaces */
bool test_iface_loop()
{
	std::cerr << "test_iface_loop()" << std::endl;
	struct in_addr curr_pref_iface;
	getPreferredInterface(curr_pref_iface); 
	time_t start = time(NULL);
	while(1)
	{
		struct in_addr pref_iface;
		std::list<struct in_addr> ifaces;
		std::list<struct in_addr>::iterator it;

		getPreferredInterface(pref_iface); 
		getLocalInterfaces(ifaces);


		std::cerr << "T(" << time(NULL) - start << ") ";

		for(it = ifaces.begin(); it != ifaces.end(); it++)
		{
			std::cerr << " I: " << rs_inet_ntoa(*it);
		}
		std::cerr << " P: " << rs_inet_ntoa(pref_iface) << std::endl;

		if (curr_pref_iface.s_addr !=
			pref_iface.s_addr)
		{
			std::cerr << "+++++++++++++ Address CHANGED ++++++++++";
			std::cerr << std::endl;
			std::cerr << "+++++++++++++ Address CHANGED ++++++++++";
			std::cerr << std::endl;
			std::cerr << "+++++++++++++ Address CHANGED ++++++++++";
			std::cerr << std::endl;

			curr_pref_iface = pref_iface;
		}

		sleep(1);
	}
	return true;
}


