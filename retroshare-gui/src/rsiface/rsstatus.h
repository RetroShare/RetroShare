#ifndef RS_STATUS_INTERFACE_H
#define RS_STATUS_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsstatus.h
 *
 * RetroShare C++ .
 *
 * Copyright 2007-2008 by Vinny Do.
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

const uint32_t RS_STATUS_OFFLINE = 0x0001;
const uint32_t RS_STATUS_AWAY    = 0x0002;
const uint32_t RS_STATUS_BUSY    = 0x0003;
const uint32_t RS_STATUS_ONLINE  = 0x0004;

std::string RsStatusString(uint32_t status);

class StatusInfo
{
	public:
	std::string id;
	uint32_t status;
};

class RsStatus
{
	public:
virtual bool getStatus(std::string id, StatusInfo& statusInfo) = 0;
virtual bool setStatus(StatusInfo& statusInfo)                 = 0;

};

std::ostream& operator<<(std::ostream& out, const StatusInfo& statusInfo);

#endif
