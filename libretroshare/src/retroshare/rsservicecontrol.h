/*******************************************************************************
 * libretroshare/src/retroshare: rsservicecontrol.h                            *
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
#ifndef RETROSHARE_SERVICE_CONTROL_GUI_INTERFACE_H
#define RETROSHARE_SERVICE_CONTROL_GUI_INTERFACE_H

#include <inttypes.h>
#include <string>
#include <list>
#include <map>
#include <set>
#include <retroshare/rstypes.h>

/* The Main Interface Class - for information about your Peers */
class RsServiceControl;

/**
 * Pointer to global instance of RsServiceControl service implementation
 * @jsonapi{development}
 */
extern RsServiceControl* rsServiceControl;

struct RsServiceInfo : RsSerializable
{
	RsServiceInfo();
	RsServiceInfo(
		const uint16_t service_type, 
		const std::string& service_name,
		const uint16_t version_major,
		const uint16_t version_minor,
		const uint16_t min_version_major,
		const uint16_t min_version_minor);

	static unsigned int RsServiceInfoUIn16ToFullServiceId(uint16_t serviceType);

	std::string mServiceName;
	uint32_t    mServiceType;
	// current version, we running.
	uint16_t    mVersionMajor;
	uint16_t    mVersionMinor;
	// minimum version can communicate with.
	uint16_t    mMinVersionMajor;
	uint16_t    mMinVersionMinor;

	// RsSerializable interface
	void serial_process(RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext &ctx) {
		RS_SERIAL_PROCESS(mServiceName);
		RS_SERIAL_PROCESS(mServiceType);
		RS_SERIAL_PROCESS(mVersionMajor);
		RS_SERIAL_PROCESS(mVersionMinor);
		RS_SERIAL_PROCESS(mMinVersionMajor);
		RS_SERIAL_PROCESS(mMinVersionMinor);
	}
};

bool ServiceInfoCompatible(const RsServiceInfo &info1, const RsServiceInfo &info2);

/* this is what is transmitted to peers */
struct RsPeerServiceInfo : RsSerializable
{
	RsPeerServiceInfo() : mPeerId(), mServiceList() {}

	RsPeerId mPeerId;
	std::map<uint32_t, RsServiceInfo> mServiceList;

	// RsSerializable interface
	void serial_process(RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext &ctx) {
		RS_SERIAL_PROCESS(mPeerId);
		RS_SERIAL_PROCESS(mServiceList);
	}
};

struct RsServicePermissions : RsSerializable
{
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

	// RsSerializable interface
	void serial_process(RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext &ctx) {
		RS_SERIAL_PROCESS(mServiceId);
		RS_SERIAL_PROCESS(mServiceName);

		RS_SERIAL_PROCESS(mDefaultAllowed);

		RS_SERIAL_PROCESS(mPeersAllowed);
		RS_SERIAL_PROCESS(mPeersDenied);
	}
};

class RsServiceControl
{
public:

	RsServiceControl() {}
	virtual ~RsServiceControl(){}

	/**
	 * @brief get a map off all services.
	 * @jsonapi{development}
	 * @param[out] info storage for service information
	 * @return always true
	 */
	virtual bool getOwnServices(RsPeerServiceInfo &info) = 0;

	/**
	 * @brief getServiceName lookup the name of a service.
	 * @jsonapi{development}
	 * @param[in] serviceId service to look up
	 * @return name of service
	 */
	virtual std::string getServiceName(uint32_t serviceId) = 0;

	/**
	 * @brief getServiceItemNames return a map of service item names.
	 * @jsonapi{development}
	 * @param[in] serviceId service to look up
	 * @param[out] names names of items
	 * @return true on success false otherwise
	 */
	virtual bool getServiceItemNames(uint32_t serviceId, std::map<uint8_t,std::string>& names) = 0;

	/**
	 * @brief getServicesAllowed return a mpa with allowed service information.
	 * @jsonapi{development}
	 * @param[in] peerId peer to look up
	 * @param[out] info map with infomration
	 * @return always true
	 */
	virtual bool getServicesAllowed(const RsPeerId &peerId, RsPeerServiceInfo &info) = 0;

	/**
	 * @brief getServicesProvided return services provided by a peer.
	 * @jsonapi{development}
	 * @param[in] peerId peer to look up
	 * @param[out] info
	 * @return true on success false otherwise.
	 */
	virtual bool getServicesProvided(const RsPeerId &peerId, RsPeerServiceInfo &info) = 0;

	/**
	 * @brief getServicePermissions return permissions of one service.
	 * @jsonapi{development}
	 * @param[in] serviceId service id to look up
	 * @param[out] permissions
	 * @return true on success false otherwise.
	 */
	virtual bool getServicePermissions(uint32_t serviceId, RsServicePermissions &permissions) = 0;

	/**
	 * @brief updateServicePermissions update service permissions of one service.
	 * @jsonapi{development}
	 * @param[in] serviceId service to update
	 * @param[in] permissions new permissions
	 * @return true on success false otherwise.
	 */
	virtual bool updateServicePermissions(uint32_t serviceId, const RsServicePermissions &permissions) = 0;

	/**
	 * @brief getPeersConnected return peers using a service.
	 * @jsonapi{development}
	 * @param[in] serviceId service to look up.
	 * @param[out] peerSet set of peers using this service.
	 */
	virtual void getPeersConnected( uint32_t serviceId,
	                                std::set<RsPeerId>& peerSet ) = 0;
};

#endif
