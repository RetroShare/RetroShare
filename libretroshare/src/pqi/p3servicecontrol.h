/*******************************************************************************
 * libretroshare/src/pqi: p3servicecontrol.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2014-2014 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef P3_SERVICE_CONTROL_HEADER
#define P3_SERVICE_CONTROL_HEADER

#include <string>
#include <map>

#include "retroshare/rsservicecontrol.h"
#include "pqi/p3cfgmgr.h"
#include "pqi/pqimonitor.h"
#include "pqi/pqiservicemonitor.h"
#include "pqi/p3linkmgr.h"

class p3ServiceServer ;

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

class ServiceControlSerialiser ;

class p3ServiceControl: public RsServiceControl, public pqiMonitor, public p3Config
{
public:

	/**
	 */
	explicit p3ServiceControl(p3LinkMgr *linkMgr);

        /**
         * checks and update all added configurations
         * @see rsserver
         */
        void	tick();

        /**
         * provided so that services don't need linkMgr, and can get all info
	 * from ServiceControl.
         * @see rsserver
         */

virtual const 	RsPeerId& getOwnId();

	/**
	 * External Interface (RsServiceControl).
	 */

virtual bool getOwnServices(RsPeerServiceInfo &info);
virtual std::string getServiceName(uint32_t service_id) ;

	// This is what is passed to peers, can be displayed by GUI too.
virtual bool getServicesAllowed(const RsPeerId &peerId, RsPeerServiceInfo &info);

	// Information provided by peer.
virtual bool getServicesProvided(const RsPeerId &peerId, RsPeerServiceInfo &info);

	// Main Permission Interface.
virtual bool getServicePermissions(uint32_t serviceId, RsServicePermissions &permissions);
virtual bool updateServicePermissions(uint32_t serviceId, const RsServicePermissions &permissions);

	// Get List of Peers using this Service.
virtual void getPeersConnected(uint32_t serviceId, std::set<RsPeerId> &peerSet);
virtual bool isPeerConnected(uint32_t serviceId, const RsPeerId &peerId);

    // Gets the list of items used by that service
virtual bool getServiceItemNames(uint32_t serviceId,std::map<uint8_t,std::string>& names) ;

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

        virtual void setServiceServer(p3ServiceServer *p) { mServiceServer = p ; }

protected:
	// configuration.
virtual bool saveList(bool &cleanup, std::list<RsItem *>&);
virtual bool loadList(std::list<RsItem *>& load);
virtual RsSerialiser *setupSerialiser() ;

private:

void    notifyServices();
void    notifyAboutFriends();

void    updatePeerConnect(const RsPeerId &peerId);
void    updatePeerDisconnect(const RsPeerId &peerId);
void    updatePeerNew(const RsPeerId &peerId);
void    updatePeerRemoved(const RsPeerId &peerId);

void 	removePeer(const RsPeerId &peerId);


bool 	updateAllFilters();
bool 	updateAllFilters_locked();
bool 	updateFilterByPeer(const RsPeerId &peerId);
bool 	updateFilterByPeer_locked(const RsPeerId &peerId);


	void    recordFilterChanges_locked(const RsPeerId &peerId,
        	ServicePeerFilter &originalFilter, ServicePeerFilter &updatedFilter);

	// Called from recordFilterChanges.
	void filterChangeAdded_locked(const RsPeerId &peerId, uint32_t serviceId);
	void filterChangeRemoved_locked(const RsPeerId &peerId, uint32_t serviceId);

bool createDefaultPermissions_locked(uint32_t serviceId, const std::string& serviceName, bool defaultOn);
bool peerHasPermissionForService_locked(const RsPeerId &peerId, uint32_t serviceId);

	p3LinkMgr *mLinkMgr;
	const RsPeerId mOwnPeerId; // const from constructor

	RsMutex mCtrlMtx; /* below is protected */

	std::set<RsPeerId> mUpdatedSet;

	// From registration / deregistration.
	std::map<uint32_t, RsServiceInfo> mOwnServices;
	// From peers.
        std::map<RsPeerId, RsPeerServiceInfo> mServicesProvided;
	// derived from all the others.
        std::map<RsPeerId, ServicePeerFilter> mPeerFilterMap;

        std::map<uint32_t, ServiceNotifications> mNotifications;
        std::list<pqiServicePeer> mFriendNotifications;

	// Map of Connected Peers per Service.
	std::map<uint32_t, std::set<RsPeerId> > mServicePeerMap;

	// Separate mutex here - must not hold both at the same time!
	RsMutex mMonitorMtx; /* below is protected */
    std::multimap<uint32_t, pqiServiceMonitor *> mMonitors;

    ServiceControlSerialiser *mSerialiser ;

    // Below here is saved in Configuration.
    std::map<uint32_t, RsServicePermissions> mServicePermissionMap;

    p3ServiceServer *mServiceServer ;
};


#endif // P3_SERVICE_CONTROL_HEADER
