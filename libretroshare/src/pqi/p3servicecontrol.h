/*
 * libretroshare/src/pqi: p3servicecontrol.h
 *
 * 3P/PQI network interface for RetroShare.
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


#ifndef P3_SERVICE_CONTROL_HEADER
#define P3_SERVICE_CONTROL_HEADER

#include <string>
#include <map>

#include "retroshare/rsservicecontrol.h"
#include "pqi/p3cfgmgr.h"
#include "pqi/pqimonitor.h"
#include "pqi/pqiservicemonitor.h"

class ServiceNotifications
{
	public:
	std::set<RsPeerId> mAdded;
	std::set<RsPeerId> mRemoved;
};

class ServicePeerFilter
{
        public:
        ServicePeerFilter()
        :mDenyAll(true), mAllowAll(false) {}

        bool mDenyAll;
        bool mAllowAll;
        std::set<uint32_t> mAllowedServices;
};

std::ostream &operator<<(std::ostream &out, const ServicePeerFilter &filter);

class p3ServiceControl: public RsServiceControl, public pqiMonitor /* , public p3Config */
{
public:

	/**
	 */
        p3ServiceControl(uint32_t configId);

        /**
         * checks and update all added configurations
         * @see rsserver
         */
        void	tick();

	/**
	 * External Interface (RsServiceControl).
	 */

virtual bool getOwnServices(RsPeerServiceInfo &info);

	// This is what is passed to peers, can be displayed by GUI too.
virtual bool getServicesAllowed(const RsPeerId &peerId, RsPeerServiceInfo &info);

	// Information provided by peer.
virtual bool getServicesProvided(const RsPeerId &peerId, RsPeerServiceInfo &info);

	// Main Permission Interface.
virtual bool getServicePermissions(uint32_t serviceId, RsServicePermissions &permissions);
virtual bool updateServicePermissions(uint32_t serviceId, const RsServicePermissions &permissions);

	/**
	 * Registration for all Services.
	 */

virtual	bool registerService(const RsServiceInfo &info, bool defaultOn);
virtual	bool deregisterService(uint32_t serviceId);

virtual	bool registerServiceMonitor(pqiServiceMonitor *monitor, uint32_t serviceId);
virtual	bool deregisterServiceMonitor(pqiServiceMonitor *monitor);

	/**
	 *
	 */


	// Filter for services.
virtual bool checkFilter(uint32_t serviceId, const RsPeerId &peerId);

	/**
	 * Interface for ServiceInfo service.
	 */

	// ServicesAllowed have changed for these peers.
virtual void getServiceChanges(std::set<RsPeerId> &updateSet);

	// Input from peers.	
virtual bool updateServicesProvided(const RsPeerId &peerId, const RsPeerServiceInfo &info);

	// pqiMonitor.
virtual void    statusChange(const std::list<pqipeer> &plist);

protected:
	// configuration.
virtual bool saveList(bool &cleanup, std::list<RsItem *>&);
virtual bool loadList(std::list<RsItem *>& load);

private:

void    notifyServices();
void    notifyAboutFriends();

bool createDefaultPermissions_locked(uint32_t serviceId, std::string serviceName, bool defaultOn);

bool 	updateAllFilters();
bool 	updateFilterByPeer(const RsPeerId &peerId);
bool 	updateFilterByPeer_locked(const RsPeerId &peerId);

void 	removePeer(const RsPeerId &peerId);

void    recordFilterChanges_locked(const RsPeerId &peerId,
        	ServicePeerFilter &originalFilter, ServicePeerFilter &updatedFilter);

void    updatePeerConnect(const RsPeerId &peerId);
void    updatePeerDisconnect(const RsPeerId &peerId);
void    updatePeerNew(const RsPeerId &peerId);
void    updatePeerRemoved(const RsPeerId &peerId);


bool peerHasPermissionForService_locked(const RsPeerId &peerId, uint32_t serviceId);


	RsMutex mCtrlMtx; /* below is protected */

	std::set<RsPeerId> mUpdatedSet;

	// From registration / deregistration.
	std::map<uint32_t, RsServiceInfo> mOwnServices;
	// From peers.
        std::map<RsPeerId, RsPeerServiceInfo> mServicesProvided;
	// derived from all the others.
        std::map<RsPeerId, ServicePeerFilter> mPeerFilterMap;

	// Below here is saved in Configuration.
	std::map<uint32_t, RsServicePermissions> mServicePermissionMap;

        std::map<uint32_t, ServiceNotifications> mNotifications;
        std::list<pqiServicePeer> mFriendNotifications;

	// Separate mutex here - must not hold both at the same time!
	RsMutex mMonitorMtx; /* below is protected */
	std::multimap<uint32_t, pqiServiceMonitor *> mMonitors;
};


#endif // P3_SERVICE_CONTROL_HEADER
