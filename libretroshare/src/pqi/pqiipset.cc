/*******************************************************************************
 * libretroshare/src/pqi: pqiipset.cc                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie <retroshare@lunamutt.com>              *
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
#include "util/rstime.h"
#include "pqi/pqiipset.h"
#include "util/rsstring.h"

bool pqiIpAddress::sameAddress(const pqiIpAddress &a) const
{
	return sockaddr_storage_same(mAddr, a.mAddr);
}


bool pqiIpAddress::validAddress() const
{
	/* filter for unlikely addresses */
	if(sockaddr_storage_isLoopbackNet(mAddr))
	{
#ifdef IPADDR_DEBUG
		std::cerr << "pqiIpAddress::validAddress() ip parameter is loopback: disgarding." << std::endl ;
#endif
		return false;
	}

	if(sockaddr_storage_isnull(mAddr))
	{
#ifdef IPADDR_DEBUG
		std::cerr << "pqiIpAddress::validAddress() ip parameter is 0.0.0.0/1, or port is 0, ignoring." << std::endl;
#endif
		return false;
	}

	return true;

}


bool 	pqiIpAddrList::updateIpAddressList(const pqiIpAddress &addr)  
{
	std::list<pqiIpAddress>::iterator it;
	bool add = false;
	bool newAddr = true;

#ifdef IPADDR_DEBUG
	std::cerr << "pqiIpAddrList::updateIpAddressList()";
	std::cerr << std::endl;
#endif

	if (mAddrs.size() < MAX_ADDRESS_LIST_SIZE)
	{
#ifdef IPADDR_DEBUG
		std::cerr << "pqiIpAddrList::updateIpAddressList() small list: Add";
		std::cerr << std::endl;
#endif
		add = true;
	}
	else if (mAddrs.back().mSeenTime < addr.mSeenTime)
	{
#ifdef IPADDR_DEBUG
		std::cerr << "pqiIpAddrList::updateIpAddressList() oldAddr: Add";
		std::cerr << std::endl;
#endif
		add = true;
	}

	if ((!add) || (!addr.validAddress()))
	{
#ifdef IPADDR_DEBUG
		std::cerr << "pqiIpAddrList::updateIpAddressList() not Add or !valid.. fail";
		std::cerr << std::endl;
#endif
		return false;
	}

	for(it = mAddrs.begin(); it != mAddrs.end(); ++it)
	{
		if (it->sameAddress(addr))
		{
#ifdef IPADDR_DEBUG
			std::cerr << "pqiIpAddrList::updateIpAddressList() found duplicate";
			std::cerr << std::endl;
#endif
			if (it->mSeenTime > addr.mSeenTime)
			{
#ifdef IPADDR_DEBUG
				std::cerr << "pqiIpAddrList::updateIpAddressList() orig better, returning";
				std::cerr << std::endl;
#endif
				/* already better -> quit */
				return false;
			}

#ifdef IPADDR_DEBUG
			std::cerr << "pqiIpAddrList::updateIpAddressList() deleting orig";
			std::cerr << std::endl;
#endif
			it = mAddrs.erase(it);
			newAddr = false;
			break;
		}
	}

	// ordered by decreaseing time. (newest at front)
	bool added = false;
	for(it = mAddrs.begin(); it != mAddrs.end(); ++it)
	{
		if (it->mSeenTime < addr.mSeenTime)
		{
#ifdef IPADDR_DEBUG
			std::cerr << "pqiIpAddrList::updateIpAddressList() added orig SeenTime: " << it->mSeenTime << " new SeenTime: " << addr.mSeenTime;
			std::cerr << std::endl;
#endif

			added = true;
			mAddrs.insert(it, addr);
			break;
		}
	}
	if (!added)
	{
#ifdef IPADDR_DEBUG
		std::cerr << "pqiIpAddrList::updateIpAddressList() pushing to back";
		std::cerr << std::endl;
#endif
		mAddrs.push_back(addr);
	}

	/* pop if necessary */
	while (mAddrs.size() > MAX_ADDRESS_LIST_SIZE)
	{
#ifdef IPADDR_DEBUG
		std::cerr << "pqiIpAddrList::updateIpAddressList() popping back";
		std::cerr << std::endl;
#endif
		mAddrs.pop_back();
	}

	return newAddr;
}

void    pqiIpAddrList::extractFromTlv(const RsTlvIpAddrSet &tlvAddrs)
{
	std::list<RsTlvIpAddressInfo>::const_iterator it;

	//for(it = tlvAddrs.addrs.begin(); it != tlvAddrs.addrs.end() ; ++it)
	for(it = tlvAddrs.mList.begin(); it != tlvAddrs.mList.end() ; ++it)
	{
		pqiIpAddress addr;
		addr.mAddr = it->addr.addr;
		addr.mSeenTime = it->seenTime;
		addr.mSrc = it->source; 

		mAddrs.push_back(addr);
	}
}

void    pqiIpAddrList::loadTlv(RsTlvIpAddrSet &tlvAddrs) const
{
	std::list<pqiIpAddress>::const_iterator it;

	for(it = mAddrs.begin(); it != mAddrs.end() ; ++it)
	{
		RsTlvIpAddressInfo addr;
		addr.addr.addr = it->mAddr;
		addr.seenTime = it->mSeenTime;
		addr.source = it->mSrc;
	
		//tlvAddrs.addrs.push_back(addr);
		tlvAddrs.mList.push_back(addr);
	}
}



void 	pqiIpAddrList::printIpAddressList(std::string &out) const
{
	std::list<pqiIpAddress>::const_iterator it;
	rstime_t now = time(NULL);
	for(it = mAddrs.begin(); it != mAddrs.end(); ++it)
	{
		out += sockaddr_storage_tostring(it->mAddr);
		rs_sprintf_append(out, "( %ld old)\n", now - it->mSeenTime);
	}
	return;
}


bool    pqiIpAddrSet::updateLocalAddrs(const pqiIpAddress &addr)
{
	return	mLocal.updateIpAddressList(addr);
}

bool    pqiIpAddrSet::updateExtAddrs(const pqiIpAddress &addr)
{
	return	mExt.updateIpAddressList(addr);
}

bool    pqiIpAddrSet::updateAddrs(const pqiIpAddrSet &addrs)
{
#ifdef IPADDR_DEBUG
	std::cerr << "pqiIpAddrSet::updateAddrs()";
	std::cerr << std::endl;
#endif

	bool newAddrs = false;
	std::list<pqiIpAddress>::const_iterator it;
	for(it = addrs.mLocal.mAddrs.begin(); it != addrs.mLocal.mAddrs.end(); ++it)
	{
		if (mLocal.updateIpAddressList(*it))
		{
#ifdef IPADDR_DEBUG
			std::cerr << "pqiIpAddrSet::updateAddrs() Updated Local Addr";
			std::cerr << std::endl;
#endif
			newAddrs = true;
		}
	}

	for(it = addrs.mExt.mAddrs.begin(); it != addrs.mExt.mAddrs.end(); ++it)
	{
		if (mExt.updateIpAddressList(*it))
		{
#ifdef IPADDR_DEBUG
			std::cerr << "pqiIpAddrSet::updateAddrs() Updated Ext Addr";
			std::cerr << std::endl;
#endif
			newAddrs = true;
		}
	}
	return newAddrs;
}



void    pqiIpAddrSet::printAddrs(std::string &out) const
{
	out += "Local Addresses: ";
	mLocal.printIpAddressList(out);
	out += "\nExt Addresses: ";
	mExt.printIpAddressList(out);
	out += "\n";
}




