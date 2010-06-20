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

int main(int argc, char **argv)
{

	test_iface();
	return 1;
}

	/* test 1: byte manipulation */
bool test_iface()
{
	struct in_addr pref_iface  = getPreferredInterface(); 
	std::list<std::string> ifaces = getLocalInterfaces();
	std::list<std::string>::iterator it;
	std::cerr << "test_iface()" << std::endl;
	for(it = ifaces.begin(); it != ifaces.end(); it++)
	{
		std::cerr << "available iface: " << *it << std::endl;
	}
	std::cerr << "preferred " << inet_ntoa(pref_iface) << std::endl;

	return true;
}


