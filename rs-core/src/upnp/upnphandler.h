#ifndef _RS_UPNP_IFACE_H
#define _RS_UPNP_IFACE_H

#include <string.h>

#include "util/rsthreads.h"
#include <string>
#include <map>

/* platform independent networking... */
#include "pqi/pqinetwork.h"
#include "pqi/pqiaddrstore.h"

class upnpentry
{
	public:
	std::string name;
	std::string id;
	struct sockaddr_in addr;
	unsigned int flags;
	int status;
	int lastTs;
};

class upnpforward
{
	public:
	std::string name;
	unsigned int flags;
	struct sockaddr_in iaddr;
	struct sockaddr_in eaddr;
	int status;
	int lastTs;
};

#define RS_UPNP_S_UNINITIALISED  0
#define RS_UPNP_S_UNAVAILABLE    1
#define RS_UPNP_S_READY          2
#define RS_UPNP_S_TCP_FAILED     3
#define RS_UPNP_S_UDP_FAILED     4
#define RS_UPNP_S_ACTIVE         5

class uPnPConfigData;

class upnphandler: public RsThread
{
	public:

	upnphandler()
	:toShutdown(false), toEnable(false), 
	toStart(false), toStop(false),
	eport(0), eport_curr(0),
	upnpState(RS_UPNP_S_UNINITIALISED), 
	upnpConfig(NULL)

	{
		return;
	}

	~upnphandler()
	{
		return;
	}

	/* RsIface */
void    enableUPnP(bool active)
{
	dataMtx.lock();   /***  LOCK MUTEX  ***/

	toEnable = active;

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/
}

	/* RsIface */
void    shutdownUPnP()
{
	dataMtx.lock();   /***  LOCK MUTEX  ***/

	toShutdown = true;

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/
}

void    setupUPnPForwarding()
{
	dataMtx.lock();   /***  LOCK MUTEX  ***/

	toStart = true;

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/
}

void    shutdownUPnPForwarding()
{
	dataMtx.lock();   /***  LOCK MUTEX  ***/

	toStop = true;

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/
}


	/* the address that the listening port is on */
void    setInternalAddress(struct sockaddr_in iaddr_in)
{
//	std::cerr << "UPnPHandler::setInternalAddress() pre Lock!" << std::endl;
	dataMtx.lock();   /***  LOCK MUTEX  ***/
//	std::cerr << "UPnPHandler::setInternalAddress() postLock!" << std::endl;

	if ((iaddr.sin_addr.s_addr != iaddr_in.sin_addr.s_addr) ||
	    (iaddr.sin_port != iaddr_in.sin_port))
	{
		iaddr = iaddr_in;
		if (toEnable)
		{
			toStop  = true;
			toStart = true;
		}
	}

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/
}

void    setExternalPort(unsigned short eport_in)
{
//	std::cerr << "UPnPHandler::getExternalPort() pre Lock!" << std::endl;
	dataMtx.lock();   /***  LOCK MUTEX  ***/
//	std::cerr << "UPnPHandler::getExternalPort() postLock!" << std::endl;

	/* flag both shutdown/start -> for restart */
	if (eport != eport_in)
	{
		eport = eport_in;
		if (toEnable)
		{
			toStop  = true;
			toStart = true;
		}
	}

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/
}

	/* as determined by uPnP */
bool    getInternalAddress(struct sockaddr_in &addr)
{
//	std::cerr << "UPnPHandler::getInternalAddress() pre Lock!" << std::endl;
	dataMtx.lock();   /***  LOCK MUTEX  ***/
//	std::cerr << "UPnPHandler::getInternalAddress() postLock!" << std::endl;

	addr = upnp_iaddr;
	bool valid = (upnpState >= RS_UPNP_S_READY);

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/

	return valid;
}

bool    getExternalAddress(struct sockaddr_in &addr)
{
//	std::cerr << "UPnPHandler::getExternalAddress() pre Lock!" << std::endl;
	dataMtx.lock();   /***  LOCK MUTEX  ***/
//	std::cerr << "UPnPHandler::getExternalAddress() postLock!" << std::endl;

	addr = upnp_eaddr;
	bool valid = (upnpState >= RS_UPNP_S_READY);

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/

	return valid;
}

int    getUPnPStatus(upnpentry &ent)
{
//	std::cerr << "UPnPHandler::getUPnPStatus() pre Lock!" << std::endl;
	dataMtx.lock();   /***  LOCK MUTEX  ***/
//	std::cerr << "UPnPHandler::getUPnPStatus() postLock!" << std::endl;

	/* TODO - define data structure first */
	int state = upnpState;

	dataMtx.unlock(); /*** UNLOCK MUTEX ***/

	return state;
}


int 	init();
int 	shutdown();
int	print();

	/* must run thread */
virtual void run();

	private:

bool initUPnPState();
void checkUPnPState();
bool printUPnPState();
bool updateUPnP();


	/* Mutex for data below */
	RsMutex dataMtx;

 	/* requested from rs */
	bool toShutdown; /* if set shuts down the thread. */

	bool toEnable;   /* overall on/off switch */
	bool toStart;  /* if set start forwarding */
	bool toStop;   /* if set stop  forwarding */

	struct sockaddr_in iaddr;
	unsigned short eport;       /* config            */
	unsigned short eport_curr;  /* current forwarded */

	/* info from upnp */
	unsigned int upnpState;
	uPnPConfigData *upnpConfig;

	struct sockaddr_in upnp_iaddr;
	struct sockaddr_in upnp_eaddr;

	/* active port forwarding */
	std::list<upnpforward> activeForwards;

};

#endif /* _RS_UPNP_IFACE_H */
