#ifndef _RS_UPNP_IFACE_H
#define _RS_UPNP_IFACE_H

#include <string.h>

#include <string>
#include <map>

/* platform independent networking... */
#include "pqi/pqinetwork.h"
#include "pqi/p3upnpmgr.h"

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

class upnphandler: public p3UpnpMgr
{
	public:

	upnphandler();
virtual	~upnphandler();

	/* External Interface */
virtual void    enableUPnP(bool active);
virtual void    shutdownUPnP();

virtual bool    getUPnPEnabled();
virtual bool    getUPnPActive();

virtual void    setInternalPort(unsigned short iport_in);
virtual void    setExternalPort(unsigned short eport_in);
virtual bool    getInternalAddress(struct sockaddr_in &addr);
virtual bool    getExternalAddress(struct sockaddr_in &addr);

	/* must run thread */
virtual void run();

	private:

bool initUPnPState();
void checkUPnPState();
bool printUPnPState();

bool checkUPnPActive();
bool updateUPnP();


	/* Mutex for data below */
	RsMutex dataMtx;

 	/* requested from rs */
	bool toShutdown; /* if set shuts down the thread. */

	bool toEnable;   /* overall on/off switch */
	bool toStart;  /* if set start forwarding */
	bool toStop;   /* if set stop  forwarding */

	unsigned short iport;
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
