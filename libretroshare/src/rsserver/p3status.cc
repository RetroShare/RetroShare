/*******************************************************************************
 * libretroshare/src/rsserver: p3status.cc                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Chris Evi-Parker.                                    *
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
#include "p3status.h"
#include "services/p3statusservice.h"

p3Status::p3Status(p3StatusService* statusSrv)
	: mStatusSrv(statusSrv)	{


}

p3Status::~p3Status(){
	return;
}

bool p3Status::getOwnStatus(StatusInfo& statusInfo){

	return mStatusSrv->getOwnStatus(statusInfo);
}

bool p3Status::getStatusList(std::list<StatusInfo>& statusInfo){

	return mStatusSrv->getStatusList(statusInfo);
}

bool p3Status::getStatus(const RsPeerId &id, StatusInfo &statusInfo)
{
	return mStatusSrv->getStatus(id, statusInfo);
}

bool p3Status::sendStatus(const RsPeerId &id, uint32_t status){

	return mStatusSrv->sendStatus(id, status);
}
