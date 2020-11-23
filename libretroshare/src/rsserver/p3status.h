/*******************************************************************************
 * libretroshare/src/rsserver: p3status.h                                      *
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
#ifndef RS_P3STATUS_INTERFACE_H
#define RS_P3STATUS_INTERFACE_H

#include "retroshare/rsstatus.h"


class p3StatusService;

//! Implements abstract interface rsStatus
/*!
 *	Interfaces with p3StatusService
 */
class p3Status : public RsStatus
{
public:

	explicit p3Status(p3StatusService* statusSrv);
	virtual ~p3Status();


	virtual bool getOwnStatus(StatusInfo& statusInfo);
	virtual bool getStatusList(std::list<StatusInfo>& statusInfo);
	virtual bool getStatus(const RsPeerId &id, StatusInfo &statusInfo);
	virtual bool sendStatus(const RsPeerId &id, uint32_t status);

private:

	p3StatusService* mStatusSrv;

};

#endif /* RS_P3STATUS_INTERFACE_H */
