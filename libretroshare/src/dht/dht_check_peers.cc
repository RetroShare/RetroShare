/*
 * libretroshare/src/dht: odhtmgr_test.cc
 *
 * Interface with OpenDHT for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
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



/***** Test for the new DHT system *****/

#include "pqi/p3dhtmgr.h"
#include "pqi/p3connmgr.h"
#include "pqi/pqimonitor.h"
#include "dht/opendhtmgr.h"

#include "util/rsnet.h"
#include "util/rsthreads.h"
#include "util/rsprint.h"

#include "tcponudp/tou_net.h"
#include "tcponudp/udpsorter.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <unistd.h>

#define BOOTSTRAP_DEBUG  1

void usage(char *name)
{
	std::cerr << "USAGE: " << name << " -o OwnId [ -p PeerId1 [ -p PeerId2 [ ... ] ] ] ";
	std::cerr << std::endl;
	exit(1);
}

bool stunPeer(struct sockaddr_in toaddr, struct sockaddr_in &ansaddr);

class pqiConnectCbStun;

class dhtStunData
{
        public:
	pqiConnectCbStun *stunCb;
        std::string id;
	struct sockaddr_in toaddr;
	struct sockaddr_in ansaddr;
};

extern "C" void* doStunPeer(void* p);



class StunDetails
{
	public:
	StunDetails()
{
	lastStatus = 0;
	lastStunResult = 0;
	stunAttempts = 0;
	stunResults = 0;
}

	std::string id;

	/* peerStatus details */
	struct sockaddr_in laddr, raddr;
	uint32_t type, mode, source;

	/* stun response */
	uint32_t stunAttempts;
	uint32_t stunResults;
	struct sockaddr_in stunaddr;

	/* timestamps */
	time_t lastStatus;
	time_t lastStunResult;

};

class pqiConnectCbStun: public pqiConnectCb
{
        public:
        pqiConnectCbStun()
{
	return;
}

virtual ~pqiConnectCbStun()
{
	return;
}

void	addPeer(std::string id)
{
	RsStackMutex stack(peerMtx); /**** LOCK MUTEX ***/
	std::map<std::string, StunDetails>::iterator it;
	it = peerMap.find(id);
	if (it == peerMap.end())
	{
		StunDetails sd;
		sd.id = id;
		peerMap[id] = sd;
	}
}

virtual void    peerStatus(std::string id,
			struct sockaddr_in laddr, struct sockaddr_in raddr,
                        uint32_t type, uint32_t mode, uint32_t source)
{

      {
	RsStackMutex stack(peerMtx); /**** LOCK MUTEX ***/

	std::map<std::string, StunDetails>::iterator it;
	it = peerMap.find(id);
	if (it == peerMap.end())
	{
		std::cerr << "peerStatus() for unknown Peer id: " << id;
		std::cerr << std::endl;
		return;
	}
	it->second.laddr = laddr;
	it->second.raddr = raddr;
	it->second.type  = type;
	it->second.mode  = mode;
	it->second.source= source;

	it->second.lastStatus = time(NULL);

	it->second.stunAttempts++; /* as we are about to try! */
      }

	printPeerStatus();
	stunPeer(id, raddr);
}

void	printPeerStatus()
{
	RsStackMutex stack(peerMtx); /**** LOCK MUTEX ***/

	time_t t = time(NULL);
	std::string timestr = ctime(&t);
	std::cerr << "BootstrapStatus: " << timestr;
	std::cerr << "BootstrapStatus: " << peerMap.size() << " Peers";
	std::cerr << std::endl;
	std::cerr << "BootstrapStatus: ID ----------  DHT ENTRY ---";
	std::cerr << " EXT PORT -- STUN OK -- %AVAIL -- LAST DHT TS";
	std::cerr << std::endl;

	std::map<std::string, StunDetails>::iterator it;

	for(it = peerMap.begin(); it != peerMap.end(); it++)
	{
		std::cerr << it->first;

		bool dhtActive = (time(NULL) - it->second.lastStatus < 1900);
		bool stunActive = (time(NULL) - it->second.lastStunResult < 1900);
		bool extPort = it->second.type & RS_NET_CONN_TCP_EXTERNAL;
		float percentAvailable = it->second.stunResults * 100.0 / (it->second.stunAttempts + 0.0001);

		if (dhtActive)
		{
			std::cerr << "    Yes --->";
		}
		else
		{
			std::cerr << "    No      ";
		}

		if (extPort)
		{
			std::cerr << "    Yes --->";
		}
		else
		{
			std::cerr << "    No      ";
		}

		if (stunActive)
		{
			std::cerr << "    Yes --->";
		}
		else
		{
			std::cerr << "    No      ";
		}

		std::cerr << "  " << std::setw(4) << percentAvailable;
		std::cerr << "  ";

		if (it->second.lastStatus == 0)
		{
			std::cerr << " NEVER ";
		}
		else
		{
			std::cerr << " " << time(NULL) - it->second.lastStatus;
			std::cerr << " secs ago ";
		}
		std::cerr << std::endl;
	}
}




void	stunPeer(std::string id, struct sockaddr_in peeraddr)
{
	std::cerr << "Should Stun Peer: " << id;
	std::cerr << std::endl;

        /* launch a publishThread */
        pthread_t tid;

        dhtStunData *pub = new dhtStunData;
        pub->stunCb = this;
        pub->id = id;
        pub->toaddr = peeraddr;

        void *data = (void *) pub;
        pthread_create(&tid, 0, &doStunPeer, data);

        return;

}


virtual void    peerConnectRequest(std::string id,
                        struct sockaddr_in raddr, uint32_t source)
{
	return;
}


virtual void    stunStatus(std::string id, struct sockaddr_in raddr, uint32_t type, uint32_t flags)
{
	return;
}

virtual void    stunSuccess(std::string id, struct sockaddr_in toaddr, struct sockaddr_in ansaddr)
{
      {
	RsStackMutex stack(peerMtx); /**** LOCK MUTEX ***/

	std::map<std::string, StunDetails>::iterator it;
	it = peerMap.find(id);
	if (it == peerMap.end())
	{
		std::cerr << "stunSuccess() for unknown Peer id: " << id;
		std::cerr << std::endl;
		return;
	}
	std::cerr << "stunSuccess() for id: " << id;
	std::cerr << std::endl;

	it->second.lastStunResult = time(NULL);
	it->second.stunResults++;
      }

	printPeerStatus();
}
	
	private:

	RsMutex peerMtx;
	std::map<std::string, StunDetails> peerMap;
};



extern "C" void* doStunPeer(void* p)
{
        dhtStunData *data = (dhtStunData *) p;
        if ((!data) || (!data->stunCb))
        {
                pthread_exit(NULL);
        }

        /* stun it! */
	if (stunPeer(data->toaddr, data->ansaddr))
	{
		data->stunCb->stunSuccess(data->id, data->toaddr, data->ansaddr);
	}

	delete data;

	pthread_exit(NULL);

	return NULL;
}




int main(int argc, char **argv)
{
	int c;
	bool setOwnId = false;
	std::string ownId;
	std::list<std::string> peerIds;
	
	while(-1 != (c = getopt(argc, argv, "o:p:")))
	{
		switch (c)
		{
		case 'o':
			ownId = optarg;
			setOwnId = true;
			break;
		case 'p':
			peerIds.push_back(std::string(optarg));
			break;
		default:
			usage(argv[0]);
			break;
		}
	}
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else
/* for static PThreads under windows... we need to init the library...
 */
  #ifdef PTW32_STATIC_LIB
         pthread_win32_process_attach_np();
  #endif

        // Windows Networking Init.
        WORD wVerReq = MAKEWORD(2,2);
        WSADATA wsaData;

        if (0 != WSAStartup(wVerReq, &wsaData))
        {
                std::cerr << "Failed to Startup Windows Networking";
                std::cerr << std::endl;
        }
        else
        {
                std::cerr << "Started Windows Networking";
                std::cerr << std::endl;
        }

#endif


	if (!setOwnId)
	{
		std::cerr << "Missing OwnId: Setting dummy Id";
		std::cerr << std::endl;

		setOwnId = true;
		ownId = "dummyOwnId";
	}

	pqiConnectCbStun cbStun;
	OpenDHTMgr  dhtTester(ownId, &cbStun, ".");

	/* startup dht */
	std::cerr << "Starting up DhtTester()" << std::endl;
	dhtTester.start();

	/* wait for a little before switching on */
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
        sleep(1);
#else
        Sleep(1000);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/


	std::cerr << "Switching on DhtTester()" << std::endl;
	dhtTester.enable(true);

	std::cerr << "Adding a List of Peers" << std::endl;
	std::list<std::string>::iterator it;
	for(it = peerIds.begin(); it != peerIds.end(); it++)
	{
		cbStun.addPeer(*it);
		dhtTester.findPeer(*it);
	}

	/* switch off Stun/Bootstrap stuff */
	dhtTester.enableStun(false);
	dhtTester.setBootstrapAllowed(false);
		

	/* wait loop */
	while(1)
	{
		cbStun.printPeerStatus();
		std::cerr << "Main waiting..." << std::endl;
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
	        sleep(30);
#else
		Sleep(30000);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
	}
};


bool stunPeer(struct sockaddr_in toaddr, struct sockaddr_in &ansaddr)
{
#ifdef BOOTSTRAP_DEBUG
        std::cerr << "stunPeer: " << toaddr << std::endl;
#endif
	/* open a socket */
	int sockfd = tounet_socket(PF_INET, SOCK_DGRAM, 0);
        if (-1 == tounet_fcntl(sockfd, F_SETFL, O_NONBLOCK))
        {
#ifdef BOOTSTRAP_DEBUG
                std::cerr << "Failed to Make Non-Blocking" << std::endl;
#endif
        }

	/* create a stun packet */
	char stunpkt[100];
	int  maxlen = 100;
	int  len = maxlen;

	UdpStun_generate_stun_pkt((void *) stunpkt, &len);

#ifdef BOOTSTRAP_DEBUG
        std::cerr << "stunPeer() Send packet length: " << len << std::endl;
#endif

	/* send stun packet */
        tounet_sendto(sockfd, stunpkt, len, 0, 
                           (struct sockaddr *) &(toaddr),
                                sizeof(toaddr));

	/* wait */
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
	        sleep(2);
#else
		Sleep(2000);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

	/* check for response */
        struct sockaddr_in fromaddr;
        socklen_t fromsize = sizeof(fromaddr);
        int insize = maxlen;

        insize = tounet_recvfrom(sockfd,stunpkt,insize,0,
                        (struct sockaddr*)&fromaddr,&fromsize);

	tounet_close(sockfd);

        if (0 >= insize)
        {
#ifdef BOOTSTRAP_DEBUG
                std::cerr << "No Stun response from: " << toaddr;
                std::cerr << std::endl;
#endif
		return false;
	}

	if (UdpStun_response(stunpkt, insize, ansaddr))
	{
#ifdef BOOTSTRAP_DEBUG
                std::cerr << "received Stun Reply from : " << fromaddr;
                std::cerr << std::endl;
                std::cerr << "External Address is: " << ansaddr;
                std::cerr << std::endl;
#endif
                return true;
        }

#ifdef BOOTSTRAP_DEBUG
        std::cerr << "received Data (not Stun Reply) from : " << fromaddr;
        std::cerr << std::endl;
#endif
        return false;
}

