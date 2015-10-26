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

#include <iostream>

#include "p3servicecontrol.h"
#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rsbaseserial.h"
#include "pqi/p3cfgmgr.h"

/*******************************/
// #define SERVICECONTROL_DEBUG	1
/*******************************/

static const uint8_t RS_PKT_SUBTYPE_SERVICE_CONTROL_SERVICE_PERMISSIONS = 0x01 ;

class RsServiceControlItem: public RsItem
{
public:
    RsServiceControlItem(uint8_t item_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_SERVICE_CONTROL,item_subtype) {}

    virtual uint32_t serial_size() const =0;
    virtual bool serialise(uint8_t *data,uint32_t size) const =0;

        bool serialise_header(void *data,uint32_t& pktsize,uint32_t& tlvsize, uint32_t& offset) const
        {
            tlvsize = serial_size() ;
            offset = 0;

            if (pktsize < tlvsize)
                return false; /* not enough space */

            pktsize = tlvsize;

            if(!setRsItemHeader(data, tlvsize, PacketId(), tlvsize))
            {
                std::cerr << "RsFileTransferItem::serialise_header(): ERROR. Not enough size!" << std::endl;
                return false ;
            }
        #ifdef RSSERIAL_DEBUG
            std::cerr << "RsFileItemSerialiser::serialiseData() Header: " << ok << std::endl;
        #endif
            offset += 8;

            return true ;
        }
};
class RsServicePermissionItem: public RsServiceControlItem, public RsServicePermissions
{
public:
    RsServicePermissionItem()
            : RsServiceControlItem(RS_PKT_SUBTYPE_SERVICE_CONTROL_SERVICE_PERMISSIONS) {}
    RsServicePermissionItem(const RsServicePermissions& perms)
            : RsServiceControlItem(RS_PKT_SUBTYPE_SERVICE_CONTROL_SERVICE_PERMISSIONS),
              RsServicePermissions(perms) {}

    virtual uint32_t serial_size() const
    {
        uint32_t s = 8 ; // header

        s += 4 ; // mServiceId
        s += GetTlvStringSize(mServiceName) ;
        s += 1 ; // mDefaultAllowed
        s += 4 ; // mPeersAllowed.size()
        s += mPeersAllowed.size() * RsPeerId::serial_size() ;
        s += 4 ; // mPeersAllowed.size()
        s += mPeersDenied.size() * RsPeerId::serial_size() ;

    return s ;
    }
    virtual bool serialise(uint8_t *data,uint32_t size) const
    {
        uint32_t tlvsize,offset=0;
        bool ok = true;

        if(!serialise_header(data,size,tlvsize,offset))
            return false ;

        /* add mandatory parts first */
        ok &= setRawUInt32(data, tlvsize, &offset, mServiceId);
        ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_NAME, mServiceName);
        ok &= setRawUInt8(data, tlvsize, &offset, mDefaultAllowed);
        ok &= setRawUInt32(data, tlvsize, &offset, mPeersAllowed.size());

        for(std::set<RsPeerId>::const_iterator it(mPeersAllowed.begin());it!=mPeersAllowed.end();++it)
            (*it).serialise(data,tlvsize,offset) ;

        ok &= setRawUInt32(data, tlvsize, &offset, mPeersDenied.size());

        for(std::set<RsPeerId>::const_iterator it(mPeersDenied.begin());it!=mPeersDenied.end();++it)
            (*it).serialise(data,tlvsize,offset) ;

        if(offset != tlvsize)
        {
            std::cerr << "RsGxsChannelSerialiser::serialiseGxsChannelGroupItem() FAIL Size Error! " << std::endl;
            ok = false;
        }

        if (!ok)
            std::cerr << "RsGxsChannelSerialiser::serialiseGxsChannelGroupItem() NOK" << std::endl;

        return ok;
    }

    static RsServicePermissionItem *deserialise(uint8_t *data,uint32_t size)
    {
        RsServicePermissionItem *item = new RsServicePermissionItem ;

        uint32_t offset = 8; // skip the header
        uint32_t rssize = getRsItemSize(data);
        bool ok = true ;

        /* add mandatory parts first */
        ok &= getRawUInt32(data, rssize, &offset, &item->mServiceId);
        ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_NAME, item->mServiceName);

    uint8_t v ;
        ok &= getRawUInt8(data, rssize, &offset, &v) ;

    if(v != 0 && v != 1)
        ok = false ;
    else
        item->mDefaultAllowed = (bool)v;

        uint32_t tmp ;
        ok &= getRawUInt32(data, rssize, &offset, &tmp);

        for(uint32_t i=0;i<tmp && offset < rssize;++i)
        {
            RsPeerId peer_id ;
            ok &= peer_id.deserialise(data,rssize,offset) ;
            item->mPeersAllowed.insert(peer_id) ;
        }

        ok &= getRawUInt32(data, rssize, &offset, &tmp);

        for(uint32_t i=0;i<tmp && offset < rssize;++i)
        {
            RsPeerId peer_id ;
            ok &= peer_id.deserialise(data,rssize,offset) ;
            item->mPeersDenied.insert(peer_id) ;
        }
        if (offset != rssize || !ok)
        {
            std::cerr << __PRETTY_FUNCTION__ << ": error while deserialising! Item will be dropped." << std::endl;
            delete(item);
            return NULL ;
        }

        return item;
    }

    virtual void clear() {}
    virtual std::ostream& print(std::ostream& out,uint16_t)
    {
        std::cerr << __PRETTY_FUNCTION__ << ": not implemented!" << std::endl;
    return out ;
    }
};

class ServiceControlSerialiser: public RsSerialType
{
public:
    ServiceControlSerialiser() : RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_SERVICE_CONTROL) {}

    virtual uint32_t size (RsItem *item)
    {
        RsServiceControlItem *scitem = dynamic_cast<RsServiceControlItem *>(item);
        if (!scitem)
        {
            return 0;
        }
        return scitem->serial_size() ;
    }
    virtual bool serialise(RsItem *item, void *data, uint32_t *size)
    {
        RsServiceControlItem *scitem = dynamic_cast<RsServiceControlItem *>(item);
        if (!scitem)
        {
            return false;
        }
        return scitem->serialise((uint8_t*)data,*size) ;
    }
    virtual RsItem *deserialise (void *data, uint32_t *size)
    {
                uint32_t rstype = getRsItemId(data);

                if(RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype) || RS_SERVICE_TYPE_SERVICE_CONTROL != getRsItemService(rstype))  { return NULL; /* wrong type */ }

                switch(getRsItemSubType(rstype))
        {
        case RS_PKT_SUBTYPE_SERVICE_CONTROL_SERVICE_PERMISSIONS:return RsServicePermissionItem::deserialise((uint8_t*)data, *size);
        default:
            return NULL ;
        }
    }
};

RsServiceControl *rsServiceControl = NULL;

p3ServiceControl::p3ServiceControl(p3LinkMgr *linkMgr)
    :RsServiceControl(), p3Config(),
          mLinkMgr(linkMgr), mOwnPeerId(linkMgr->getOwnId()),
	mCtrlMtx("p3ServiceControl"), mMonitorMtx("P3ServiceControl::Monitor")
{
    mSerialiser = new ServiceControlSerialiser ;
}

RsSerialiser *p3ServiceControl::setupSerialiser()
{
    RsSerialiser *serial = new RsSerialiser;
    serial->addSerialType(new ServiceControlSerialiser) ;

    return serial ;
}

const 	RsPeerId& p3ServiceControl::getOwnId()
{
	return mOwnPeerId;
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

#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::registerService() Registered ServiceID: " << info.mServiceType;
	std::cerr << std::endl;
	std::cerr << "p3ServiceControl::registerService() ServiceName: " << info.mServiceName;
	std::cerr << std::endl;
#endif


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

#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::deregisterService() Removed ServiceID: " << serviceId;
	std::cerr << std::endl;
#endif
	mOwnServices.erase(it);
	return true;
}


/* Interface for Services */
bool p3ServiceControl::registerServiceMonitor(pqiServiceMonitor *monitor, uint32_t serviceId)
{
	RsStackMutex stack(mMonitorMtx); /***** LOCK STACK MUTEX ****/

#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::registerServiceMonitor() for ServiceId: ";
	std::cerr << serviceId;
	std::cerr << std::endl;
#endif

	mMonitors.insert(std::make_pair(serviceId, monitor));
	return true;
}


bool p3ServiceControl::deregisterServiceMonitor(pqiServiceMonitor *monitor)
{
	RsStackMutex stack(mMonitorMtx); /***** LOCK STACK MUTEX ****/

#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::deregisterServiceMonitor()";
	std::cerr << std::endl;
#endif

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
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::getServiceChanges()";
	std::cerr << std::endl;
#endif

	std::set<RsPeerId>::iterator it;
	for (it = mUpdatedSet.begin(); it != mUpdatedSet.end(); ++it)
	{
		updateSet.insert(*it);
	}

	mUpdatedSet.clear();
}


bool p3ServiceControl::getOwnServices(RsPeerServiceInfo &info)
{
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::getOwnServices()";
	std::cerr << std::endl;
#endif

	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	info.mPeerId.clear() ;
	info.mServiceList = mOwnServices;
	return true;
}


bool p3ServiceControl::getServicesAllowed(const RsPeerId &peerId, RsPeerServiceInfo &info)
{
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::getServicesAllowed(" << peerId.toStdString() << ")";
	std::cerr << std::endl;
#endif

	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	info.mPeerId = peerId;

	// For each registered Service.. check if peer has permissions.
	std::map<uint32_t, RsServiceInfo>::iterator it;
	for(it = mOwnServices.begin(); it != mOwnServices.end(); ++it)
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
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::peerHasPermissionForService_locked()";
	std::cerr << std::endl;
#endif

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

#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::getServicesProvided()";
	std::cerr << std::endl;
#endif

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

#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::updateServicesProvided() from: " << peerId.toStdString();
	std::cerr << std::endl;
	std::cerr << info;
	std::cerr << std::endl;
#endif

	mServicesProvided[peerId] = info;
    updateFilterByPeer_locked(peerId);

    IndicateConfigChanged() ;
    return true;
}

/* External Interface to RsServiceControl */
bool p3ServiceControl::getServicePermissions(uint32_t serviceId, RsServicePermissions &permissions)
{
	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::getServicePermissions()";
	std::cerr << std::endl;
#endif

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
        IndicateConfigChanged() ;
        return true;
	}
	return false;
}


bool p3ServiceControl::updateServicePermissions(uint32_t serviceId, const RsServicePermissions &permissions)
{
	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::updateServicePermissions()";
	std::cerr << std::endl;
#endif

	std::map<uint32_t, RsServicePermissions>::iterator it;
	it = mServicePermissionMap.find(serviceId);
	if (it == mServicePermissionMap.end())
	{
		std::cerr << "p3ServiceControl::updateServicePermissions()";
		std::cerr << " ERROR missing previous permissions";
		std::cerr << std::endl;
		// ERROR.
		return false;
	}

	std::list<RsPeerId> onlinePeers;
	mLinkMgr->getOnlineList(onlinePeers);
	std::list<RsPeerId>::const_iterator pit;
	if (it != mServicePermissionMap.end())
	{
		for(pit = onlinePeers.begin(); pit != onlinePeers.end(); ++pit)
		{
			if (it->second.peerHasPermission(*pit) != 
				permissions.peerHasPermission(*pit))
			{
				mUpdatedSet.insert(*pit);
			}
		}
	}

 	it->second = permissions;
	it->second.mServiceId = serviceId; // just to make sure!

	// This is overkill - but will update everything.
	updateAllFilters_locked();
    IndicateConfigChanged() ;
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

#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::checkFilter() ";
	std::cerr << " ServiceId: " << serviceId;
#endif

	std::map<uint32_t, RsServiceInfo>::iterator it;
	it = mOwnServices.find(serviceId);
	if (it != mOwnServices.end())
	{
#ifdef SERVICECONTROL_DEBUG
		std::cerr << " ServiceName: " << it->second.mServiceName;
#endif
	}
	else
	{
#ifdef SERVICECONTROL_DEBUG
		std::cerr << " ServiceName: Unknown! ";
#endif
	}

#ifdef SERVICECONTROL_DEBUG
	std::cerr << " PeerId: " << peerId.toStdString();
	std::cerr << std::endl;
#endif

	// must allow ServiceInfo through, or we have nothing!
#define FULLID_SERVICEINFO ((((uint32_t) RS_PKT_VERSION_SERVICE) << 24) + ((RS_SERVICE_TYPE_SERVICEINFO) << 8)) 

	//if (serviceId == RS_SERVICE_TYPE_SERVICEINFO)
	if (serviceId == FULLID_SERVICEINFO)
	{
#ifdef SERVICECONTROL_DEBUG
		std::cerr << "p3ServiceControl::checkFilter() Allowed SERVICEINFO";
		std::cerr << std::endl;
#endif
		return true;
	}


	std::map<RsPeerId, ServicePeerFilter>::const_iterator pit;
	pit = mPeerFilterMap.find(peerId);
	if (pit == mPeerFilterMap.end())
	{
#ifdef SERVICECONTROL_DEBUG
		std::cerr << "p3ServiceControl::checkFilter() Denied No PeerId";
		std::cerr << std::endl;
#endif
		return false;
	}

	if (pit->second.mDenyAll)
	{
#ifdef SERVICECONTROL_DEBUG
		std::cerr << "p3ServiceControl::checkFilter() Denied Peer.DenyAll";
		std::cerr << std::endl;
#endif
		return false;
	}

	if (pit->second.mAllowAll)
	{
#ifdef SERVICECONTROL_DEBUG
		std::cerr << "p3ServiceControl::checkFilter() Allowed Peer.AllowAll";
		std::cerr << std::endl;
#endif
		return true;
	}

	std::set<uint32_t>::const_iterator sit;
	sit = pit->second.mAllowedServices.find(serviceId);
	if (sit == pit->second.mAllowedServices.end())
	{
#ifdef SERVICECONTROL_DEBUG
		std::cerr << "p3ServiceControl::checkFilter() Denied !Peer.find(serviceId)";
		std::cerr << std::endl;
#endif
		return false;
	}
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::checkFilter() Allowed Peer.find(serviceId)";
	std::cerr << std::endl;
#endif
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
#ifdef SERVICECONTROL_DEBUG
		std::cerr << "servicesCompatible: Type/Name mismatch";
		std::cerr << std::endl;
		std::cerr << "Info1 ID: " << info1.mServiceType;
		std::cerr << " " << info1.mServiceName;
		std::cerr << std::endl;
		std::cerr << "Info2 ID: " << info2.mServiceType;
		std::cerr << " " << info2.mServiceName;
		std::cerr << std::endl;
#endif
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
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::updateFilterByPeer()";
	std::cerr << std::endl;
#endif

	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/
	return updateFilterByPeer_locked(peerId);
}


bool	p3ServiceControl::updateAllFilters()
{
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::updateAllFilters()";
	std::cerr << std::endl;
#endif

	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	return updateAllFilters_locked();
}


bool	p3ServiceControl::updateAllFilters_locked()
{
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::updateAllFilters_locked()";
	std::cerr << std::endl;
#endif

	// Create a peerSet from ServicesProvided + PeerFilters.
	// This will completely refresh the Filters.
	std::set<RsPeerId> peerSet;
	std::set<RsPeerId>::const_iterator pit;

	std::map<RsPeerId, RsPeerServiceInfo>::const_iterator it;
	for(it = mServicesProvided.begin(); it != mServicesProvided.end(); ++it)
	{
		peerSet.insert(it->first);
	}

	std::map<RsPeerId, ServicePeerFilter>::const_iterator fit;
	for(fit = mPeerFilterMap.begin(); fit != mPeerFilterMap.end(); ++fit)
	{
		peerSet.insert(fit->first);
	}

	for(pit = peerSet.begin(); pit != peerSet.end(); ++pit)
	{
		updateFilterByPeer_locked(*pit);
	}
	return true;
}


// create filter. (the easy way).
bool	p3ServiceControl::updateFilterByPeer_locked(const RsPeerId &peerId)
{
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::updateFilterByPeer_locked() : " << peerId.toStdString();
	std::cerr << std::endl;
#endif

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
#ifdef SERVICECONTROL_DEBUG
		std::cerr << "p3ServiceControl::updateFilterByPeer_locked() Empty ... Clearing";
		std::cerr << std::endl;
#endif
	
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

#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::updateFilterByPeer_locked() Comparing lists";
	std::cerr << std::endl;
#endif


	while((oit != eoit) && (tit != etit))
	{
		if (oit->first == tit->first)
		{
#ifdef SERVICECONTROL_DEBUG
			std::cerr << "\tChecking Matching Service ID: " << oit->first;
			std::cerr << std::endl;
#endif
			/* match of service IDs */
			/* check if compatible */
			if (ServiceInfoCompatible(oit->second, tit->second))
			{
				if (peerHasPermissionForService_locked(peerId, oit->first))
				{
#ifdef SERVICECONTROL_DEBUG
					std::cerr << "\t\tMatched Service ID: " << oit->first;
					std::cerr << std::endl;
#endif
					peerFilter.mAllowedServices.insert(oit->first);
				}
			}
			++oit;
			++tit;
		}
		else
		{
			if (oit->first < tit->first)
			{
#ifdef SERVICECONTROL_DEBUG
				std::cerr << "\tSkipping Only Own Service ID: " << oit->first;
				std::cerr << std::endl;
#endif
				++oit;
			}
			else
			{
#ifdef SERVICECONTROL_DEBUG
				std::cerr << "\tSkipping Only Peer Service ID: " << tit->first;
				std::cerr << std::endl;
#endif
				++tit;
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
#ifdef SERVICECONTROL_DEBUG
		std::cerr << "p3ServiceControl::updateFilterByPeer_locked() Empty(2) ... Clearing";
		std::cerr << std::endl;
#endif
	
		if (fit != mPeerFilterMap.end())
		{
			mPeerFilterMap.erase(fit);
		}
	}
	else
	{
#ifdef SERVICECONTROL_DEBUG
		std::cerr << "p3ServiceControl::updateFilterByPeer_locked() Installing PeerFilter";
		std::cerr << std::endl;
#endif
		mPeerFilterMap[peerId] = peerFilter;
	}
	recordFilterChanges_locked(peerId, originalFilter, peerFilter);
	return true;
}

void	p3ServiceControl::recordFilterChanges_locked(const RsPeerId &peerId, 
	ServicePeerFilter &originalFilter, ServicePeerFilter &updatedFilter)
{
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::recordFilterChanges_locked()";
	std::cerr << std::endl;
	std::cerr << "PeerId: " << peerId.toStdString();
	std::cerr << std::endl;
	std::cerr << "OriginalFilter: " << originalFilter;
	std::cerr << std::endl;
	std::cerr << "UpdatedFilter: " << updatedFilter;
	std::cerr << std::endl;
#endif

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
#ifdef SERVICECONTROL_DEBUG
			std::cerr << "Removed Service: " << *it1;
			std::cerr << std::endl;
#endif
			// removal
			changes[*it1] = false;
			filterChangeRemoved_locked(peerId, *it1);
			++it1;
		}
		else if (*it2 < *it1)
		{
#ifdef SERVICECONTROL_DEBUG
			std::cerr << "Added Service: " << *it2;
			std::cerr << std::endl;
#endif
			// addition.
			filterChangeAdded_locked(peerId, *it2);
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
	for(; it1 != eit1; ++it1)
	{
#ifdef SERVICECONTROL_DEBUG
		std::cerr << "Removed Service: " << *it1;
		std::cerr << std::endl;
#endif
		// removal
		changes[*it1] = false;
		filterChangeRemoved_locked(peerId, *it1);
	}

	for(; it2 != eit2; ++it2)
	{
#ifdef SERVICECONTROL_DEBUG
		std::cerr << "Added Service: " << *it2;
		std::cerr << std::endl;
#endif
		// addition.
		changes[*it2] = true;
		filterChangeAdded_locked(peerId, *it2);
	}

	// Can remove changes map... as only used below.
#if 0
	// now we to store for later notifications.
	std::map<uint32_t, bool>::const_iterator cit;
	for(cit = changes.begin(); cit != changes.end(); ++cit)
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
#endif

}


// when they go offline, etc.
void	p3ServiceControl::removePeer(const RsPeerId &peerId)
{
	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::removePeer() : " << peerId.toStdString();
	std::cerr << std::endl;
#endif

	ServicePeerFilter originalFilter;
	bool hadFilter = false;
	{
		std::map<RsPeerId, ServicePeerFilter>::iterator fit;
		fit = mPeerFilterMap.find(peerId);
		if (fit != mPeerFilterMap.end())
		{
#ifdef SERVICECONTROL_DEBUG
			std::cerr << "p3ServiceControl::removePeer() clearing mPeerFilterMap";
			std::cerr << std::endl;
#endif

			hadFilter = true;
			originalFilter = fit->second;
			mPeerFilterMap.erase(fit);
		}
		else
		{
#ifdef SERVICECONTROL_DEBUG
			std::cerr << "p3ServiceControl::removePeer() Nothing in mPeerFilterMap";
			std::cerr << std::endl;
#endif
		}
	}

	{
		std::map<RsPeerId, RsPeerServiceInfo>::iterator sit;
		sit = mServicesProvided.find(peerId);
		if (sit != mServicesProvided.end())
		{
#ifdef SERVICECONTROL_DEBUG
			std::cerr << "p3ServiceControl::removePeer() clearing mServicesProvided";
			std::cerr << std::endl;
#endif

			mServicesProvided.erase(sit);
		}
		else
		{
#ifdef SERVICECONTROL_DEBUG
			std::cerr << "p3ServiceControl::removePeer() Nothing in mServicesProvided";
			std::cerr << std::endl;
#endif
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
// need to provide list of connected peers per service.
// these are collected here.

void p3ServiceControl::filterChangeRemoved_locked(const RsPeerId &peerId, uint32_t serviceId)
{
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::filterChangeRemoved_locked(" << peerId.toStdString();
	std::cerr << ", " << serviceId << ")";
	std::cerr << std::endl;
#endif

	std::map<uint32_t, std::set<RsPeerId> >::iterator mit;

	std::set<RsPeerId> &peerSet = mServicePeerMap[serviceId];
	std::set<RsPeerId>::iterator sit;

	sit = peerSet.find(peerId);
	if (sit != peerSet.end())
	{
		peerSet.erase(sit);
	}
	else
	{
		// ERROR
		std::cerr << "p3ServiceControl::filterChangeRemoved_locked() ERROR NOT FOUND";
		std::cerr << std::endl;
	}

	// Add to Notifications too.
	ServiceNotifications &notes = mNotifications[serviceId];
	notes.mRemoved.insert(peerId);
}


void p3ServiceControl::filterChangeAdded_locked(const RsPeerId &peerId, uint32_t serviceId)
{
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::filterChangeAdded_locked(" << peerId.toStdString();
	std::cerr << ", " << serviceId << ")";
	std::cerr << std::endl;
#endif

	std::map<uint32_t, std::set<RsPeerId> >::iterator mit;

	std::set<RsPeerId> &peerSet = mServicePeerMap[serviceId];

	// This bit is only for error checking.
	std::set<RsPeerId>::iterator sit = peerSet.find(peerId);
	if (sit != peerSet.end())
	{
		// ERROR.
		std::cerr << "p3ServiceControl::filterChangeAdded_locked() ERROR NOT FOUND";
		std::cerr << std::endl;
	}
	peerSet.insert(peerId);

	// Add to Notifications too.
	ServiceNotifications &notes = mNotifications[serviceId];
	notes.mAdded.insert(peerId);
}



void p3ServiceControl::getPeersConnected(const uint32_t serviceId, std::set<RsPeerId> &peerSet)
{
	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	std::map<uint32_t, std::set<RsPeerId> >::iterator mit;
	mit = mServicePeerMap.find(serviceId);
	if (mit != mServicePeerMap.end())
	{
		peerSet = mit->second;
	}
	else
	{
		peerSet.clear();
	}
}


bool p3ServiceControl::isPeerConnected(const uint32_t serviceId, const RsPeerId &peerId)
{
	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	std::map<uint32_t, std::set<RsPeerId> >::iterator mit;
	mit = mServicePeerMap.find(serviceId);
	if (mit != mServicePeerMap.end())
	{
		std::set<RsPeerId>::iterator sit;
		sit = mit->second.find(peerId);
		return (sit != mit->second.end());
	}

	return false;
}


/****************************************************************************/
/****************************************************************************/

void	p3ServiceControl::tick()
{
	notifyAboutFriends();
	notifyServices();

#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::tick()";
	std::cerr << std::endl;
#endif
}

// configuration.
bool p3ServiceControl::saveList(bool &cleanup, std::list<RsItem *> &saveList)
{
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::saveList()";
	std::cerr << std::endl;
#endif
    cleanup = true ;

    for(std::map<uint32_t, RsServicePermissions>::iterator it(mServicePermissionMap.begin());it!=mServicePermissionMap.end();++it)
    {
        RsServicePermissionItem *item = new RsServicePermissionItem(it->second) ;
        saveList.push_back(item) ;
    }
	return true;
}

bool p3ServiceControl::loadList(std::list<RsItem *>& loadList)
{
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::loadList()";
	std::cerr << std::endl;
#endif
    for(std::list<RsItem*>::const_iterator it(loadList.begin());it!=loadList.end();++it)
    {
        RsServicePermissionItem *item = dynamic_cast<RsServicePermissionItem*>(*it) ;

        if(item == NULL)
            continue ;

        mServicePermissionMap[item->mServiceId] = *item ;
    }

	return true;
}


/****************************************************************************/
/****************************************************************************/

	// pqiMonitor.
void    p3ServiceControl::statusChange(const std::list<pqipeer> &plist)
{
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::statusChange()";
	std::cerr << std::endl;
#endif

	std::list<pqipeer>::const_iterator pit;
	for(pit =  plist.begin(); pit != plist.end(); ++pit)
	{
#ifdef SERVICECONTROL_DEBUG
		std::cerr << "p3ServiceControl::statusChange() for peer: ";
		std::cerr << " peer: " << (pit->id).toStdString();
		std::cerr << " state: " << pit->state;
		std::cerr << " actions: " << pit->actions;
		std::cerr << std::endl;
#endif
		if (pit->state & RS_PEER_S_FRIEND)
		{
			// Connected / Disconnected. (interal actions).
			if (pit->actions & RS_PEER_CONNECTED)
			{
				updatePeerConnect(pit->id);
			}
			else if (pit->actions & RS_PEER_DISCONNECTED)
			{
				updatePeerDisconnect(pit->id);
			}

			// Added / Removed. (pass on notifications).
			if (pit->actions & RS_PEER_NEW)
			{
				updatePeerNew(pit->id);
			}
		}
		else
		{
			if (pit->actions & RS_PEER_MOVED)
			{
				updatePeerRemoved(pit->id);
			}
		}
	}
	return;
}

// Update Peer status.
void    p3ServiceControl::updatePeerConnect(const RsPeerId &peerId)
{
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::updatePeerConnect(): " << peerId.toStdString();
	std::cerr << std::endl;
#endif
	return;
}

void    p3ServiceControl::updatePeerDisconnect(const RsPeerId &peerId)
{
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::updatePeerDisconnect(): " << peerId.toStdString();
	std::cerr << std::endl;
#endif

	removePeer(peerId);
	return;
}


// Update Peer status.
void    p3ServiceControl::updatePeerNew(const RsPeerId &peerId)
{
	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::updatePeerNew(): " << peerId.toStdString();
	std::cerr << std::endl;
#endif

	pqiServicePeer peer;
	peer.id = peerId;
	peer.actions = RS_SERVICE_PEER_NEW;
	mFriendNotifications.push_back(peer);

	return;
}

void    p3ServiceControl::updatePeerRemoved(const RsPeerId &peerId)
{
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "p3ServiceControl::updatePeerRemoved(): " << peerId.toStdString();
	std::cerr << std::endl;
#endif

	removePeer(peerId);

	RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

	pqiServicePeer peer;
	peer.id = peerId;
	peer.actions = RS_SERVICE_PEER_REMOVED;
	mFriendNotifications.push_back(peer);

	return;
}

/****************************************************************************/
/****************************************************************************/


void	p3ServiceControl::notifyAboutFriends()
{
	std::list<pqiServicePeer> friendNotifications;
	{
		RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

		if (mFriendNotifications.empty())
		{
			return;
		}
#ifdef SERVICECONTROL_DEBUG
		std::cerr << "p3ServiceControl::notifyAboutFriends(): Something has changed!";
		std::cerr << std::endl;
#endif

		mFriendNotifications.swap(friendNotifications);
	}

	{
		RsStackMutex stack(mMonitorMtx); /***** LOCK STACK MUTEX ****/

		std::multimap<uint32_t, pqiServiceMonitor *>::const_iterator sit;
		for(sit = mMonitors.begin(); sit != mMonitors.end(); ++sit)
		{
			sit->second->statusChange(friendNotifications);
		}
	}
}


void	p3ServiceControl::notifyServices()
{
	std::map<uint32_t, ServiceNotifications> notifications;
	{
		RsStackMutex stack(mCtrlMtx); /***** LOCK STACK MUTEX ****/

		if (mNotifications.empty())
		{
			return;
		}

#ifdef SERVICECONTROL_DEBUG
		std::cerr << "p3ServiceControl::notifyServices()";
		std::cerr << std::endl;
#endif

		mNotifications.swap(notifications);
	}

	{
		RsStackMutex stack(mMonitorMtx); /***** LOCK STACK MUTEX ****/

		std::map<uint32_t, ServiceNotifications>::const_iterator it;
		std::multimap<uint32_t, pqiServiceMonitor *>::const_iterator sit, eit;
		for(it = notifications.begin(); it != notifications.end(); ++it)
		{
#ifdef SERVICECONTROL_DEBUG
			std::cerr << "p3ServiceControl::notifyServices(): Notifications for Service: " << it->first;
			std::cerr << std::endl;
#endif

			sit = mMonitors.lower_bound(it->first);
			eit = mMonitors.upper_bound(it->first);
			if (sit == eit)
			{
				/* nothing to notify - skip */
#ifdef SERVICECONTROL_DEBUG
				std::cerr << "p3ServiceControl::notifyServices(): Noone Monitoring ... skipping";
				std::cerr << std::endl;
#endif
	
				continue;
			}
	
			std::list<pqiServicePeer> peers;
			std::set<RsPeerId>::const_iterator pit;
			for(pit = it->second.mAdded.begin(); 
				pit != it->second.mAdded.end(); ++pit)
			{
				pqiServicePeer peer;
				peer.id = *pit;
				peer.actions = RS_SERVICE_PEER_CONNECTED;
	
				peers.push_back(peer);
	
#ifdef SERVICECONTROL_DEBUG
				std::cerr << "p3ServiceControl::notifyServices(): Peer: " << *pit << " CONNECTED";
				std::cerr << std::endl;
#endif
			}
	
			for(pit = it->second.mRemoved.begin(); 
				pit != it->second.mRemoved.end(); ++pit)
			{
				pqiServicePeer peer;
				peer.id = *pit;
				peer.actions = RS_SERVICE_PEER_DISCONNECTED;
	
				peers.push_back(peer);
	
#ifdef SERVICECONTROL_DEBUG
				std::cerr << "p3ServiceControl::notifyServices(): Peer: " << *pit << " DISCONNECTED";
				std::cerr << std::endl;
#endif
			}
	
			for(; sit != eit; ++sit)
			{
#ifdef SERVICECONTROL_DEBUG
				std::cerr << "p3ServiceControl::notifyServices(): Sending to Monitoring Service";
				std::cerr << std::endl;
#endif
	
				sit->second->statusChange(peers);
			}
		}
	}
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
#ifdef SERVICECONTROL_DEBUG
	std::cerr << "RsServicePermissions::peerHasPermission()";
	std::cerr << std::endl;
#endif

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

void RsServicePermissions::setPermission(const RsPeerId& peerId)
{
    std::set<RsPeerId>::const_iterator it;
    if (mDefaultAllowed)
    {
        it = mPeersDenied.find(peerId);
        mPeersDenied.erase(it) ;
    }
    else
        mPeersAllowed.insert(peerId);
}
void RsServicePermissions::resetPermission(const RsPeerId& peerId)
{
    std::set<RsPeerId>::const_iterator it;
    if (!mDefaultAllowed)
    {
        it = mPeersAllowed.find(peerId);
        mPeersAllowed.erase(it) ;
    }
    else
        mPeersDenied.insert(peerId);
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
	for(it = info.mServiceList.begin(); it != info.mServiceList.end(); ++it)
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
	for(it = filter.mAllowedServices.begin(); it != filter.mAllowedServices.end(); ++it)
	{
	 	out << *it << " ";
	}
	out << std::endl;
	return out;
}


