
/*
 * "$Id:$"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2010-2011 by Cyril Soler
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



#include "util/dnsresolver.h"

#include <iostream>
#include <list>
#include <string>

int main()
{
	std::list<std::string> names;

	DNSResolver *r = new DNSResolver ;

	names.push_back("cortinaire.inrialpes.fr") ;
	names.push_back("www.google.com") ;
	names.push_back("www.ego.cn") ;
	names.push_back("free.fr") ;

	for(int i=0;i<5;++i)
	{
		for(std::list<std::string>::const_iterator it = names.begin(); it != names.end(); it++)
		{
			in_addr addr ;
			bool res = r->getIPAddressFromString(*it,addr) ;

			if(res)
				std::cerr << "Lookup of " << *it << ": " << (res?"done":"pending") << ": addr = " << (void*)addr.s_addr << std::endl;
			else
				std::cerr << "Lookup of " << *it << ": " << (res?"done":"pending") << std::endl;
		}

		std::cerr << std::endl;
		usleep(200000) ;
	}

	r->reset() ;

	for(int i=0;i<5;++i)
	{
		for(std::list<std::string>::const_iterator it = names.begin(); it != names.end(); it++)
		{
			in_addr addr ;
			bool res = r->getIPAddressFromString(*it,addr) ;

			if(res)
				std::cerr << "Lookup of " << *it << ": " << (res?"done":"pending") << ": addr = " << (void*)addr.s_addr << std::endl;
			else
				std::cerr << "Lookup of " << *it << ": " << (res?"done":"pending") << std::endl ;
		}
		std::cerr << std::endl;
		usleep(200000) ;
	}

	delete r ;
}

