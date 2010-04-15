#ifndef RS_STATUS_INTERFACE_H
#define RS_STATUS_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsstatus.h
 *
 * RetroShare C++ .
 *
 * Copyright 2007-2008 by Vinny Do, Chris Evi-Parker.
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

class RsStatus;

extern RsStatus *rsStatus;

#include <iostream>
#include <string>
#include <inttypes.h>
#include <list>

const uint32_t RS_STATUS_OFFLINE = 0x0001;
const uint32_t RS_STATUS_AWAY    = 0x0002;
const uint32_t RS_STATUS_BUSY    = 0x0003;
const uint32_t RS_STATUS_ONLINE  = 0x0004;


class StatusInfo
{
	public:
	std::string id;
	uint32_t status;
	time_t time_stamp; /// for owner time set, and for their peers time sent
};

class RsStatus
{
	public:
virtual bool getStatus(std::list<StatusInfo>& statusInfo) = 0;
virtual bool sendStatus(StatusInfo& statusInfo)                 = 0;
virtual bool statusAvailable() = 0;

virtual void getStatusString(uint32_t status, std::string& statusString) = 0;

};


#endif
