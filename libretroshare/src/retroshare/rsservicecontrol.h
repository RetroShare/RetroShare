#ifndef RETROSHARE_SERVICE_CONTROL_GUI_INTERFACE_H
#define RETROSHARE_SERVICE_CONTROL_GUI_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsservicecontrol.h
 *
 * RetroShare C++ Interface.
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

#include <inttypes.h>
#include <string>
#include <list>
#include <map>
#include <set>
#include <retroshare/rstypes.h>

/* The Main Interface Class - for information about your Peers */
class RsServiceControl;
extern RsServiceControl *rsServiceControl;

class RsServiceInfo
{
	public:
	RsServiceInfo();
	RsServiceInfo(
		const uint16_t service_type, 
		const std::string service_name, 
		const uint16_t version_major,
		const uint16_t version_minor,
		const uint16_t min_version_major,
		const uint16_t min_version_minor);

	std::string mServiceName;
	uint32_t    mServiceType;
	// current version, we running.
	uint16_t    mVersionMajor;
	uint16_t    mVersionMinor;
	// minimum version can communicate with.
	uint16_t    mMinVersionMajor;
	uint16_t    mMinVersionMinor;
};


bool ServiceInfoCompatible(const RsServiceInfo &info1, const RsServiceInfo &info2);



/* this is what is transmitted to peers */
class RsPeerServiceInfo
{
	public:
	RsPeerServiceInfo()
	:mPeerId(), mServiceList() { return; }

        RsPeerId mPeerId;
	std::map<uint32_t, RsServiceInfo> mServiceList;
};

std::ostream &operator<<(std::ostream &out, const RsPeerServiceInfo &info);
std::ostream &operator<<(std::ostream &out, const RsServiceInfo &info);


class RsServicePermissions
{
public:
    RsServicePermissions();

    bool peerHasPermission(const RsPeerId &peerId) const;

    void setPermission(const RsPeerId& peerId) ;
    void resetPermission(const RsPeerId& peerId) ;

    uint32_t mServiceId;
    std::string mServiceName;

    bool mDefaultAllowed;
    // note only one of these is checked.
    // if DefaultAllowed = true,  then only PeersDenied  is checked.
    // if DefaultAllowed = false, then only PeersAllowed is checked.
    std::set<RsPeerId> mPeersAllowed;
    std::set<RsPeerId> mPeersDenied;
};

class RsServiceControl
{
	public:

	RsServiceControl()  { return; }
virtual ~RsServiceControl() { return; }

virtual bool getOwnServices(RsPeerServiceInfo &info) = 0;
virtual std::string getServiceName(uint32_t service_id) = 0;

virtual bool getServicesAllowed(const RsPeerId &peerId, RsPeerServiceInfo &info) = 0;
virtual bool getServicesProvided(const RsPeerId &peerId, RsPeerServiceInfo &info) = 0;
virtual bool getServicePermissions(uint32_t serviceId, RsServicePermissions &permissions) = 0;
virtual bool updateServicePermissions(uint32_t serviceId, const RsServicePermissions &permissions) = 0;

virtual void getPeersConnected(const uint32_t serviceId, std::set<RsPeerId> &peerSet) = 0;

};

#endif
