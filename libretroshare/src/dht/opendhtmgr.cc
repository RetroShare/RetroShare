/*
 * libretroshare/src/dht: opendhtmgr.cc
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

#include "dht/opendhtmgr.h"
#include "dht/opendht.h"
#include "util/rsthreads.h" /* for pthreads headers */


class dhtSearchData
{
	public:
	OpenDHTMgr *mgr;
	DHTClient *client;
	std::string key;
};


class dhtPublishData
{
	public:
	OpenDHTMgr *mgr;
	DHTClient *client;
	std::string key;
	std::string value;
	uint32_t    ttl;
};

/* Thread routines */

extern "C" void* doDhtPublish(void* p)
{
	dhtPublishData *data = (dhtPublishData *) p;
  	if ((!data) || (!data->mgr) || (!data->client))
	{
		pthread_exit(NULL);
	}

	/* publish it! */
	data->client->publishKey(data->key, data->value, data->ttl);

	delete data;
	pthread_exit(NULL);

	return NULL;
}


extern "C" void* doDhtSearch(void* p)
{
	dhtSearchData *data = (dhtSearchData *) p;
  	if ((!data) || (!data->mgr) || (!data->client))
	{
		pthread_exit(NULL);

		return NULL;
	}

	/* search it! */
	std::list<std::string> values;

	if (data->client->searchKey(data->key, values))
	{
		/* callback */
		std::list<std::string>::iterator it;
		for(it = values.begin(); it != values.end(); it++)
		{
			data->mgr->resultDHT(data->key, *it);
		}
	}

	delete data;
	pthread_exit(NULL);

	return NULL;
}




OpenDHTMgr::OpenDHTMgr(std::string ownId, pqiConnectCb* cb, std::string configdir)
	:p3DhtMgr(ownId, cb), mConfigDir(configdir)
{
	return;
}


        /********** OVERLOADED FROM p3DhtMgr ***************/
bool    OpenDHTMgr::init()
{
	std::string configpath = mConfigDir;

	/* load up DHT gateways */
	mClient = new OpenDHTClient();
	//mClient = new DHTClientDummy();

	std::string filename = configpath;
	if (configpath.size() > 0)
	{
		filename += "/";
	}
	filename += "ODHTservers.txt";

	/* check file date first */
	if (mClient -> checkServerFile(filename))
	{
		return mClient -> loadServers(filename);
	}
	else if (!mClient -> loadServersFromWeb(filename))
	{
		return mClient -> loadServers(filename);
	}
	return true;
}

bool    OpenDHTMgr::shutdown()
{
	/* do nothing */
	if (mClient)
	{
		delete mClient;
		mClient = NULL;
		return true;
	}

	return false;
}

bool    OpenDHTMgr::dhtActive()
{
	/* do nothing */
	if ((mClient) && (mClient -> dhtActive()))
	{
		return true;
	}
	return false;
}

int     OpenDHTMgr::status(std::ostream &out)
{
	/* do nothing */
	return 1;
}


/* Blocking calls (only from thread) */
bool OpenDHTMgr::publishDHT(std::string key, std::string value, uint32_t ttl)
{
	/* launch a publishThread */
	pthread_t tid;

	dhtPublishData *pub = new dhtPublishData;
	pub->mgr = this;
	pub->client = mClient;
	pub->key = key;
	pub->value = value;
	pub->ttl = ttl;

        void *data = (void *) pub;
	pthread_create(&tid, 0, &doDhtPublish, data);
	pthread_detach(tid); /* so memory is reclaimed in linux */

	return true;
}

bool OpenDHTMgr::searchDHT(std::string key)
{
	/* launch a publishThread */
	pthread_t tid;

	dhtSearchData *dht = new dhtSearchData;
	dht->mgr = this;
	dht->client = mClient;
	dht->key = key;

        void *data = (void *) dht;
	pthread_create(&tid, 0, &doDhtSearch, data);
	pthread_detach(tid); /* so memory is reclaimed in linux */

	return true;
}

        /********** OVERLOADED FROM p3DhtMgr ***************/





