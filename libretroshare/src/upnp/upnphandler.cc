
/* This stuff is actually C */

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef  __cplusplus
} /* extern C */
#endif
/* This stuff is actually C */

#include "upnp/upnputil.h"
#include "upnp/upnphandler.h"

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


void upnphandler::run()
{

	/* infinite loop */
	while(1)
	{
		std::cerr << "UPnPHandler::Run()" << std::endl;
		int allowedSleep = 30; /* check every 30 seconds */

		/* lock it up */
		dataMtx.lock(); /* LOCK MUTEX */

		bool shutdown = toShutdown;
		int state = upnpState;

		dataMtx.unlock(); /* UNLOCK MUTEX */

		if (shutdown)
		{
			return;
		}

		/* do the work! */
		checkUPnPState();

		/* check new state for sleep period */

		dataMtx.lock(); /* LOCK MUTEX */

		state = upnpState;

		dataMtx.unlock(); /* UNLOCK MUTEX */


		/* state machine */
		switch(state)
		{
			case RS_UPNP_S_UNINITIALISED:
			case RS_UPNP_S_UNAVAILABLE:
				/* failed ... try again in 30 min. */
				allowedSleep = 1800;
			break;

			case RS_UPNP_S_READY:
			case RS_UPNP_S_TCP_FAILED: 
			case RS_UPNP_S_UDP_FAILED:
			case RS_UPNP_S_ACTIVE:
				/* working ... normal 15 seconds */
				allowedSleep = 15;
			break;

			default:
				/* default??? how did it get here? */
			break;
		}

		std::cerr << "UPnPHandler::Run() sleeping for:" << allowedSleep << std::endl;
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
		sleep(allowedSleep); 
#else
		Sleep(1000 * allowedSleep); 
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
	}
	return;
}

void upnphandler::checkUPnPState()
{
	dataMtx.lock(); /* LOCK MUTEX */

	int state = upnpState;

	dataMtx.unlock(); /* UNLOCK MUTEX */

	/* state machine */
	switch(state)
	{
		case RS_UPNP_S_UNINITIALISED:
		case RS_UPNP_S_UNAVAILABLE:
			initUPnPState();
		break;

		case RS_UPNP_S_READY:
		case RS_UPNP_S_TCP_FAILED: 
		case RS_UPNP_S_UDP_FAILED:
		case RS_UPNP_S_ACTIVE:
			printUPnPState();
			updateUPnP();
		break;

	}

	return;
}


bool upnphandler::initUPnPState()
{
	/* allocate memory */
	uPnPConfigData *upcd = new uPnPConfigData;

	upcd->devlist = upnpDiscover(2000);
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

	dataMtx.lock(); /* LOCK MUTEX */

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

	dataMtx.unlock(); /* UNLOCK MUTEX */

	return 1;
}



bool upnphandler::updateUPnP()
{
	dataMtx.lock(); /* LOCK MUTEX */


	uPnPConfigData *config = upnpConfig;
	if (!((upnpState >= RS_UPNP_S_READY) && (config)))
	{
		return false;
	}

	char eprot1[] = "TCP";
	char eprot2[] = "UDP";

	/* if we're to unload -> unload */
	if ((toStop) && (eport_curr > 0))
	{
		toStop = false;

		char eport1[256];
		char eport2[256];

		snprintf(eport1, 256, "%d", eport_curr);
		snprintf(eport2, 256, "%d", eport_curr + 1);

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
	}


	/* if we're to load -> load */
	if (toStart)
	{
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

		toStart = false;

		/* our port */
		char in_addr[256];
		char in_port1[256];
		char in_port2[256];
		char eport1[256];
		char eport2[256];

		upnp_iaddr.sin_port = htons(iport);
		struct sockaddr_in localAddr = upnp_iaddr;

		snprintf(in_port1, 256, "%d", ntohs(localAddr.sin_port));
		snprintf(in_port2, 256, "%d", ntohs(localAddr.sin_port) + 1);
		snprintf(in_addr, 256, "%d.%d.%d.%d", 
			 ((localAddr.sin_addr.s_addr >> 0) & 0xff),
			 ((localAddr.sin_addr.s_addr >> 8) & 0xff),
			 ((localAddr.sin_addr.s_addr >> 16) & 0xff),
			 ((localAddr.sin_addr.s_addr >> 24) & 0xff));

		snprintf(eport1, 256, "%d", eport_curr);
		snprintf(eport2, 256, "%d", eport_curr + 1);

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
	}

	dataMtx.unlock(); /* UNLOCK MUTEX */


	return true;

}


/************************ External Interface *****************************
 *
 *
 *
 */

upnphandler::upnphandler()
	:toShutdown(false), toEnable(false), 
	toStart(false), toStop(false),
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

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/
}

void    upnphandler::shutdownUPnP()
{
        dataMtx.lock();   /***  LOCK MUTEX  ***/

        toShutdown = true;

        dataMtx.unlock(); /*** UNLOCK MUTEX ***/
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

	addr = upnp_iaddr;
	bool valid = (upnpState >= RS_UPNP_S_READY);

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/

	return valid;
}

bool    upnphandler::getExternalAddress(struct sockaddr_in &addr)
{
//	std::cerr << "UPnPHandler::getExternalAddress() pre Lock!" << std::endl;
	dataMtx.lock();   /***  LOCK MUTEX  ***/
//	std::cerr << "UPnPHandler::getExternalAddress() postLock!" << std::endl;

	addr = upnp_eaddr;
	bool valid = (upnpState >= RS_UPNP_S_READY);

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/

	return valid;
}


