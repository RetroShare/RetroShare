
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

	upcd->devlist = upnpDiscover(2000, NULL, NULL);
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

	upnpState = RS_UPNP_S_UNAVAILABLE;
	delete upcd;
	upnpConfig = NULL;

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
void  upnphandler::enableUPnP(bool active)
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


void    upnphandler::shutdownUPnP()
{
	/* blocking call to shutdown upnp */

	shutdown_upnp();
}


void    upnphandler::restartUPnP()
{
	/* non-blocking call to shutdown upnp, and startup again. */
	background_setup_upnp(true, true);
}



bool    upnphandler::getUPnPEnabled()
{
	dataMtx.lock();   /***  LOCK MUTEX  ***/

	bool on = toEnable;

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/

	return on;
}

bool    upnphandler::getUPnPActive()
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


