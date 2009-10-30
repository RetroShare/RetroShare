
/* This stuff is actually C */

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef  __cplusplus
} /* extern C */
#endif
/* This stuff is actually C */

#include "upnp/upnphandler.h"

#include "util/rsnet.h"

bool upnphandler::initUPnPState()
{
	std::cerr << "upnphandler::initUPnPState" << std::endl;
	cUPnPControlPoint = new CUPnPControlPoint(2000);

	bool IGWDetected = cUPnPControlPoint->GetIGWDeviceDetected();

	if (IGWDetected) {
	    upnpState = RS_UPNP_S_READY;
	    std::cerr << "upnphandler::initUPnPState READY" << std::endl;

	    /* MODIFY STATE */
	    dataMtx.lock(); /* LOCK MUTEX */
	    std::cerr << "upnphandler::initUPnPState cUPnPControlPoint internal ip adress : ";
	    std::cerr << cUPnPControlPoint->getInternalIpAddress() << std::endl;

	    //const char ipaddr = cUPnPControlPoint->getInternalIpAddress().c_str();
	    inet_aton(cUPnPControlPoint->getInternalIpAddress(), &(upnp_iaddr.sin_addr));
	    upnp_iaddr.sin_port = htons(iport);


	    dataMtx.unlock(); /* UNLOCK MUTEX */

	} else {
	    upnpState = RS_UPNP_S_UNAVAILABLE;
	    std::cerr << "upnphandler::initUPnPState UNAVAILABLE" << std::endl;
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
	std::cerr << "doSetupUPnP Creating upnp thread." << std::endl;
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
	pthread_t tid;

	/* launch thread */
	std::cerr << "background_setup_upnp Creating upnp thread." << std::endl;
	upnpThreadData *data = new upnpThreadData();
	data->handler = this;
	data->start = start;
	data->stop = stop;

	pthread_create(&tid, 0, &doSetupUPnP, (void *) data);
	pthread_detach(tid); /* so memory is reclaimed in linux */

	return true;
}

bool upnphandler::start_upnp()
{
	if (!(upnpState >= RS_UPNP_S_READY))
	{
		std::cerr << "upnphandler::start_upnp() Not Ready" << std::endl;
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
		    std::cerr << "Using LocalPort for extPort!";
		    std::cerr << std::endl;
	    }

	    if (!eport_curr)
	    {
		    std::cerr << "Invalid eport ... ";
		    std::cerr << std::endl;
		    return false;
	    }


	    /* our port */
	    char in_addr[256];
	    char in_port1[256];
	    char eport1[256];

	    upnp_iaddr.sin_port = htons(iport);
	    localAddr = upnp_iaddr;
	    uint32_t linaddr = ntohl(localAddr.sin_addr.s_addr);

	    snprintf(in_port1, 256, "%d", ntohs(localAddr.sin_port));
	    snprintf(in_addr, 256, "%d.%d.%d.%d",
		     ((linaddr >> 24) & 0xff),
		     ((linaddr >> 16) & 0xff),
		     ((linaddr >> 8) & 0xff),
		     ((linaddr >> 0) & 0xff));

	    snprintf(eport1, 256, "%d", eport_curr);

	    std::cerr << "Attempting Redirection: InAddr: " << in_addr;
	    std::cerr << " InPort: " << in_port1;
	    std::cerr << " ePort: " << eport1;
	    std::cerr << " eProt: " << "TCP";
	    std::cerr << std::endl;
	}

	//build port mapping config
	std::vector<CUPnPPortMapping> upnpPortMapping1;
	CUPnPPortMapping cUPnPPortMapping1 = CUPnPPortMapping(eport_curr, ntohs(localAddr.sin_port), "TCP", true, "tcp retroshare redirection");
	upnpPortMapping1.push_back(cUPnPPortMapping1);
	bool res = cUPnPControlPoint->AddPortMappings(upnpPortMapping1);

	std::vector<CUPnPPortMapping> upnpPortMapping2;
	CUPnPPortMapping cUPnPPortMapping2 = CUPnPPortMapping(eport_curr, ntohs(localAddr.sin_port), "UDP", true, "udp retroshare redirection");
	upnpPortMapping2.push_back(cUPnPPortMapping2);
	bool res2 = cUPnPControlPoint->AddPortMappings(upnpPortMapping2);

	struct sockaddr_in extAddr;
	bool extAddrResult = getExternalAddress(extAddr);

	{
		RsStackMutex stack(dataMtx); /* LOCK STACK MUTEX */

		if (extAddrResult && (res || res2)) {
		    upnpState = RS_UPNP_S_ACTIVE;
		} else {
		    upnpState = RS_UPNP_S_UDP_FAILED;
		}

		toStart = false;
	}

	return (upnpState == RS_UPNP_S_ACTIVE);

}

bool upnphandler::shutdown_upnp()
{
	RsStackMutex stack(dataMtx); /* LOCK STACK MUTEX */

	//stopping os ok, set starting to true for next net reset
	toStop = false;
	toStart = true;
	upnpState = RS_UPNP_S_UNINITIALISED;

	/* always attempt this (unless no port number) */
	if (eport_curr > 0 && eport > 0 && (upnpState >= RS_UPNP_S_ACTIVE))
	{
		std::cerr << "upnphandler::shutdown_upnp() : Attempting To Remove Redirection: port: " << eport_curr;
		std::cerr << " Prot: TCP";
		std::cerr << std::endl;

		std::vector<CUPnPPortMapping> upnpPortMapping1;
		CUPnPPortMapping cUPnPPortMapping1 = CUPnPPortMapping(eport_curr, 0, "TCP", true, "tcp redirection");
		upnpPortMapping1.push_back(cUPnPPortMapping1);
		cUPnPControlPoint->DeletePortMappings(upnpPortMapping1);

		std::cerr << " : Attempting To Remove Redirection: port: " << eport_curr;
		std::cerr << " Prot: UDP";
		std::cerr << std::endl;

		std::vector<CUPnPPortMapping> upnpPortMapping2;
		CUPnPPortMapping cUPnPPortMapping2 = CUPnPPortMapping(eport_curr, 0, "UDP", true, "udp redirection");
		upnpPortMapping2.push_back(cUPnPPortMapping2);
		cUPnPControlPoint->DeletePortMappings(upnpPortMapping2);

		//destroy the upnp object
		cUPnPControlPoint->~CUPnPControlPoint();
		upnpState = RS_UPNP_S_UNINITIALISED;
	} else {
    	    #ifdef CONN_DEBUG
		    std::cerr << "upnphandler::shutdown_upnp() : avoid upnp connection for shutdows A net flag went down." << std::endl;
	    #endif
	}

	return true;

}

/************************ External Interface *****************************
 *
 *
 *
 */

upnphandler::upnphandler()
	:toEnable(false), toStart(false), toStop(false),
	eport(0), eport_curr(0)
{
}

upnphandler::~upnphandler()
{
		return;
}

	/* RsIface */
void  upnphandler::enable(bool active)
{
	std::cerr << "upnphandler::enable called with argument active : " << active << std::endl;
	std::cerr << "toEnable : " << toEnable << std::endl;
	std::cerr << "toStart : " << toStart << std::endl;
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
	std::cerr << "upnphandler::restart() called." << std::endl;
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

	std::cerr <<"GetActive Called result : " << (upnpState == RS_UPNP_S_ACTIVE) << std::endl;

	bool on = (upnpState == RS_UPNP_S_ACTIVE);

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/

	return on;
}

	/* the address that the listening port is on */
void    upnphandler::setInternalPort(unsigned short iport_in)
{
	std::cerr << "UPnPHandler::setInternalAddress() pre Lock!" << std::endl;
	dataMtx.lock();   /***  LOCK MUTEX  ***/
	std::cerr << "UPnPHandler::setInternalAddress() postLock!" << std::endl;

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
	std::cerr << "UPnPHandler::setExternalPort() pre Lock!" << std::endl;
	dataMtx.lock();   /***  LOCK MUTEX  ***/
	std::cerr << "UPnPHandler::setExternalPort() postLock!" << std::endl;

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
bool    upnphandler::getInternalAddress(struct sockaddr_in &addr)
{
//	std::cerr << "UPnPHandler::getInternalAddress() pre Lock!" << std::endl;
	dataMtx.lock();   /***  LOCK MUTEX  ***/
//	std::cerr << "UPnPHandler::getInternalAddress() postLock!" << std::endl;

	std::cerr << "UPnPHandler::getInternalAddress()" << std::endl;

	addr = upnp_iaddr;
	bool valid = (upnpState >= RS_UPNP_S_ACTIVE);

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/

	return valid;
}

bool    upnphandler::getExternalAddress(struct sockaddr_in &addr)
{
	std::string externalAdress = cUPnPControlPoint->getExternalAddress();

	if(!externalAdress.empty())
	{
		const char* externalIPAddress = externalAdress.c_str();

		std::cerr << "Stored External address: " << externalIPAddress;
		std::cerr << ":" << eport_curr;
		std::cerr << std::endl;

		dataMtx.lock();   /***  LOCK MUTEX  ***/
		sockaddr_clear(&upnp_eaddr);
		inet_aton(externalIPAddress, &(upnp_eaddr.sin_addr));
		upnp_eaddr.sin_family = AF_INET;
		upnp_eaddr.sin_port = htons(eport_curr);
		dataMtx.unlock(); /*** UNLOCK MUTEX ***/

		addr = upnp_eaddr;
		return true;
	}
	else
	{
		return false;
	}
}
