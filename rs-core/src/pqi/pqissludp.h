/*
 * "$Id: pqissludp.h,v 1.8 2007-02-18 21:46:49 rmf24 Exp $"
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



#ifndef MRK_PQI_SSL_UDP_HEADER
#define MRK_PQI_SSL_UDP_HEADER

#include <openssl/ssl.h>

// operating system specific network header.
#include "pqi/pqinetwork.h"

#include <string>
#include <map>

#include "pqi/pqissl.h"

// function for discovering certificates.
//int     connectForExchange(struct sockaddr_in addr);
//
//
//
//
 /* So pqissludp is the special firewall breaking protocol.
  * This class will implement the basics of streaming
  * ssl over udp using a tcponudp library....
  * and a small extension to ssl.
  */

class pqissludp;
class pqiudpproxy;
class p3udpproxy;
class cert;

/* no such thing as a listener .... actually each one
 * can receive connections..... its the stateless you see!.
 *
 * so pqiudplistener
 *
 * will be the support udp listener for proxy connections.
 * this will be used by the p3proxy for external addresses.
 */

class pqiudplistener
{
	public:

	pqiudplistener(p3udpproxy *p, struct sockaddr_in addr);

int 	resetlisten();
int     setListenAddr(struct sockaddr_in addr);
int 	setuplisten();

int	tick(); /* does below */
int     status();

int	recvfrom(void *data, int *size, struct sockaddr_in &addr);
int	reply(void *data, int size, struct sockaddr_in &addr);

	private:
int     serverStun();
bool    response(void *data, int size, struct sockaddr_in &addr);
int    	checkExtAddr(struct sockaddr_in &src_addr,
		void *data, int size, struct sockaddr_in &ext_addr);

bool generate_stun_pkt(void *stun_pkt, int *len);
void *generate_stun_reply(struct sockaddr_in *stun_addr, int *len);

	p3udpproxy *p3u;

	int sockfd;
	struct sockaddr_in laddr;
	bool active;

	int laststun, firstattempt, lastattempt;

	/* last stun packet sent out */
	struct sockaddr_in stun_addr;
	void *stunpkt;
	int  stunpktlen;
};


/* This provides a NetBinInterface, which is 
 * primarily inherited from pqissl.
 * fns declared here are different -> all others are identical.
 */

class pqissludp: public pqissl
{
public:
	pqissludp(cert *c, PQInterface *parent, pqiudpproxy *prxy);
virtual ~pqissludp();

	// NetInterface.
	// listen fns call the udpproxy.
virtual int listen();
virtual int stoplistening();
virtual int tick();
virtual int reset();

	// BinInterface.
	// These are reimplemented.	
virtual bool moretoread();
virtual bool cansend();
	/* UDP always through firewalls -> always bandwidth Limited */
virtual bool bandwidthLimited() { return true; } 

	// pqissludp specific.
	// These three functions must 
	// be called to initiate a connection;

int 	attach(sockaddr_in&);
	// stun packet handling.
void 	*generate_stun_pkt(struct sockaddr_in *stun_addr, int *len);
int 	getStunReturnedAddr(void *pkt, int len, struct sockaddr_in *stun_addr);

protected:

virtual int Request_Proxy_Connection();// Overloaded -> all 7.
virtual int Check_Proxy_Connection();
virtual int Request_Local_Address(); 
virtual int Determine_Local_Address(); 
virtual int Determine_Remote_Address(); 
virtual int Initiate_Connection(); 
virtual int Basic_Connection_Complete();
virtual int Reattempt_Connection();

//protected internal fns that are overloaded for udp case.
virtual int net_internal_close(int fd);
virtual int net_internal_SSL_set_fd(SSL *ssl, int fd);
virtual int net_internal_fcntl_nonblock(int fd);

private:

	BIO *tou_bio;  // specific to ssludp.
	pqiudpproxy *udpproxy;

	// are these needed at all?? 
	struct sockaddr_in local_addr; // ssludp
	struct sockaddr_in firewall_addr; // ssludp


	struct sockaddr_in stun_addr; //pqissl.
	int stun_timeout;
	int stun_attempts;
	int remote_timeout;
	int proxy_timeout;
	int udp_connect_timeout;

	long listen_checktime;
};

#endif // MRK_PQI_SSL_UDP_HEADER
