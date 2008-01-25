/*
 * libretroshare/src/dht: opendht.h
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

#ifndef RS_OPEN_DHT_CLIENT_H
#define RS_OPEN_DHT_CLIENT_H

#include "pqi/pqinetwork.h"
#include "util/rsthreads.h"

#include <inttypes.h>
#include <string>
#include <list>
#include <map>

#include "dht/dhtclient.h"

class dhtServer
{
	public:

	std::string host;
	uint16_t    port;
	uint16_t    failed;
	time_t      ts;
	struct sockaddr_in addr;
};

class OpenDHTClient: public DHTClient
{
	public:

virtual	bool publishKey(std::string key, std::string value, uint32_t ttl);
virtual	bool searchKey(std::string key, std::list<std::string> &values);

	/* Fns accessing data */
virtual bool loadServers(std::string filename);
virtual bool dhtActive();

	private:
bool 	getServer(std::string &host, uint16_t &port, struct sockaddr_in &addr);
bool 	setServerIp(std::string host, struct sockaddr_in addr);
void 	setServerFailed(std::string host);

	private:

	/* generic send msg */
	bool openDHT_sendMessage(std::string msg, std::string &response);

	RsMutex dhtMutex;
	std::map<std::string, dhtServer> mServers;
	uint32_t mDHTFailCount;

};


#endif


