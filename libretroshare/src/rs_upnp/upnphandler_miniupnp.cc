/*******************************************************************************
 * libretroshare/src/upnp: upnphandler_miniupnp.cc                             *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2018 Retroshare Team <retroshare.project@gmail.com>          *
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
// Windows / Mac version.
/* This stuff is actually C */

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef  __cplusplus
} /* extern C */
#endif
/* This stuff is actually C */

#include "rs_upnp/upnphandler_miniupnp.h"
#include "rs_upnp/upnputil.h"

class uPnPConfigData
{
	public:
		struct UPNPDev * devlist;
		struct UPNPUrls urls;
		struct IGDdatas data;
		char lanaddr[16];	/* my ip address on the LAN */
};

#include <iostream>

#include "util/rsnet.h"

bool upnphandler::initUPnPState()
{
	/* allocate memory */
	uPnPConfigData *upcd = new uPnPConfigData;
#if MINIUPNPC_API_VERSION >= 14 //1.9 2015/07/23
	/* $Id: miniupnpc.h,v 1.44 2015/07/23 20:40:10 nanard Exp $ */
	//upnpDiscover(int delay, const char * multicastif,
	//             const char * minissdpdsock, int sameport,
	//             int ipv6, unsigned char ttl,
	//             int * error);
	unsigned char ttl = 2;	/* defaulting to 2 */
	upcd->devlist = upnpDiscover(2000, NULL,
	                             NULL, 0,
	                             0, ttl,
	                             NULL);
#else
#if MINIUPNPC_API_VERSION >= 8 //1.5 2011/04/18
	/* $Id: miniupnpc.h,v 1.41 2015/05/22 10:23:48 nanard Exp $ */
	/* $Id: miniupnpc.h,v 1.23 2011/04/11 08:21:46 nanard Exp $ */
	//upnpDiscover(int delay, const char * multicastif,
	//             const char * minissdpdsock, int sameport,
	//             int ipv6,
	//             int * error);
	upcd->devlist = upnpDiscover(2000, NULL,
	                             NULL, 0,
	                             0,
	                             NULL);
#else
#if MINIUPNPC_API_VERSION >= 6//1.5 2011/03/14
	/* $Id: miniupnpc.h,v 1.21 2011/03/14 13:37:12 nanard Exp $ */
	//upnpDiscover(int delay, const char * multicastif,
	//             const char * minissdpdsock, int sameport,
	//             int * error);
	upcd->devlist = upnpDiscover(2000, NULL,
	                             NULL, 0,
	                             NULL);
#else
#if MINIUPNPC_API_VERSION >= -4//1.1 2008/09/25
	/* $Id: miniupnpc.h,v 1.20 2011/02/07 16:46:05 nanard Exp $ */
	/* $Id: miniupnpc.h,v 1.18 2008/09/25 18:02:50 nanard Exp $ */
	//upnpDiscover(int delay, const char * multicastif,
	//             const char * minissdpdsock, int sameport);
	upcd->devlist = upnpDiscover(2000, NULL,
	                             NULL, 0);
#else
#if MINIUPNPC_API_VERSION >= -5//1.0 2007/12/19
	/* $Id: miniupnpc.h,v 1.17 2007/12/19 14:58:54 nanard Exp $ */
	//upnpDiscover(int delay, const char * multicastif,
	//             const char * minissdpdsock);
	upcd->devlist = upnpDiscover(2000, NULL,
	                             NULL);
#else
#if MINIUPNPC_API_VERSION >= -6//1.0 2007/10/16
	/* $Id: miniupnpc.h,v 1.15 2007/10/16 15:07:32 nanard Exp $ */
	//LIBSPEC struct UPNPDev * upnpDiscover(int delay, const char * multicastif);
	upcd->devlist = upnpDiscover(2000, NULL);
#else
#if MINIUPNPC_API_VERSION >= -7//1.0 2006/09/04
	/* $Id: miniupnpc.h,v 1.14 2007/10/01 13:42:52 nanard Exp $ */
	/* $Id: miniupnpc.h,v 1.9 2006/09/04 09:30:17 nanard Exp $ */
	//struct UPNPDev * upnpDiscover(int);
	upcd->devlist = upnpDiscover(2000);
#else
#error MINIUPNPC_API_VERSION is not defined. You may define one follow miniupnpc library version
	//2006/09/04 to 2007/10/01 => -7//Start return struct UPNPDev * for upnpDiscover
	//2007/10/16 => -6 upnpDiscover
	//2007/12/19 => -5 upnpDiscover
	//2008/09/25 => -4 upnpDiscover
	//2009/04/17 => -3 UPNP_AddPortMapping
	//2010/12/09 => -2 //struct IGDdatas_service CIF;
	//2011/02/15 => -1 UPNP_AddPortMapping
	//2011/03/14 => 6 //Start of MINIUPNPC_API_VERSION
#endif//>=-7
#endif//>=-6
#endif//>=-5
#endif//>=-4
#endif//>=6
#endif//>=8
#endif//>=14

	if(upcd->devlist)
	{
		struct UPNPDev * device;
		printf("List of UPNP devices found on the network :\n");
		for(device=upcd->devlist;device;device=device->pNext)
		{
			printf("\n desc: %s\n st: %s\n",
				   device->descURL, device->st);
		}
		putchar('\n');
		if(UPNP_GetValidIGD(upcd->devlist, &(upcd->urls),
				&(upcd->data), upcd->lanaddr,
				sizeof(upcd->lanaddr)))
		{
			printf("Found valid IGD : %s\n",
					upcd->urls.controlURL);
			printf("Local LAN ip address : %s\n",
					upcd->lanaddr);

			/* MODIFY STATE */
			dataMtx.lock(); /* LOCK MUTEX */

			/* convert to ipaddress. */
			inet_aton(upcd->lanaddr, &(upnp_iaddr.sin_addr));
			upnp_iaddr.sin_port = htons(iport);

			upnpState = RS_UPNP_S_READY;
			if (upnpConfig)
			{
				delete upnpConfig;
			}
			upnpConfig = upcd;   /* */

			dataMtx.unlock(); /* UNLOCK MUTEX */


			/* done -> READY */
			return 1;

		}
		else
		{
			fprintf(stderr, "No valid UPNP Internet Gateway Device found.\n");
		}


		freeUPNPDevlist(upcd->devlist);
		upcd->devlist = 0;
	}
	else
	{
		fprintf(stderr, "No IGD UPnP Device found on the network !\n");
	}

	/* MODIFY STATE */
	dataMtx.lock(); /* LOCK MUTEX */

	upnpState = RS_UPNP_S_UNAVAILABLE;
	delete upcd;
	upnpConfig = NULL;

	dataMtx.unlock(); /* UNLOCK MUTEX */

	/* done, FAILED -> NOT AVAILABLE */

	return 0;
}

bool upnphandler::printUPnPState()
{
	std::cerr << "upnphandler::printUPnPState() ... locking";
	std::cerr << std::endl;

	RsStackMutex stack(dataMtx); /* LOCK STACK MUTEX */

	std::cerr << "upnphandler::printUPnPState() ... locked";
	std::cerr << std::endl;

	uPnPConfigData *config = upnpConfig;
	if ((upnpState >= RS_UPNP_S_READY) && (config))
	{
		DisplayInfos(&(config -> urls), &(config->data));
		GetConnectionStatus(&(config -> urls), &(config->data));
		ListRedirections(&(config -> urls), &(config->data));
	}
	else
	{
		std::cerr << "UPNP not Ready" << std::endl;
	}

	return 1;
}


bool upnphandler::checkUPnPActive()
{
	RsStackMutex stack(dataMtx); /* LOCK STACK MUTEX */

	uPnPConfigData *config = upnpConfig;
	if ((upnpState > RS_UPNP_S_READY) && (config))
	{
		char eprot1[] = "TCP";
		char eprot2[] = "UDP";

		char in_addr[256];
		char in_port1[256];
		char in_port2[256];
		char eport1[256];
		char eport2[256];

		struct sockaddr_in localAddr = upnp_iaddr;
		uint32_t linaddr = ntohl(localAddr.sin_addr.s_addr);

		snprintf(in_port1, 256, "%d", ntohs(localAddr.sin_port));
		snprintf(in_port2, 256, "%d", ntohs(localAddr.sin_port));

		snprintf(in_addr, 256, "%d.%d.%d.%d",
			((linaddr >> 24) & 0xff),
			((linaddr >> 16) & 0xff),
			((linaddr >> 8) & 0xff),
			((linaddr >> 0) & 0xff));

		snprintf(eport1, 256, "%d", eport_curr);
		snprintf(eport2, 256, "%d", eport_curr);

		std::cerr << "upnphandler::checkUPnPState()";
		std::cerr << " Checking Redirection: InAddr: " << in_addr;
		std::cerr << " InPort: " << in_port1;
		std::cerr << " ePort: " << eport1;
		std::cerr << " eProt: " << eprot1;
		std::cerr << std::endl;


		bool tcpOk = TestRedirect(&(config -> urls), &(config->data),
				in_addr, in_port1, eport1, eprot1);
		bool udpOk = TestRedirect(&(config -> urls), &(config->data),
				in_addr, in_port2, eport2, eprot2);

		if ((!tcpOk) || (!udpOk))
		{
			std::cerr << "upnphandler::checkUPnPState() ... Redirect Expired, restarting";
			std::cerr << std::endl;

			toStop = true;
			toStart = true;
		}
	}

	return true;
}

class upnpThreadData
{
	public:
		upnphandler *handler;
		bool start;
		bool stop;
};

	/* Thread routines */
extern "C" void* doSetupUPnP(void* p)
{
	upnpThreadData *data = (upnpThreadData *) p;
	if ((!data) || (!data->handler))
	{
		pthread_exit(NULL);
	}

	/* publish it! */
	if (data -> stop)
	{
		data->handler->shutdown_upnp();
	}

	if (data -> start)
	{
		data->handler->initUPnPState();
		data->handler->start_upnp();
	}

	data->handler->printUPnPState();

	delete data;
	pthread_exit(NULL);

	return NULL;
}

bool upnphandler::background_setup_upnp(bool start, bool stop)
{
	pthread_t tid;

	/* launch thread */
	upnpThreadData *data = new upnpThreadData();
	data->handler = this;
	data->start = start;
	data->stop = stop;

	if(!pthread_create(&tid, 0, &doSetupUPnP, (void *) data))
	{
		pthread_detach(tid); /* so memory is reclaimed in linux */
		return true;
	}
        else
        {
            delete data ;
            std::cerr << "(EE) Failed to start upnp thread." << std::endl;
            return false ;
        }
}

bool upnphandler::start_upnp()
{
	RsStackMutex stack(dataMtx); /* LOCK STACK MUTEX */

	uPnPConfigData *config = upnpConfig;
	if (!((upnpState >= RS_UPNP_S_READY) && (config)))
	{
		RsInfo() << __PRETTY_FUNCTION__ << " Not Ready" << std::endl;
		return false;
	}

	char eprot1[] = "TCP";
	char eprot2[] = "UDP";

	/* if we're to load -> load */
	/* select external ports */
	eport_curr = eport;
	if(!eport_curr)
	{
		/* use local port if eport is zero */
		eport_curr = iport;
		RsInfo() << __PRETTY_FUNCTION__ << " Using LocalPort for extPort"
		         << std::endl;
	}

	if (!eport_curr)
	{
		RsErr() << __PRETTY_FUNCTION__ << " Invalid eport" << std::endl;
		return false;
	}


	/* our port */
	char in_addr[256];
	char in_port1[256];
	char in_port2[256];
	char eport1[256];
	char eport2[256];

	upnp_iaddr.sin_port = htons(iport);
	struct sockaddr_in localAddr = upnp_iaddr;
	uint32_t linaddr = ntohl(localAddr.sin_addr.s_addr);

	snprintf(in_port1, 256, "%d", ntohs(localAddr.sin_port));
	snprintf(in_port2, 256, "%d", ntohs(localAddr.sin_port));
	snprintf(in_addr, 256, "%d.%d.%d.%d",
		 ((linaddr >> 24) & 0xff),
		 ((linaddr >> 16) & 0xff),
		 ((linaddr >> 8) & 0xff),
		 ((linaddr >> 0) & 0xff));

	snprintf(eport1, 256, "%d", eport_curr);
	snprintf(eport2, 256, "%d", eport_curr);

	std::cerr << "Attempting Redirection: InAddr: " << in_addr;
	std::cerr << " InPort: " << in_port1;
	std::cerr << " ePort: " << eport1;
	std::cerr << " eProt: " << eprot1;
	std::cerr << std::endl;

	if (!SetRedirectAndTest(&(config -> urls), &(config->data),
	                        in_addr, in_port1, eport1, eprot1,
	                        NULL /*leaseDuration*/, "RetroShare_TCP" /*description*/,
	                        0))
	{
		upnpState = RS_UPNP_S_TCP_FAILED;
	}
	else if (!SetRedirectAndTest(&(config -> urls), &(config->data),
	                             in_addr, in_port2, eport2, eprot2,
	                             NULL /*leaseDuration*/, "RetroShare_UDP" /*description*/,
		                         0))
	{
		upnpState = RS_UPNP_S_UDP_FAILED;
	}
	else
	{
		upnpState = RS_UPNP_S_ACTIVE;
	}


	/* now store the external address */
	char externalIPAddress[32];
	UPNP_GetExternalIPAddress(config -> urls.controlURL,
#if MINIUPNPC_API_VERSION >= -2//1.4 2010/12/09
	                          config->data.first.servicetype,
#else
#if MINIUPNPC_API_VERSION >= -7//1.0 2006/09/04
	                          config->data.servicetype,
#else
#error MINIUPNPC_API_VERSION is not defined. You may define one follow miniupnpc library version
#endif
#endif
	                          externalIPAddress);

	sockaddr_clear(&upnp_eaddr);

	if(externalIPAddress[0])
	{
		std::cerr << "Stored External address: " << externalIPAddress;
		std::cerr << ":" << eport_curr;
		std::cerr << std::endl;

		inet_aton(externalIPAddress, &(upnp_eaddr.sin_addr));
		upnp_eaddr.sin_family = AF_INET;
		upnp_eaddr.sin_port = htons(eport_curr);
	}
	else
	{
		std::cerr << "FAILED To get external Address";
		std::cerr << std::endl;
	}

	toStart = false;

	return true;

}

bool upnphandler::shutdown_upnp()
{
	RsStackMutex stack(dataMtx); /* LOCK STACK MUTEX */

	uPnPConfigData *config = upnpConfig;
	if (!((upnpState >= RS_UPNP_S_READY) && (config)))
	{
		return false;
	}

	char eprot1[] = "TCP";
	char eprot2[] = "UDP";

	/* always attempt this (unless no port number) */
	if (eport_curr > 0)
	{

		char eport1[256];
		char eport2[256];

		snprintf(eport1, 256, "%d", eport_curr);
		snprintf(eport2, 256, "%d", eport_curr);

		std::cerr << "Attempting To Remove Redirection: port: " << eport1;
		std::cerr << " Prot: " << eprot1;
		std::cerr << std::endl;

		RemoveRedirect(&(config -> urls), &(config->data),
				eport1, eprot1);


		std::cerr << "Attempting To Remove Redirection: port: " << eport2;
		std::cerr << " Prot: " << eprot2;
		std::cerr << std::endl;

		RemoveRedirect(&(config -> urls), &(config->data),
				eport2, eprot2);

		upnpState = RS_UPNP_S_READY;
		toStop = false;
	}

	return true;

}

/************************ External Interface *****************************
 *
 *
 *
 */


upnphandler::upnphandler()
	: dataMtx("upnpState"), toEnable(false), toStart(false), toStop(false),
	eport(0), eport_curr(0),
	upnpState(RS_UPNP_S_UNINITIALISED),
	upnpConfig(NULL)
{
	return;
}

upnphandler::~upnphandler()
{
		return;
}

	/* RsIface */
void  upnphandler::enable(bool active)
{
	dataMtx.lock();   /***  LOCK MUTEX  ***/

	if (active != toEnable)
	{
		if (active)
		{
			toStart = true;
		}
		else
		{
			toStop = true;
		}
	}
	toEnable = active;

	bool start = toStart;

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/

	if (start)
	{
		/* make background thread to startup UPnP */
		background_setup_upnp(true, false);
	}


}


void    upnphandler::shutdown()
{
	/* blocking call to shutdown upnp */

	shutdown_upnp();
}


void    upnphandler::restart()
{
	/* non-blocking call to shutdown upnp, and startup again. */
	background_setup_upnp(true, true);
}



bool    upnphandler::getEnabled()
{
	dataMtx.lock();   /***  LOCK MUTEX  ***/

	bool on = toEnable;

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/

	return on;
}

bool    upnphandler::getActive()
{
	dataMtx.lock();   /***  LOCK MUTEX  ***/

	bool on = (upnpState == RS_UPNP_S_ACTIVE);

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/

	return on;
}

	/* the address that the listening port is on */
void    upnphandler::setInternalPort(unsigned short iport_in)
{
//	std::cerr << "UPnPHandler::setInternalAddress() pre Lock!" << std::endl;
	dataMtx.lock();   /***  LOCK MUTEX  ***/
//	std::cerr << "UPnPHandler::setInternalAddress() postLock!" << std::endl;

	std::cerr << "UPnPHandler::setInternalPort(" << iport_in << ") current port: ";
	std::cerr << iport << std::endl;

	if (iport != iport_in)
	{
		iport = iport_in;
		if ((toEnable) &&
			(upnpState == RS_UPNP_S_ACTIVE))
		{
			toStop  = true;
			toStart = true;
		}
	}
	dataMtx.unlock(); /*** UNLOCK MUTEX ***/
}

void    upnphandler::setExternalPort(unsigned short eport_in)
{
//	std::cerr << "UPnPHandler::getExternalPort() pre Lock!" << std::endl;
	dataMtx.lock();   /***  LOCK MUTEX  ***/
//	std::cerr << "UPnPHandler::getExternalPort() postLock!" << std::endl;

	std::cerr << "UPnPHandler::setExternalPort(" << eport_in << ") current port: ";
	std::cerr << eport << std::endl;

	/* flag both shutdown/start -> for restart */
	if (eport != eport_in)
	{
		eport = eport_in;
		if ((toEnable) &&
			(upnpState == RS_UPNP_S_ACTIVE))
		{
			toStop  = true;
			toStart = true;
		}
	}

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/
}

/* as determined by uPnP */
bool upnphandler::getInternalAddress(struct sockaddr_storage &addr)
{
	RS_STACK_MUTEX(dataMtx);

	// copy to universal addr.
	sockaddr_storage_clear(addr);
	sockaddr_storage_setipv4(addr, &upnp_iaddr);

	bool valid = (upnpState >= RS_UPNP_S_ACTIVE);

	Dbg2() << __PRETTY_FUNCTION__ << " valid: " << valid
	       << " addr: " << addr << std::endl;

	return valid;
}

bool upnphandler::getExternalAddress(sockaddr_storage &addr)
{
	RS_STACK_MUTEX(dataMtx);

	// copy to universal addr.
	sockaddr_storage_clear(addr);
	sockaddr_storage_setipv4(addr, &upnp_eaddr);

	bool valid = (upnpState == RS_UPNP_S_ACTIVE);

	Dbg2() << __PRETTY_FUNCTION__ << " valid: " << valid
	       << " addr: " << addr << std::endl;

	return valid;
}

