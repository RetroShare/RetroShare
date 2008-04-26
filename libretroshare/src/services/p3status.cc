/*
 * libretroshare/src/services: p3status.cc
 *
 * RetroShare C++ .
 *
 * Copyright 2008 by Vinny Do.
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

#include "services/p3status.h"

std::ostream& operator<<(std::ostream& out, const StatusInfo& si)
{
	out << "StatusInfo: " << std::endl;
	out << "id: " << si.id << std::endl;
	out << "status: " << si.status << std::endl;
	return out;
}

RsStatus *rsStatus = NULL;

p3Status::p3Status()
{
	loadDummyData();
}

p3Status::~p3Status()
{
}

/********* RsStatus ***********/

bool p3Status::getStatus(std::string id, StatusInfo& statusInfo)
{
	std::map<std::string, StatusInfo>::iterator it;
	it = mStatusInfoMap.find(id);
	if (it == mStatusInfoMap.end())
	{
		return false;
	}

	statusInfo.id = (it->second).id;
	statusInfo.status = (it->second).status;

	return true;
}

bool p3Status::setStatus(StatusInfo& statusInfo)
{
	mStatusInfoMap[statusInfo.id] = statusInfo;

	return true;
}

/******************************/

void p3Status::loadDummyData()
{
	StatusInfo si;

	si.id = "id01";
	si.status = RS_STATUS_OFFLINE;

	setStatus(si);
	
	si.id = "id02";
	si.status = RS_STATUS_AWAY;

	setStatus(si);

	si.id = "id03";
	si.status = RS_STATUS_BUSY;

	setStatus(si);

	si.id = "id04";
	si.status = RS_STATUS_ONLINE;

	setStatus(si);
}
