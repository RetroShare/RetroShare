/*
 * libretroshare/src/rsserver: p3msgs.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2007-2008 by Robert Fernie.
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


#include "p3status.h"
#include "services/p3statusservice.h"

p3Status::p3Status(p3StatusService* statusSrv)
	: mStatusSrv(statusSrv)	{


}

p3Status::~p3Status(){
	return;
}

bool p3Status::getStatus(std::list<StatusInfo >& statusInfo){

	return mStatusSrv->getStatus(statusInfo);
}


bool p3Status::sendStatus(StatusInfo& statusInfo){

	return mStatusSrv->sendStatus(statusInfo);
}

bool p3Status::statusAvailable(){

	return mStatusSrv->statusAvailable();
}

void p3Status::getStatusString(uint32_t status, std::string& statusString){

	if (status == RS_STATUS_OFFLINE){

		statusString = "Offline";

	}else if (status == RS_STATUS_AWAY){

		statusString = "Away";

		}else if (status == RS_STATUS_BUSY){

		statusString = "Busy";

		}else if (status == RS_STATUS_ONLINE){

			statusString = "Online";
		}

	return;
}
