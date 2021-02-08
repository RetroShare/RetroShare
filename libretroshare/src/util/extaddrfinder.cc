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

const uint32_t MAX_IP_STORE =	300; /* seconds ip address timeout */

//#define EXTADDRSEARCH_DEBUG

static const std::string ADDR_AGENT  = "Mozilla/5.0";

static std::string scan_ip(const std::string& text)
{
	std::set<unsigned char> digits ;
	digits.insert('0') ; digits.insert('3') ; digits.insert('6') ;
	digits.insert('1') ; digits.insert('4') ; digits.insert('7') ;
	digits.insert('2') ; digits.insert('5') ; digits.insert('8') ;
	digits.insert('9') ; 

	for(int i=0;i<(int)text.size();++i)
	{
		while(i < (int)text.size() && digits.find(text[i])==digits.end()) ++i ;

		if(i>=(int)text.size())
			return "" ;

		unsigned int a,b,c,d ;

		if(sscanf(text.c_str()+i,"%u.%u.%u.%u",&a,&b,&c,&d) != 4)
			continue ;

		if(a < 256 && b<256 && c<256 && d<256)
		{
			std::string s ;
			rs_sprintf(s, "%u.%u.%u.%u", a, b, c, d) ;
			return s;
		}
	}
	return "" ;
}

static void getPage(const std::string& server_name,std::string& page)
{
	page = "" ;
	int sockfd,n=0;                   // socket descriptor
	struct sockaddr_in serveur;       // server's parameters
	memset(&serveur.sin_zero, 0, sizeof(serveur.sin_zero));

	char buf[1024];
	char request[1024];
#ifdef EXTADDRSEARCH_DEBUG
	std::cout << "ExtAddrFinder: connecting to " << server_name << std::endl ;
#endif
	// socket creation

	sockfd = unix_socket(PF_INET,SOCK_STREAM,0);
	if (sockfd < 0)
	{
		std::cerr << "ExtAddrFinder: Failed to create socket" << std::endl;
		return ;
	}

	serveur.sin_family = AF_INET;

	// get server's ipv4 adress

    	in_addr in ;
        
	if(!rsGetHostByName(server_name.c_str(),in))  /* l'hôte n'existe pas */
	{
		std::cerr << "ExtAddrFinder: Unknown host " << server_name << std::endl;
		unix_close(sockfd);
		return ;
	}
	serveur.sin_addr = in ;
	serveur.sin_port = htons(80);

#ifdef EXTADDRSEARCH_DEBUG
	printf("Connection attempt\n");
#endif
    	std::cerr << "ExtAddrFinder: resolved hostname " << server_name << " to " << rs_inet_ntoa(in) << std::endl;

	sockaddr_storage server;
	sockaddr_storage_setipv4(server, &serveur);
	sockaddr_storage_setport(server, 80);
	if(unix_connect(sockfd, server) == -1)
	{
		std::cerr << "ExtAddrFinder: Connection error to " << server_name << std::endl ;
		unix_close(sockfd);
		return ;
	}
#ifdef EXTADDRSEARCH_DEBUG
	std::cerr << "ExtAddrFinder: Connection established to " << server_name << std::endl ;
#endif

	// envoi 
	if(snprintf( request, 
			1024,
			"GET / HTTP/1.0\r\n"
			"Host: %s:%d\r\n"
			"Connection: Close\r\n"
			"\r\n", 
			server_name.c_str(), 80) > 1020)
	{
		std::cerr << "ExtAddrFinder: buffer overrun. The server name \"" << server_name << "\" is too long. This is quite unexpected." << std::endl;
		unix_close(sockfd);
		return ;
	}

	if(send(sockfd,request,strlen(request),0)== -1)
	{
		std::cerr << "ExtAddrFinder: Could not send request to " << server_name << std::endl ;
		unix_close(sockfd);
		return ;
	}
	// recéption 

	while((n = recv(sockfd, buf, sizeof buf - 1, 0)) > 0)
	{
		buf[n] = '\0';
		page += std::string(buf,n) ;
	}
	// fermeture de la socket

	unix_close(sockfd);
#ifdef EXTADDRSEARCH_DEBUG
	std::cerr << "ExtAddrFinder: Got full page from " << server_name << std::endl ;
#endif
}


void* doExtAddrSearch(void *p)
{
	
	std::vector<std::string> res ;

	ExtAddrFinder *af = (ExtAddrFinder*)p ;

	for(std::list<std::string>::const_iterator it(af->_ip_servers.begin());it!=af->_ip_servers.end();++it)
	{
		std::string page ;

		getPage(*it,page) ;
		std::string ip = scan_ip(page) ;

		if(ip != "")
			res.push_back(ip) ;
#ifdef EXTADDRSEARCH_DEBUG
		std::cout << "ip found through " << *it << ": \"" << ip << "\"" << std::endl ;
#endif
	}

	if(res.empty())
	{
		// thread safe copy results.
		//
		{
			RsStackMutex mtx(af->mAddrMtx) ;

			af->mFound = false ;
			af->mFoundTS = time(NULL) ;
			af->mSearching = false ;
		}
		return NULL ;
	}

	sort(res.begin(),res.end()) ; // eliminates outliers.



	if(!sockaddr_storage_ipv4_aton(af->mAddr, res[res.size()/2].c_str()))
	{
		std::cerr << "ExtAddrFinder: Could not convert " << res[res.size()/2] << " into an address." << std::endl ;
		{
			RsStackMutex mtx(af->mAddrMtx) ;
			af->mFound = false ;
			af->mFoundTS = time(NULL) ;
			af->mSearching = false ;
		}
		return NULL ;
	}

	{
		RsStackMutex mtx(af->mAddrMtx) ;
		af->mFound = true ;
		af->mFoundTS = time(NULL) ;
		af->mSearching = false ;
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
            	std::cerr << "(EE) Could not start ExtAddrFinder thread." << std::endl;
}

bool ExtAddrFinder::hasValidIP(struct sockaddr_storage &addr)
{
#ifdef EXTADDRSEARCH_DEBUG
	std::cerr << "ExtAddrFinder: Getting ip." << std::endl ;
#endif

	{
		RsStackMutex mut(mAddrMtx) ;
		if(mFound)
		{
#ifdef EXTADDRSEARCH_DEBUG
			std::cerr << "ExtAddrFinder: Has stored ip: responding with this ip." << std::endl ;
#endif
            sockaddr_storage_copyip(addr,mAddr);	// just copy the IP so we dont erase the port.
		}
	}
	rstime_t delta;
	{
		RsStackMutex mut(mAddrMtx) ;
		//timeout the current ip
		delta = time(NULL) - mFoundTS;
	}
	if((uint32_t)delta > MAX_IP_STORE) {//launch a research
		if( mAddrMtx.trylock())
		{
			if(!mSearching)
			{
#ifdef EXTADDRSEARCH_DEBUG
				std::cerr << "ExtAddrFinder: No stored ip: Initiating new search." << std::endl ;
#endif
				mSearching = true ;
				start_request() ;
			}
#ifdef EXTADDRSEARCH_DEBUG
			else
				std::cerr << "ExtAddrFinder: Already searching." << std::endl ;
#endif
			mAddrMtx.unlock();
		}
#ifdef EXTADDRSEARCH_DEBUG
		else
			std::cerr << "ExtAddrFinder: (Note) Could not acquire lock. Busy." << std::endl ;
#endif
	}

	RsStackMutex mut(mAddrMtx) ;
	return mFound ;
}

void ExtAddrFinder::reset()
{
	RsStackMutex mut(mAddrMtx) ;

	mFound = false ;
	mSearching = false ;
	mFoundTS = time(NULL) - MAX_IP_STORE;
}

ExtAddrFinder::~ExtAddrFinder()
{
#ifdef EXTADDRSEARCH_DEBUG
	std::cerr << "ExtAddrFinder: Deleting ExtAddrFinder." << std::endl ;
#endif

}

ExtAddrFinder::ExtAddrFinder() : mAddrMtx("ExtAddrFinder")
{
#ifdef EXTADDRSEARCH_DEBUG
	std::cerr << "ExtAddrFinder: Creating new ExtAddrFinder." << std::endl ;
#endif
	RsStackMutex mut(mAddrMtx) ;

	mFound = false;
	mSearching = false;
	mFoundTS = time(NULL) - MAX_IP_STORE;
	sockaddr_storage_clear(mAddr);

	_ip_servers.push_back(std::string( "checkip.dyndns.org" )) ;
	_ip_servers.push_back(std::string( "www.myip.dk"   )) ;
	_ip_servers.push_back(std::string( "showip.net"         )) ;
	_ip_servers.push_back(std::string( "www.displaymyip.com")) ;
}

