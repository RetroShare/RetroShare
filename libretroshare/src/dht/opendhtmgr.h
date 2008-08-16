/*
 * libretroshare/src/dht: opendhtmgr.h
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

#ifndef RS_OPEN_DHT_MGR_H
#define RS_OPEN_DHT_MGR_H

#include "pqi/p3dhtmgr.h"
#include "dht/dhtclient.h"

#include <string>
#include <inttypes.h>
#include <list>

class OpenDHTMgr: public p3DhtMgr
{
	public:

	OpenDHTMgr(std::string ownId, pqiConnectCb* cb, std::string configdir);

        protected:

	/********** OVERLOADED FROM p3DhtMgr ***************/
virtual bool    dhtInit();
virtual bool    dhtShutdown();
virtual bool    dhtActive();
virtual int     status(std::ostream &out);

/* Blocking calls (only from thread) */
virtual bool publishDHT(std::string key, std::string value, uint32_t ttl);
virtual bool searchDHT(std::string key);

	/********** OVERLOADED FROM p3DhtMgr ***************/

	private:

	DHTClient *mClient;
	std::string mConfigDir;
};


#endif


