/*******************************************************************************
 * librssimulator/testing/: SetServiceTester.cc                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
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

#include <list>
#include "time.h"
#include "retroshare/rsids.h"
#include "serialiser/rsserial.h"

#include "SetServiceTester.h"
#include "peer/PeerNode.h"

/**
 * #define DEBUG_TEST	1
 **/

SetServiceTester::SetServiceTester()
:mDropFilter(SetFilter::FILTER_NONE), 
 mCaptureFilter(SetFilter::FILTER_ALL), 
 mFinishFilter(SetFilter::FILTER_NONE)
{
	mRsSerialiser = new RsSerialiser();
	mRefTime = time(NULL);
}

SetServiceTester::~SetServiceTester()
{
	std::map<RsPeerId, PeerNode *>::iterator pit;
	for(pit = mNodes.begin(); pit != mNodes.end(); ++pit)
	{
		delete (pit->second);
	}
	mNodes.clear();

	std::vector<SetPacket>::iterator it;
	for(it = mPackets.begin(); it != mPackets.end(); ++it)
	{
		delete it->mItem;
	}
	mPackets.clear();

	delete mRsSerialiser;
}

bool SetServiceTester::addNode(const RsPeerId &peerId, std::list<RsPeerId> friendIds)
{
	PeerNode *node = new PeerNode(peerId, friendIds, false);
	mNodes[peerId] = node;
	return true;
}


bool SetServiceTester::addNode(const RsPeerId &peerId, PeerNode *node)
{
	mNodes[peerId] = node;
	return true;
}

bool SetServiceTester::startup()
{
	std::map<RsPeerId, PeerNode *>::iterator pit;
	for(pit = mNodes.begin(); pit != mNodes.end(); ++pit)
	{
		pit->second->notifyOfFriends();
	}
	return true;
}


bool SetServiceTester::bringOnline(const RsPeerId &peerId, std::list<RsPeerId> peers)
{
	std::map<RsPeerId, PeerNode *>::iterator pit;
	pit = mNodes.find(peerId);
	if (pit != mNodes.end())
	{
		pit->second->bringOnline(peers);
	}
	else
	{
		throw std::logic_error("SetServiceTester::bringOnline() invalid index");
	}
	return true;
}

/***************************************************************************************************/
/***************************************************************************************************/


uint32_t  SetServiceTester::getPacketCount()
{
	return mPackets.size();
}

SetPacket &SetServiceTester::examinePacket(uint32_t idx)
{
	if (idx >= mPackets.size())
	{
		throw std::logic_error("SetServiceTester::examinePacket() invalid index");
	}
	return mPackets[idx];
}

bool SetServiceTester::injectPacket(const SetPacket &/*pkt*/)
{
	std::cerr << "SetServiceTester::injectPacket() Incomplete";
	std::cerr << std::endl;

	throw std::logic_error("SetServiceTester::injectPacket() incomplete");
	return false;
}

uint32_t  SetServiceTester::getNodeCount()
{
	return mNodes.size();
}

PeerNode * SetServiceTester::getPeerNode(const RsPeerId &id)
{
	std::map<RsPeerId, PeerNode *>::iterator pit;
	pit = mNodes.find(id);
	if (pit == mNodes.end())
	{
		throw std::logic_error("SetServiceTester::getPeerNode() invalid index");
	}
	return pit->second;
}


/***************************************************************************************************/
/***************************************************************************************************/

bool SetServiceTester::tick()
{
	tickUntilEvent(1, UNTIL_NONE);
	return true;
}

bool SetServiceTester::tickUntilCapturedPacket(int max_ticks, uint32_t& idx)
{
	bool eventOccured = tickUntilEvent(max_ticks, UNTIL_CAPTURE);
	if (eventOccured)
	{
		idx = mPackets.size() - 1;
	}
	return eventOccured;
}


bool SetServiceTester::tickUntilFinish(int max_ticks)
{
	return tickUntilEvent(max_ticks, UNTIL_FINISH);
}

bool SetServiceTester::tickUntilEvent(int max_ticks, EventType eventType)
{
#ifdef DEBUG_TEST
	std::cerr << "SetServiceTester::tickUntilEvent()";
	std::cerr << std::endl;
#endif

	for(int i = 0; i < max_ticks; i++)
	{
		std::map<RsPeerId, PeerNode *>::iterator pit;
		for(pit = mNodes.begin(); pit != mNodes.end(); ++pit)
		{
			pit->second->tick();
			while (pit->second->haveOutgoingPackets())
			{
#ifdef DEBUG_TEST
				std::cerr << "SetServiceTester::tickUntilEvent() ";
				std::cerr << "packet from: " << pit->first.toStdString();
				std::cerr << std::endl;
#endif

				bool finished = false;
				double ts = time(NULL) - mRefTime;
				RsRawItem *rawItem = pit->second->outgoing();
				RsItem *item = convertToRsItem(rawItem, false);
				RsPeerId destId = rawItem->PeerId();
				RsPeerId srcId = pit->second->id();

				SetPacket pkt(ts, srcId, destId, item);
				if (filter(pkt))
				{
#ifdef DEBUG_TEST
					std::cerr << "Dropping Packet: ";
					std::cerr << std::endl;
					item->print(std::cerr);
					std::cerr << std::endl;
#endif

					delete rawItem;
					delete item;
					continue;
				}

				if (eventType == UNTIL_FINISH)
				{
					if (finish(pkt))
					{
#ifdef DEBUG_TEST
						std::cerr << "Finish Packet: ";
						std::cerr << std::endl;
						item->print(std::cerr);
						std::cerr << std::endl;
#endif

						finished = true;
					}
				}

				if (capture(pkt))
				{
#ifdef DEBUG_TEST
					std::cerr << "Capture Packet: ";
					std::cerr << std::endl;
					item->print(std::cerr);
					std::cerr << std::endl;
#endif

					mPackets.push_back(pkt);
					if (eventType == UNTIL_CAPTURE)
					{
						finished = true;
					}
				}
				else
				{
					delete item;
				}

				// Pass on Item.
				if (rawItem)
				{
					rawItem->PeerId(srcId);
					std::map<RsPeerId, PeerNode *>::iterator pit2;
					pit2 = mNodes.find(destId);
					if (pit2 != mNodes.end())
					{
						pit2->second->incoming(rawItem);
					}
					else
					{
						// Error.
						delete rawItem;
						throw std::logic_error("SetServiceTester::tickUntilEvent() invalid destId");
					}
				}

				if (finished)
				{
					return true;
				}
			}
		}
	}
	return false;
}


/***************************************************************************************************/
/***************************************************************************************************/

bool SetServiceTester::filter(const SetPacket& packet)
{
        return mDropFilter.filter(packet);
}

bool SetServiceTester::capture(const SetPacket& packet)
{
        return mCaptureFilter.filter(packet);
}

bool SetServiceTester::finish(const SetPacket& packet)
{
        return mFinishFilter.filter(packet);
}

/***************************************************************************************************/
/***************************************************************************************************/

void    SetServiceTester::addSerialType(RsSerialType *st)
{
	mRsSerialiser->addSerialType(st);
}



RsItem *SetServiceTester::convertToRsItem(RsRawItem *rawitem, bool toDelete)
{
	if (rawitem)
	{
#ifdef DEBUG_TEST
		/* convert back to standard item for convenience */
		std::cerr << "SetServiceTester::getPacket() have RsRawItem";
		std::cerr << std::endl;
#endif

		/* convert to RsServiceItem */
		uint32_t size = rawitem->getRawLength();
		RsItem *item = mRsSerialiser->deserialise(rawitem->getRawData(), &size);
		if ((!item) || (size != rawitem->getRawLength()))
		{
			/* error in conversion */
			std::cerr << "SetServiceTester::getPacket() Error";
			std::cerr << std::endl;
			std::cerr << "SetServiceTester::getPacket() Size: " << size;
			std::cerr << std::endl;
			std::cerr << "SetServiceTester::getPacket() RawLength: " << rawitem->getRawLength();
			std::cerr << std::endl;
			if (item)
			{
				std::cerr << "p3Service::recv() Bad Item:";
				std::cerr << std::endl;
				item->print(std::cerr, 0);
				std::cerr << std::endl;

				delete item;
				item=NULL ;
			}
		}
		else
		{
			item->PeerId(rawitem->PeerId());
		}

		if (toDelete)
		{
			delete rawitem;
		}
		return item;
	}
	return NULL;
}

RsRawItem *SetServiceTester::convertToRsRawItem(RsItem *item, bool toDelete)
{
#ifdef DEBUG_TEST
	std::cerr << "SetServiceTester::convertToRawItem()";
	std::cerr << std::endl;
#endif

	/* try to convert */
	uint32_t size = mRsSerialiser->size(item);
	if (!size)
	{
		std::cerr << "SetServiceTesterconvertToRsRawItem() ERROR size == 0";
		std::cerr << std::endl;

		/* can't convert! */
		if (toDelete)
		{
			delete item;
		}
		return NULL;
	}

	RsRawItem *raw = new RsRawItem(item->PacketId(), size);
	if (!mRsSerialiser->serialise(item, raw->getRawData(), &size))
	{
		std::cerr << "SetServiceTesterconvertToRsRawItem() ERROR serialise failed";
		std::cerr << std::endl;

		delete raw;
		raw = NULL;
	}

	if ((raw) && (size != raw->getRawLength()))
	{
		std::cerr << "SetServiceTesterconvertToRsRawItem() ERROR serialise size mismatch";
		std::cerr << std::endl;

		delete raw;
		raw = NULL;
	}

	if (raw)
	{
		raw->PeerId(item->PeerId());
	}

	if (toDelete)
	{
		delete item;
	}
	return raw;
}

	
