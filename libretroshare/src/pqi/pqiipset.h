/*******************************************************************************
 * libretroshare/src/pqi: pqiipset.h                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2009-2010 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef PQI_IP_SET_H
#define PQI_IP_SET_H

#include "util/rsnet.h"
#include "serialiser/rstlvaddrs.h"
#include "util/rstime.h"

#define MAX_ADDRESS_LIST_SIZE 10

class pqiIpAddress
{
	public:
	bool sameAddress(const pqiIpAddress &a) const;
	bool validAddress() const;

	struct sockaddr_storage mAddr;
	rstime_t mSeenTime;
	uint32_t mSrc;
};


class pqiIpAddrList
{
	public:

	// returns true if new address added.
	bool 	updateIpAddressList(const pqiIpAddress &addr);
	void 	printIpAddressList(std::string &out) const;
	void    extractFromTlv(const RsTlvIpAddrSet &tlvAddrs);
	void    loadTlv(RsTlvIpAddrSet &tlvAddrs) const;

	// sorted list... based on seen time.
	std::list<pqiIpAddress> mAddrs;
};


class pqiIpAddrSet
{
	public:

	bool 	updateLocalAddrs(const pqiIpAddress &addr);
	bool 	updateExtAddrs(const pqiIpAddress &addr);
	bool 	updateAddrs(const pqiIpAddrSet &addrs);
	void 	printAddrs(std::string &out) const;
	pqiIpAddrList mLocal;
	pqiIpAddrList mExt;

	void clear()
	{
		mLocal.mAddrs.clear();
		mExt.mAddrs.clear();
	}
};

	
#endif	
