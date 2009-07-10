/*
 * libretroshare/src/dht: odhtstr_test.cc
 *
 * Interface with OpenDHT for RetroShare.
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

#include "opendhtstr.h"
#include "b64.h"
#include <iostream>

int main()
{

	std::string host = "abc.bgg.trer.dgg";
	uint16_t    port = 9242;

	std::string agent = "Hand-Crafted Thread";
	std::string key   = "BigKey";
	std::string value = "96324623924";
	uint32_t    ttl   = 600;
	std::string client= "Retroshare v0.4";
	uint32_t   maxresp= 1024;

	/* some encodings */
	std::string i1 = "color";
	std::string i2 = "green";
	std::string i3 = "blue";
	std::string i4 = "abdflhdffjadlgfal12=345==";

	std::string o1 = convertToBase64(i1);
	std::string o2 = convertToBase64(i2);
	std::string o3 = convertToBase64(i3);
	std::string o4 = convertToBase64(i4);
	std::string o5 = "bdD+gAEUW+xKEtDiLacRxJcNAAs=";
	
	std::cerr << "In:" << i1 << " encoded:" << o1 << " decoded:" << convertFromBase64(o1);
	std::cerr << std::endl;
	std::cerr << "In:" << i2 << " encoded:" << o2 << " decoded:" << convertFromBase64(o2);
	std::cerr << std::endl;
	std::cerr << "In:" << i3 << " encoded:" << o3 << " decoded:" << convertFromBase64(o3);
	std::cerr << std::endl;
	std::cerr << "In:" << i4 << " encoded:" << o4 << " decoded:" << convertFromBase64(o4);
	std::cerr << std::endl;
	std::cerr << "Encoded:" << o5 << " decoded:" << convertFromBase64(o5);
	std::cerr << std::endl;


	/* create some strings */

	std::string req1 = createOpenDHT_put(key, value, ttl, client);
	std::string req2 = createOpenDHT_get(key, maxresp, client);

	std::string putheader = createHttpHeader(host, port, agent, req1.length());
	std::string getheader = createHttpHeader(host, port, agent, req2.length());

	std::string putreq = putheader + req1;
	std::string getreq = getheader + req2;


	std::cerr << "Example Put Request is:" << std::endl;
	std::cerr << putreq << std::endl;

	std::cerr << "Example Get Request is:" << std::endl;
	std::cerr << getreq << std::endl;

	return 1;
}


