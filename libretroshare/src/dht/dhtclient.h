/*
 * libretroshare/src/dht: dhtclient.h
 *
 * Interface with DHT Client for RetroShare.
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

#ifndef RS_GENERIC_DHT_CLIENT_H
#define RS_GENERIC_DHT_CLIENT_H

#include <inttypes.h>
#include <string>
#include <list>

class DHTClient
{
	public:

	/* initialise from file */
virtual bool checkServerFile(std::string filename) = 0;
virtual bool loadServers(std::string filename) = 0;
virtual bool loadServersFromWeb(std::string storename) = 0;
virtual bool loadServers(std::istream&) = 0;

	/* check that its working */
virtual bool dhtActive() = 0;

	/* publish / search */
virtual	bool publishKey(std::string key, std::string value, uint32_t ttl) = 0;
virtual	bool searchKey(std::string key, std::list<std::string> &values) = 0;

};


class DHTClientDummy: public DHTClient
{
	public:

	/* initialise from file */
virtual bool checkServerFile(std::string) { return true; }
virtual bool loadServers(std::string) { return true; }
virtual bool loadServersFromWeb(std::string) { return true; }
virtual bool loadServers(std::istream&) { return true; }

	/* check that its working */
virtual bool dhtActive() { return true; }

	/* publish / search */
virtual	bool publishKey(std::string, std::string, uint32_t) { return true; }
virtual	bool searchKey(std::string, std::list<std::string> &)   { return true; }

};


#endif

