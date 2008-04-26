#ifndef RS_P3_STATUS_INTERFACE_H
#define RS_P3_STATUS_INTERFACE_H

/*
 * libretroshare/src/services: p3status.h
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

#include <map>

#include "rsiface/rsstatus.h"

class p3Status: public RsStatus
{
	public:

	p3Status();
virtual ~p3Status();

/********* RsStatus ***********/

virtual bool getStatus(std::string id, StatusInfo& statusInfo);
virtual bool setStatus(StatusInfo& statusInfo);

/******************************/

	private:

void 	loadDummyData();
std::map<std::string, StatusInfo> mStatusInfoMap;

};

#endif
