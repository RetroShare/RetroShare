/*
 * libretroshare/src/services: p3statusservice.cc
 *
 * RetroShare C++ .
 *
 * Copyright 2008 by Vinny Do, Chris Evi-Parker.
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

#include "services/p3statusservice.h"
#include "serialiser/rsstatusitems.h"
#include "rsserver/p3face.h"
#include "retroshare/rsiface.h"

#include <iostream>
#include <map>
#include <list>
#include <string>
#include <time.h>

#include "pqi/p3linkmgr.h"

std::ostream& operator<<(std::ostream& out, const StatusInfo& si)
{
	out << "StatusInfo: " << std::endl;
	out << "id: " << si.id << std::endl;
	out << "status: " << si.status << std::endl;
	out << "time_stamp: " << si.time_stamp << std::endl;
	return out;
}

RsStatus *rsStatus = NULL;

p3StatusService::p3StatusService(p3LinkMgr *cm)
	:p3Service(), p3Config(CONFIG_TYPE_STATUS), mLinkMgr(cm), mStatusMtx("p3StatusService")
{
	addSerialType(new RsStatusSerialiser());

}

p3StatusService::~p3StatusService()
{
}


const std::string STATUS_APP_NAME = "status";
const uint16_t STATUS_APP_MAJOR_VERSION	= 	1;
const uint16_t STATUS_APP_MINOR_VERSION  = 	0;
const uint16_t STATUS_MIN_MAJOR_VERSION  = 	1;
const uint16_t STATUS_MIN_MINOR_VERSION	=	0;

RsServiceInfo p3StatusService::getServiceInfo()
{
	return RsServiceInfo(RS_SERVICE_TYPE_STATUS, 
		STATUS_APP_NAME,
		STATUS_APP_MAJOR_VERSION, 
		STATUS_APP_MINOR_VERSION, 
		STATUS_MIN_MAJOR_VERSION, 
		STATUS_MIN_MINOR_VERSION);
}


bool p3StatusService::getOwnStatus(StatusInfo& statusInfo)
{
#ifdef STATUS_DEBUG
	std::cerr << "p3StatusService::getOwnStatus() " << std::endl;
#endif

	std::map<RsPeerId, StatusInfo>::iterator it;
	const RsPeerId& ownId = mLinkMgr->getOwnId();

	RsStackMutex stack(mStatusMtx);
	it = mStatusInfoMap.find(ownId);

	if (it == mStatusInfoMap.end()){
		std::cerr << "p3StatusService::saveList() :" << "Did not find your status" << ownId << std::endl;

		// own status not set, set it to online
		statusInfo.id = ownId;
		statusInfo.status = RS_STATUS_ONLINE;

		std::pair<RsPeerId, StatusInfo> pr(ownId, statusInfo);
		mStatusInfoMap.insert(pr);
		IndicateConfigChanged();

		return true;
	}

	statusInfo = it->second;

	return true;
}

bool p3StatusService::getStatusList(std::list<StatusInfo>& statusInfo)
{
#ifdef STATUS_DEBUG
	std::cerr << "p3StatusService::getStatusList() " << std::endl;
#endif

	statusInfo.clear();

	RsStackMutex stack(mStatusMtx);

	// fill up statusInfo list with this information
	std::map<RsPeerId, StatusInfo>::iterator mit;
	for(mit = mStatusInfoMap.begin(); mit != mStatusInfoMap.end(); mit++){
		statusInfo.push_back(mit->second);
	}
	
	return true;
}

bool p3StatusService::getStatus(const RsPeerId &id, StatusInfo &statusInfo)
{
#ifdef STATUS_DEBUG
	std::cerr << "p3StatusService::getStatus() " << std::endl;
#endif

	RsStackMutex stack(mStatusMtx);

	std::map<RsPeerId, StatusInfo>::iterator mit = mStatusInfoMap.find(id);
	
	if (mit == mStatusInfoMap.end()) {
		/* return fake status info as offline */
		statusInfo = StatusInfo();

		statusInfo.id = id;
		statusInfo.status = RS_STATUS_OFFLINE;
		return false;
	}
	
	statusInfo = mit->second;

	return true;
}

/* id = "", status is sent to all online peers */
bool p3StatusService::sendStatus(const RsPeerId &id, uint32_t status)
{
	StatusInfo statusInfo;
	std::list<RsPeerId> onlineList;

	{
		RsStackMutex stack(mStatusMtx);

		statusInfo.id = mLinkMgr->getOwnId();
		statusInfo.status = status;

		// don't save inactive status
		if(statusInfo.status != RS_STATUS_INACTIVE){

			// If your id is not set, set it
			if(mStatusInfoMap.find(statusInfo.id) == mStatusInfoMap.end()){

				std::pair<RsPeerId, StatusInfo> pr(statusInfo.id, statusInfo);
				mStatusInfoMap.insert(pr);
				IndicateConfigChanged();
			} else if(mStatusInfoMap[statusInfo.id].status != statusInfo.status){

				IndicateConfigChanged();
				mStatusInfoMap[statusInfo.id] = statusInfo;
			}
		}

		if (id.isNull()) {
			mLinkMgr->getOnlineList(onlineList);
		} else {
			onlineList.push_back(id);
		}
	}

	std::list<RsPeerId>::iterator it;

#ifdef STATUS_DEBUG
	std::cerr << "p3StatusService::sendStatus() " << std::endl;
	std::cerr << statusInfo;
#endif

	// send to all peers online
	for(it = onlineList.begin(); it != onlineList.end(); it++){
		RsStatusItem* statusItem = new RsStatusItem();
		statusItem->sendTime = time(NULL);
		statusItem->status = statusInfo.status;
		statusItem->PeerId(*it);
		sendItem(statusItem);
	}

	/* send notify of own status change */
	RsServer::notify()->notifyPeerStatusChanged(statusInfo.id.toStdString(), statusInfo.status);

	return true;
}

/******************************/

void p3StatusService::receiveStatusQueue()
{
	std::map<RsPeerId, uint32_t> changed;

	{
		RsStackMutex stack(mStatusMtx);

		RsItem* item;

		while(NULL != (item = recvItem())){

			RsStatusItem* status_item = dynamic_cast<RsStatusItem*>(item);

			if(status_item == NULL) {
				std::cerr << "p3Status::getStatusQueue() " << "Failed to cast Item \n" << std::endl;
				delete (item);
				continue;
			}

#ifdef STATUS_DEBUG
			std::cerr << "p3StatusService::getStatusQueue()" << std::endl;
			std::cerr << "PeerId : " << status_item->PeerId() << std::endl;
			std::cerr << "Status: " << status_item->status << std::endl;
			std::cerr << "Got status Item" << std::endl;
#endif

			std::map<RsPeerId, StatusInfo>::iterator mit = mStatusInfoMap.find(status_item->PeerId());

			if(mit != mStatusInfoMap.end()){
				mit->second.id = status_item->PeerId();
				
				if (mit->second.status != status_item->status) {
					changed [mit->second.id] = status_item->status;
				}

				mit->second.status = status_item->status;
				mit->second.time_stamp = status_item->sendTime;
#ifdef	STATUS_DEBUG
			} else {
				std::cerr << "getStatus() " << "Could not find Peer" << status_item->PeerId();
				std::cerr << std::endl;
#endif
			}

			delete (status_item);
		}

	} /* UNLOCKED */

	if (changed.size()) {
		std::map<RsPeerId, uint32_t>::iterator it;
		for (it = changed.begin(); it != changed.end(); it++) {
			RsServer::notify()->notifyPeerStatusChanged(it->first.toStdString(), it->second);
		}
		RsServer::notify()->notifyPeerStatusChangedSummary();
	}
}

/* p3Config */

RsSerialiser* p3StatusService::setupSerialiser(){

	RsSerialiser *rss = new RsSerialiser;
	rss->addSerialType(new RsStatusSerialiser);

	return rss;
}

bool p3StatusService::saveList(bool& cleanup, std::list<RsItem*>& ilist){

	// save your status before quiting
	cleanup = true;
	RsStatusItem* own_status = new RsStatusItem;
	StatusInfo own_info;

	std::map<RsPeerId, StatusInfo>::iterator it;

	{
		RsStackMutex stack(mStatusMtx);
		it = mStatusInfoMap.find(mLinkMgr->getOwnId());

		if(it == mStatusInfoMap.end()){
			std::cerr << "p3StatusService::saveList() :" << "Did not find your status"
				      << mLinkMgr->getOwnId() << std::endl;
			delete own_status;
			return false;
		}

		own_info = it->second;
	}

	own_status->PeerId(own_info.id);
	own_status->sendTime = own_info.time_stamp;
	own_status->status = own_info.status;

	ilist.push_back(own_status);

	return true;
}

bool p3StatusService::loadList(std::list<RsItem*>& load){

	// load your status from last rs session
	StatusInfo own_info;
	std::list<RsItem*>::const_iterator it = load.begin();

	if(it == load.end()){
		std::cerr << "p3StatusService::loadList(): Failed to load " << std::endl;
		return false;
	}

	for(; it != load.end(); it++){
	RsStatusItem* own_status = dynamic_cast<RsStatusItem* >(*it);


	if(own_status != NULL){

		own_info.id = mLinkMgr->getOwnId();
		own_info.status = own_status->status;
		own_info.time_stamp = own_status->sendTime;
		delete own_status;

		{
			RsStackMutex stack(mStatusMtx);
			std::pair<RsPeerId, StatusInfo> pr(mLinkMgr->getOwnId(), own_info);
			mStatusInfoMap.insert(pr);
		}

		return true;
	}else{
		std::cerr << "p3StatusService::loadList " << "Failed to load list "
				  << std::endl;
	}

	}
	return false;
}


int p3StatusService::tick()
{
	if (receivedItems()) {
		receiveStatusQueue();
	}

	return 0;
}

int p3StatusService::status(){
	return 1;
}

/*************** pqiMonitor callback ***********************/

void p3StatusService::statusChange(const std::list<pqipeer> &plist)
{
	bool changedState = false;

	StatusInfo statusInfo;
	std::list<pqipeer>::const_iterator it;
	for (it = plist.begin(); it != plist.end(); it++) {
		if (it->state & RS_PEER_S_FRIEND) {
			if (it->actions & RS_PEER_DISCONNECTED)
			{
				{
					RsStackMutex stack(mStatusMtx);
					/* remove peer from status map */
					mStatusInfoMap.erase(it->id);
				} /* UNLOCKED */

				changedState = true;
				RsServer::notify()->notifyPeerStatusChanged(it->id.toStdString(), RS_STATUS_OFFLINE);
			}

			if (it->actions & RS_PEER_CONNECTED) {
				/* send current status, only call getOwnStatus once in the loop */
				if (statusInfo.id.isNull() == false || getOwnStatus(statusInfo)) {
					sendStatus(it->id, statusInfo.status);
				}

				{
					RsStackMutex stack(mStatusMtx);

					/* We assume that the peer is online. If not, he send us a new status */
					StatusInfo info;
					info.id = it->id;
					info.status = RS_STATUS_ONLINE;
					info.time_stamp = time(NULL);

					mStatusInfoMap[it->id] = info;
				} /* UNLOCKED */

				changedState = true;
				RsServer::notify()->notifyPeerStatusChanged(it->id.toStdString(), RS_STATUS_ONLINE);
			}
		} else if (it->actions & RS_PEER_MOVED) {
			/* now handle remove */
			{
				RsStackMutex stack(mStatusMtx);
				/* remove peer from status map */
				mStatusInfoMap.erase(it->id);
			} /* UNLOCKED */

			changedState = true;
			RsServer::notify()->notifyPeerStatusChanged(it->id.toStdString(), RS_STATUS_OFFLINE);
		}
	}

	if (changedState)
	{
		RsServer::notify()->notifyPeerStatusChangedSummary();
	}
}
