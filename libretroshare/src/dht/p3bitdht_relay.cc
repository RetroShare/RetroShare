/*******************************************************************************
 * libretroshare/src/dht: p3bitdht_relay.cc                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2009-2011 by Robert Fernie. <drbob@lunamutt.com>                  *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/

#include "util/rsnet.h"
#include "dht/p3bitdht.h"

#include "tcponudp/udprelay.h"
#include "bitdht/bdstddht.h"

#include "rsitems/rsconfigitems.h"


/***********************************************************************************************
 ********** External RsDHT Interface for Dht-Relay Control *************************************
************************************************************************************************/

int p3BitDht::setupRelayDefaults()
{
	RsDhtRelayMode mode = RsDhtRelayMode::ENABLED | RsDhtRelayMode::OFF;
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


RsDhtRelayMode p3BitDht::getRelayMode()
{
	RsStackMutex stack(dhtMtx); /*********** LOCKED **********/

	return mRelayMode;
}

int p3BitDht::setRelayMode(RsDhtRelayMode mode)
{
	std::cerr << "p3BitDht::setRelayMode(" << mode << ")";
	std::cerr << std::endl;

	if (!!(mode & RsDhtRelayMode::ENABLED))
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

	RsDhtRelayMode relaymode = mode & RsDhtRelayMode::MASK;

	switch(relaymode)
	{
	    case RsDhtRelayMode::OFF:
			mUdpBitDht->setDhtMode(BITDHT_MODE_RELAYSERVERS_IGNORED);
			break;
	    case RsDhtRelayMode::ON:
			pushRelayServers();
			mUdpBitDht->setDhtMode(BITDHT_MODE_RELAYSERVERS_FLAGGED);
			break;
	    case RsDhtRelayMode::SERVER:
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

int p3BitDht::getRelayAllowance(RsDhtRelayClass classIdx, uint32_t &count, uint32_t &bandwidth)
{
	std::cerr << "p3BitDht::getRelayAllowance(" << static_cast<typename std::underlying_type<RsDhtRelayClass>::type>(classIdx) << "): ";
	if ((classIdx >= RsDhtRelayClass::ALL) && (classIdx < RsDhtRelayClass::NUM_CLASS))
	{
		count = mRelay->getRelayClassMax(static_cast<typename std::underlying_type<RsDhtRelayClass>::type>(classIdx));
		bandwidth = mRelay->getRelayClassBandwidth(static_cast<typename std::underlying_type<RsDhtRelayClass>::type>(classIdx));

		std::cerr << " count: " << count << " bandwidth: " << bandwidth;
		std::cerr << std::endl;
		return 1;
	}
	std::cerr << " Invalid classIdx";
	std::cerr << std::endl;

	return 0;
}

int p3BitDht::setRelayAllowance(RsDhtRelayClass classIdx, uint32_t count, uint32_t bandwidth)
{
	std::cerr << "p3BitDht::getRelayAllowance(" << static_cast<typename std::underlying_type<RsDhtRelayClass>::type>(classIdx) << ", ";
	std::cerr << ", " << count << ", " << bandwidth << ")";
	std::cerr << std::endl;

	int retval = mRelay->setRelayClassMax(static_cast<typename std::underlying_type<RsDhtRelayClass>::type>(classIdx), count, bandwidth);
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
	for(i = 0; i < static_cast<typename std::underlying_type<RsDhtRelayClass>::type>(RsDhtRelayClass::NUM_CLASS); ++i)
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

	//std::cerr << "BITDHT Load Item:";
	//std::cerr << std::endl;

	//config->print(std::cerr, 0);

	std::list<std::string> servers;
	int peers[static_cast<typename std::underlying_type<RsDhtRelayClass>::type>(RsDhtRelayClass::NUM_CLASS)] = {0};
	int bandwidth[static_cast<typename std::underlying_type<RsDhtRelayClass>::type>(RsDhtRelayClass::NUM_CLASS)] = {0};

	bool haveMode = false;
	RsDhtRelayMode mode = RsDhtRelayMode::DISABLED;

	std::list<RsTlvKeyValue>::iterator it;
	for(it = config->tlvkvs.pairs.begin(); it != config->tlvkvs.pairs.end(); ++it)
	{
		std::string key = it->key;
		std::string value = it->value;
		if (0 == strncmp(key.c_str(), "RELAY_SERVER", 12))
		{
			/* add to RELAY_SERVER List */
			servers.push_back(value);
			//std::cerr << "p3BitDht::loadList() Found Server: " << value;
			//std::cerr << std::endl;
		}
		else if (0 == strncmp(key.c_str(), "RELAY_MODE", 10))
		{
			mode = static_cast<RsDhtRelayMode>(atoi(value.c_str()));
			haveMode = true;

			//std::cerr << "p3BitDht::loadList() Found Mode: " << mode;
			//std::cerr << std::endl;
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
				//std::cerr << "p3BitDht::loadList() Found Count(" << idx << "): ";
				//std::cerr << val;
				//std::cerr << std::endl;
				peers[idx] = val;
			}
			else
			{
				//std::cerr << "p3BitDht::loadList() Found Bandwidth(" << idx << "): ";
				//std::cerr << val;
				//std::cerr << std::endl;
				bandwidth[idx] = val;
			}
		}
		else
		{
			//std::cerr << "p3BitDht::loadList() Unknown Key:value: " << key;
			//std::cerr << ":" << value;
			//std::cerr << std::endl;
		}
	}


	// Cleanup config.
	delete config; 

	{
		RsStackMutex stack(dhtMtx); /*********** LOCKED **********/
		mRelayServerList = servers;
	}
	
	int i;
	for(i = 0; i < static_cast<typename std::underlying_type<RsDhtRelayClass>::type>(RsDhtRelayClass::NUM_CLASS); ++i)
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

