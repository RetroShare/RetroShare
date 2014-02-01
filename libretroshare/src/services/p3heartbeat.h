/*
 * libretroshare/src/services: p3heartbeat.h
 *
 * Services for RetroShare.
 *
 * Copyright 2004-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#ifndef MRK_SERVICES_HEARTBEAT_H
#define MRK_SERVICES_HEARTBEAT_H

// Moved Heartbeat to a seperate service.

#include "pqi/p3linkmgr.h"
#include "pqi/pqipersongrp.h"
#include "services/p3service.h"


class p3heartbeat: public p3Service
{
	public:

	p3heartbeat(p3LinkMgr *linkMgr, pqipersongrp *pqipg);
virtual ~p3heartbeat();

	int	tick();

	private:

	void sendHeartbeat(const std::string &toId);
	void recvHeartbeat(const std::string &fromId);

	private:

	p3LinkMgr *mLinkMgr;
	pqipersongrp *mPqiPersonGrp;

	/* data */
	RsMutex mHeartMtx;

	time_t mLastHeartbeat;
};

#endif // MRK_SERVICES_HEARTBEAT_H
