/*
 * "$Id: pqiudpproxy.cc,v 1.7 2007-02-18 21:46:50 rmf24 Exp $"
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





#include "pqi/pqiudpproxy.h"
#include "pqi/pqitunnelproxyudp.h"
#include "pqi/pqissludp.h"
#include "pqi/p3disc.h"

#include <sstream>
#include "pqi/pqidebug.h"

#include "tcponudp/tou.h"

const int pqiudpproxyzone = 6846;

class udpAddrPair
{
public:
	udpAddrPair(cert *n1, cert *n2, int fd);
	~udpAddrPair();

	int     match(cert *src, cert *peer, struct sockaddr_in &addr);
	bool    bothMatched();
	
	cert *cert1, *cert2;
	int  m1count, m2count;
	bool matched1, matched2;
	struct sockaddr_in addr1, addr2;
	char *key1, *key2;
	int  keysize1, keysize2;
	int sockfd;
};


udpAddrPair:: udpAddrPair(cert *n1, cert *n2, int fd)
	:cert1(n1), cert2(n2), 
	matched1(false), matched2(false), 
	key1(NULL), key2(NULL), 
	keysize1(0), keysize2(0), sockfd(fd)
{

	/* choose a random number 20 - 100 */
	keysize1 = 20 + (int) (80.0 * (rand() / (RAND_MAX + 1.0)));
	keysize2 = 20 + (int) (80.0 * (rand() / (RAND_MAX + 1.0)));
	int i;

	/* generate keys */
	key1 = (char *) malloc(keysize1);
	key2 = (char *) malloc(keysize2);
	for(i = 0; i < keysize1; i++)
	{
		key1[i] = (char) (256.0 * (rand() / (RAND_MAX + 1.0)));
	}
	for(i = 0; i < keysize2; i++)
	{
		key2[i] = (char) (256.0 * (rand() / (RAND_MAX + 1.0)));
	}
	/* done! */
}

udpAddrPair::~udpAddrPair()
{
	free(key1);
	free(key2);
}

int     udpAddrPair::match(cert *src, cert *peer, struct sockaddr_in &addr)
{
	if ((src == cert1) && (peer == cert2))
	{
		/* matched 1 */
		matched1 = true;
		m1count++;
		addr1 = addr;

		std::ostringstream out;
		out << "udpAddrPair::match() Matched Cert 1" << std::endl;
		out << "FROM: " << inet_ntoa(addr.sin_addr) << ":";
		out << ntohs(addr.sin_port) << " count: " << m1count;
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
		return true;

	}

	if ((src == cert2) && (peer == cert1 ))
	{
		matched2 = true;
		m2count++;
		addr2 = addr;

		std::ostringstream out;
		out << "udpAddrPair::match() Matched Key 2" << std::endl;
		out << "FROM: " << inet_ntoa(addr.sin_addr) << ":";
		out << ntohs(addr.sin_port) << " count: " << m1count;
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
		return true;
	}

	pqioutput(PQL_DEBUG_ALL, pqiudpproxyzone, "not matched");
	return false;
}

bool    udpAddrPair::bothMatched()
{
	return (matched1 && matched2);
}


	p3udpproxy::p3udpproxy(p3disc *p)
	        :p3proxy(p)
{
	/* if we have a udp_server,
	 * setup a listener.
	 */

	/* check that the sslcert flags are good. */
	//if ((sslcert->isFirewalled() && sslcert->isForwarded() || 
	//		(!(sslcert->isFirewalled())))
	{
		/* then check address */
		/* attempt to setup a udp port to listen...
		 */
		struct sockaddr_in *addr = &(getSSLRoot()->getOwnCert()->localaddr); 
	{
		std::ostringstream out;
		out << "p3udpproxy::p3udpproxy()";
		out << " Creating a UDP Listening port:";
		out << inet_ntoa(addr->sin_addr) << ":" << ntohs(addr->sin_port);
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
	}

		int sockfd = tou_socket(0,0,0);
		tou_bind(sockfd, (struct sockaddr *) addr, 
				sizeof(struct sockaddr_in));

		tou_listen(sockfd, 1);
		tou_extaddr(sockfd, (struct sockaddr *) addr, 
				sizeof(struct sockaddr_in));
	}

	return;
}

p3udpproxy::~p3udpproxy()
{
	/* should clean up! */
	return;
}


// PQTunnelService Overloading.
//
// Major overloaded function.
//
// The receive function now has two distinct roles.
// 1) Proxy role: receive Ext Addresses (in an established connection)
// 	-> when multiple recieved, return address pair to initiate
// 	-> transaction.
//
// The generic (p3proxy) recieve should split the 
// packet depending on if we are proxy, or end destination.
//
// expects to recieve
// 	1) if we are the proxy ... add address to the 
//		waiting pairs.
//
//		if have both -> respond.
//
//	2) if receive as destination, and they've
//		sent the addr pair, then we can 
//		commence the connection.


	/* only two types of packets are expected over the
	 * proxy connection......
	 * 1) ProxyAddress + Key.
	 * 2) discovered addresses.
	 *
	 * This information comes from the Proxy for us.
	 * and is encapsulated in a PQProxyUdp packet.
	 */

int p3udpproxy::receiveAsProxy(PQTunnelProxy* pqtp, cert *src, cert *dest)
{
	/* if connected (checked already!) */
        {
		std::ostringstream out;
		out << "p3udpproxy::receiveAsProxy()";
		out << " Received PQTunnel Packet! - Yet to do anything!" << std::endl;
		pqtp -> print(out);
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
	}
	
	/* add to waiting */
	PQProxyUdp udppkt;
	if (pqtp->size != udppkt.in(pqtp->data, pqtp->size))
	{
		delete pqtp;
		pqioutput(PQL_WARNING, pqiudpproxyzone, 
			"PQTunnel is Not a PQProxyUdp Pkt...Deleting");
		return -1;
	}

	if ((cert *) pqtp->p != src)
	{
		// error
		std::ostringstream out;
		out << "p3udpproxy::receiveAsProxy() ERROR pqtp->p != src";
		pqioutput(PQL_ALERT, pqiudpproxyzone, out.str());
		return -1;
	}

	// order very important.... packet from src, for connection to dest!.
	checkUdpPacket(&udppkt, src, dest);

	/* cleanup */
	delete pqtp;
	return 1;
}

int p3udpproxy::receiveAsError(PQTunnelProxy* pqtp, cert *src, cert *from, cert *dest)
{
	/* don't do much */
	return 1;
}

int p3udpproxy::receiveAsDestination(PQTunnelProxy* pqtp, cert *src, cert *from, cert *dest, pqiproxy *pqip)


{
	{
		std::ostringstream out;
		out << "p3udpproxy::receiveAsDestination(";
		out << src -> Name();
		out << " -> " << from -> Name();
		out << " -> " << dest -> Name() << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
	}

	PQProxyUdp udppkt;
	if (pqtp->size != udppkt.in(pqtp->data, pqtp->size))
	{
		delete pqtp;
		pqioutput(PQL_WARNING, pqiudpproxyzone, 
			"PQTunnel is Not a PQProxyUdp Pkt...Deleting");
		return -1;
	}

	// inform the pqiudpproxy
	pqiudpproxy *udpproxy = dynamic_cast<pqiudpproxy *>(pqip);
	if (udpproxy == NULL)
	{
		std::ostringstream out;
		out << "pqiproxy -> NOT a pqiudpproxy....Trouble";
		pqioutput(PQL_WARNING, pqiudpproxyzone, out.str());

		// mismatch reject.
		delete pqtp;
		return -1;
	}
	
	if (udppkt.type != PQI_TPUDP_TYPE_2)
	{
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, 
			"Unknown Proxy Packet");
		delete pqtp;
		return -1;
	}


	pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, 
		"Packet -> Discovered Address");
	udpproxy->discoveredaddresses(&(udppkt.naddr), &(udppkt.paddr), 
					udppkt.connectMode);

	/* we have now completed that proxy connection.
	 * shut it down 
	 */

	delete pqtp;
	return 1;
}

/* This is called by the pqiudpproxy, when an address should
 * be transmitted.....
 */

int      p3udpproxy::sendExternalAddress(pqiudpproxy *pqip, 
					struct sockaddr_in &ext_addr)
{
       // first find the certs.
       cert *dest = pqip -> getContact();
       cert *own  = sroot -> getOwnCert();

	std::map<cert *, cert *>::iterator it;
	it = connectionmap.find(dest);
	/* check certs for basic safety */
	if (it == connectionmap.end())
	{
		/* cannot */
		return -1;
	}
	cert *prxy = it -> second;

	std::ostringstream out;
	out << "p3udpproxy::sendExternalAddress():";
	out << " & " << inet_ntoa(ext_addr.sin_addr);
	out << ":" << ntohs(ext_addr.sin_port);
	pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());

        PQProxyUdp *udppkt = new PQProxyUdp(&ext_addr, &ext_addr, sizeof(ext_addr));

	/* now pack them in PQTunnelProxy pkts */
	PQTunnelProxy *tp1 = createProxyUdpPkt(dest, prxy, own, udppkt);
	delete udppkt;

	/* send them off */
	outPkts.push_back(tp1);
	return 1;
}
	


int      p3udpproxy::connectionCompletedAsProxy(cert *n1, cert *n2)
{
	/* As the proxy, we do nothing at the completion of the connection
	 * except wait for the two external addresses to be forwarded to us
	 * (this is handled in receiveAsProxy()).
	 */

	{
		std::ostringstream out;
		out << "p3udpproxy::completeConnectAsProxy()";
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
	}
	
        /* generate keys */
	udpAddrPair *uc = new udpAddrPair(n1, n2, 1); /* sockfd - legacy = 1? */

	/* save keys in list */
	keysWaiting.push_back(uc);
	return 1;
}


int      p3udpproxy::checkUdpPacket(PQProxyUdp *pkt, cert *src, cert *dest)
{
	{
		std::ostringstream out;
		out << "p3udpproxy::checkUdpPacket()";
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
	}
	
	/* only if we are the proxy */
	struct sockaddr_in ext_addr = pkt->paddr;

	/* the packet should just contain the key */
	std::list<udpAddrPair *>::iterator it;
	for(it = keysWaiting.begin(); it != keysWaiting.end(); it++)
	{
		if ((*it) -> match(src, dest, ext_addr))
		{
			std::ostringstream out;
			out << "p3udpproxy::checkUdpPacket() Matched Key!";
			pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
		}
		else
		{
			// only way to continue loop!
			continue;
		}

		if ((*it) -> bothMatched())
		{
			udpAddrPair *uc = (*it);

			std::ostringstream out;
			out << "p3udpproxy::checkUdpPacket() Both Keys Matched!";
			out << "p3udpproxy::checkUdpPacket() Sending addresses: ";
			out << inet_ntoa(uc->addr1.sin_addr) << ":";
			out << ntohs(uc->addr1.sin_port);
			out << " & " << inet_ntoa(uc->addr2.sin_addr);
			out << ":" << ntohs(uc->addr2.sin_port);
			pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());


			/* remove packet, and send addresses to both! 
			 * Arbitarily decide which is active (one with addr1) 
			 */
			PQProxyUdp *p1 = new PQProxyUdp(
						&(uc->addr1), &(uc->addr2), 1);
			PQProxyUdp *p2 = new PQProxyUdp(
						&(uc->addr2), &(uc->addr1), 0);


			/* now pack them in PQTunnelProxy pkts */
			PQTunnelProxy *tp1 = createProxyUdpPkt(uc->cert1, NULL, uc->cert2, p1);
			PQTunnelProxy *tp2 = createProxyUdpPkt(uc->cert2, NULL, uc->cert1, p2);

			delete p1;
			delete p2;

			/* send them off */
			outPkts.push_back(tp1);
			outPkts.push_back(tp2);

			/* remove */
			it = keysWaiting.erase(it);

		}
		/* if we matched -> then good! */
		return 1;
	}

	{
		std::ostringstream out;
		out << "p3udpproxy::checkUdpPacket() FAILED to Match Incoming UdpPkt";
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
	}

	return 0;
}

int	p3udpproxy::reset(pqiproxy *p, cert *c)
{
	{
		std::ostringstream out;
		out << "p3udpproxy::reset()";
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
	}
	// restart.
	// remove from keysWaiting ???
	return p3proxy::reset(p,c);
}


int	p3udpproxy::outgoingpkt(pqiproxy *p, cert *c, void *d, int size)
{
	{
		std::ostringstream out;
		out << "p3udpproxy::outgoingpkt() For: " << c -> Name();
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
	}
	/* fails in udpproxy */
	return 0;
}


int	p3udpproxy::incomingpkt(pqiproxy *p, void *d, int maxsize)
{
	{
		std::ostringstream out;
		out << "p3udpproxy::incomingpkt()";
		pqioutput(PQL_DEBUG_ALL, pqiudpproxyzone, out.str());
	}
	/* fails in udpproxy */
	return 0;
}


int 	p3udpproxy::status()
{
	{
		std::ostringstream out;
		out << "p3udpproxy::status()";
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
	}
	p3proxy::status();
	return 1;
}


int	p3udpproxy::processincoming()
{
	{
		std::ostringstream out;
		out << "p3udpproxy::processincoming()";
		pqioutput(PQL_DEBUG_ALL, pqiudpproxyzone, out.str());
	}

	return 1;
}


/* this is only called by the proxy */
PQTunnelProxy *p3udpproxy::createProxyUdpPkt(cert *dest, cert *prxy, cert *src, PQProxyUdp *uap)
{
	PQTunnelProxy *pi = new PQTunnelProxy();
	cert *o = sroot -> getOwnCert();

	{
		std::ostringstream out;
		out << "p3udpproxy::createProxyUdpPkt()" << std::endl;
		out << "Dest: " << dest -> Name() << std::endl;
		if (!prxy)
		{
			out << "Proxy: US!" << std::endl;
		}
		else
		{
			out << "Proxy: " << prxy -> Name() << std::endl;
		}
		out << "Src: " << src -> Name() << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
	}

	// Fill in Details....
	// only src = peer to fill in .
	if (!sroot -> getcertsign(src, pi -> src))
	{
		std::ostringstream out;
		out << "p3proxy::createProxyPkt()";
		out << " Error: Failed to Get Signature 1...";
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());

		delete pi;
		return NULL;
	}
	if (!sroot -> getcertsign(dest, pi -> dest))
	{
		std::ostringstream out;
		out << "p3proxy::createProxyPkt()";
		out << " Error: Failed to Get Signature 2...";
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());

		delete pi;
		return NULL;
	}

	// initialise packets.
	pi -> sid = 0;   // doesn't matter.
	pi -> seq = 0;

	/* HACK HERE */
	if (prxy == NULL) /* then we are the proxy */
	{
		/* send on */
		pi -> cid = dest -> cid; // destination.
		pi -> p = o; // from us;
	}
	else
	{
		pi -> cid = prxy -> cid; // send to proxy.
		pi -> p = o; // from us;
	}
		

	// ProxyInit Details.
	pi -> size = uap -> getSize();
	pi -> data = malloc(pi->size);
	uap->out(pi->data, pi->size);
	// Sign Packet.

	return pi;
}


/************************ PQI PROXY *************************/

#define		PUP_CLOSED		0
#define		PUP_WAITING_PROXY	1
#define		PUP_PROXY_CONNECT	2
#define		PUP_WAITING_DISC	3
#define		PUP_WAITING_UDP		4
#define		PUP_WAITING_SSL		5
#define		PUP_CONNECTED		6

pqiudpproxy::pqiudpproxy(cert *c, p3udpproxy *l, PQInterface *p)
	:pqipeerproxy(c, l, p), state(PUP_CLOSED)
{
	{
		std::ostringstream out;
		out << "pqiudpproxy::pqiudpproxy()";
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
	}

	return;
}

pqiudpproxy::~pqiudpproxy()
{
	{
		std::ostringstream out;
		out << "pqiudpproxy::~pqiudpproxy()";
		//pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
		pqioutput(PQL_ALERT, pqiudpproxyzone, out.str());
	}
	state = PUP_CLOSED;
	return;
}

	// pqiconnect Interface.
int 	pqiudpproxy::connectattempt() // as pqiproxy/
{
	{
		std::ostringstream out;
		out << "pqiudpproxy::connectattempt()";
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
	}
	state = PUP_WAITING_PROXY;
	return pqipeerproxy::connectattempt();
}


int	pqiudpproxy::listen()
{
	{
		std::ostringstream out;
		out << "pqiudpproxy::listen()";
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
	}
	state = PUP_WAITING_PROXY;
	return pqipeerproxy::listen();
}


int	pqiudpproxy::stoplistening()
{
	//state = PUP_WAITING_PROXY;
	return pqipeerproxy::stoplistening();
}

int 	pqiudpproxy::disconnected()
{
	pqipeerproxy::disconnected();
	state = PUP_CLOSED;
	reset();
	return 1;
}

/* pqiproxy interface */
int	pqiudpproxy::connected(bool act)
{
	{
		std::ostringstream out;
		out << "pqiudpproxy::connected() - Set State!";
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
	}
	state = PUP_PROXY_CONNECT;
	return pqipeerproxy::connected(act);
}

// called by the p3udpproxy .... means attempt failed....
int 	pqiudpproxy::disconnect()
{
	{
		std::ostringstream out;
		out << "pqiudpproxy::disconnect()";
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
	}
	/* if we haven't got remote address yet! */
	if (state < PUP_WAITING_SSL)
	{
		disconnected();
	}
	/* else is okay */
	return 1;
}


int 	pqiudpproxy::reset()
{
	{
		std::ostringstream out;
		out << "pqiudpproxy::reset()";
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
	}
	pqipeerproxy::reset();
	state = PUP_CLOSED;
	return 1;
}



	// Overloaded from PQInterface
int 	pqiudpproxy::status()
{
	{
		std::ostringstream out;
		out << "pqiudpproxy::status() state = " << state;
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
	}
	pqipeerproxy::status();
	return 1;
}



int	pqiudpproxy::tick()
{
	{
		std::ostringstream out;
		out << "pqiudpproxy::tick()";
		pqioutput(PQL_DEBUG_ALL, pqiudpproxyzone, out.str());
	}

	if (state == PUP_CLOSED) /* silent state */
	{

	}
	else if (state == PUP_WAITING_PROXY) /* just started connection */
	{
		//state = PUP_WAITING_DISC;
	}
	else if (state == PUP_PROXY_CONNECT)
	{
		//state = PUP_WAITING_DISC;
	}
	else if (state == PUP_WAITING_DISC)
	{
		//state = PUP_WAITING_DISC;
	}
	else if (state == PUP_WAITING_SSL)
	{

	}

	pqipeerproxy::tick();
	return 1;
}

int pqiudpproxy::notifyEvent(int type)
{
	{
		std::ostringstream out;
		out << "pqiudpproxy::notifyEvent(" << type << ")";
		pqioutput(PQL_DEBUG_BASIC, pqiudpproxyzone, out.str());
	}
	return pqipeerproxy::notifyEvent(type);
}


int   p3udpproxy::requestStunServer(struct sockaddr_in &addr)
{
	if (potentials.size() == 0)
	{
		potentials =  p3d -> requestStunServers();
	}

	if (potentials.size() > 0)
	{
		/* pop the first one off and return it */
		addr = potentials.front();
		potentials.pop_front();
		return 1;
	}

	return 0;
}

/* The pqissludp interface is below, in order of execution 
 */

bool    pqiudpproxy::isConnected(int &mode)
{
	if (isactive())
	{
		mode = connect_mode;
		return true;
	}
	return false;
}


bool    pqiudpproxy::hasFailed()
{
	return (state == PUP_CLOSED);
}

int   pqiudpproxy::requestStunServer(struct sockaddr_in &addr)
{
	return ((p3udpproxy *) p3p) -> requestStunServer(addr);
}

        // notification from pqissludp.
bool    pqiudpproxy::sendExternalAddress(struct sockaddr_in &ext_addr)
{
	return ((p3udpproxy *) p3p) -> 
		sendExternalAddress(this, ext_addr);
}

// 
int     pqiudpproxy::discoveredaddresses(struct sockaddr_in *our_addr, 
			struct sockaddr_in *peer, int cMode)
{
	/* we have our own address!, and peers address */
	{
		std::ostringstream out;
		out << "pqiudpproxy::discoveredaddresses() Connect";
		out << std::endl;
		out << "Our Address: " << inet_ntoa(our_addr->sin_addr) << ":";
		out << ntohs(our_addr->sin_port);
		out << std::endl;
		out << " Peers Address: " << inet_ntoa(peer->sin_addr) << ":";
		out << ntohs(peer->sin_port);

		pqioutput(PQL_ALERT, pqiudpproxyzone, out.str());
	}

	remote_addr = *peer;
	if (cMode)
	{
		connectMode = 1; /* active */
	}
	else
	{
		connectMode = 0; /* passive */
	}
	state = PUP_WAITING_UDP;

	// shutdown the proxy Connection.
	// by calling the pqipeerproxy::reset().
	pqipeerproxy::reset();	
	
	pqioutput(PQL_ALERT, pqiudpproxyzone, 
		"sent pqipeerproxy::reset() should shutdown proxy connect");
	return 1; 
}

        // poll by pqissludp for state completion.
bool    pqiudpproxy::gotRemoteAddress(struct sockaddr_in &raddr, int &cMode)
{
	if (state == PUP_WAITING_UDP)
	{
		raddr = remote_addr;
		cMode = connectMode;
		return true;
	}
	return false;
}



