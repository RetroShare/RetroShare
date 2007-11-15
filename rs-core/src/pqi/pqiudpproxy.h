/*
 * "$Id: pqiudpproxy.h,v 1.6 2007-02-18 21:46:50 rmf24 Exp $"
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



#ifndef MRK_PQI_UDP_PROXY_HEADER
#define MRK_PQI_UDP_PROXY_HEADER


#include <openssl/ssl.h>


#include <string>
#include <map>
#include <list>

#include "pqi/pqiproxy.h"

class pqiudpproxy;
class udpAddrPair;
class pqissludp;
class PQProxyUdp;

class p3udpproxy: public p3proxy
{
	public:
	p3udpproxy(p3disc *p);
virtual ~p3udpproxy();

	// New udp functions.
	// this first one will be called from
	// inside p3proxy, at the appropriate time.
	int checkUdpPacket(PQProxyUdp *, cert *src, cert *peer);


	// PQTunnelService Overloading. (none)
	// Overloaded from p3proxy.
	// receive now plays role of 
	// collecting address packets.

	// interface to pqiproxy.
//int	attach(pqiproxy *p, cert *c);
//int	detach(pqiproxy *p, cert *c);

//int	listen(pqiproxy *p, cert *c);
//int	stoplistening(pqiproxy *p, cert *c);

int	reset(pqiproxy *p, cert *c);

virtual int	outgoingpkt(pqiproxy *p, cert *, void *d, int size);
virtual int	incomingpkt(pqiproxy *p, void *d, int maxsize);

int	requestStunServer(struct sockaddr_in &addr);
int 	sendExternalAddress(pqiudpproxy *pqip,
                           struct sockaddr_in &ext_addr);


// overloaded from many sources.
//virtual int tick();

int 	status();

//int 	connectattempt(pqiproxy*, cert*);
//int 	connectattempt();
//int     nextconnectattempt(cert *, cert *);

	protected: /* interface not clean enough for private */

	// overload connection Completion.
virtual int 	connectionCompletedAsProxy(cert*, cert*);
//virtual int 	connectionCompletedAsPeer(cert*, cert*);

	// overload receiving Tunnel Data.
virtual int 	receiveAsProxy(PQTunnelProxy *pqtp, cert *src, cert *dest);
virtual int 	receiveAsDestination(PQTunnelProxy *pqtp, 
			cert *src, cert *from, cert *dest, pqiproxy *pqip);
virtual int 	receiveAsError(PQTunnelProxy *pqtp, cert *src, cert *from, cert *dest);

virtual int	processincoming();


	PQTunnelProxy *createProxyUdpPkt(cert *dest, cert *prxy, cert *src, PQProxyUdp *uap);

	std::list<udpAddrPair *> keysWaiting;
	std::list<struct sockaddr_in> potentials; // stun addresses.
};


// pqiconnect, derives from pqistreamer/PQInterface.
//
// pqiproxy provide.
// 1) read/write data.
// 2) connect functions.
//
// pqiudpproxy provides the
// linking, and redirects
// the sending the udpssl layer below.
//
// small extensions to the pqiproxy interface
// are required.

class pqiudpproxy: public pqipeerproxy
{
public:
	pqiudpproxy(cert *c, p3udpproxy *l, PQInterface *parent);
virtual ~pqiudpproxy();

        // Net Interface.
virtual int     connectattempt();
virtual int     listen();
virtual int     stoplistening();
virtual int     reset();
virtual int     disconnect();

// PQInterface
virtual int     tick();
virtual int     status();

	// Bin Interface disabled.
virtual int senddata(void*, int) { return 0; }
virtual int readdata(void*, int) { return 0; }
virtual int netstatus() 	 { return 0; }
//virtual int isactive(); -> same as peer.
virtual bool moretoread() 	 { return false; }

	// The interface to p3proxy.
virtual int notifyEvent(int type);
	// unneeded notification.

	// notification from p3proxy.
virtual int	connected(bool active);
virtual int	disconnected();

	// Below is the pqissludp interface to us.
	// in order of execution.....

	// poll by pqissludp for state completion.
bool	isConnected(int &mode);
bool	hasFailed();

	// requests from pqissludp layer.
int	requestStunServer(struct sockaddr_in &addr);
	// notification from pqissludp.
bool 	sendExternalAddress(struct sockaddr_in &ext_addr);

	// 	notification of external address exchange!
virtual int	discoveredaddresses(struct sockaddr_in *our_addr, 
					struct sockaddr_in *peer, int cMode);

	// poll by pqissludp for state completion.
bool 	gotRemoteAddress(struct sockaddr_in &remote_addr, int &cMode);

private:
	int state;
	struct sockaddr_in ext_addr;
	struct sockaddr_in remote_addr;
	int connectMode; /* sent with the remote addresses */
};

/* Documenting the virtual (proxy) pqi interface.
 *
 * This is used to create proxy interface
 * for people behind firewalls.
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
 * 					Determine Local
 * 					External Addr
 * Determine Local
 * External Addr
 * 
 * send ext addr -------> 
 * 			collect addresses.
 * 				<---------- send ext addr -------> 
 *			complete proxy part.
 *		<-------- send addresses -------> 
 *
 *
 * Initiate Udp Connection. --------------------> Initiate Udp Connection.
 *		<--------------------------------
 *
 *
 * So the initial stuff is the same.....
 * but there is no pipe at the end
 * just the exchange of addresses.
 *
 * there is a udp listener on the same address
 * as the ssllistener (just udp). This port 
 * will be used by the stunServer, and will supply
 * external addresses to clients that request them.
 *
 *
 * Order of udp connection:
 * (1) establish a proxy connection.
 * (2) determine external address.
 * (3) send external address to proxy.
 * (4) recieve address pair
 * (5) initiate a udp connection.
 *
 * Each udpproxy must stun to get their connection addr.
 * this might end up with lots of stuns?
 * does that matter?
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

