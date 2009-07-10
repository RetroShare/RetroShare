/*
 * libretroshare/src/dht: odhtport_test.cc
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


#include "opendht.h"
#include "util/rsprint.h"

#include <openssl/sha.h>

int main()
{
	std::string agent = "Hand-Crafted Thread";
	std::string keyIn = "color";
	//std::string value = "aaaaBBBBccccDDDDe";
	std::string value = "1234567890aaaaBBBBccccDDDDe";
	//std::string value = "12345678901234567890aaaaBBBBccccDDDDe";
	//std::string value = "aaa12345678901234567890aaaaBBBBccccDDDDe";
	uint32_t    ttl   = 600;
	std::string client= "Retroshare v0.4";
	uint32_t   maxresp= 1024;

	std::string key = RsUtil::HashId(keyIn, false);

	OpenDHTClient dht;
	dht.loadServers("./servers.txt");

	dht.publishKey(key, value, ttl);

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
        sleep(10);
#else
        Sleep(10000);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

	std::list<std::string> values;
	dht.searchKey(key, values);


	return 1;
}

