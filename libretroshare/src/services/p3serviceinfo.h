/*******************************************************************************
 * libretroshare/src/services: p3serviceinfo.h                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2014 Robert Fernie <retroshare@lunamutt.com>                      *
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
#ifndef SERVICE_RSSERVICEINFO_HEADER
#define SERVICE_RSSERVICEINFO_HEADER

#include <string>
#include <list>
#include <map>

#include "pqi/p3servicecontrol.h"
#include "pqi/pqimonitor.h"

#include "services/p3service.h"

#include "rsitems/rsserviceinfoitems.h"

//!The ServiceInfo service.
 /**
  *
  * Exchange list of Available Services with peers.
  */

class p3ServiceInfo: public p3Service, public pqiMonitor 
{
	public:
		p3ServiceInfo(p3ServiceControl *serviceControl);
		virtual RsServiceInfo getServiceInfo();

		virtual int   tick();
		virtual int   status();

		/*************** pqiMonitor callback ***********************/
		virtual void statusChange(const std::list<pqipeer> &plist);

	private:

		bool    sendPackets();
		bool 	processIncoming();

		bool 	recvServiceInfoList(RsServiceInfoListItem *item);
		int 	sendServiceInfoList(const RsPeerId &peerid);

	private:
		RsMutex mInfoMtx;

		std::set<RsPeerId> mPeersToUpdate;
		p3ServiceControl *mServiceControl;
};

#endif // SERVICE_RSSERVICEINFO_HEADER

