/*******************************************************************************
 * libretroshare/src/services: p3heartbeat.cc                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2013 Robert Fernie <retroshare@lunamutt.com>                 *
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
#include "util/rstime.h"

#include "services/p3heartbeat.h"
#include "rsitems/rsheartbeatitems.h"

#include "pqi/p3servicecontrol.h"
#include "pqi/pqipersongrp.h"

//#define HEART_DEBUG	1


p3heartbeat::p3heartbeat(p3ServiceControl *sc, pqipersongrp *pqipg)
:p3Service(), mServiceCtrl(sc), mPqiPersonGrp(pqipg), 
	mHeartMtx("p3heartbeat")
{
	RsStackMutex stack(mHeartMtx); /********** STACK LOCKED MTX ******/

	addSerialType(new RsHeartbeatSerialiser());

#ifdef HEART_DEBUG
	std::cerr << "p3heartbeat::p3heartbeat()";
	std::cerr << std::endl;
#endif

	mLastHeartbeat = 0;

	return;
}

p3heartbeat::~p3heartbeat()
{
	return;
	
}


const std::string HEARTBEAT_APP_NAME = "heartbeat";
const uint16_t HEARTBEAT_APP_MAJOR_VERSION  =       1;
const uint16_t HEARTBEAT_APP_MINOR_VERSION  =       0;
const uint16_t HEARTBEAT_MIN_MAJOR_VERSION  =       1;
const uint16_t HEARTBEAT_MIN_MINOR_VERSION  =       0;

RsServiceInfo p3heartbeat::getServiceInfo()
{
        return RsServiceInfo(RS_SERVICE_TYPE_HEARTBEAT,
                HEARTBEAT_APP_NAME,
                HEARTBEAT_APP_MAJOR_VERSION,
                HEARTBEAT_APP_MINOR_VERSION,
                HEARTBEAT_MIN_MAJOR_VERSION,
                HEARTBEAT_MIN_MINOR_VERSION);
}


int p3heartbeat::tick()
{
        //send a heartbeat to all connected peers
	RsStackMutex stack(mHeartMtx); /********** STACK LOCKED MTX ******/

	if (time(NULL) - mLastHeartbeat > HEARTBEAT_REPEAT_TIME) 
	{
		mLastHeartbeat = time(NULL);

		std::set<RsPeerId> peers;
		std::set<RsPeerId>::const_iterator pit;

		mServiceCtrl->getPeersConnected(getServiceInfo().mServiceType, peers);
		for (pit = peers.begin(); pit != peers.end(); ++pit) 
		{
			sendHeartbeat(*pit);
		}
	}

	int nhandled = 0;
	RsItem *item = NULL;

	// While messages read
	while(NULL != (item = recvItem()))
	{
		RsHeartbeatItem *beat = NULL;
		nhandled++;

		// if discovery reply then respond if haven't already.
		if (NULL != (beat = dynamic_cast<RsHeartbeatItem *> (item)))	
		{
			recvHeartbeat(beat->PeerId());
		}
		else
		{
			// unknown.
		}

		delete item;
	}

	return nhandled ;
}

void p3heartbeat::sendHeartbeat(const RsPeerId &toId)
{

#ifdef HEART_DEBUG
	std::cerr << "p3heartbeat::sendHeartbeat() to " << toId;
	std::cerr << std::endl;
#endif
	RsHeartbeatItem *item = new RsHeartbeatItem();
	item->PeerId(toId);
	sendItem(item);
}


void p3heartbeat::recvHeartbeat(const RsPeerId &fromId)
{

#ifdef HEART_DEBUG
	std::cerr << "p3heartbeat::recvHeartbeat() from " << fromId;
	std::cerr << std::endl;
#endif

	mPqiPersonGrp->tagHeartbeatRecvd(fromId);
}


