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

//#define EXTADDRSEARCH_DEBUG

class ZeroInt
{
public:
	ZeroInt() : n(0) {}
	uint32_t n ;
};

void* doExtAddrSearch(void *p)
{
	
	std::vector<std::string> res ;

	ExtAddrFinder *af = (ExtAddrFinder*)p ;

	for(std::list<std::string>::const_iterator it(af->_ip_servers.begin());it!=af->_ip_servers.end();++it)
	{
		std::string ip = "";
		rsGetHostByNameSpecDNS(*it,"myip.opendns.com",ip,2);
		if(ip != "")
			res.push_back(ip) ;
#ifdef EXTADDRSEARCH_DEBUG
		RsDbg(__PRETTY_FUNCTION__, " ip found through DNS ", *it, ": \"", ip, "\"");
#endif
	}

	if(res.empty())
	{
		af->reset();
		return NULL ;
	}

	std::map<sockaddr_storage,ZeroInt> addrV4_votes;
	std::map<sockaddr_storage,ZeroInt> addrV6_votes;
	std::string addrV4_Found;
	std::string addrV6_Found;

	for(auto curRes : res)
	{
		sockaddr_storage addr;
		sockaddr_storage_clear(addr);
		//sockaddr_storage_inet_pton convert IPv4 to IPv6
		struct sockaddr_in * addrv4p = (struct sockaddr_in *) &addr;
		struct sockaddr_in6 * addrv6p = (struct sockaddr_in6 *) &addr;
		if( inet_pton(AF_INET, curRes.c_str(), &(addrv4p->sin_addr)) )
		{
			addr.ss_family = AF_INET;
			addrV4_votes[addr].n++ ;
			addrV4_Found += sockaddr_storage_tostring(addr) + "\n";
		}
		else if( inet_pton(AF_INET6, curRes.c_str(), &(addrv6p->sin6_addr)) )
		{
			addr.ss_family = AF_INET6;
			addrV6_votes[addr].n++ ;
			addrV6_Found += sockaddr_storage_tostring(addr) + "\n";
		}
		else
			RsErr(__PRETTY_FUNCTION__, " Invalid addresse reported: ", curRes) ;

	}

	if( (0 == addrV4_votes.size()) && (0 == addrV6_votes.size()) )
	{
		RsErr(__PRETTY_FUNCTION__, " Could not find any external address.");
		af->reset();
		return NULL ;
	}

	if( 1 < addrV4_votes.size() )
		RsErr(__PRETTY_FUNCTION__, " Multiple external IPv4 addresses reported: "
		      , addrV4_Found ) ;

	if( 1 < addrV6_votes.size() )
		RsErr(__PRETTY_FUNCTION__, " Multiple external IPv6 addresses reported: "
		      , addrV6_Found ) ;

	{
		RsStackMutex mtx(af->mAddrMtx) ;
		af->mSearching = false ;
		af->mFoundTS = time(NULL) ;

		// Only save more reported address if not only once.
		uint32_t admax = 0 ;
		sockaddr_storage_clear(af->mAddrV4);
		for (auto it : addrV4_votes)
			if (admax < it.second.n)
			{
				af->mAddrV4 = it.first ;
				af->mFoundV4 = true ;
				admax = it.second.n ;
			}

		admax = 0 ;
		sockaddr_storage_clear(af->mAddrV6);
		for (auto it : addrV6_votes)
			if (admax < it.second.n)
			{
				af->mAddrV6 = it.first ;
				af->mFoundV6 = true ;
				admax = it.second.n ;
			}

	}

	return NULL ;
}


void ExtAddrFinder::start_request()
{
	void *data = (void *)this;
	pthread_t tid ;

	if(! pthread_create(&tid, 0, &doExtAddrSearch, data))
		pthread_detach(tid); /* so memory is reclaimed in linux */
	else
		RsErr(__PRETTY_FUNCTION__, " Could not start ExtAddrFinder thread.");
}

bool ExtAddrFinder::hasValidIPV4(struct sockaddr_storage &addr)
{
#ifdef EXTADDRSEARCH_DEBUG
	RsDbg(__PRETTY_FUNCTION__, " Getting ip.");
#endif

	{
		RsStackMutex mut(mAddrMtx) ;
		if(mFoundV4)
		{
#ifdef EXTADDRSEARCH_DEBUG
			RsDbg(__PRETTY_FUNCTION__, " Has stored ip responding with this ip:", sockaddr_storage_iptostring(mAddrV4)) ;
#endif
			sockaddr_storage_copyip(addr,mAddrV4);	// just copy the IP so we dont erase the port.
		}
	}

	testTimeOut();

	RsStackMutex mut(mAddrMtx) ;
	return mFoundV4;
}

bool ExtAddrFinder::hasValidIPV6(struct sockaddr_storage &addr)
{
#ifdef EXTADDRSEARCH_DEBUG
	RsDbg(__PRETTY_FUNCTION__, " Getting ip.");
#endif

	{
		RsStackMutex mut(mAddrMtx) ;
		if(mFoundV6)
		{
#ifdef EXTADDRSEARCH_DEBUG
			RsDbg(__PRETTY_FUNCTION__, " Has stored ip responding with this ip:", sockaddr_storage_iptostring(mAddrV6)) ;
#endif
			sockaddr_storage_copyip(addr,mAddrV6);	// just copy the IP so we dont erase the port.
		}
	}

	testTimeOut();

	RsStackMutex mut(mAddrMtx) ;
	return mFoundV6;
}

void ExtAddrFinder::testTimeOut()
{
	bool timeOut;
	{
		RsStackMutex mut(mAddrMtx) ;
		//timeout the current ip
		timeOut = (mFoundTS + MAX_IP_STORE < time(NULL));
	}
	if(timeOut || mFirstTime) {//launch a research
		if( mAddrMtx.trylock())
		{
			if(!mSearching)
			{
#ifdef EXTADDRSEARCH_DEBUG
				RsDbg(__PRETTY_FUNCTION__, " No stored ip: Initiating new search.");
#endif
				mSearching = true ;
				start_request() ;
			}
#ifdef EXTADDRSEARCH_DEBUG
			else
				RsDbg(__PRETTY_FUNCTION__, " Already searching.");
#endif
			mFirstTime = false;
			mAddrMtx.unlock();
		}
#ifdef EXTADDRSEARCH_DEBUG
		else
			RsDbg(__PRETTY_FUNCTION__, " (Note) Could not acquire lock. Busy.");
#endif
	}
}

void ExtAddrFinder::reset(bool firstTime /*=false*/)
{
#ifdef EXTADDRSEARCH_DEBUG
	RsDbg(__PRETTY_FUNCTION__, " firstTime=", firstTime?"true":"false");
#endif
	RsStackMutex mut(mAddrMtx) ;

	mSearching = false ;
	mFoundV4 = false ;
	mFoundV6 = false ;
	mFirstTime = firstTime;
	mFoundTS = time(NULL);
	sockaddr_storage_clear(mAddrV4);
	sockaddr_storage_clear(mAddrV6);
}

ExtAddrFinder::~ExtAddrFinder()
{
#ifdef EXTADDRSEARCH_DEBUG
	RsDbg(__PRETTY_FUNCTION__, " Deleting ExtAddrFinder.");
#endif

}

ExtAddrFinder::ExtAddrFinder() : mAddrMtx("ExtAddrFinder")
{
#ifdef EXTADDRSEARCH_DEBUG
	RsDbg(__PRETTY_FUNCTION__, " Creating new ExtAddrFinder.");
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

