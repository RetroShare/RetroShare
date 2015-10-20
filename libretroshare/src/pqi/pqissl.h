/*
 * "$Id: pqissl.h,v 1.18 2007-03-11 14:54:22 rmf24 Exp $"
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



#ifndef MRK_PQI_SSL_HEADER
#define MRK_PQI_SSL_HEADER

// operating system specific network header.
#include "pqi/pqinetwork.h"

#include <string>
#include <map>

#include "pqi/pqi_base.h"
#include "pqi/authssl.h"

#define WAITING_NOT            0
#define WAITING_DELAY	       1
#define WAITING_SOCK_CONNECT   2
#define WAITING_SSL_CONNECTION 3
#define WAITING_SSL_AUTHORISE  4
#define WAITING_FAIL_INTERFACE 5

#define PQISSL_PASSIVE  0x00
#define PQISSL_ACTIVE   0x01

const int PQISSL_LOCAL_FLAG = 0x01;
const int PQISSL_REMOTE_FLAG = 0x02;
const int PQISSL_DNS_FLAG = 0x04;

/* not sure about the value? */
const int PQISSL_UDP_FLAG = 0x02;

/* TCP buffer size for Windows systems */
const int WINDOWS_TCP_BUFFER_SIZE = 512 * 1024; // 512 KB

/***************************** pqi Net SSL Interface *********************************
 * This provides the base SSL interface class, 
 * and handles most of the required functionality.
 *
 * there are a series of small fn's that can be overloaded
 * to provide alternative behaviour....
 *
 * Classes expected to inherit from this are:
 *
 * pqissllistener 	-> pqissllistener  (tcp only)
 * 			-> pqixpgplistener (tcp only)	
 *
 * pqissl	 	-> pqissltcp
 * 			-> pqissludp
 * 			-> pqixpgptcp
 * 			-> pqixpgpudp
 *
 */

class pqissl;
class cert;

class pqissllistener;
class p3LinkMgr;
class RsPeerCryptoParams ;

class pqissl: public NetBinInterface
{
public:
	pqissl(pqissllistener *l, PQInterface *parent, 
                p3LinkMgr *lm);
virtual ~pqissl();

	// NetInterface
virtual int connect(const struct sockaddr_storage &raddr);
virtual int listen();
virtual int stoplistening();
virtual int reset();
virtual int disconnect();
virtual int getConnectAddress(struct sockaddr_storage &raddr);

virtual bool connect_parameter(uint32_t /*type*/, const std::string & /*value*/) { return false; }
virtual bool connect_parameter(uint32_t type, uint32_t value);

	// BinInterface
virtual int	tick();
virtual int     status();

virtual int senddata(void*, int);
virtual int readdata(void*, int);
virtual int netstatus();
virtual int isactive();
virtual bool moretoread(uint32_t usec);
virtual bool cansend(uint32_t usec);

virtual int close(); /* BinInterface version of reset() */
virtual RsFileHash gethash(); /* not used here */
virtual bool bandwidthLimited() { return true ; } // replace by !sameLAN to avoid bandwidth limiting on LAN

public:

/* Completion of the SSL connection, 
 * this is public, so it can be called by
 * the listener (should make friends??) 
 */

int	accept(SSL *ssl, int fd, const struct sockaddr_storage &foreign_addr); 
void getCryptoParams(RsPeerCryptoParams& params) ;
bool actAsServer();


protected:


	/* no mutex protection for these ones */

	p3LinkMgr *mLinkMgr;
	pqissllistener *pqil; 

	RsMutex mSslMtx; /**** MUTEX protects data and fn below ****/

virtual int reset_locked();
int	accept_locked(SSL *ssl, int fd, const struct sockaddr_storage &foreign_addr); 

	// A little bit of information to describe 
	// the SSL state, this is needed
	// to allow full Non-Blocking Connect behaviour.
	// This fn loops through the following fns.
	// to complete an SSL.

int 	ConnectAttempt();

virtual int Failed_Connection();

	// Start up connection with delay...
virtual int Delay_Connection();

	// These two fns are overloaded for udp/etc connections.
virtual int Initiate_Connection();
virtual int Basic_Connection_Complete();

	// These should be identical for all cases,
	// differences are achieved via the net_internal_* fns.
int Initiate_SSL_Connection();
int SSL_Connection_Complete();
int Authorise_SSL_Connection();

int Extract_Failed_SSL_Certificate(); // try to get cert anyway.

	// check connection timeout.
bool  	CheckConnectionTimeout();


	//protected internal fns that are overloaded for udp case.
virtual int net_internal_close(int fd);
virtual int net_internal_SSL_set_fd(SSL *ssl, int fd);
virtual int net_internal_fcntl_nonblock(int fd);


	/* data */
	bool active;
	bool certvalid;

	int waiting; 

	// addition for udp (tcp version == ACTIVE).
	int sslmode;     

	SSL *ssl_connection;
	int sockfd;

	struct sockaddr_storage remote_addr;

	void *readpkt;
	int pktlen;
	int total_len ; // saves the reading state accross successive calls.

	int attempt_ts;

	int n_read_zero; /* a counter to determine if the connection is really dead */
	time_t mReadZeroTS; /* timestamp of first READ_ZERO occurance */

	int ssl_connect_timeout; /* timeout to ensure that we don't get stuck (can happen on udp!) */

	uint32_t mConnectDelay;
	time_t   mConnectTS;
	uint32_t mConnectTimeout;
	time_t   mTimeoutTS;


private:
	// ssl only fns.
int connectInterface(const struct sockaddr_storage &addr);

};




#endif // MRK_PQI_SSL_HEADER
