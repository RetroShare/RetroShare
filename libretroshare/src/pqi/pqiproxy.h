/*
 * "$Id: pqiproxy.h,v 1.7 2007-02-18 21:46:49 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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



#ifndef MRK_PQI_PROXY_HEADER
#define MRK_PQI_PROXY_HEADER


#include <openssl/ssl.h>


#include <string>
#include <map>
#include <list>

#include "pqi/pqi.h"

#include "pqi/pqiperson.h"
#include "pqi/pqitunnel.h"
#include "pqi/pqitunnelproxy.h"

class cert;

class pqiproxy;
class pqifilter;

class p3disc;

// PQItem derived classes.
class PQTunnelInit;
class PQTunnelProxyInit;

class PQTunnel;
class PQTunnelProxy;

/*
 * #ifdef PQI_USE_XPGP
 *	#include "xpgpcert.h"
 */

class p3proxy : public PQTunnelService
{
	public:

	p3proxy(p3disc *p)
	:PQTunnelService(PQI_TUNNEL_PROXY_TYPE), p3d(p) 
	{ 
		sroot = getSSLRoot(); 
		return; 
	}

virtual ~p3proxy() { return; }

	// PQTunnelService Overloading.
virtual int             receive(PQTunnel *);
virtual PQTunnel *      send();

virtual int             receive(PQTunnelInit *);
virtual PQTunnelInit *  sendInit();

	// interface to pqiproxy.
int	attach(pqiproxy *p, cert *c);
int	detach(pqiproxy *p, cert *c);

int	listen(pqiproxy *p, cert *c);
int	stoplistening(pqiproxy *p, cert *c);
int	stopconnecting(pqiproxy *p, cert *c);

int	reset(pqiproxy *p, cert *c);

int	outgoingpkt(pqiproxy *p, cert *, void *d, int size);
int	incomingpkt(pqiproxy *p, void *d, int maxsize);

// overloaded from many sources.
virtual int tick();

int 	status();

int 	connectattempt(pqiproxy*, cert*);
int 	connectattempt();
int     nextconnectattempt(cert *, cert *);

	protected:

	// Fn to translate the sig to a cert.
bool filter(PQTunnelProxy *in);

cert *findcert(certsign &cs);

PQTunnelProxyInit *createProxyInit(cert *dest, cert *proxy);
PQTunnelProxy *createProxyPkt(cert *dest, cert *proxy, long nextseq,
	                               const char *data, int size);

	// Register Connections for which we are the Proxy.
	// (only channels registered this way can send proxy packets)
int     registerProxyConnection(cert *src, cert *proxy, 
			cert *dest, PQTunnelProxyInit *pinit);
	// indicates if a proxy connection has been established.
int     isConnection(cert *src, cert *dest);

	// Init Proxy Connections.
int respondConnect(cert *o, cert *p, PQTunnelProxyInit *in);
int completeConnect(cert *o, cert *p, PQTunnelProxyInit *in);
int endConnect(cert *o, cert *p, PQTunnelProxyInit *in);

	// process incoming.
virtual int	processincoming();

	// Overload to provide alternative behaviour.
	// These fns are called from receive(PQTunnel *);
virtual int 	connectionCompletedAsProxy(cert *n1, cert *n2)     { return 1; }
virtual int     connectionCompletedAsPeer(cert *proxy, cert *peer) { return 1; }


	// Overload to provide alternative behaviour.
	// These fns are called from receive(PQTunnel *);
virtual int 	receiveAsProxy(PQTunnelProxy* pqtp, cert *src, cert *dest);
virtual int 	receiveAsDestination(PQTunnelProxy* pqtp, 
				cert *src, cert *from, cert *dest, pqiproxy *pqip);

virtual int 	receiveAsError(PQTunnelProxy* pqtp, cert *src, cert *from, cert *dest);

	// Proxy Control functions.
int 	sendProxyInit(cert *c);

	// Fn to send
int     sendEndProxyConnectionPkt(cert *other, cert *proxy);

	bool active;

	// the registered proxies.
	// All of these maps are 
	// indexed by the destination, not the proxy.
	// which is the second cert is some cases.

	std::map<cert *, pqiproxy *> proxymap;
	std::list<cert *> listenqueue; // the ones to listen for.
	std::list<cert *> connectqueue; // the ones to attempt to connect.
	std::map<cert *, cert *> initmap; // ones we sent init, wait for init.
	std::map<cert *, cert *> replymap; // ones that need a auth reply.
	std::map<cert *, cert *> connectionmap;
	std::map<cert *, cert *> lastproxymap;
	std::map<cert *, unsigned long> connecttimemap; // timeout map.
	std::map<cert *, bool> passivemap; // who inited connection.

	// double direction map.
	//std::map<pqiproxy *, ChanId> outmap;
	std::map<pqiproxy *, std::list<PQTunnelProxy *> > inqueue;

	// filters. (remove bad packets/cache data).
	std::list<pqifilter *> filters;

	// Map of the connections we are proxy for.
	std::map<std::pair<cert *, cert *>, int> proxyconnect;

	p3disc *p3d; // needed to find proxies.
	sslroot *sroot; // for certificate references.

	// input fn for details.
int     addOutInitPkt(PQTunnelInit *pkt);

	std::list<PQTunnel *> outPkts;
	std::list<PQTunnelInit *> outInitPkts;

};



// pqiconnect, derives from pqistreamer/PQInterface.
//
// this class needs to provide.
// 1) read/write data.
// 2) connect functions.

// base interface for pqiproxy....
class pqiproxy
{
public:
	pqiproxy(cert *c, p3proxy *l)
	:sslcert(c), p3p(l) { return; }
virtual ~pqiproxy() { return; }

	// The interface to p3proxy.
virtual int notifyEvent(int type) = 0;

	// notification from p3proxy.
virtual int	connected(bool active) = 0;
virtual int	disconnected() = 0;

	protected:
	cert *sslcert;
	p3proxy *p3p;
};

	

/* use a common peer as a proxy, to pass messages
 * between proxied peers.
 */

class pqipeerproxy: public pqiproxy, public NetBinInterface
{
public:
	pqipeerproxy(cert *c, p3proxy *l, PQInterface *parent);
virtual ~pqipeerproxy();

	// Net Interface.
virtual int 	connectattempt();
virtual int	listen();
virtual int	stoplistening();
virtual int 	reset();
virtual int 	disconnect();

	// Overloaded from PQInterface
virtual int 	status();
virtual int	tick();
virtual cert *	getContact();

	// Bin Interface.
virtual int senddata(void*, int);
virtual int readdata(void*, int);
virtual int netstatus();
virtual int isactive();
virtual bool moretoread();
virtual bool cansend();

	// The interface to p3proxy.
virtual int notifyEvent(int type);

	// notification from p3proxy.
virtual int	connected(bool active);
virtual int	disconnected();

protected:

	/* data */
	//p3proxy *p3p;

	bool active;
	int connect_mode;

	void *pkt;
	int  maxpktlen;
	int  pktlen;
	int  pktn;

};


/* Documenting the virtual (proxy) pqi interface.
 *
 * This is used to create proxy interface
 * for people behind firewalls.
 *
 * There are two options for such an 
 * interface.
 * 1) Encrypted Tunneling - This has the 
 * the advantage/disadvantage that it 
 * allows multiple layers of proxying, 
 * and very private.
 *
 * 2) Open Proxying - This lets people
 * see the data, and allows restriction to one
 * layer. This option allow the proxier, 
 * to collect the files passing through.
 * (benefit for allowing proxying).
 *
 * As this option will less impact on the 
 * persons bandwidth, and give them some
 * benefit, it'll be implemented first.
 *
 * The data will be saved in a cache, 
 * with a first in, first out policy, 
 *
 * maybe show a window of cached files, 
 * and allow the user to save any from
 * a list.
 *
 * So to do this we need
 * i) a cache system.
 * ii) extra message types to indicate
 *     proxied files, and searches etc.
 * iii) a proxy server, to handle proxy messages.
 *
 * actually, this class will need to
 *
 * a proxy interface in someway or another.
 *
 *
 * P1                 PROXY                P2
 * requests a
 * proxy interface.
 * --------->         
 *  end attempt <----- IF Not
 *                Available.
 *
 *                else ---------------> Check for proxy/person auth
 * end attempt  <------ cancel -------  If Not Auth.
 * 					
 * 					else, if allowed.
 * 					build proxy server.
 * Build proxy <------ setup ---------- send okay
 * server              connection.
 *
 * pqissl.
 *   -> proxy interface.
 * 		Send()
 * return proxymsg locally
 * pulled by proxy server.
 *
 * ps -> send proxy msg
 *      to correct pqissl.
 *           ----------> popped out to 
 *                     the proxy server.
 *                     if file cache.
 *                      redirect ------> recieved by pqissl.
 *                                       pulled by proxy server.
 *
 *                                       proxy server
 *
 * all proxied information is
 * going to be signed/or encrypted.
 * 
 * very simple - that is the only 
 * necessary change.
 *
 * proxy msg.
 * ---------------------
 *  type : init
 *         end
 *         msg (signed)
 *         tunnelled (includes init of ssl.)
 *  dest cert signature:
 *
 *  encryption/signature mode.
 *
 *  signature len:
 *  signature:
 *  
 *  data len:
 *  data 
 *
 *
 * 
 *
 *
 * is this the right place to add it?
 * or is pqistreamer the answer.
 *
 */

#endif

