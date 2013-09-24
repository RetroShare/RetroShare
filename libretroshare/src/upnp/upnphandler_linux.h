#ifndef _RS_UPNP_IFACE_H
#define _RS_UPNP_IFACE_H

#include <string.h>

/* platform independent networking... */
#include "pqi/pqinetwork.h"
#include "pqi/pqiassist.h"

#include "util/rsthreads.h"

#include <upnp/upnp.h>
#include "upnp/UPnPBase.h"

#define RS_UPNP_S_UNINITIALISED  0
#define RS_UPNP_S_UNAVAILABLE    1
#define RS_UPNP_S_READY          2
#define RS_UPNP_S_TCP_AND_FAILED 3
//#define RS_UPNP_S_UDP_FAILED     4
#define RS_UPNP_S_ACTIVE         5

class upnphandler: public pqiNetAssistFirewall
{
	public:

	    upnphandler();
	    virtual	~upnphandler();

	    /* External Interface (pqiNetAssistFirewall) */
	    virtual void    enable(bool active);
	    virtual void    shutdown();
	    virtual void    restart();

	    virtual bool    getEnabled();
	    virtual bool    getActive();

	    virtual void    setInternalPort(unsigned short iport_in);
	    virtual void    setExternalPort(unsigned short eport_in);
	    virtual bool    getInternalAddress(struct sockaddr_storage &addr);
	    virtual bool    getExternalAddress(struct sockaddr_storage &addr);

            /* TO IMPLEMENT: New Port Forward interface to support as many ports as necessary */
	    virtual bool    requestPortForward(const PortForwardParams &params) { return false; }
	    virtual bool    statusPortForward(const uint32_t fwdId, PortForwardParams &params) { return false; }

	    /* Public functions - for background thread operation,
	     * but effectively private from rest of RS, as in derived class
	     */
	    unsigned int upnpState;

	    bool start_upnp();
	    bool shutdown_upnp();

	    bool initUPnPState();

	    /* Mutex for data below */
	    RsMutex dataMtx;

	private:

	    CUPnPControlPoint *cUPnPControlPoint;

	    bool background_setup_upnp(bool, bool);


	    bool toEnable;   /* overall on/off switch */
	    bool toStart;  /* if set start forwarding */
	    bool toStop;   /* if set stop  forwarding */

	    unsigned short iport;
	    unsigned short eport;       /* config            */
	    unsigned short eport_curr;  /* current forwarded */

	    /* info from upnp */
	    struct sockaddr_in upnp_iaddr;
	    struct sockaddr_in upnp_eaddr;
};

/* info from upnp */
int CtrlPointCallbackEventHandler(Upnp_EventType ,void* , void*);

#endif /* _RS_UPNP_IFACE_H */
