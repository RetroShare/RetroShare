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

#include <iostream>
#include <map>
#include <list>
#include <string>

std::ostream& operator<<(std::ostream& out, const StatusInfo& si)
{
	out << "StatusInfo: " << std::endl;
	out << "id: " << si.id << std::endl;
	out << "status: " << si.status << std::endl;
	out << "time_stamp: " << si.time_stamp << std::endl;
	return out;
}

RsStatus *rsStatus = NULL;

p3StatusService::p3StatusService(p3ConnectMgr *cm)
	:p3Service(RS_SERVICE_TYPE_STATUS), p3Config(CONFIG_TYPE_STATUS), mConnMgr(cm)
{
	addSerialType(new RsStatusSerialiser());

}

p3StatusService::~p3StatusService()
{
}


bool p3StatusService::getStatus(std::list<StatusInfo>& statusInfo)
{

	time_t time_now = time(NULL);

#ifdef STATUS_DEBUG
	std::cerr << "p3StatusService::getStatus() " << std::endl;
#endif

	statusInfo.clear();

	std::list<RsStatusItem* > status_items = getStatusQueue();
	std::list<RsStatusItem* >::iterator rit;
	std::map<std::string, StatusInfo>::iterator mit;
	std::list<std::string> peers, peersOnline;
	std::list<std::string>::iterator pit, pit_online;

	{
		RsStackMutex stack(mStatusMtx);

		/* first update map */

		mConnMgr->getFriendList(peers);
		mConnMgr->getOnlineList(peersOnline);
		pit_online = peersOnline.begin();

		// ensure member map is up to date with all client's peers
		for(pit = peers.begin(); pit != peers.end(); pit++){

			mit = mStatusInfoMap.find(*pit);

			if(mit == mStatusInfoMap.end()){

				StatusInfo info;
				info.id = *pit;
				info.status = RS_STATUS_ONLINE;
				info.time_stamp = time_now;
				std::pair<std::string, StatusInfo> pr(*pit, info);
				mStatusInfoMap.insert(pr);
			}

		}

		// now note members who have sent specific status updates
		for(rit = status_items.begin(); rit != status_items.end(); rit++){

			RsStatusItem* si = dynamic_cast<RsStatusItem* >(*rit);

			if(si == NULL){
				std::cerr << "p3Status::getStatus() " << "Failed to cast Item \n" << std::endl;
			}


			mit = mStatusInfoMap.find(si->PeerId());


#ifdef	STATUS_DEBUG
			if(mit == mStatusInfoMap.end()){
				std::cerr << "p3GetStatus() " << "Could not find Peer" << si->PeerId();
				std::cerr << std::endl;
			}
#endif

			mit->second.id = si->PeerId();
			mit->second.status = si->status;
			mit->second.time_stamp = si->sendTime;

		}

		// then fill up statusInfo list with this information
		for(mit = mStatusInfoMap.begin(); mit != mStatusInfoMap.end(); mit++){
			statusInfo.push_back(mit->second);
		}

	}

	return true;
}

bool p3StatusService::sendStatus(StatusInfo& statusInfo)
{
	std::list<std::string> onlineList;

	{
		RsStackMutex stack(mStatusMtx);

		if(statusInfo.id != mConnMgr->getOwnId())
			return false;

		// don't save inactive status
		if(statusInfo.status != RS_STATUS_INACTIVE){

			// If your id is not set, set it
			if(mStatusInfoMap.find(statusInfo.id) == mStatusInfoMap.end()){

				std::pair<std::string, StatusInfo> pr(statusInfo.id, statusInfo);
				mStatusInfoMap.insert(pr);
				IndicateConfigChanged();
			}else
			if(mStatusInfoMap[statusInfo.id].status != statusInfo.status){

				IndicateConfigChanged();
				mStatusInfoMap[statusInfo.id] = statusInfo;
			}
		}

		mConnMgr->getOnlineList(onlineList);
	}

	std::list<std::string>::iterator it;

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


	return true;
}


bool p3StatusService::statusAvailable(){
	return receivedItems();
}

/******************************/

std::list<RsStatusItem* > p3StatusService::getStatusQueue(){


	time_t time_now = time(NULL);

	RsItem* item;
	std::list<RsStatusItem* > ilist;

	while(NULL != (item = recvItem())){

		RsStatusItem* status_item = dynamic_cast<RsStatusItem*>(item);

		if(status_item != NULL){
#ifdef STATUS_DEBUG
			std::cerr << "p3StatusService::getStatusQueue()" << std::endl;
			std::cerr << "PeerId : " << status_item->PeerId() << std::endl;
			std::cerr << "Status: " << status_item->status << std::endl;
			std::cerr << "Got status Item" << std::endl;
#endif
			status_item->recvTime = time_now;
			ilist.push_back(status_item);
		}
	}

	return ilist;
}

/* p3Config */

RsSerialiser* p3StatusService::setupSerialiser(){

	RsSerialiser *rss = new RsSerialiser;
	rss->addSerialType(new RsStatusSerialiser);

	return rss;
}

std::list<RsItem*> p3StatusService::saveList(bool& cleanup){

	// save your status before quiting
	cleanup = true;
	RsStatusItem* own_status = new RsStatusItem;
	StatusInfo own_info;
	std::list<RsItem*> ilist;
	std::map<std::string, StatusInfo>::iterator it;

	{
		RsStackMutex stack(mStatusMtx);
		it = mStatusInfoMap.find(mConnMgr->getOwnId());

		if(it == mStatusInfoMap.end()){
			std::cerr << "p3StatusService::saveList() :" << "Did not find your status"
				      << mConnMgr->getOwnId() << std::endl;
			delete own_status;
			return ilist;
		}

		own_info = it->second;
	}

	own_status->PeerId(own_info.id);
	own_status->sendTime = own_info.time_stamp;
	own_status->status = own_info.status;

	ilist.push_back(own_status);

	return ilist;
}

bool p3StatusService::loadList(std::list<RsItem*> load){

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

		own_info.id = mConnMgr->getOwnId();
		own_info.status = own_status->status;
		own_info.time_stamp = own_status->sendTime;
		delete own_status;

		{
			RsStackMutex stack(mStatusMtx);
			std::pair<std::string, StatusInfo> pr(mConnMgr->getOwnId(), own_info);
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


int p3StatusService::tick(){
	return 0;
}

int p3StatusService::status(){
	return 1;
}



