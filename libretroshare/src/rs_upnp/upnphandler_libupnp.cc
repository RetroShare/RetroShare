/*******************************************************************************
 * libretroshare/src/upnp: upnphandler_libupnp.cc                                *
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

#include "util/rsdebuglevel2.h"

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef  __cplusplus
} /* extern C */
#endif
/* This stuff is actually C */

#include "rs_upnp/upnphandler_libupnp.h"

#include "util/rsnet.h"

bool upnphandler::initUPnPState()
{
	#ifdef UPNP_DEBUG
	std::cerr << "upnphandler::initUPnPState" << std::endl;
	#endif
	cUPnPControlPoint = new CUPnPControlPoint(2000);

	bool IGWDetected = cUPnPControlPoint->GetIGWDeviceDetected();

	if (IGWDetected) {
	    /* MODIFY STATE */
	    dataMtx.lock(); /* LOCK MUTEX */
	    upnpState = RS_UPNP_S_READY;

	    #ifdef UPNP_DEBUG
	    std::cerr << "upnphandler::initUPnPState cUPnPControlPoint internal ip adress : ";
	    std::cerr << cUPnPControlPoint->getInternalIpAddress() << std::endl;
	    #endif

		const char* addrStr = cUPnPControlPoint->getInternalIpAddress();
		inet_aton(addrStr ? addrStr : "127.0.0.1", &(upnp_iaddr.sin_addr));
		upnp_iaddr.sin_port = htons(iport);

	    #ifdef UPNP_DEBUG
	    std::cerr << "upnphandler::initUPnPState READY" << std::endl;
	    #endif
	    dataMtx.unlock(); /* UNLOCK MUTEX */

	} else {
	    upnpState = RS_UPNP_S_UNAVAILABLE;
	    #ifdef UPNP_DEBUG
	    std::cerr << "upnphandler::initUPnPState UNAVAILABLE" << std::endl;
	    #endif
	}

	return 0;
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
	#ifdef UPNP_DEBUG
	std::cerr << "doSetupUPnP Creating upnp thread." << std::endl;
	#endif
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

        delete data;
        pthread_exit(NULL);

        return NULL;
}

bool upnphandler::background_setup_upnp(bool start, bool stop)
{
	RS_DBG1("start: ", start, " stop: ", stop);
	print_stacktrace();

	pthread_t tid;

	/* launch thread */
	upnpThreadData *data = new upnpThreadData();
	data->handler = this;
	data->start = start;
	data->stop = stop;

    if(! pthread_create(&tid, 0, &doSetupUPnP, (void *) data))
    {
	pthread_detach(tid); /* so memory is reclaimed in linux */

	return true;
    }
    else
    {
        delete data ;
        std::cerr << "(EE) Could not start background upnp thread!" << std::endl;
        return false ;
    }
}

bool upnphandler::start_upnp()
{
	if (!(upnpState >= RS_UPNP_S_READY))
	{
		#ifdef UPNP_DEBUG
		std::cerr << "upnphandler::start_upnp() Not Ready" << std::endl;
		#endif
		return false;
	}

	struct sockaddr_in localAddr;
	{
	    RsStackMutex stack(dataMtx); /* LOCK STACK MUTEX */

	    /* if we're to load -> load */
	    /* select external ports */
	    eport_curr = eport;
	    if (!eport_curr)
	    {
		    /* use local port if eport is zero */
		    eport_curr = iport;
		    #ifdef UPNP_DEBUG
		    std::cerr << "upnphandler::start_upnp() Using LocalPort for extPort." << std::endl;
		    #endif
	    }

	    if (!eport_curr)
	    {
		    #ifdef UPNP_DEBUG
		    std::cerr << "upnphandler::start_upnp() Invalid eport ... " << std::endl;
		    #endif
		    return false;
	    }

	    /* our port */
	    char in_addr[256];
	    char in_port1[256];

	    upnp_iaddr.sin_port = htons(iport);
	    localAddr = upnp_iaddr;
	    uint32_t linaddr = ntohl(localAddr.sin_addr.s_addr);
	    snprintf(in_port1, 256, "%d", ntohs(localAddr.sin_port));
	    snprintf(in_addr, 256, "%d.%d.%d.%d",
		     ((linaddr >> 24) & 0xff),
		     ((linaddr >> 16) & 0xff),
		     ((linaddr >> 8) & 0xff),
		     ((linaddr >> 0) & 0xff));

	    #ifdef UPNP_DEBUG
	    std::cerr << "Attempting Redirection: InAddr: " << in_addr;
	    std::cerr << " InPort: " << in_port1;
	    std::cerr << " ePort: " << eport_curr;
	    std::cerr << " eProt: " << "TCP and UDP";
	    std::cerr << std::endl;
	    #endif
	}

	//first of all, build the mappings
	std::vector<CUPnPPortMapping> upnpPortMapping1;
	CUPnPPortMapping cUPnPPortMapping1 = CUPnPPortMapping(eport_curr, ntohs(localAddr.sin_port), "TCP", true, "tcp retroshare redirection");
	upnpPortMapping1.push_back(cUPnPPortMapping1);
	std::vector<CUPnPPortMapping> upnpPortMapping2;
	CUPnPPortMapping cUPnPPortMapping2 = CUPnPPortMapping(eport_curr, ntohs(localAddr.sin_port), "UDP", true, "udp retroshare redirection");
	upnpPortMapping2.push_back(cUPnPPortMapping2);

	//attempt to remove formal port redirection rules
	cUPnPControlPoint->DeletePortMappings(upnpPortMapping1);
	cUPnPControlPoint->DeletePortMappings(upnpPortMapping2);

	//add new rules
	bool res = cUPnPControlPoint->RequestPortsForwarding(upnpPortMapping1);
	bool res2 = cUPnPControlPoint->RequestPortsForwarding(upnpPortMapping2);

	struct sockaddr_storage extAddr;
	bool extAddrResult = getExternalAddress(extAddr);

	{
		RsStackMutex stack(dataMtx); /* LOCK STACK MUTEX */

		if (extAddrResult && (res || res2)) {
		    upnpState = RS_UPNP_S_ACTIVE;
		} else {
		    upnpState = RS_UPNP_S_TCP_AND_FAILED;
		}

		toStart = false;
	}

	return (upnpState == RS_UPNP_S_ACTIVE);

}

bool upnphandler::shutdown_upnp()
{
	RsStackMutex stack(dataMtx); /* LOCK STACK MUTEX */

	/* always attempt this (unless no port number) */
	if (eport_curr > 0 && eport > 0 && (upnpState >= RS_UPNP_S_ACTIVE))
	{
		#ifdef UPNP_DEBUG
		std::cerr << "upnphandler::shutdown_upnp() : Attempting To Remove Redirection: port: " << eport_curr;
		std::cerr << " Prot: TCP";
		std::cerr << std::endl;
		#endif

		std::vector<CUPnPPortMapping> upnpPortMapping1;
		CUPnPPortMapping cUPnPPortMapping1 = CUPnPPortMapping(eport_curr, 0, "TCP", true, "tcp redirection");
		upnpPortMapping1.push_back(cUPnPPortMapping1);
		cUPnPControlPoint->DeletePortMappings(upnpPortMapping1);

		#ifdef UPNP_DEBUG
		std::cerr << "upnphandler::shutdown_upnp()  : Attempting To Remove Redirection: port: " << eport_curr;
		std::cerr << " Prot: UDP";
		std::cerr << std::endl;
		#endif

		std::vector<CUPnPPortMapping> upnpPortMapping2;
		CUPnPPortMapping cUPnPPortMapping2 = CUPnPPortMapping(eport_curr, 0, "UDP", true, "udp redirection");
		upnpPortMapping2.push_back(cUPnPPortMapping2);
		cUPnPControlPoint->DeletePortMappings(upnpPortMapping2);

		//destroy the upnp object
		cUPnPControlPoint->~CUPnPControlPoint();
        cUPnPControlPoint=NULL ;
    } else {
    	    #ifdef UPNP_DEBUG
		    std::cerr << "upnphandler::shutdown_upnp() : avoid upnp connection for shutdonws because probably a net flag went down." << std::endl;
	    #endif
	}

	//stopping os ok, set starting to true for next net reset
	toStop = false;
	toStart = true;
	upnpState = RS_UPNP_S_UNINITIALISED;

	return true;

}

/************************ External Interface *****************************
 *
 *
 *
 */



	/* RsIface */
void  upnphandler::enable(bool active)
{
    	#ifdef UPNP_DEBUG
	    std::cerr << "upnphandler::enable called with argument active : " << active << std::endl;
	    std::cerr << "toEnable : " << toEnable << std::endl;
	    std::cerr << "toStart : " << toStart << std::endl;
	#endif

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

	#ifdef UPNP_DEBUG
	std::cerr << "upnphandler::shutdown() called." << std::endl;
	#endif
	shutdown_upnp();
}


void    upnphandler::restart()
{
	/* non-blocking call to shutdown upnp, and startup again. */
        #ifdef UPNP_DEBUG
	std::cerr << "upnphandler::restart() called." << std::endl;
        #endif
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

    	#ifdef UPNP_DEBUG
	    std::cerr <<"upnphandler::getActive() result : " << (upnpState == RS_UPNP_S_ACTIVE) << std::endl;
	#endif

	bool on = (upnpState == RS_UPNP_S_ACTIVE);

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/

	return on;
}

	/* the address that the listening port is on */
void    upnphandler::setInternalPort(unsigned short iport_in)
{
	dataMtx.lock();   /***  LOCK MUTEX  ***/
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
	dataMtx.lock();   /***  LOCK MUTEX  ***/
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
bool    upnphandler::getInternalAddress(struct sockaddr_storage &addr)
{
	dataMtx.lock();   /***  LOCK MUTEX  ***/

	bool valid = (upnpState >= RS_UPNP_S_ACTIVE);

	// copy to universal addr.
	sockaddr_storage_clear(addr);
	sockaddr_storage_setipv4(addr, &upnp_iaddr);

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/

	return valid;
}

bool    upnphandler::getExternalAddress(struct sockaddr_storage &addr)
{
	std::string externalAdress = cUPnPControlPoint->getExternalAddress();

        if(!externalAdress.empty() && externalAdress != "")
	{
		const char* externalIPAddress = externalAdress.c_str();

		#ifdef UPNP_DEBUG
		std::cerr << " upnphandler::getExternalAddress() : " << externalIPAddress;
		std::cerr << ":" << eport_curr;
		std::cerr << std::endl;
		#endif

		dataMtx.lock();   /***  LOCK MUTEX  ***/
		sockaddr_clear(&upnp_eaddr);
		inet_aton(externalIPAddress, &(upnp_eaddr.sin_addr));
		upnp_eaddr.sin_family = AF_INET;
		upnp_eaddr.sin_port = htons(eport_curr);

		// copy to universal addr.
		sockaddr_storage_clear(addr);
		sockaddr_storage_setipv4(addr, &upnp_eaddr);

		dataMtx.unlock(); /*** UNLOCK MUTEX ***/

		return true;
	}
	else
	{
		return false;
	}
}
