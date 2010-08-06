#ifndef RS_P3_STATUS_INTERFACE_H
#define RS_P3_STATUS_INTERFACE_H

/*
 * libretroshare/src/services: p3statusService.h
 *
 * RetroShare C++
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

#include <map>
#include <list>

#include "serialiser/rsstatusitems.h"
#include "retroshare/rsstatus.h"
#include "services/p3service.h"
#include "pqi/p3connmgr.h"

//! handles standard status messages (busy, away, online, offline) set by user
/*!
 * The is a retroshare service which allows peers
 * to inform each other about their status in a standard way, as opposed to
 * custom string.
 * @see rsiface/rsstatus.h for status constants
 */
class p3StatusService: public p3Service, public p3Config, public pqiMonitor
{
	public:

	p3StatusService(p3ConnectMgr* );
virtual ~p3StatusService();

/***** overloaded from p3Service *****/
virtual int tick();
virtual int status();

/*************** pqiMonitor callback ***********************/
virtual void    statusChange(const std::list<pqipeer> &plist);

/********* RsStatus ***********/

/**
 * Status is set to offline as default if no info received from relevant peer
 */
virtual bool getOwnStatus(StatusInfo& statusInfo);
virtual bool getStatus(std::list<StatusInfo>& statusInfo);
/* id = "", status is sent to all online peers */
virtual bool sendStatus(const std::string &id, uint32_t status);
virtual bool statusAvailable();

/******************************/


/** implemented from p3Config **/

/*!
 * @return The serialiser the enables storage of save info
 */
virtual RsSerialiser *setupSerialiser();

/*!
 * This stores information on what your status was before you exited rs
 */
virtual std::list<RsItem*> saveList(bool& cleanup);

/*!
 * @param load Should contain a single item which is clients status from last rs session
 */
virtual bool loadList(std::list<RsItem*> load);

	private:

virtual void getStatusQueue(std::list<RsStatusItem* > &ilist);

p3ConnectMgr *mConnMgr;

std::map<std::string, StatusInfo> mStatusInfoMap;

RsMutex mStatusMtx;

};

#endif
