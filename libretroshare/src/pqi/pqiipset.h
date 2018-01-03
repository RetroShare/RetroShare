#ifndef PQI_IP_SET_H
#define PQI_IP_SET_H

/*
 * libretroshare/src/pqi: pqiipset.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2009-2010 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
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

#include "util/rsnet.h"
#include "serialiser/rstlvaddrs.h"

#define MAX_ADDRESS_LIST_SIZE 4

class pqiIpAddress
{
	public:
	bool sameAddress(const pqiIpAddress &a) const;
	bool validAddress() const;

	struct sockaddr_storage mAddr;
	time_t mSeenTime;
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
