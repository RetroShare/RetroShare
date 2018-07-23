/*******************************************************************************
 * libretroshare/src/services: p3serviceinfo.cc                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2014 Robert Fernie <retroshare@lunamutt.com>                      *
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
 *******************************************************************************/
#include "pqi/p3linkmgr.h"
#include "pqi/p3netmgr.h"

#include "util/rsnet.h"

#include "services/p3serviceinfo.h"
#include "rsitems/rsbanlistitems.h"

#include <sys/time.h>

/****
 * #define DEBUG_INFO		1
 ****/

/************ IMPLEMENTATION NOTES *********************************
 * 
 * Send Info to peers about services we are providing.
 */

p3ServiceInfo::p3ServiceInfo(p3ServiceControl *serviceControl)
	:p3Service(), mInfoMtx("p3ServiceInfo"),
	mServiceControl(serviceControl)
{
	addSerialType(new RsServiceInfoSerialiser());
}

const std::string SERVICE_INFO_APP_NAME = "serviceinfo";
const uint16_t SERVICE_INFO_APP_MAJOR_VERSION  =       1;
const uint16_t SERVICE_INFO_APP_MINOR_VERSION  =       0;
const uint16_t SERVICE_INFO_MIN_MAJOR_VERSION  =       1;
const uint16_t SERVICE_INFO_MIN_MINOR_VERSION  =       0;

RsServiceInfo p3ServiceInfo::getServiceInfo()
{
        return RsServiceInfo(RS_SERVICE_TYPE_SERVICEINFO,
                SERVICE_INFO_APP_NAME,
                SERVICE_INFO_APP_MAJOR_VERSION,
                SERVICE_INFO_APP_MINOR_VERSION,
                SERVICE_INFO_MIN_MAJOR_VERSION,
                SERVICE_INFO_MIN_MINOR_VERSION);
}



int	p3ServiceInfo::tick()
{
	processIncoming();
	sendPackets();
	return 0;
}

int	p3ServiceInfo::status()
{
	return 1;
}


/***** Implementation ******/

bool p3ServiceInfo::processIncoming()
{
	/* for each packet - pass to specific handler */
	RsItem *item = NULL;
	while(NULL != (item = recvItem()))
	{
#ifdef DEBUG_INFO
		std::cerr << "p3ServiceInfo::processingIncoming() Received Item:";
		std::cerr << std::endl;
		item->print(std::cerr);
		std::cerr << std::endl;
#endif
		switch(item->PacketSubType())
		{
			default:
				break;
			case RS_PKT_SUBTYPE_SERVICELIST_ITEM:
			{
				// Order is important!.	
				RsServiceInfoListItem *listItem = dynamic_cast<RsServiceInfoListItem *>(item);
				if (listItem)
				{
					recvServiceInfoList(listItem);
				}
				else
				{
                    // error.
					std::cerr << "p3ServiceInfo::processingIncoming() Error with Received Item:";
					std::cerr << std::endl;
					item->print(std::cerr);
					std::cerr << std::endl;
				}
			}
				break;
		}

		/* clean up */
		delete item;
	}
	return true ;
} 
	

bool convertServiceInfoToItem(
		const RsPeerServiceInfo &info, 
		RsServiceInfoListItem *item)
{
	item->mServiceInfo = info.mServiceList;
	item->PeerId(info.mPeerId);
	return true;
}

bool convertServiceItemToInfo(
		const RsServiceInfoListItem *item,
		RsPeerServiceInfo &info)
{
	info.mServiceList = item->mServiceInfo;
	info.mPeerId = item->PeerId();
	return true;
}


bool p3ServiceInfo::recvServiceInfoList(RsServiceInfoListItem *item)
{
	RsPeerId peerId = item->PeerId();

#ifdef DEBUG_INFO
    std::cerr << "p3ServiceInfo::recvServiceInfoList() from: " << peerId.toStdString();
    std::cerr << std::endl;
#endif

	RsPeerServiceInfo info;
	if (convertServiceItemToInfo(item, info))
	{

#ifdef DEBUG_INFO
		std::cerr << "p3ServiceInfo::recvServiceInfoList() Info: ";
		std::cerr << std::endl;
		std::cerr << info;
		std::cerr << std::endl;
#endif

		/* update service control */
		mServiceControl->updateServicesProvided(peerId, info);
		return true;
	}
	return false;
}


bool	p3ServiceInfo::sendPackets()
{
	std::set<RsPeerId> updateSet;

	{
		RsStackMutex stack(mInfoMtx); /****** LOCKED MUTEX *******/
		updateSet = mPeersToUpdate;
		mPeersToUpdate.clear();
	}

	mServiceControl->getServiceChanges(updateSet);

	RsStackMutex stack(mInfoMtx); /****** LOCKED MUTEX *******/
	std::set<RsPeerId>::iterator it;
	for(it = updateSet.begin(); it != updateSet.end(); ++it)
	{
		sendServiceInfoList(*it);
	}

	return (!updateSet.empty());
}


int p3ServiceInfo::sendServiceInfoList(const RsPeerId &peerId)
{
#ifdef DEBUG_INFO
    std::cerr << "p3ServiceInfo::sendServiceInfoList() to " << peerId.toStdString();
    std::cerr << std::endl;
#endif

	RsServiceInfoListItem *item = new RsServiceInfoListItem();

	RsPeerServiceInfo info;
	bool sent = false;
	if (mServiceControl->getServicesAllowed(peerId, info))
	{
#ifdef DEBUG_INFO
        std::cerr << "p3ServiceInfo::sendServiceInfoList() Info: ";
		std::cerr << std::endl;
		std::cerr << info;
        std::cerr << std::endl;
#endif

		if (convertServiceInfoToItem(info, item))
		{
			item->PeerId(peerId);

			sent = true;
			sendItem(item);
		}
	}

	if (!sent)
	{
		delete item;
	}

	return sent;
}

void p3ServiceInfo::statusChange(const std::list<pqipeer> &plist)
{
#ifdef DEBUG_INFO
    std::cerr << "p3ServiceInfo::statusChange()";
    std::cerr << std::endl;
#endif

	std::list<pqipeer>::const_iterator it;
	for (it = plist.begin(); it != plist.end(); ++it)
	{
		if (it->state & RS_PEER_S_FRIEND) 
		{
			if (it->actions & RS_PEER_CONNECTED) 
			{
#ifdef DEBUG_INFO
                std::cerr << "p3ServiceInfo::statusChange()";
				std::cerr << "Peer: " << it->id;
				std::cerr << " Connected";
                std::cerr << std::endl;
#endif

				RsStackMutex stack(mInfoMtx); /****** LOCKED MUTEX *******/
				mPeersToUpdate.insert(it->id);
			}
		}
	}
}



