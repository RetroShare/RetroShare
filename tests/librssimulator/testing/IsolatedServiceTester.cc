/*******************************************************************************
 * librssimulator/testing/: IsolatedServiceTester.cc                           *
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
#include "retroshare/rsids.h"
#include "rsitems/rsitem.h"
#include "serialiser/rsserial.h"

#include "testing/IsolatedServiceTester.h"

#include "peer/PeerNode.h"

IsolatedServiceTester::IsolatedServiceTester(RsPeerId ownId, std::list<RsPeerId> peers)
{
	mNode = new PeerNode(ownId, peers, false);
	mRsSerialiser = new RsSerialiser();
}


IsolatedServiceTester::~IsolatedServiceTester()
{
	delete mNode;
	delete mRsSerialiser;
}

void    IsolatedServiceTester::addSerialType(RsSerialType *st)
{
	mRsSerialiser->addSerialType(st);
}

bool IsolatedServiceTester::startup()
{
	mNode->notifyOfFriends();
	return true;
}


bool IsolatedServiceTester::tick()
{
	mNode->tick();
	return true;
}

bool IsolatedServiceTester::tickUntilPacket(int max_ticks)
{
	for(int i = 0; i < max_ticks; i++)
	{
		tick();
		if (mNode->haveOutgoingPackets())
		{
			return true;
		}
	}
	return false;
}


RsItem *IsolatedServiceTester::getPacket()
{
	RsRawItem *rawitem = mNode->outgoing();
	if (rawitem)
	{
#ifdef DEBUG
		/* convert back to standard item for convenience */
		std::cerr << "IsolatedServiceTester::getPacket() have RsRawItem";
		std::cerr << std::endl;
#endif

		/* convert to RsServiceItem */
		uint32_t size = rawitem->getRawLength();
		RsItem *item = mRsSerialiser->deserialise(rawitem->getRawData(), &size);
		if ((!item) || (size != rawitem->getRawLength()))
		{
			/* error in conversion */
			std::cerr << "IsolatedServiceTester::getPacket() Error";
			std::cerr << std::endl;
			std::cerr << "IsolatedServiceTester::getPacket() Size: " << size;
			std::cerr << std::endl;
			std::cerr << "IsolatedServiceTester::getPacket() RawLength: " << rawitem->getRawLength();
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
		delete rawitem;
		return item;
	}
	return NULL;
}

bool IsolatedServiceTester::sendPacket(RsItem *si)
{
#ifdef DEBUG
	std::cerr << "IsolatedServiceTester::sendPacket()";
	std::cerr << std::endl;
#endif

	/* try to convert */
	uint32_t size = mRsSerialiser->size(si);
	if (!size)
	{
		std::cerr << "IsolatedServiceTester::sendPacket() ERROR size == 0";
		std::cerr << std::endl;

		/* can't convert! */
		delete si;
		return false;
	}

	RsRawItem *raw = new RsRawItem(si->PacketId(), size);
	if (!mRsSerialiser->serialise(si, raw->getRawData(), &size))
	{
		std::cerr << "IsolatedServiceTester::sendPacket() ERROR serialise failed";
		std::cerr << std::endl;

		delete raw;
		raw = NULL;
	}

	if ((raw) && (size != raw->getRawLength()))
	{
		std::cerr << "IsolatedServiceTester::sendPacket() ERROR serialise size mismatch";
		std::cerr << std::endl;

		delete raw;
		raw = NULL;
	}

	if (raw)
	{
#ifdef DEBUG
		std::cerr << "IsolatedServiceTester::sendPacket()";
		std::cerr << std::endl;
		si->print(std::cerr);
		std::cerr << "IsolatedServiceTester::sendPacket() passing to Node: ";
		std::cerr << std::endl;
#endif

		raw->PeerId(si->PeerId());
		mNode->incoming(raw);
	}
	delete si;
	return (raw != NULL);
}

	
