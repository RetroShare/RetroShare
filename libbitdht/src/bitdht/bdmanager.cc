/*******************************************************************************
 * bitdht/bdmanager.cc                                                         *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright 2010 by Robert Fernie <bitdht@lunamutt.com>                       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

/*******
 * Node Manager.
 ******/

/******************************************
 * 1) Maintains a list of ids to search for.
 * 2) Sets up initial search for own node.
 * 3) Checks on status of queries.
 * 4) Callback on successful searches.
 *
 * This is pretty specific to RS requirements.
 ****/

#include "bitdht/bdiface.h"
#include "bitdht/bdstddht.h"
#include "bitdht/bdmanager.h"
#include "bitdht/bdmsgs.h"
#include "bitdht/bencode.h"
#include "bitdht/bdquerymgr.h"
#include "bitdht/bdfilter.h"

#include <algorithm>
#include <iomanip>
#include <string.h>

#include "util/bdnet.h"
#include "util/bdrandom.h"
#include "util/bdstring.h"

/***
 * #define DEBUG_MGR 1
 * #define DEBUG_MGR_PKT 1
 ***/

//#define DEBUG_MGR 1

//#define LOCAL_NET_FLAG		(BITDHT_PEER_STATUS_DHT_APPL)
#define LOCAL_NET_FLAG		(BITDHT_PEER_STATUS_DHT_ENGINE)
// This is eventually what we want.
//#define LOCAL_NET_FLAG		(BITDHT_PEER_STATUS_DHT_ENGINE_VERSION)

#define QUERY_UPDATE_PERIOD 8 	// under refresh period - so it'll happen at the MAX_REFRESH_PERIOD


bdNodeManager::bdNodeManager(bdNodeId *id, std::string dhtVersion, std::string bootfile, std::string bootfilebak, const std::string& filterfile, bdDhtFunctions *fns)
	:bdNode(id, dhtVersion, bootfile, bootfilebak,filterfile, fns, this)
{
	mMode = BITDHT_MGR_STATE_OFF;
	mDhtFns = fns;
	mModeTS = 0 ;
	mStartTS = 0;
	mSearchingDone = false;
	mSearchTS = 0;

        mNetworkSize = 0;
        mBdNetworkSize = 0;

	std::string bfilter = "edff727f3a49f55c0504ad99d4282f7a26b3f69b59ebc6ca496879c6805a0aa567dffb755f17fdfd44dd24180bf2b61ebfbe68e9a53e79d7893f002140882daf7efbfed66f36eb170064208286040001fbefbbbbef1fa7fdf4a21128d050a208cd3a529a7efdc672c8255130e022b134bc6c77dfbf455d054349c575774d427b";

	mBloomFilter.setFilterBits(bfilter);

#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::bdNodeManager() ID: ";
	mDhtFns->bdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;
#endif

	mLocalNetEnhancements = true;
}

int	bdNodeManager::stopDht()
{
	time_t now = time(NULL);

	/* clean up node */
	shutdownNode();

	/* flag queries as inactive */
	/* check if exists already */
	std::map<bdNodeId, bdQueryPeer>::iterator it;	
	for(it = mActivePeers.begin(); it != mActivePeers.end(); it++)
	{
		it->second.mStatus = BITDHT_QUERY_READY;
	}

	/* set state flag */
	mMode = BITDHT_MGR_STATE_OFF;
	mModeTS = now;

	return 1;
}

int	bdNodeManager::startDht()
{
	time_t now = time(NULL);

	/* set startup mode */
	restartNode();

	mMode = BITDHT_MGR_STATE_STARTUP;
	mModeTS = now;

	mStartTS = now;
	mSearchingDone = false;
	mSearchTS = now;

	return 1;
}

 /* STOPPED, STARTING, ACTIVE, FAILED */
int 	bdNodeManager::stateDht()
{
	return mMode;
}

uint32_t bdNodeManager::statsNetworkSize()
{
        return mNetworkSize;
}

/* same version as us! */
uint32_t bdNodeManager::statsBDVersionSize()
{
        return mBdNetworkSize;
}

uint32_t bdNodeManager::setDhtMode(uint32_t dhtFlags)
{
	/* handle options here? */
	setNodeDhtMode(dhtFlags);

	return dhtFlags;
}


bool bdNodeManager::setAttachMode(bool on)
{
	if (on)
	{
        	setNodeOptions(BITDHT_OPTIONS_MAINTAIN_UNSTABLE_PORT);
	}
	else
	{
        	setNodeOptions(0);
	}
        return on;
}

        /* Friend Tracking */
void bdNodeManager::addBadPeer(const struct sockaddr_in &/*addr*/, uint32_t /*source*/, uint32_t /*reason*/, uint32_t /*age*/)
{
	std::cerr << "bdNodeManager::addBadPeer() not implemented yet!";
	std::cerr << std::endl;
}

void bdNodeManager::updateKnownPeer(const bdId *id, uint32_t /* type */, uint32_t flags)
{
	mFriendList.updatePeer(id, flags);
}

void bdNodeManager::addFindNode(bdNodeId *id, uint32_t qflags)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::addFindNode() ";
	mDhtFns->bdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;
#endif
	/* check if exists already */
	std::map<bdNodeId, bdQueryPeer>::iterator it;	
	it = mActivePeers.find(*id);
	if (it != mActivePeers.end())
	{
#ifdef DEBUG_MGR
		std::cerr << "bdNodeManager::addFindNode() Found existing....";
		std::cerr << std::endl;
#endif
		return;
	}

	/* add to map */
	bdQueryPeer peer;
	peer.mId.id = (*id);
	peer.mStatus = BITDHT_QUERY_READY; //QUERYING;
	peer.mQFlags = qflags;

	peer.mDhtAddr.sin_addr.s_addr = 0;
	peer.mDhtAddr.sin_port = 0;

	peer.mCallbackTS = 0;

	mActivePeers[*id] = peer;
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::addFindNode() Added QueryPeer as READY....";
	std::cerr << std::endl;
#endif
	//addQuery(id, qflags | BITDHT_QFLAGS_DISGUISE); 
	return;
}

	/* finds a queued query, and starts it */
void bdNodeManager::startQueries()
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::startQueries() ";
	std::cerr << std::endl;
#endif
	/* check if exists already */
	std::map<bdNodeId, bdQueryPeer>::iterator it;	
	for(it = mActivePeers.begin(); it != mActivePeers.end(); it++)
	{
		if (it->second.mStatus == BITDHT_QUERY_READY)
		{
#ifdef DEBUG_MGR
			std::cerr << "bdNodeManager::startQueries() Found READY Query.";
			std::cerr << std::endl;
#endif
			it->second.mStatus = BITDHT_QUERY_QUERYING;

			uint32_t qflags = it->second.mQFlags | BITDHT_QFLAGS_DISGUISE;
			mQueryMgr->addQuery(&(it->first), qflags); 

			// add all queries at the same time!
			//return;
		}
	}
	return;
}


void bdNodeManager::removeFindNode(bdNodeId *id)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::removeFindNode() ";
	mDhtFns->bdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;
#endif
	std::map<bdNodeId, bdQueryPeer>::iterator it;	
	it = mActivePeers.find(*id);
	if (it == mActivePeers.end())
	{
		return;
	}

	/* cleanup any actions */
	mQueryMgr->clearQuery(&(it->first));
	//clearPing(&(it->first));

	/* remove from map */
	mActivePeers.erase(it);
	return;
}

void bdNodeManager::iteration()
{

	time_t now = time(NULL);
	time_t modeAge = now - mModeTS;
	switch(mMode)
	{
		case BITDHT_MGR_STATE_OFF:
			{
#ifdef DEBUG_MGR
				std::cerr << "bdNodeManager::iteration(): OFF";
				std::cerr << std::endl;
#endif
			}
			break;

		case BITDHT_MGR_STATE_STARTUP:
			/* 10 seconds startup .... then switch to ACTIVE */

			if (modeAge > MAX_STARTUP_TIME)
			{
#ifdef DEBUG_MGR
				std::cerr << "bdNodeManager::iteration(): STARTUP -> REFRESH";
				std::cerr << std::endl;
#endif
				bdNodeId id;
				getOwnId(&id);
				mQueryMgr->addQuery(&id, BITDHT_QFLAGS_DO_IDLE | BITDHT_QFLAGS_DISGUISE); 
				
				mMode = BITDHT_MGR_STATE_FINDSELF;
				mModeTS = now;
			}
			break;

		case BITDHT_MGR_STATE_FINDSELF:
			/* 60 seconds further startup .... then switch to ACTIVE 
			 * if we reach TRANSITION_OP_SPACE_SIZE before this time, transition immediately...
			 * if, after 60 secs, we haven't reached MIN_OP_SPACE_SIZE, restart....
			 */
			
#define TRANSITION_OP_SPACE_SIZE	50 /* 1 query / sec, should take 12-15 secs */
#define MAX_FINDSELF_TIME		180 /* increased, as std rate has been dropped */
#define MIN_OP_SPACE_SIZE		10
// testing parameters.
//#define MAX_FINDSELF_TIME		10
//#define MIN_OP_SPACE_SIZE		2   // for testing. self + oneother.

			{
				uint32_t nodeSpaceSize = mNodeSpace.calcSpaceSize();

#ifdef DEBUG_MGR
				std::cerr << "bdNodeManager::iteration() Finding Oneself: ";
				std::cerr << "NodeSpace Size:" << nodeSpaceSize;
				std::cerr << std::endl;
#endif

				if (nodeSpaceSize > TRANSITION_OP_SPACE_SIZE)
				{
					mMode = BITDHT_MGR_STATE_REFRESH;
					mModeTS = now;
				}

				if (modeAge > MAX_FINDSELF_TIME) 
				{
					if (nodeSpaceSize >= MIN_OP_SPACE_SIZE)
					{
						mMode = BITDHT_MGR_STATE_REFRESH;
						mModeTS = now;
					}
					else
					{
						mMode = BITDHT_MGR_STATE_FAILED;
						mModeTS = now;
					}
				}
			}
			
			break;

		case BITDHT_MGR_STATE_ACTIVE:
			if (modeAge >= MAX_REFRESH_TIME)
			{
#ifdef DEBUG_MGR
				std::cerr << "bdNodeManager::iteration(): ACTIVE -> REFRESH";
				std::cerr << std::endl;
#endif
				mMode = BITDHT_MGR_STATE_REFRESH;
				mModeTS = now;
			}

			break;

		case BITDHT_MGR_STATE_REFRESH:
			{
#ifdef DEBUG_MGR
				std::cerr << "bdNodeManager::iteration(): REFRESH -> ACTIVE";
				std::cerr << std::endl;
#endif
				/* select random ids, and perform searchs to refresh space */
				mMode = BITDHT_MGR_STATE_ACTIVE;
				mModeTS = now;

#ifdef DEBUG_MGR
				std::cerr << "bdNodeManager::iteration(): Starting Query";
				std::cerr << std::endl;
#endif

				startQueries();

#ifdef DEBUG_MGR
				std::cerr << "bdNodeManager::iteration(): Updating Stores";
				std::cerr << std::endl;
#endif

				updateStore();

#ifdef DEBUG_MGR
				std::cerr << "bdNodeManager::iteration(): Cleaning up Filter (should do less frequently)";
				std::cerr << std::endl;
#endif

                mFilterPeers.cleanupFilter();


#ifdef DEBUG_MGR
				std::cerr << "bdNodeManager::iteration(): Do App Search";
				std::cerr << std::endl;
#endif


				/* This stuff is only important for "LocalNet based Features */
				if (mLocalNetEnhancements)
				{	
					/* run a random search for ourselves, from own App DHT peer */
					QueryRandomLocalNet();
	
#define SEARCH_MAX_SIZE 10					
	 				if (mBdNetworkSize < SEARCH_MAX_SIZE)
					{
#ifdef DEBUG_MGR
						std::cerr << "Local Netsize: " << mBdNetworkSize << " to small...searching";
						std::cerr << std::endl;
#endif
	
						/* if the network size is very small */
						SearchForLocalNet();
						mSearchingDone = false;
					}
					else
					{
						if (!mSearchingDone)
						{
							mSearchingDone = true;
							mSearchTS = now;
#ifdef DEBUG_MGR
							std::cerr << "Completed LocalNet Search in : " << mSearchTS-mStartTS;
							std::cerr << std::endl;
#endif
						}
					}
				}
	
#ifdef DEBUG_MGR
				std::cerr << "bdNodeManager::iteration(): REFRESH ";
				std::cerr << std::endl;
#endif

				status(); /* calculates mNetworkSize */

#ifdef DEBUG_MGR
				mAccount.printStats(std::cerr);
#endif

				/* Finally, Fail, and restart if we lose all peers */
				uint32_t nodeSpaceSize = mNodeSpace.calcSpaceSize();
				if (nodeSpaceSize < MIN_OP_SPACE_SIZE)
				{
					std::cerr << "bdNodeManager::iteration(): SpaceSize to Small: " << nodeSpaceSize;
					std::cerr << std::endl;
					std::cerr << "bdNodeManager::iteration(): REFRESH ==> FAILED";
					std::cerr << std::endl;

					mMode = BITDHT_MGR_STATE_FAILED;
					mModeTS = now;
				}
			}
			break;

		case BITDHT_MGR_STATE_QUIET:
			{
#ifdef DEBUG_MGR
				std::cerr << "bdNodeManager::iteration(): QUIET";
				std::cerr << std::endl;
#endif


			}
			break;

		default:
		case BITDHT_MGR_STATE_FAILED:
			{
				std::cerr << "bdNodeManager::iteration(): FAILED ==> STARTUP";
				std::cerr << std::endl;
#ifdef DEBUG_MGR
#endif
				stopDht();
				startDht();
			}
			break;
	}

	if (mMode == BITDHT_MGR_STATE_OFF)
	{
		bdNode::iterationOff();
	}
	else
	{
		/* tick parent */
		bdNode::iteration();
	}
}

	/* NB: This is a bit of a hack, the code is duplicated from bdnode & bdquery.
	 * should use fn calls into their functions for good generality
	 */

#define RANDOM_SEARCH_FRAC	(0.1)

int bdNodeManager::QueryRandomLocalNet()
{
        bdId id;
	bdNodeId targetNodeId;

	uint32_t withFlag = LOCAL_NET_FLAG;
	if (mNodeSpace.findRandomPeerWithFlag(id, withFlag))
	{
		/* if we've got a very small network size... then ask them about a random peer.
		 * (so we get there 159/158 boxes!
		 */
		bool isRandom = false;
 		if ((mBdNetworkSize < SEARCH_MAX_SIZE) || (RANDOM_SEARCH_FRAC > bdRandom::random_f32()))
		{
			bdStdRandomNodeId(&targetNodeId);
			isRandom = true;
		}
		else
		{
			/* calculate mid point */
			mDhtFns->bdRandomMidId(&mOwnId, &(id.id), &targetNodeId);
		}

		/* do standard find_peer message */
		mQueryMgr->addWorthyPeerSource(&id); /* Tell BitDHT that we really want to ping their peers */
		send_query(&id, &targetNodeId, true);
			
#ifdef DEBUG_MGR
		std::cerr << "bdNodeManager::QueryRandomLocalNet() Querying : ";
		mDhtFns->bdPrintId(std::cerr, &id);
		std::cerr << " searching for : ";
		mDhtFns->bdPrintNodeId(std::cerr, &targetNodeId);

		bdMetric dist;
		mDhtFns->bdDistance(&targetNodeId, &(mOwnId), &dist);
		int bucket = mDhtFns->bdBucketDistance(&dist);
		std::cerr << " in Bucket: " << bucket;
		std::cerr << std::endl;
#endif

		if (isRandom)
		{
#ifdef DEBUG_MGR
			std::cerr << "bdNodeManager::QueryRandomLocalNet() Search is Random!";
			std::cerr << std::endl;
#endif
		}

		return 1;
	}
	else
	{
#ifdef DEBUG_MGR
		std::cerr << "bdNodeManager::QueryRandomLocalNet() No LocalNet Peer Found";
		std::cerr << std::endl;
#endif
	}

	return 0;
}



void bdNodeManager::SearchForLocalNet()
{


#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::SearchForLocalNet()";
	std::cerr << std::endl;
#endif

	/* Check how many "Search Queries" we've got going. */

	/* check queries */
        std::map<bdNodeId, bdQueryStatus>::iterator it;
        std::map<bdNodeId, bdQueryStatus> queryStatus;


	mQueryMgr->QueryStatus(queryStatus);

	int numSearchQueries = 0;
	for(it = queryStatus.begin(); it != queryStatus.end(); it++)
	{
		if (it->second.mQFlags & BITDHT_QFLAGS_INTERNAL)
		{
#ifdef DEBUG_MGR
            std::cerr << "bdNodeManager::SearchForLocalNet() Existing Internal Search: ";
			mDhtFns->bdPrintNodeId(std::cerr, &(it->first));
            std::cerr << std::endl;
#endif

			numSearchQueries++;
		}
	}

#define MAX_SEARCH_QUERIES 5

	for(;numSearchQueries < MAX_SEARCH_QUERIES; numSearchQueries++)
	{
		/* install a new query */
		bdNodeId targetNodeId;

		/* get something that filter approves of */
		bool filterOk = false;
		int i;

#define MAX_FILTER_ATTEMPTS	3000

		for(i = 0; (!filterOk) && (i < MAX_FILTER_ATTEMPTS); i++)
		{
			bdStdRandomNodeId(&targetNodeId);
			std::string tststr;
			bdStdPrintNodeId(tststr, &targetNodeId, false);

			if (mBloomFilter.test(tststr))
			{
				filterOk = true;
			}
		}

		if (filterOk)
		{
#ifdef DEBUG_MGR
			std::cerr << "bdNodeManager::SearchForLocalNet() " << i << " Attempts to find OkNode: ";
			mDhtFns->bdPrintNodeId(std::cerr, &targetNodeId);
			std::cerr << std::endl;
#endif
		}
		else
		{
#ifdef DEBUG_MGR
			std::cerr << "bdNodeManager::SearchForLocalNet() Failed to Find FilterOk this time: ";
			mDhtFns->bdPrintNodeId(std::cerr, &targetNodeId);
			std::cerr << std::endl;
#endif
		}


		uint32_t qflags = BITDHT_QFLAGS_INTERNAL | BITDHT_QFLAGS_DISGUISE;
		mQueryMgr->addQuery(&targetNodeId, qflags); 

#ifdef DEBUG_MGR
		std::cerr << "bdNodeManager::SearchForLocalNet() Adding New Internal Search: ";
		mDhtFns->bdPrintNodeId(std::cerr, &(targetNodeId));
		std::cerr << std::endl;
#endif
	}
}


int bdNodeManager::status()
{
	/* do status of bdNode */
#ifdef DEBUG_MGR
	printState();
#endif

	checkStatus();
	checkBadPeerStatus();

	/* update the network numbers */
	mNetworkSize = mNodeSpace.calcNetworkSize();
	mBdNetworkSize = mNodeSpace.calcNetworkSizeWithFlag(
					LOCAL_NET_FLAG);

#ifdef DEBUG_MGR
	std::cerr << "BitDHT NetworkSize: " << mNetworkSize << std::endl;
	std::cerr << "BitDHT App NetworkSize: " << mBdNetworkSize << std::endl;
#endif

	return 1;
}


int bdNodeManager::checkStatus()
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::checkStatus()";
	std::cerr << std::endl;
#endif

	/* check queries */
        std::map<bdNodeId, bdQueryStatus>::iterator it;
        std::map<bdNodeId, bdQueryStatus> queryStatus;


	mQueryMgr->QueryStatus(queryStatus);

	for(it = queryStatus.begin(); it != queryStatus.end(); it++)
	{
		bool doPing = false;
		bool doRemove = false;
		bool doCallback = false;
		bool doSaveAddress = false;
		uint32_t callbackStatus = 0;

		switch(it->second.mStatus)
		{
			default:
			case BITDHT_QUERY_QUERYING:
				{
#ifdef DEBUG_MGR
					std::cerr << "bdNodeManager::checkStatus() Query in Progress id: ";
					mDhtFns->bdPrintNodeId(std::cerr, &(it->first));
					std::cerr << std::endl;
#endif
				}
				break;

			case BITDHT_QUERY_FAILURE:
				{
#ifdef DEBUG_MGR
					std::cerr << "bdNodeManager::checkStatus() Query Failed: id: ";
					mDhtFns->bdPrintNodeId(std::cerr, &(it->first));
					std::cerr << std::endl;
#endif
					// BAD.
					doRemove = true;
					doCallback = true;
					callbackStatus = BITDHT_MGR_QUERY_FAILURE;
				}
				break;

			case BITDHT_QUERY_FOUND_CLOSEST:
				{
#ifdef DEBUG_MGR
					std::cerr << "bdNodeManager::checkStatus() Found Closest: id: ";
					mDhtFns->bdPrintNodeId(std::cerr, &(it->first));
					std::cerr << std::endl;
#endif

					doRemove = true;
					doCallback = true;
					callbackStatus = BITDHT_MGR_QUERY_PEER_OFFLINE;
				}
				break;

			case BITDHT_QUERY_PEER_UNREACHABLE:
				{
#ifdef DEBUG_MGR
					std::cerr << "bdNodeManager::checkStatus() the Peer Online but Unreachable: id: ";
					mDhtFns->bdPrintNodeId(std::cerr, &(it->first));
					std::cerr << std::endl;
#endif

					doRemove = true;
					doCallback = true;
					callbackStatus = BITDHT_MGR_QUERY_PEER_UNREACHABLE;
				}
				break;

			case BITDHT_QUERY_SUCCESS:
				{
#ifdef DEBUG_MGR
					std::cerr << "bdNodeManager::checkStatus() Found Query: id: ";
					mDhtFns->bdPrintNodeId(std::cerr, &(it->first));
					std::cerr << std::endl;
#endif
					//foundId = 
					doRemove = true;
					doCallback = true;
					doSaveAddress = true;
					callbackStatus = BITDHT_MGR_QUERY_PEER_ONLINE;
				}
				break;
		}

		/* remove done queries */
		if (doRemove) 
		{
			if (it->second.mQFlags & BITDHT_QFLAGS_DO_IDLE)
			{
				doRemove = false;
			}
		}

		if (doRemove) 
		{
#ifdef DEBUG_MGR
		std::cerr << "bdNodeManager::checkStatus() Removing query: id: ";
		mFns->bdPrintNodeId(std::cerr, &(it->first));
		std::cerr << std::endl;
#endif
			mQueryMgr->clearQuery(&(it->first));
		}

		/* FIND in activePeers */
		std::map<bdNodeId, bdQueryPeer>::iterator pit;
		pit = mActivePeers.find(it->first);

		if (pit == mActivePeers.end())
		{
			/* only internal! - disable Callback / Ping */
			doPing = false;
			doCallback = false;
#ifdef DEBUG_MGR
			std::cerr << "bdNodeManager::checkStatus() Internal: no cb for id: ";
			mDhtFns->bdPrintNodeId(std::cerr, &(it->first));
			std::cerr << std::endl;
#endif
		}
		else 
		{
			if (pit->second.mStatus == it->second.mStatus)
			{
				/* status is unchanged */
#ifdef DEBUG_MGR
				std::cerr << "bdNodeManager::checkStatus() Status unchanged for : ";
				mFns->bdPrintNodeId(std::cerr, &(it->first));
				std::cerr << " status: " << it->second.mStatus;
				std::cerr << std::endl;
#endif


				time_t now = time(NULL);
				/* now we check if we've done a callback before... */
				if (it->second.mQFlags & BITDHT_QFLAGS_UPDATES)
				{
					if (now - pit->second.mCallbackTS > QUERY_UPDATE_PERIOD)
					{
						// keep flags.
#ifdef DEBUG_MGR
						std::cerr << "bdNodeManager::checkStatus() Doing Update Callback for";
						mFns->bdPrintNodeId(std::cerr, &(it->first));
						std::cerr << " status: " << it->second.mStatus;
						std::cerr << std::endl;
#endif
					}
					else
					{
						/* no callback this time */
						doPing = false;
						doCallback = false;
					}
				}
				else
				{
					doPing = false;
					doCallback = false;
				}
			}
			else
			{

#ifdef DEBUG_MGR
				std::cerr << "bdNodeManager::checkStatus() Updating External Status for : ";
				mFns->bdPrintNodeId(std::cerr, &(it->first));
				std::cerr << " to: " << it->second.mStatus;
				std::cerr << std::endl;
#endif
				/* update status */
				pit->second.mStatus = it->second.mStatus;
			}

			if (doSaveAddress)
			{
				if (it->second.mResults.size() > 0)
				{
					pit->second.mDhtAddr = it->second.mResults.front().addr;
				}
				else
				{
					pit->second.mDhtAddr.sin_addr.s_addr = 0;
					pit->second.mDhtAddr.sin_port = 0;
				}
			}

			/* add successful queries to ping list */
			if (doPing)
			{
#ifdef DEBUG_MGR
				std::cerr << "bdNodeManager::checkStatus() Starting Ping (TODO): id: ";
				mFns->bdPrintNodeId(std::cerr, &(it->first));
				std::cerr << std::endl;
#endif
				/* add first matching peer */
				//addPeerPing(foundId);
			}

			/* callback on new successful queries */
			if (doCallback)
			{
#ifdef DEBUG_MGR
				std::cerr << "bdNodeManager::checkStatus() Doing Callback: id: ";
				mFns->bdPrintNodeId(std::cerr, &(it->first));
				std::cerr << std::endl;
#endif
				
				time_t now = time(NULL);
				pit->second.mCallbackTS = now;
				bdId id(it->first,pit->second.mDhtAddr);
				doPeerCallback(&id, callbackStatus);
			}
		}
	}
	return 1;
}

#if 0
bdNodeManager::checkPingStatus()
{

	/* check queries */
        std::map<bdNodeId, bdPingStatus>::iterator it;
        std::map<bdNodeId, bdPingStatus> pingStatus;

        PingStatus(pingStatus);

	for(it = pingStatus.begin(); it != pingStatus.end(); it++)
	{
		switch(it->second.mStatus)
		{
			case BITDHT_QUERY_QUERYING:
				{

				}
				break;

			case BITDHT_QUERY_FAILURE:
				{
					// BAD.
					doRemove = true;
				}
				break;

			case BITDHT_QUERY_FOUND_CLOSEST:
				{
					
					doRemove = true;
				}
				break;

			case BITDHT_QUERY_SUCCESS:
				{
					foundId = 
					doRemove = true;
				}
				break;
		}

		/* remove done queries */
		if (doRemove)
		{
			clearQuery(it->first);
		}

		/* add successful queries to ping list */
		if (doPing)
		{
			/* add first matching peer */
			addPeerPing(foundId);
		}

		/* callback on new successful queries */
		if (doCallback)
		{

		}
	}
}
#endif

int bdNodeManager::checkBadPeerStatus()
{
	bdId id;
	uint32_t flags;
	std::string nullstr;

	while(mBadPeerQueue.popPeer(&id, flags))
	{
		doInfoCallback(&id, BITDHT_INFO_CB_TYPE_BADPEER, flags, nullstr);
	}
	return 1;
}

int bdNodeManager::SearchOutOfDate()
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::SearchOutOfDate()";
	std::cerr << std::endl;
#endif

	std::map<bdNodeId, bdQueryPeer>::iterator it;

	/* search for out-of-date peers */
	for(it = mActivePeers.begin(); it != mActivePeers.end(); it++)
	{
#if 0
		if (old)
		{
			addQuery(it->first);
		}
#endif
	}

	return 1;
}



        /***** Functions to Call down to bdNodeManager ****/
        /* Request DHT Peer Lookup */
        /* Request Keyword Lookup */

void bdNodeManager::findDhtValue(bdNodeId * /*id*/, std::string /*key*/, uint32_t /*mode*/)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::findDhtValue() TODO";
	std::cerr << std::endl;
#endif
}


        /***** Get Results Details *****/
int bdNodeManager::getDhtPeerAddress(const bdNodeId *id, struct sockaddr_in &from)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::getDhtPeerAddress() Id: ";
	mDhtFns->bdPrintNodeId(std::cerr, id);
	std::cerr << " ... ? TODO" << std::endl;
#else
	(void) id;
#endif

	std::map<bdNodeId, bdQueryPeer>::iterator pit;
	pit = mActivePeers.find(*id);

	std::cerr << "bdNodeManager::getDhtPeerAddress() Id: ";
	mDhtFns->bdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;

	if (pit != mActivePeers.end())
	{
		std::cerr << "bdNodeManager::getDhtPeerAddress() Found ActiveQuery";
		std::cerr << std::endl;

		if (pit->second.mStatus == BITDHT_QUERY_SUCCESS)
		{
			from = pit->second.mDhtAddr;

			std::cerr << "bdNodeManager::getDhtPeerAddress() Found Peer Address:";
			std::cerr << bdnet_inet_ntoa(from.sin_addr) << ":" << htons(from.sin_port);
			std::cerr << std::endl;

			return 1;
		}
	}
	return 0;

}

int bdNodeManager::getDhtValue(const bdNodeId *id, std::string key, std::string & /*value*/)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::getDhtValue() Id: ";
	mDhtFns->bdPrintNodeId(std::cerr, id);
	std::cerr << " key: " << key;
	std::cerr << " ... ? TODO" << std::endl;
#else
	(void) id;
	(void) key;
#endif

	return 1;
}

int bdNodeManager::getDhtBucket(const int idx, bdBucket &bucket)
{
	return mNodeSpace.getDhtBucket(idx, bucket);
}

int bdNodeManager::getDhtQueries(std::map<bdNodeId, bdQueryStatus> &queries)
{
	mQueryMgr->QueryStatus(queries);
	return 1;
}

int bdNodeManager::getDhtQueryStatus(const bdNodeId *id, bdQuerySummary &query)
{
	return mQueryMgr->QuerySummary(id, query);
}


        /***** Add / Remove Callback Clients *****/
void bdNodeManager::addCallback(BitDhtCallback *cb)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::addCallback()";
	std::cerr << std::endl;
#endif
        /* search list */
        std::list<BitDhtCallback *>::iterator it;
        it = std::find(mCallbacks.begin(), mCallbacks.end(), cb);
        if (it == mCallbacks.end())
        {
                /* add it */
                mCallbacks.push_back(cb);
        }
}

void bdNodeManager::removeCallback(BitDhtCallback *cb)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::removeCallback()";
	std::cerr << std::endl;
#endif
        /* search list */
        std::list<BitDhtCallback *>::iterator it;
        it = std::find(mCallbacks.begin(), mCallbacks.end(), cb);
        if (it == mCallbacks.end())
        {
                /* not found! */
                return;
        }
        it = mCallbacks.erase(it);
}


void bdNodeManager::addPeer(const bdId *id, uint32_t peerflags)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::addPeer() Overloaded (doing Callback)";
	std::cerr << std::endl;
#endif
	doNodeCallback(id, peerflags);

	// call parent.
	bdNode::addPeer(id, peerflags);

        return;
}



void bdNodeManager::doNodeCallback(const bdId *id, uint32_t peerflags)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::doNodeCallback() ";
	mDhtFns->bdPrintId(std::cerr, id);
	std::cerr << "peerflags: " << peerflags;
	std::cerr << std::endl;
#endif

        /* search list */
        std::list<BitDhtCallback *>::iterator it;
        for(it = mCallbacks.begin(); it != mCallbacks.end(); it++)
        {
                (*it)->dhtNodeCallback(id, peerflags);
        }
        return;
}

void bdNodeManager::doPeerCallback(const bdId *id, uint32_t status)
{

#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::doPeerCallback()";
	mDhtFns->bdPrintId(std::cerr, id);
	std::cerr << "status: " << status;
	std::cerr << std::endl;
#endif

        /* search list */
        std::list<BitDhtCallback *>::iterator it;
        for(it = mCallbacks.begin(); it != mCallbacks.end(); it++)
        {
                (*it)->dhtPeerCallback(id, status);
        }
        return;
}

void bdNodeManager::doValueCallback(const bdNodeId *id, std::string key, uint32_t status)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::doValueCallback()";
	std::cerr << std::endl;
#endif
        /* search list */
        std::list<BitDhtCallback *>::iterator it;
        for(it = mCallbacks.begin(); it != mCallbacks.end(); it++)
        {
                (*it)->dhtValueCallback(id, key, status);
        }
        return;
}


void bdNodeManager::doInfoCallback(const bdId *id, uint32_t type, uint32_t flags, std::string info)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::doInfoCallback()";
	std::cerr << std::endl;
#endif
        /* search list */
        std::list<BitDhtCallback *>::iterator it;
        for(it = mCallbacks.begin(); it != mCallbacks.end(); it++)
        {
                (*it)->dhtInfoCallback(id, type, flags, info);
        }
        return;
}

void bdNodeManager::doIsBannedCallback(const sockaddr_in *addr, bool *isAvailable, bool *isBanned)
{
#ifdef DEBUG_MGR
	std::cerr << "bdNodeManager::doIsBannedCallback()";
	std::cerr << std::endl;
#endif
	/* search list */
	std::list<BitDhtCallback *>::iterator it;
	*isBanned = false;
	*isAvailable = false;
	for(it = mCallbacks.begin(); it != mCallbacks.end(); it++)
	{
		// set isBanned to true as soon as one callback answers with true
		bool banned;
		if((*it)->dhtIsBannedCallback(addr, &banned))
		{
			*isBanned = *isBanned || banned;
			*isAvailable = true;
		}
	}
}


#define BITDHT_IDENTITY_STRING_V1  	"d1:"
#define BITDHT_IDENTITY_SIZE_V1    	3
#define BITDHT_PACKET_MIN_SIZE		4

        /******************* Internals *************************/
int     bdNodeManager::isBitDhtPacket(char *data, int size, struct sockaddr_in & from)

{

	/* use a very simple initial check */
        if (size < BITDHT_PACKET_MIN_SIZE)
                return 0;

        return (0 == strncmp(data, BITDHT_IDENTITY_STRING_V1, BITDHT_IDENTITY_SIZE_V1));

	/* Below is the old version! */


#ifdef DEBUG_MGR_PKT
	std::cerr << "bdNodeManager::isBitDhtPacket() *******************************";
        std::cerr << " from " << inet_ntoa(from.sin_addr);
        std::cerr << ":" << ntohs(from.sin_port);
	std::cerr << std::endl;
	{
		/* print the fucker... only way to catch bad ones */
		std::string out;
		for(int i = 0; i < size; i++)
		{
			if (isascii(data[i]))
			{
				out += data[i];
			}
			else
			{
				bd_sprintf_append(out, "[%02lx]", (uint32_t) data[i]);
			}
			if ((i % 16 == 0) && (i != 0))
			{
				out += "\n";
			}
		}
		std::cerr << out;
	}
	std::cerr << "bdNodeManager::isBitDhtPacket() *******************************";
	std::cerr << std::endl;
#else
	(void) from;
#endif

	/* try to parse it! */
        /* convert to a be_node */
        be_node *node = be_decoden(data, size);
        if (!node)
        {
                /* invalid decode */
#ifdef DEBUG_MGR
                std::cerr << "bdNodeManager::isBitDhtPacket() be_decode failed. dropping";
		std::cerr << std::endl;
		std::cerr << "bdNodeManager::BadPacket ******************************";
        	std::cerr << " from " << inet_ntoa(from.sin_addr);
        	std::cerr << ":" << ntohs(from.sin_port);
		std::cerr << std::endl;
		{
			/* print the fucker... only way to catch bad ones */
			std::string out;
			for(int i = 0; i < size; i++)
			{
				if (isascii(data[i]))
				{
					out += data[i];
				}
				else
				{
					bd_sprintf_append(out, "[%02lx]", (uint32_t) data[i]);
				}
				if ((i % 16 == 0) && (i != 0))
				{
					out += "\n";
				}
			}
			std::cerr << out;
		}
		std::cerr << "bdNodeManager::BadPacket ******************************";
		std::cerr << std::endl;
#endif
		return 0;
	}

        /* find message type */
        uint32_t beType = beMsgType(node);	
	int ans = (beType != BITDHT_MSG_TYPE_UNKNOWN);
	be_free(node);

#ifdef DEBUG_MGR_PKT
	if (ans)
	{
		std::cerr << "bdNodeManager::isBitDhtPacket() YES";
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "bdNodeManager::isBitDhtPacket() NO: Unknown Type";
		std::cerr << std::endl;
	}
	
#endif
	return ans;
}


bdDebugCallback::~bdDebugCallback()
{
}

int bdDebugCallback::dhtPeerCallback(const bdId *id, uint32_t status)
{
	/* remove unused parameter warnings */
	(void) status;

#ifdef DEBUG_MGR
	std::cerr << "bdDebugCallback::dhtPeerCallback() Id: ";
#endif
	bdStdPrintId(std::cerr, id);
#ifdef DEBUG_MGR
	std::cerr << " status: " << std::hex << status << std::dec << std::endl;
#endif
	return 1;
}

int bdDebugCallback::dhtValueCallback(const bdNodeId *id, std::string key, uint32_t status)
{
	/* remove unused parameter warnings */
	(void) key;
	(void) status;

#ifdef DEBUG_MGR
	std::cerr << "bdDebugCallback::dhtValueCallback() Id: ";
#endif
	bdStdPrintNodeId(std::cerr, id);
#ifdef DEBUG_MGR
	std::cerr << " key: " << key;
	std::cerr << " status: " << std::hex << status << std::dec << std::endl;
#endif

	return 1;
}





/******************* Connection Stuff ********************/



bool bdNodeManager::ConnectionRequest(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode, uint32_t delay, uint32_t start)
{
	std::cerr << "bdNodeManager::ConnectionRequest()";
	std::cerr << std::endl;

	return mConnMgr->requestConnection(laddr, target, mode, delay, start);
}

void bdNodeManager::ConnectionAuth(bdId *srcId, bdId *proxyId, bdId *destId, uint32_t mode, uint32_t loc, uint32_t bandwidth, uint32_t delay, uint32_t answer)
{
	std::cerr << "bdNodeManager::ConnectionAuth()";
	std::cerr << std::endl;

	if (answer == BITDHT_CONNECT_ANSWER_OKAY)
	{
		mConnMgr->AuthConnectionOk(srcId, proxyId, destId, mode, loc, bandwidth, delay);
	}
	else
	{
		mConnMgr->AuthConnectionNo(srcId, proxyId, destId, mode, loc, answer);
	}
}

void bdNodeManager::ConnectionOptions(uint32_t allowedModes, uint32_t flags)
{
	mConnMgr->setConnectionOptions(allowedModes, flags);
}


        /***** Connections Requests *****/

        // Overloaded from bdnode for external node callback. 
void bdNodeManager::callbackConnect(bdId *srcId, bdId *proxyId, bdId *destId, int mode, int point, int param, int cbtype, int errcode)
{
	std::cerr << "bdNodeManager::callbackConnect()";
	std::cerr << std::endl;

#ifdef DEBUG_MGR
#endif
        /* search list */
        std::list<BitDhtCallback *>::iterator it;
        for(it = mCallbacks.begin(); it != mCallbacks.end(); it++)
        {
                (*it)->dhtConnectCallback(srcId, proxyId, destId, mode, point, param, cbtype, errcode);
        }
        return;
}

int bdDebugCallback::dhtConnectCallback(const bdId *srcId, const bdId *proxyId, const bdId *destId,
		uint32_t mode, uint32_t point, uint32_t param, uint32_t cbtype, uint32_t errcode)
{
	/* remove unused parameter warnings */
	(void) srcId;
	(void) proxyId;
	(void) destId;
	(void) mode;
	(void) point;
	(void) param;
	(void) cbtype;
	(void) errcode;

#ifdef DEBUG_MGR
	std::cerr << "bdDebugCallback::dhtConnectCallback() Type: " << cbtype;
	std::cerr << " errCode: " << errcode;
	std::cerr << " srcId: ";
	bdStdPrintId(std::cerr, srcId);
	std::cerr << " proxyId: ";
	bdStdPrintId(std::cerr, proxyId);
	std::cerr << " destId: ";
	bdStdPrintId(std::cerr, destId);
	std::cerr << " mode: " << mode;
	std::cerr << " param: " << param;
	std::cerr << " point: " << point << std::endl;
#endif

	return 1;
}


int bdDebugCallback::dhtInfoCallback(const bdId *id, uint32_t type, uint32_t flags, std::string info)
{
	/* remove unused parameter warnings */
	(void) id;
	(void) type;
	(void) flags;
	(void) info;

#ifdef DEBUG_MGR
	std::cerr << "bdDebugCallback::dhtInfoCallback() Type: " << type;
	std::cerr << " Id: ";
	bdStdPrintId(std::cerr, id);
	std::cerr << " flags: " << flags;
	std::cerr << " info: " << info;
	std::cerr << std::endl;
#endif

	return 1;
}



