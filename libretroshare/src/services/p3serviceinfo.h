/*
 * libretroshare/src/services/p3serviceinfo.h
 *
 * Exchange list of Service Information.
 *
 * Copyright 2014 by Robert Fernie.
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

