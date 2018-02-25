#ifndef RS_P3STATUS_INTERFACE_H
#define RS_P3STATUS_INTERFACE_H

/*
 * libretroshare/src/rsserver: p3status.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2007-2008 by Chris Evi-Parker.
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
