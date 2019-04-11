/*******************************************************************************
 * libretroshare/src/services: p3statusservice.h                               *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008 by Vinny Do, Chris Evi-Parker.                               *
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
#ifndef RS_P3_STATUS_INTERFACE_H
#define RS_P3_STATUS_INTERFACE_H

#include <map>
#include <list>

#include "rsitems/rsstatusitems.h"
#include "retroshare/rsstatus.h"
#include "services/p3service.h"
#include "pqi/p3cfgmgr.h"
#include "pqi/pqiservicemonitor.h"

class p3ServiceControl;

//! handles standard status messages (busy, away, online, offline) set by user
/*!
 * The is a retroshare service which allows peers
 * to inform each other about their status in a standard way, as opposed to
 * custom string.
 * @see rsiface/rsstatus.h for status constants
 */
class p3StatusService: public p3Service, public p3Config, public pqiServiceMonitor
{
	public:

	p3StatusService(p3ServiceControl *sc);
virtual ~p3StatusService();

virtual RsServiceInfo getServiceInfo();

/***** overloaded from p3Service *****/
virtual int tick();
virtual int status();

/*************** pqiMonitor callback ***********************/
virtual void    statusChange(const std::list<pqiServicePeer> &plist);

/********* RsStatus ***********/

/**
 * Status is set to offline as default if no info received from relevant peer
 */
virtual bool getOwnStatus(StatusInfo& statusInfo);
virtual bool getStatusList(std::list<StatusInfo>& statusInfo);
virtual bool getStatus(const RsPeerId &id, StatusInfo &statusInfo);
/* id = "", status is sent to all online peers */
virtual bool sendStatus(const RsPeerId &id, uint32_t status);

/******************************/


/** implemented from p3Config **/

/*!
 * @return The serialiser the enables storage of save info
 */
virtual RsSerialiser *setupSerialiser();

/*!
 * This stores information on what your status was before you exited rs
 */
virtual bool saveList(bool& cleanup, std::list<RsItem*>&);

/*!
 * @param load Should contain a single item which is clients status from last rs session
 */
virtual bool loadList(std::list<RsItem*>& load);

	private:

virtual void receiveStatusQueue();

p3ServiceControl *mServiceCtrl;

std::map<RsPeerId, StatusInfo> mStatusInfoMap;

RsMutex mStatusMtx;

};

#endif
