/*******************************************************************************
 * libretroshare/src/services: p3heartbeat.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2013 Robert Fernie <retroshare@lunamutt.com>                 *
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
#ifndef MRK_SERVICES_HEARTBEAT_H
#define MRK_SERVICES_HEARTBEAT_H

// Moved Heartbeat to a seperate service.

#include "services/p3service.h"

class p3ServiceControl;
class pqipersongrp;

class p3heartbeat: public p3Service
{
	public:

	p3heartbeat(p3ServiceControl *sc, pqipersongrp *pqipg);
virtual ~p3heartbeat();

virtual RsServiceInfo getServiceInfo();

	int	tick();

	private:

	void sendHeartbeat(const RsPeerId &toId);
	void recvHeartbeat(const RsPeerId &fromId);

	private:

	p3ServiceControl *mServiceCtrl;
	pqipersongrp *mPqiPersonGrp;

	/* data */
	RsMutex mHeartMtx;

	rstime_t mLastHeartbeat;
};

#endif // MRK_SERVICES_HEARTBEAT_H
