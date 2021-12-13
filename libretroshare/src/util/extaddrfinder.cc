/*******************************************************************************
 * libretroshare/src/util: extaddrfinder.cc                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2017 Retroshare Team <retroshare.project@gmail.com>           *
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

//#define EXTADDRSEARCH_DEBUG

#include "extaddrfinder.h"

#include "pqi/pqinetwork.h"
#include "rsdebug.h"
#include "util/rsstring.h"
#include "util/rsmemory.h"

#ifndef WIN32
#include <netdb.h>
#endif

#include <string.h>
#include <string>
#include <iostream>
#include <set>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include "util/rstime.h"

#include <map>

const uint32_t MAX_IP_STORE = 300; /* seconds ip address timeout */

class ZeroInt
{
public:
	ZeroInt() : n(0) {}
	uint32_t n ;
};

void ExtAddrFinder::run()
{
	
	std::vector<std::string> res ;

	for(auto& it : _ip_servers)
	{
		std::string ip = "";
		rsGetHostByNameSpecDNS(it,"myip.opendns.com",ip,2);
		if(ip != "")
			res.push_back(ip) ;
#ifdef EXTADDRSEARCH_DEBUG
		RS_DBG("ip found through DNS ", it, ": \"", ip, "\"");
#endif
	}

	if(res.empty())
	{
		reset();
		return ;
	}

	std::map<sockaddr_storage,ZeroInt> addrV4_votes;
	std::map<sockaddr_storage,ZeroInt> addrV6_votes;
	std::string all_addrV4_Found;
	std::string all_addrV6_Found;

	for(auto curRes : res)
	{
		sockaddr_storage addr;
		sockaddr_storage_clear(addr);
		bool validIP =   sockaddr_storage_inet_pton(addr, curRes)
		              && sockaddr_storage_isValidNet(addr);
		bool isIPv4 = sockaddr_storage_ipv6_to_ipv4(addr);
		if( validIP && isIPv4 )
		{
			addr.ss_family = AF_INET;
			addrV4_votes[addr].n++ ;
			all_addrV4_Found += sockaddr_storage_tostring(addr) + "\n";
		}
		else if( validIP && !isIPv4)
		{
			addr.ss_family = AF_INET6;
			addrV6_votes[addr].n++ ;
			all_addrV6_Found += sockaddr_storage_tostring(addr) + "\n";
		}
		else
			RS_ERR("Invalid addresse reported: ", curRes) ;

	}

	if( (0 == addrV4_votes.size()) && (0 == addrV6_votes.size()) )
	{
		RS_ERR("Could not find any external address.");
		reset();
		return ;
	}

	if( 1 < addrV4_votes.size() )
		RS_ERR("Multiple external IPv4 addresses reported: "
		      , all_addrV4_Found ) ;

	if( 1 < addrV6_votes.size() )
		RS_ERR("Multiple external IPv6 addresses reported: "
		      , all_addrV6_Found ) ;

	{
		RS_STACK_MUTEX(mAddrMtx);

		mSearching = false ;
		mFoundTS = time(NULL) ;

		// Only save more reported address if not only once.
		uint32_t admax = 0 ;
		sockaddr_storage_clear(mAddrV4);
		for (auto it : addrV4_votes)
			if (admax < it.second.n)
			{
				mAddrV4 = it.first ;
				mFoundV4 = true ;
				admax = it.second.n ;
			}

		admax = 0 ;
		sockaddr_storage_clear(mAddrV6);
		for (auto it : addrV6_votes)
			if (admax < it.second.n)
			{
				mAddrV6 = it.first ;
				mFoundV6 = true ;
				admax = it.second.n ;
			}

	}

	return ;
}

void ExtAddrFinder::start_request()
{
	if (!isRunning())
		start("ExtAddrFinder");
}

bool ExtAddrFinder::hasValidIPV4(struct sockaddr_storage &addr)
{
#ifdef EXTADDRSEARCH_DEBUG
	RS_DBG("Getting ip.");
#endif

	{
		RS_STACK_MUTEX(mAddrMtx) ;
		if(mFoundV4)
		{
#ifdef EXTADDRSEARCH_DEBUG
			RS_DBG("Has stored ip responding with this ip:", sockaddr_storage_iptostring(mAddrV4)) ;
#endif
			sockaddr_storage_copyip(addr,mAddrV4);	// just copy the IP so we dont erase the port.
		}
	}

	testTimeOut();

	RS_STACK_MUTEX(mAddrMtx) ;
	return mFoundV4;
}

bool ExtAddrFinder::hasValidIPV6(struct sockaddr_storage &addr)
{
#ifdef EXTADDRSEARCH_DEBUG
	RS_DBG("Getting ip.");
#endif

	{
		RS_STACK_MUTEX(mAddrMtx) ;
		if(mFoundV6)
		{
#ifdef EXTADDRSEARCH_DEBUG
			RS_DBG("Has stored ip responding with this ip:", sockaddr_storage_iptostring(mAddrV6)) ;
#endif
			sockaddr_storage_copyip(addr,mAddrV6);	// just copy the IP so we dont erase the port.
		}
	}

	testTimeOut();

	RS_STACK_MUTEX(mAddrMtx) ;
	return mFoundV6;
}

void ExtAddrFinder::testTimeOut()
{
	bool timeOut;
	{
		RS_STACK_MUTEX(mAddrMtx) ;
		//timeout the current ip
		timeOut = (mFoundTS + MAX_IP_STORE < time(NULL));
	}
	if(timeOut || mFirstTime) {//launch a research
		if( mAddrMtx.trylock())
		{
			if(!mSearching)
			{
#ifdef EXTADDRSEARCH_DEBUG
				RS_DBG("No stored ip: Initiating new search.");
#endif
				mSearching = true ;
				start_request() ;
			}
#ifdef EXTADDRSEARCH_DEBUG
			else
				RS_DBG("Already searching.");
#endif
			mFirstTime = false;
			mAddrMtx.unlock();
		}
#ifdef EXTADDRSEARCH_DEBUG
		else
			RS_DBG("(Note) Could not acquire lock. Busy.");
#endif
	}
}

void ExtAddrFinder::reset(bool firstTime /*=false*/)
{
#ifdef EXTADDRSEARCH_DEBUG
	RS_DBG("firstTime=", firstTime);
#endif
	RS_STACK_MUTEX(mAddrMtx) ;

	mSearching = false ;
	mFoundV4 = false ;
	mFoundV6 = false ;
	mFirstTime = firstTime;
	mFoundTS = time(nullptr);
	sockaddr_storage_clear(mAddrV4);
	sockaddr_storage_clear(mAddrV6);
}

ExtAddrFinder::~ExtAddrFinder()
{
#ifdef EXTADDRSEARCH_DEBUG
	RS_DBG("Deleting ExtAddrFinder.");
#endif
}

ExtAddrFinder::ExtAddrFinder() : mAddrMtx("ExtAddrFinder")
{
#ifdef EXTADDRSEARCH_DEBUG
	RS_DBG("Creating new ExtAddrFinder.");
#endif
	reset( true );

//https://unix.stackexchange.com/questions/22615/how-can-i-get-my-external-ip-address-in-a-shell-script
	//Enter direct ip so local DNS cannot change it.
	//DNS servers must recognize "myip.opendns.com"
	_ip_servers.push_back(std::string( "208.67.222.222" )) ;//resolver1.opendns.com
	_ip_servers.push_back(std::string( "208.67.220.220" )) ;//resolver2.opendns.com
	_ip_servers.push_back(std::string( "208.67.222.220" )) ;//resolver3.opendns.com
	_ip_servers.push_back(std::string( "208.67.220.222" )) ;//resolver4.opendns.com
	_ip_servers.push_back(std::string( "2620:119:35::35" )) ;//resolver1.opendns.com
	_ip_servers.push_back(std::string( "2620:119:53::53" )) ;//resolver2.opendns.com
}
