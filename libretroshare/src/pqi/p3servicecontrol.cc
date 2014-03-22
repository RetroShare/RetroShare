/*
 * libretroshare/src/pqi: p3servicecontrol.cc
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


#include "p3servicecontrol.h"
#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"

RsServiceControl *rsServiceControl = NULL;

p3ServiceControl::p3ServiceControl(uint32_t configId)
        :RsServiceControl(), /* p3Config(configId), pqiMonitor(),  */
	mCtrlMtx("p3ServiceControl")
{
}

/* Interface for Services */
bool p3ServiceControl::registerService(const RsServiceInfo &info, bool defaultOn)
{
	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	std::map<uint32_t, RsServiceInfo>::iterator it;
	it = mOwnServices.find(info.mServiceType);
	if (it != mOwnServices.end())
	{
		std::cerr << "p3ServiceControl::registerService() ERROR Duplicate Service ID";
		std::cerr << std::endl;
		return false;
	}

	/* sanity check ServiceInfo */
	mOwnServices[info.mServiceType] = info;

	std::cerr << "p3ServiceControl::registerService() Registered ServiceID: " << info.mServiceType;
	std::cerr << std::endl;
	std::cerr << "p3ServiceControl::registerService() ServiceName: " << info.mServiceName;
	std::cerr << std::endl;


	/* create default permissions for this service
	 * this will be overwritten by configuration (if it exists).
	 */

	createDefaultPermissions_locked(info.mServiceType, info.mServiceName, defaultOn);
	return true;
}

bool p3ServiceControl::deregisterService(uint32_t serviceId)
{
	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	std::map<uint32_t, RsServiceInfo>::iterator it;
	it = mOwnServices.find(serviceId);
	if (it == mOwnServices.end())
	{
		std::cerr << "p3ServiceControl::deregisterService() ERROR No matching Service ID";
		std::cerr << std::endl;
		return false;
	}

	std::cerr << "p3ServiceControl::deregisterService() Removed ServiceID: " << serviceId;
	std::cerr << std::endl;
	mOwnServices.erase(it);
	return true;
}


/* Interface for Services */
bool p3ServiceControl::registerServiceMonitor(pqiServiceMonitor *monitor, uint32_t serviceId)
{
	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	std::cerr << "p3ServiceControl::registerServiceMonitor() for ServiceId: ";
	std::cerr << serviceId;
	std::cerr << std::endl;

	mMonitors.insert(std::make_pair(serviceId, monitor));
	return true;
}


bool p3ServiceControl::deregisterServiceMonitor(pqiServiceMonitor *monitor)
{
	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	std::cerr << "p3ServiceControl::deregisterServiceMonitor()";
	std::cerr << std::endl;

	std::multimap<uint32_t, pqiServiceMonitor *>::iterator it;
	for(it = mMonitors.begin(); it != mMonitors.end(); )
	{
		if (it->second == monitor)
		{
			mMonitors.erase(it++);
		}
		else
		{
			++it;
		}
	}
	return true;
}

/* Interface to p3ServiceInfo */
void p3ServiceControl::getServiceChanges(std::set<RsPeerId> &updateSet)
{
	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	std::cerr << "p3ServiceControl::getServiceChanges()";
	std::cerr << std::endl;

	std::set<RsPeerId>::iterator it;
	for (it = mUpdatedSet.begin(); it != mUpdatedSet.end(); it++)
	{
		updateSet.insert(*it);
	}

	mUpdatedSet.clear();
}


bool p3ServiceControl::getOwnServices(RsPeerServiceInfo &info)
{
	std::cerr << "p3ServiceControl::getOwnServices()";
	std::cerr << std::endl;

	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	info.mPeerId = "";
	info.mServiceList = mOwnServices;
	return true;
}


bool p3ServiceControl::getServicesAllowed(const RsPeerId &peerId, RsPeerServiceInfo &info)
{
	std::cerr << "p3ServiceControl::getServicesAllowed(" << peerId.toStdString() << ")";
	std::cerr << std::endl;

	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	info.mPeerId = peerId;

	// For each registered Service.. check if peer has permissions.
	std::map<uint32_t, RsServiceInfo>::iterator it;
	for(it = mOwnServices.begin(); it != mOwnServices.end(); it++)
	{
		if (peerHasPermissionForService_locked(peerId, it->first))
		{
			info.mServiceList[it->first] = it->second;
		}
	}
	return true;
}

bool p3ServiceControl::peerHasPermissionForService_locked(const RsPeerId &peerId, uint32_t serviceId)
{
	std::cerr << "p3ServiceControl::peerHasPermissionForService_locked()";
	std::cerr << std::endl;

	std::map<uint32_t, RsServicePermissions>::iterator it;
	it = mServicePermissionMap.find(serviceId);
	if (it == mServicePermissionMap.end())
	{
		return false;
	}
	return it->second.peerHasPermission(peerId);
}

// This is used by both RsServiceControl and p3ServiceInfo.
bool p3ServiceControl::getServicesProvided(const RsPeerId &peerId, RsPeerServiceInfo &info)
{
	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	std::cerr << "p3ServiceControl::getServicesProvided()";
	std::cerr << std::endl;

	std::map<RsPeerId, RsPeerServiceInfo>::iterator it;
	it = mServicesProvided.find(peerId);
	if (it == mServicesProvided.end())
	{
		return false;
	}

	info = it->second;
	return true;
}

bool p3ServiceControl::updateServicesProvided(const RsPeerId &peerId, const RsPeerServiceInfo &info)
{
	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	std::cerr << "p3ServiceControl::updateServicesProvided() from: " << peerId.toStdString();
	std::cerr << std::endl;

	std::cerr << info;
	std::cerr << std::endl;

	mServicesProvided[peerId] = info;
	updateFilterByPeer_locked(peerId);
	return true;
}

/* External Interface to RsServiceControl */
bool p3ServiceControl::getServicePermissions(uint32_t serviceId, RsServicePermissions &permissions)
{
	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	std::cerr << "p3ServiceControl::getServicePermissions()";
	std::cerr << std::endl;

	std::map<uint32_t, RsServicePermissions>::iterator it;

	it = mServicePermissionMap.find(serviceId);
	if (it == mServicePermissionMap.end())
	{
		return false;
	}
	permissions = it->second;
	return true;
}

bool p3ServiceControl::createDefaultPermissions_locked(uint32_t serviceId, std::string serviceName, bool defaultOn)
{
	std::map<uint32_t, RsServicePermissions>::iterator it;
	it = mServicePermissionMap.find(serviceId);
	if (it == mServicePermissionMap.end())
	{
		RsServicePermissions perms;
		perms.mServiceId = serviceId;
		perms.mServiceName = serviceName;
		perms.mDefaultAllowed = defaultOn;

		mServicePermissionMap[serviceId] = perms;
		return true;
	}
	return false;
}


bool p3ServiceControl::updateServicePermissions(uint32_t serviceId, const RsServicePermissions &permissions)
{
	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	std::cerr << "p3ServiceControl::updateServicePermissions()";
	std::cerr << std::endl;

	std::map<uint32_t, RsServicePermissions>::iterator it;
	it = mServicePermissionMap.find(serviceId);


	/* here we have to check which peers - permissions have changed for, 
	 * and flag for updates.
	 *
	 * This is far more complex.
	 * need to check if versions are compatible and peers are providing service.
	 */
#if 0
	std::list<std::string> onlinePeers;
	std::list<std::string>::const_iterator pit;
	if (it != mServicePermissionMap.end())
	{
		for(pit = onlinePeers.begin(); pit != onlinePeers.end(); pit++)
		{
			if (it->second.peerHasPermission(*pit) != 
				permissions.peerHasPermission(*pit))
			{
				mUpdatedSet.insert(*pit);
			}
		}
	}
	else
	{
		// ERROR!
		it = mServicePermissionMap.find[serviceId];
		for(pit = onlinePeers.begin(); pit != onlinePeers.end(); pit++)
		{
			mUpdatedSet.insert(*pit);
		}
	}
#endif

 	it->second = permissions;
 	it->second.mServiceId = serviceId; // just to make sure!
	return true;
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

// This is the basic system...
// which will be a little slow (2x maps).
//
// Potential improvement is to push the RsPeerFilter
// to the pqiStreamer, (when items have been sorted by peers already!).
// but we will do this later.

bool	p3ServiceControl::checkFilter(uint32_t serviceId, const RsPeerId &peerId)
{
	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	std::cerr << "p3ServiceControl::checkFilter() ";
	std::cerr << " ServiceId: " << serviceId;

	std::map<uint32_t, RsServiceInfo>::iterator it;
	it = mOwnServices.find(serviceId);
	if (it != mOwnServices.end())
	{
		std::cerr << " ServiceName: " << it->second.mServiceName;
	}
	else
	{
		std::cerr << " ServiceName: Unknown! ";
	}

	std::cerr << " PeerId: " << peerId.toStdString();
	std::cerr << std::endl;

	// must allow ServiceInfo through, or we have nothing!
#define FULLID_SERVICEINFO ((((uint32_t) RS_PKT_VERSION_SERVICE) << 24) + ((RS_SERVICE_TYPE_SERVICEINFO) << 8)) 

	//if (serviceId == RS_SERVICE_TYPE_SERVICEINFO)
	if (serviceId == FULLID_SERVICEINFO)
	{
		std::cerr << "p3ServiceControl::checkFilter() Allowed SERVICEINFO";
		std::cerr << std::endl;
		return true;
	}


	std::map<RsPeerId, ServicePeerFilter>::const_iterator pit;
	pit = mPeerFilterMap.find(peerId);
	if (pit == mPeerFilterMap.end())
	{
		std::cerr << "p3ServiceControl::checkFilter() Denied No PeerId";
		std::cerr << std::endl;
		return false;
	}

	if (pit->second.mDenyAll)
	{
		std::cerr << "p3ServiceControl::checkFilter() Denied Peer.DenyAll";
		std::cerr << std::endl;
		return false;
	}

	if (pit->second.mAllowAll)
	{
		std::cerr << "p3ServiceControl::checkFilter() Allowed Peer.AllowAll";
		std::cerr << std::endl;
		return true;
	}

	std::set<uint32_t>::const_iterator sit;
	sit = pit->second.mAllowedServices.find(serviceId);
	if (sit == pit->second.mAllowedServices.end())
	{
		std::cerr << "p3ServiceControl::checkFilter() Denied !Peer.find(serviceId)";
		std::cerr << std::endl;
		return false;
	}
	std::cerr << "p3ServiceControl::checkFilter() Allowed Peer.find(serviceId)";
	std::cerr << std::endl;
	return true;
}

bool versionOkay(uint16_t version_major, uint16_t version_minor,
	uint16_t min_version_major, uint16_t min_version_minor)
{
	if (version_major > min_version_major)
	{
		return true;
	}
	if (version_major == min_version_major)
	{
		return (version_minor >= min_version_minor);
	}
	return false;
}


bool ServiceInfoCompatible(const RsServiceInfo &info1, const RsServiceInfo &info2)
{
	// Id, or Name mismatch.
	if ((info1.mServiceType != info2.mServiceType) ||
		(info1.mServiceName != info2.mServiceName))
	{
		std::cerr << "servicesCompatible: Type/Name mismatch";
		std::cerr << std::endl;
		std::cerr << "Info1 ID: " << info1.mServiceType;
		std::cerr << " " << info1.mServiceName;
		std::cerr << std::endl;
		std::cerr << "Info2 ID: " << info2.mServiceType;
		std::cerr << " " << info2.mServiceName;
		std::cerr << std::endl;
		return false;
	}

	// ensure that info1 meets minimum requirements for info2
	if (!versionOkay(info1.mVersionMajor, info1.mVersionMinor, 
		info2.mMinVersionMajor, info2.mMinVersionMinor))
	{
		return false;
	}

	// ensure that info2 meets minimum requirements for info1
	if (!versionOkay(info2.mVersionMajor, info2.mVersionMinor, 
		info1.mMinVersionMajor, info1.mMinVersionMinor))
	{
		return false;
	}
	return true;
}
	

bool	p3ServiceControl::updateFilterByPeer(const RsPeerId &peerId)
{
	std::cerr << "p3ServiceControl::updateFilterByPeer()";
	std::cerr << std::endl;

	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/
	return updateFilterByPeer_locked(peerId);
}


bool	p3ServiceControl::updateAllFilters()
{
	std::cerr << "p3ServiceControl::updateAllFilters()";
	std::cerr << std::endl;

	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	// Create a peerSet from ServicesProvided + PeerFilters.
	// This will completely refresh the Filters.
	std::set<RsPeerId> peerSet;
	std::set<RsPeerId>::const_iterator pit;

	std::map<RsPeerId, RsPeerServiceInfo>::const_iterator it;
	for(it = mServicesProvided.begin(); it != mServicesProvided.end(); it++)
	{
		peerSet.insert(it->first);
	}

        std::map<RsPeerId, ServicePeerFilter>::const_iterator fit;
	for(fit = mPeerFilterMap.begin(); fit != mPeerFilterMap.end(); fit++)
	{
		peerSet.insert(fit->first);
	}

	for(pit = peerSet.begin(); pit != peerSet.end(); pit++)
	{
		updateFilterByPeer_locked(*pit);
	}
	return true;
}


// create filter. (the easy way).
bool	p3ServiceControl::updateFilterByPeer_locked(const RsPeerId &peerId)
{
	std::cerr << "p3ServiceControl::updateFilterByPeer_locked() : " << peerId.toStdString();
	std::cerr << std::endl;

	ServicePeerFilter originalFilter;
	ServicePeerFilter peerFilter;

        std::map<RsPeerId, ServicePeerFilter>::iterator fit;
	fit = mPeerFilterMap.find(peerId);
	if (fit != mPeerFilterMap.end())
	{
		originalFilter = fit->second;
	}

	std::map<RsPeerId, RsPeerServiceInfo>::iterator it;
	it = mServicesProvided.find(peerId);
	if (it == mServicesProvided.end())
	{
		std::cerr << "p3ServiceControl::updateFilterByPeer_locked() Empty ... Clearing";
		std::cerr << std::endl;
	
		// empty, remove... 
		recordFilterChanges_locked(peerId, originalFilter, peerFilter);
		if (fit != mPeerFilterMap.end())
		{
			std::cerr << std::endl;
			mPeerFilterMap.erase(fit);
		}
		return true;
	}

	// For each registered Service.. check if services compatible.
	// then check for permissions.

	// similar maps, can iterate through in parallel.
	std::map<uint32_t, RsServiceInfo>::const_iterator oit = mOwnServices.begin();
	std::map<uint32_t, RsServiceInfo>::const_iterator eoit = mOwnServices.end();
	std::map<uint32_t, RsServiceInfo>::const_iterator tit = it->second.mServiceList.begin();
	std::map<uint32_t, RsServiceInfo>::const_iterator etit = it->second.mServiceList.end();

	std::cerr << "p3ServiceControl::updateFilterByPeer_locked() Comparing lists";
	std::cerr << std::endl;


	while((oit != eoit) && (tit != etit))
	{
		if (oit->first == tit->first)
		{
			std::cerr << "\tChecking Matching Service ID: " << oit->first;
			std::cerr << std::endl;
			/* match of service IDs */
			/* check if compatible */
			if (ServiceInfoCompatible(oit->second, tit->second))
			{
				if (peerHasPermissionForService_locked(peerId, oit->first))
				{
					std::cerr << "\t\tMatched Service ID: " << oit->first;
					std::cerr << std::endl;
					peerFilter.mAllowedServices.insert(oit->first);
				}
			}
			oit++;
			tit++;
		}
		else
		{
			if (oit->first < tit->first)
			{
				std::cerr << "\tSkipping Only Own Service ID: " << oit->first;
				std::cerr << std::endl;
				oit++;
			}
			else
			{
				std::cerr << "\tSkipping Only Peer Service ID: " << tit->first;
				std::cerr << std::endl;
				tit++;
			}
		}
	}

	// small optimisations.
	if (peerFilter.mAllowedServices.empty())
	{
		peerFilter.mDenyAll = true;
	}
	else
	{
		peerFilter.mDenyAll = false;
		if (peerFilter.mAllowedServices.size() == mOwnServices.size())
		{
			peerFilter.mAllowAll = true;
		}
	}

	// update or remove.
	if (peerFilter.mDenyAll)
	{
		std::cerr << "p3ServiceControl::updateFilterByPeer_locked() Empty(2) ... Clearing";
		std::cerr << std::endl;
	
		if (fit != mPeerFilterMap.end())
		{
			mPeerFilterMap.erase(fit);
		}
	}
	else
	{
		std::cerr << "p3ServiceControl::updateFilterByPeer_locked() Installing PeerFilter";
		std::cerr << std::endl;
		mPeerFilterMap[peerId] = peerFilter;
	}
	recordFilterChanges_locked(peerId, originalFilter, peerFilter);
	return true;
}

void	p3ServiceControl::recordFilterChanges_locked(const RsPeerId &peerId, 
	ServicePeerFilter &originalFilter, ServicePeerFilter &updatedFilter)
{
	std::cerr << "p3ServiceControl::recordFilterChanges_locked()";
	std::cerr << std::endl;
	std::cerr << "PeerId: " << peerId.toStdString();
	std::cerr << std::endl;
	std::cerr << "OriginalFilter: " << originalFilter;
	std::cerr << std::endl;
	std::cerr << "UpdatedFilter: " << updatedFilter;
	std::cerr << std::endl;

	/* find differences */
	std::map<uint32_t, bool> changes;
	std::set<uint32_t>::const_iterator it1, it2, eit1, eit2;
	it1 = originalFilter.mAllowedServices.begin();
	eit1 = originalFilter.mAllowedServices.end();
	it2 = updatedFilter.mAllowedServices.begin();
	eit2 = updatedFilter.mAllowedServices.end();

	while((it1 != eit1) && (it2 != eit2))
	{
		if (*it1 < *it2)
		{
			std::cerr << "Removed Service: " << *it1;
			std::cerr << std::endl;
			// removal
			changes[*it1] = false;
			++it1;
		}
		else if (*it2 < *it1)
		{
			std::cerr << "Added Service: " << *it2;
			std::cerr << std::endl;
			// addition.
			changes[*it2] = true;
			++it2;
		}
		else
		{
			++it1;
			++it2;
		}
	}

	// Handle the unfinished Set.
	for(; it1 != eit1; it1++)
	{
		std::cerr << "Removed Service: " << *it1;
		std::cerr << std::endl;
		// removal
		changes[*it1] = false;
	}

	for(; it2 != eit2; it2++)
	{
		std::cerr << "Added Service: " << *it2;
		std::cerr << std::endl;
		// addition.
		changes[*it2] = true;
	}

	// now we to store for later notifications.
	std::map<uint32_t, bool>::const_iterator cit;
	for(cit = changes.begin(); cit != changes.end(); cit++)
	{
		ServiceNotifications &notes = mNotifications[cit->first];
		if (cit->second)
		{
			notes.mAdded.insert(peerId);
		}
		else
		{
			notes.mRemoved.insert(peerId);
		}
	}
}


// when they go offline, etc.
void	p3ServiceControl::removePeer(const RsPeerId &peerId)
{
	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	std::cerr << "p3ServiceControl::removePeer()";
	std::cerr << std::endl;

	ServicePeerFilter originalFilter;
	bool hadFilter = false;
	{
		std::map<RsPeerId, ServicePeerFilter>::iterator fit;
		fit = mPeerFilterMap.find(peerId);
		if (fit != mPeerFilterMap.end())
		{
			hadFilter = true;
			originalFilter = fit->second;
			mPeerFilterMap.erase(fit);
		}
	}

	{
		std::map<RsPeerId, RsPeerServiceInfo>::iterator sit;
		sit = mServicesProvided.find(peerId);
		if (sit != mServicesProvided.end())
		{
			mServicesProvided.erase(sit);
		}
	}

	if (hadFilter)
	{
		ServicePeerFilter emptyFilter;
		recordFilterChanges_locked(peerId, originalFilter, emptyFilter);
	}
}


/****************************************************************************/
/****************************************************************************/

void	p3ServiceControl::tick()
{
	std::cerr << "p3ServiceControl::tick()";
	std::cerr << std::endl;
}

// configuration.
bool p3ServiceControl::saveList(bool &cleanup, std::list<RsItem *> &saveList)
{
	std::cerr << "p3ServiceControl::saveList()";
	std::cerr << std::endl;

	return true;
}

bool p3ServiceControl::loadList(std::list<RsItem *>& loadList)
{
	std::cerr << "p3ServiceControl::loadList()";
	std::cerr << std::endl;

	return true;
}


/****************************************************************************/
/****************************************************************************/

        // pqiMonitor.
void    p3ServiceControl::statusChange(const std::list<pqipeer> &plist)
{
	std::cerr << "p3ServiceControl::statusChange()";
	std::cerr << std::endl;

        std::list<pqipeer>::const_iterator pit;
        for(pit =  plist.begin(); pit != plist.end(); pit++)
        {
                if (pit->state & RS_PEER_S_FRIEND)
		{
			if (pit->actions & RS_PEER_CONNECTED)
			{
				updatePeerConnect(pit->id);
			}
			else if (pit->actions & RS_PEER_DISCONNECTED)
			{
				updatePeerDisconnect(pit->id);
			}
                }
	}
	return;
}

// Update Peer status.
void    p3ServiceControl::updatePeerConnect(const RsPeerId &peerId)
{
	std::cerr << "p3ServiceControl::updatePeerConnect(): " << peerId.toStdString();
	std::cerr << std::endl;
	return;
}

void    p3ServiceControl::updatePeerDisconnect(const RsPeerId &peerId)
{
	std::cerr << "p3ServiceControl::updatePeerDisconnect(): " << peerId.toStdString();
	std::cerr << std::endl;

	removePeer(peerId);
	return;
}

/****************************************************************************/
/****************************************************************************/

void	p3ServiceControl::notifyServices()
{
	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

        std::map<uint32_t, ServiceNotifications>::const_iterator it;
        std::multimap<uint32_t, pqiServiceMonitor *>::const_iterator sit, eit;
	for(it = mNotifications.begin(); it != mNotifications.end(); it++)
	{
		sit = mMonitors.lower_bound(it->first);
		eit = mMonitors.upper_bound(it->first);
		if (sit == eit)
		{
			/* nothing to notify - skip */
			continue;
		}

		std::list<pqiServicePeer> peers;
		std::set<RsPeerId>::const_iterator pit;
		for(pit = it->second.mAdded.begin(); 
			pit != it->second.mAdded.end(); pit++)
		{
			pqiServicePeer peer;
			peer.id = *pit;
			peer.actions = RS_SERVICE_PEER_CONNECTED;

			peers.push_back(peer);
		}

		for(pit = it->second.mRemoved.begin(); 
			pit != it->second.mRemoved.end(); pit++)
		{
			pqiServicePeer peer;
			peer.id = *pit;
			peer.actions = RS_SERVICE_PEER_REMOVED;

			peers.push_back(peer);
		}

		for(; sit != eit; sit++)
		{
			sit->second->statusChange(peers);
		}
	}
	mNotifications.clear();
}


/****************************************************************************/
/****************************************************************************/


RsServicePermissions::RsServicePermissions()
:mServiceId(0), mServiceName(), mDefaultAllowed(false)
{
	return;
}

bool RsServicePermissions::peerHasPermission(const RsPeerId &peerId) const
{
	std::cerr << "RsServicePermissions::peerHasPermission()";
	std::cerr << std::endl;

	std::set<RsPeerId>::const_iterator it;
	if (mDefaultAllowed)
	{
		it = mPeersDenied.find(peerId);
		return (it == mPeersDenied.end());
	}
	else
	{
		it = mPeersAllowed.find(peerId);
		return (it != mPeersAllowed.end());
	}
}

RsServiceInfo::RsServiceInfo(
                const uint16_t service_type, 
                const std::string service_name, 
                const uint16_t version_major,
                const uint16_t version_minor,
                const uint16_t min_version_major,
                const uint16_t min_version_minor)
 :mServiceName(service_name), 
  mServiceType((((uint32_t) RS_PKT_VERSION_SERVICE) << 24) + (((uint32_t) service_type) << 8)), 
  mVersionMajor(version_major), 
  mVersionMinor(version_minor),
  mMinVersionMajor(min_version_major), 
  mMinVersionMinor(min_version_minor)
{
	return;
}


RsServiceInfo::RsServiceInfo()
 :mServiceName("unknown"), 
  mServiceType(0),
  mVersionMajor(0),
  mVersionMinor(0),
  mMinVersionMajor(0),
  mMinVersionMinor(0)
{
	return;
}

std::ostream &operator<<(std::ostream &out, const RsPeerServiceInfo &info)
{
	out << "RsPeerServiceInfo(" << info.mPeerId << ")";
	out << std::endl;
        std::map<uint32_t, RsServiceInfo>::const_iterator it;
	for(it = info.mServiceList.begin(); it != info.mServiceList.end(); it++)
	{
		out << "\t Service:" << it->first << " : ";
		out << it->second;
		out << std::endl;
	}
	return out;
}


std::ostream &operator<<(std::ostream &out, const RsServiceInfo &info)
{
	out << "RsServiceInfo(" << info.mServiceType << "): " << info.mServiceName;
	out << " Version(" << info.mVersionMajor << "," << info.mVersionMinor << ")";
	out << " MinVersion(" << info.mMinVersionMajor << "," << info.mMinVersionMinor << ")";
	return out;
}

std::ostream &operator<<(std::ostream &out, const ServicePeerFilter &filter)
{
        out << "ServicePeerFilter DenyAll: " << filter.mDenyAll;
	out << " AllowAll: " << filter.mAllowAll;
	out << " Matched Services: ";
        std::set<uint32_t>::const_iterator it;
	for(it = filter.mAllowedServices.begin(); it != filter.mAllowedServices.end(); it++)
	{
         	out << *it << " ";
	}
	out << std::endl;
	return out;
}


