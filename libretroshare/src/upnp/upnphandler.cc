//Linux and macos implementation
#ifndef WINDOWS_SYS

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
	#ifdef UPNP_DEBUG
	std::cerr << "upnphandler::initUPnPState" << std::endl;
	#endif
	cUPnPControlPoint = new CUPnPControlPoint(2000);

	bool IGWDetected = cUPnPControlPoint->GetIGWDeviceDetected();

	if (IGWDetected) {
	    /* MODIFY STATE */
	    dataMtx.lock(); /* LOCK MUTEX */
	    upnpState = RS_UPNP_S_READY;

	    std::cerr << "upnphandler::initUPnPState cUPnPControlPoint internal ip adress : ";
	    std::cerr << cUPnPControlPoint->getInternalIpAddress() << std::endl;

	    //const char ipaddr = cUPnPControlPoint->getInternalIpAddress().c_str();
	    inet_aton(cUPnPControlPoint->getInternalIpAddress(), &(upnp_iaddr.sin_addr));
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
	pthread_t tid;

	/* launch thread */
	#ifdef UPNP_DEBUG
	std::cerr << "background_setup_upnp Creating upnp thread." << std::endl;
	#endif
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
		    upnpState = RS_UPNP_S_TCP_AND_FAILED;
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
		upnpState = RS_UPNP_S_UNINITIALISED;
	} else {
    	    #ifdef UPNP_DEBUG
		    std::cerr << "upnphandler::shutdown_upnp() : avoid upnp connection for shutdonws because probably a net flag went down." << std::endl;
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
bool    upnphandler::getInternalAddress(struct sockaddr_in &addr)
{
	dataMtx.lock();   /***  LOCK MUTEX  ***/
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
		dataMtx.unlock(); /*** UNLOCK MUTEX ***/

		addr = upnp_eaddr;
		return true;
	}
	else
	{
		return false;
	}
}
#endif




#ifdef WINDOWS_SYS

/* This stuff is actually C */

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef  __cplusplus
} /* extern C */
#endif
/* This stuff is actually C */

#include "upnp/upnphandler.h"
#include "upnp/upnputil.h"

class uPnPConfigData
{
	public:
		struct UPNPDev * devlist;
		struct UPNPUrls urls;
		struct IGDdatas data;
		char lanaddr[16];	/* my ip address on the LAN */
};

#include <iostream>
#include <sstream>

#include "util/rsnet.h"

bool upnphandler::initUPnPState()
{
	/* allocate memory */
	uPnPConfigData *upcd = new uPnPConfigData;

#if MINIUPNPC_VERSION >= 11
	/* Starting from version 1.1, miniupnpc api has a new parameter (int sameport) */
	upcd->devlist = upnpDiscover(2000, NULL, NULL, 0);
#else
	upcd->devlist = upnpDiscover(2000, NULL, NULL);
#endif

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

	pthread_create(&tid, 0, &doSetupUPnP, (void *) data);
	pthread_detach(tid); /* so memory is reclaimed in linux */

	return true;
}

bool upnphandler::start_upnp()
{
	RsStackMutex stack(dataMtx); /* LOCK STACK MUTEX */

	uPnPConfigData *config = upnpConfig;
	if (!((upnpState >= RS_UPNP_S_READY) && (config)))
	{
		std::cerr << "upnphandler::start_upnp() Not Ready";
		std::cerr << std::endl;
		return false;
	}

	char eprot1[] = "TCP";
	char eprot2[] = "UDP";

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
			in_addr, in_port1, eport1, eprot1))
	{
		upnpState = RS_UPNP_S_TCP_FAILED;
	}
	else if (!SetRedirectAndTest(&(config -> urls), &(config->data),
			in_addr, in_port2, eport2, eprot2))
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
				  config->data.servicetype,
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
	:toEnable(false), toStart(false), toStop(false),
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
//	std::cerr << "UPnPHandler::getExternalAddress() pre Lock!" << std::endl;
	dataMtx.lock();   /***  LOCK MUTEX  ***/
//	std::cerr << "UPnPHandler::getExternalAddress() postLock!" << std::endl;

	std::cerr << "UPnPHandler::getExternalAddress()" << std::endl;
	addr = upnp_eaddr;
	bool valid = (upnpState == RS_UPNP_S_ACTIVE);

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/

	return valid;
}

#endif
