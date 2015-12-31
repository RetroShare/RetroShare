/*
 * libretroshare/src/dht: p3bitdht.h
 *
 * BitDht interface for RetroShare.
 *
 * Copyright 2009-2011 by Robert Fernie.
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

#include "util/rsnet.h"
#include "dht/p3bitdht.h"

#include "tcponudp/udprelay.h"
#include "bitdht/bdstddht.h"

#include "serialiser/rsconfigitems.h"


/***********************************************************************************************
 ********** External RsDHT Interface for Dht-Relay Control *************************************
************************************************************************************************/

int p3BitDht::setupRelayDefaults()
{
	uint32_t mode = RSDHT_RELAY_ENABLED | RSDHT_RELAY_MODE_OFF;
	setRelayMode(mode);
	
	return 1;
}


/**** THIS IS A TEMPORARY HACK INTERFACE - UNTIL WE HAVE MORE SOFISTICATED SYSTEM 
 * NB: using bdNodeIds here, rather than SSL IDS.
 *****/

int p3BitDht::getRelayServerList(std::list<std::string> &ids)
{
	RsStackMutex stack(dhtMtx); /*********** LOCKED **********/

	ids = mRelayServerList;	

	return 1;
}

int p3BitDht::addRelayServer(std::string id)
{
	RsStackMutex stack(dhtMtx); /*********** LOCKED **********/

	std::list<std::string>::iterator it;
	it = std::find(mRelayServerList.begin(), mRelayServerList.end(), id);
	if (it == mRelayServerList.end())
	{
		mRelayServerList.push_back(id);
	}

	IndicateConfigChanged();
	return 1;
}

int p3BitDht::removeRelayServer(std::string id)
{

	RsStackMutex stack(dhtMtx); /*********** LOCKED **********/

	std::list<std::string>::iterator it;
	it = std::find(mRelayServerList.begin(), mRelayServerList.end(), id);
	if (it != mRelayServerList.end())
	{
		mRelayServerList.erase(it);
	}

	IndicateConfigChanged();
	return 1;
}

int p3BitDht::pushRelayServers()
{
	std::list<std::string> servers;

	{
		RsStackMutex stack(dhtMtx); /*********** LOCKED **********/
		
		servers = mRelayServerList;
	}

	std::list<std::string>::iterator it;
	for(it = servers.begin(); it != servers.end(); ++it)
	{
		/* push it to dht */
		uint32_t bdflags = BITDHT_PEER_STATUS_DHT_RELAY_SERVER;
		bdId id;
		bdStdLoadNodeId(&(id.id), *it);

		mUdpBitDht->updateKnownPeer(&id, 0, bdflags);

	}
	return 1;
}


uint32_t p3BitDht::getRelayMode()
{
	RsStackMutex stack(dhtMtx); /*********** LOCKED **********/

	return mRelayMode;
}

int p3BitDht::setRelayMode(uint32_t mode)
{
	std::cerr << "p3BitDht::setRelayMode(" << mode << ")";
	std::cerr << std::endl;

	if (mode & RSDHT_RELAY_ENABLED)
	{
		mUdpBitDht->ConnectionOptions(
			BITDHT_CONNECT_MODE_DIRECT | BITDHT_CONNECT_MODE_PROXY | BITDHT_CONNECT_MODE_RELAY,
			BITDHT_CONNECT_OPTION_AUTOPROXY);
	}
	else
	{
		mUdpBitDht->ConnectionOptions(
			BITDHT_CONNECT_MODE_DIRECT | BITDHT_CONNECT_MODE_PROXY,
			BITDHT_CONNECT_OPTION_AUTOPROXY);
	}

	int relaymode = mode & RSDHT_RELAY_MODE_MASK;

	switch(relaymode)
	{
		case RSDHT_RELAY_MODE_OFF:
			mUdpBitDht->setDhtMode(BITDHT_MODE_RELAYSERVERS_IGNORED);
			break;
		case RSDHT_RELAY_MODE_ON:
			pushRelayServers();
			mUdpBitDht->setDhtMode(BITDHT_MODE_RELAYSERVERS_FLAGGED);
			break;
		case RSDHT_RELAY_MODE_SERVER:
			pushRelayServers();
			mUdpBitDht->setDhtMode(BITDHT_MODE_RELAYSERVERS_SERVER);
			break;
	}

	{
		RsStackMutex stack(dhtMtx); /*********** LOCKED **********/
		mRelayMode = mode;
	}

	IndicateConfigChanged();

	return 1;
}

int p3BitDht::getRelayAllowance(int classIdx, uint32_t &count, uint32_t &bandwidth)
{
	std::cerr << "p3BitDht::getRelayAllowance(" << classIdx << "): ";
	if ((classIdx >= 0) && (classIdx < RSDHT_RELAY_NUM_CLASS))
	{
		count = mRelay->getRelayClassMax(classIdx);
		bandwidth = mRelay->getRelayClassBandwidth(classIdx);

		std::cerr << " count: " << count << " bandwidth: " << bandwidth;
		std::cerr << std::endl;
		return 1;
	}
	std::cerr << " Invalid classIdx";
	std::cerr << std::endl;

	return 0;
}

int p3BitDht::setRelayAllowance(int classIdx, uint32_t count, uint32_t bandwidth)
{
	std::cerr << "p3BitDht::getRelayAllowance(" << classIdx << ", ";
	std::cerr << ", " << count << ", " << bandwidth << ")";
	std::cerr << std::endl;

	int retval = mRelay->setRelayClassMax(classIdx, count, bandwidth);
	IndicateConfigChanged();

	return retval;
}


/***********************************************************************************************
 ********** External RsDHT Interface for Dht-Relay Control *************************************
************************************************************************************************/

/*****************************************************************/
/***********************  p3config  ******************************/
        /* Key Functions to be overloaded for Full Configuration */
RsSerialiser *p3BitDht::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser ;
	rss->addSerialType(new RsGeneralConfigSerialiser());
	return rss ;
}


bool p3BitDht::saveList(bool &cleanup, std::list<RsItem *> &saveList)
{
	cleanup = true;

	std::cerr << "p3BitDht::saveList()";
	std::cerr << std::endl;


	RsStackMutex stack(dhtMtx); /*********** LOCKED **********/
	RsConfigKeyValueSet *config = new RsConfigKeyValueSet();

	RsTlvKeyValue kv;

	/* Push Relay Class Stuff */
	int i;
	for(i = 0; i < RSDHT_RELAY_NUM_CLASS; ++i)
	{
		rs_sprintf(kv.key, "RELAY_CLASS%d_COUNT", i);
		rs_sprintf(kv.value, "%d", mRelay->getRelayClassMax(i));
		config->tlvkvs.pairs.push_back(kv);

        rs_sprintf(kv.key, "RELAY_CLASS%d_BANDWIDTH", i);
        rs_sprintf(kv.value, "%d", mRelay->getRelayClassBandwidth(i));
        config->tlvkvs.pairs.push_back(kv);
	}

	/* add RelayMode */
	{
		kv.key = "RELAY_MODE";
		rs_sprintf(kv.value, "%lu", mRelayMode);
		config->tlvkvs.pairs.push_back(kv);
	}

	/* add Servers */
	std::list<std::string>::iterator it;
	for(i = 0, it = mRelayServerList.begin(); it != mRelayServerList.end(); ++it, ++i)
	{
		rs_sprintf(kv.key, "RELAY_SERVER%d", i);
		kv.value = *it;
		config->tlvkvs.pairs.push_back(kv);
	}

	std::cerr << "BITDHT Save Item:";
	std::cerr << std::endl;

	config->print(std::cerr, 0);

    saveList.push_back(config);

	return true;
}

void p3BitDht::saveDone()
{
	return;
}

bool    p3BitDht::loadList(std::list<RsItem *>& load)
{
	std::cerr << "p3BitDht::loadList()";
	std::cerr << std::endl;

	if (load.empty() || (load.size() > 1))
	{
		/* error */
		std::cerr << "p3BitDht::loadList() Error only expecting 1 item";
		std::cerr << std::endl;
        
        	for(std::list<RsItem*>::iterator it=load.begin();it!=load.end();++it)
		    delete *it ;
            
            	load.clear() ;
		return false;
	}
	RsItem *item = load.front();

	RsConfigKeyValueSet *config = dynamic_cast<RsConfigKeyValueSet *>(item);

	if (!config)
	{
		/* error */
		std::cerr << "p3BitDht::loadList() Error expecting item = config";
		std::cerr << std::endl;
		delete item ;
		return false;
	}

	std::cerr << "BITDHT Load Item:";
	std::cerr << std::endl;

	config->print(std::cerr, 0);

	std::list<std::string> servers;
	int peers[RSDHT_RELAY_NUM_CLASS] = {0};
	int bandwidth[RSDHT_RELAY_NUM_CLASS] = {0};

	bool haveMode = false;
	int mode = 0;

	std::list<RsTlvKeyValue>::iterator it;
	for(it = config->tlvkvs.pairs.begin(); it != config->tlvkvs.pairs.end(); ++it)
	{
		std::string key = it->key;
		std::string value = it->value;
		if (0 == strncmp(key.c_str(), "RELAY_SERVER", 12))
		{
			/* add to RELAY_SERVER List */
			servers.push_back(value);
			std::cerr << "p3BitDht::loadList() Found Server: " << value;
			std::cerr << std::endl;
		}
		else if (0 == strncmp(key.c_str(), "RELAY_MODE", 10))
		{
			mode = atoi(value.c_str());
			haveMode = true;

			std::cerr << "p3BitDht::loadList() Found Mode: " << mode;
			std::cerr << std::endl;
		}
		else if (0 == strncmp(key.c_str(), "RELAY_CLASS", 11))
		{
			/* len check */
			if (key.length() < 14)
				continue;

			int idx = 0;
			uint32_t val = atoi(value.c_str());
			switch(key[11])
			{
				default:
				case '0':
					idx = 0;
					break;
				case '1':
					idx = 1;
					break;
				case '2':
					idx = 2;
					break;
				case '3':
					idx = 3;
					break;
			}

			if (key[13] == 'C')
			{
				std::cerr << "p3BitDht::loadList() Found Count(" << idx << "): ";
				std::cerr << val;
				std::cerr << std::endl;
				peers[idx] = val;
			}
			else
			{
				std::cerr << "p3BitDht::loadList() Found Bandwidth(" << idx << "): ";
				std::cerr << val;
				std::cerr << std::endl;
				bandwidth[idx] = val;
			}
		}
		else
		{
			std::cerr << "p3BitDht::loadList() Unknown Key:value: " << key;
			std::cerr << ":" << value;
			std::cerr << std::endl;
		}
	}


	// Cleanup config.
	delete config; 

	{
		RsStackMutex stack(dhtMtx); /*********** LOCKED **********/
		mRelayServerList = servers;
	}
	
	int i;
	for(i = 0; i < RSDHT_RELAY_NUM_CLASS; ++i)
	{
		mRelay->setRelayClassMax(i, peers[i], bandwidth[i]);
	}

	if (haveMode)
	{
		setRelayMode(mode);
	}

    	load.clear() ;
	return true;
}

/*****************************************************************/

