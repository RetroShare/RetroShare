/*
 * "$Id: pqissludp.cc,v 1.16 2007-02-18 21:46:49 rmf24 Exp $"
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





#include "pqi/pqissludp.h"
#include "pqi/pqiudpproxy.h"
#include "pqi/pqinetwork.h"

#include "tcponudp/tou.h"
#include "tcponudp/bio_tou.h"

#include <errno.h>
#include <openssl/err.h>

#include "pqi/pqidebug.h"
#include <sstream>

const int pqissludpzone = 3144;

static const int PQI_SSLUDP_STUN_TIMEOUT = 2;
static const int PQI_SSLUDP_STUN_ATTEMPTS = 15;

	/* we need a long timeout to give the local address 
	 * discovery a chance. This must also allow for the 
	 * time for the peer to check for a proxy connect (before lad starts).
	 * Remote = Check Time + 40
	 */
static const int PQI_SSLUDP_PROXY_CHECK_TIME = 29; 
static const int PQI_SSLUDP_REMOTE_TIMEOUT   = 70; 

	/* we need a long timeout to give the udpproxy time 
	 * to find proxies (at startup) 
	 * this is only an emergency timeout anyway
	 */
static const int PQI_SSLUDP_PROXY_TIMEOUT   = 180; 

	/* a final timeout, to ensure this never blocks completely
	 * 300 secs to complete udp/tcp/ssl connection.
	 * This is long as the udp connect can take some time.
	 */
static const int PQI_SSLUDP_CONNECT_TIMEOUT = 300; 

/********** PQI SSL UDP STUFF **************************************/

pqissludp::pqissludp(cert *c, PQInterface *parent, pqiudpproxy *prxy)
	:pqissl(c, NULL, parent), tou_bio(NULL), udpproxy(prxy), 
	listen_checktime(0)

{
	sslmode = PQISSL_PASSIVE;
	stun_addr.sin_addr.s_addr = 0;
	return;
}


pqissludp::~pqissludp()
{ 
        pqioutput(PQL_ALERT, pqissludpzone,  
            "pqissludp::~pqissludp -> destroying pqissludp (+ pqiudproxy)");

	/* must call reset from here, so that the
	 * virtual functions will still work.
	 * -> as they stop working in base class destructor.
	 *
	 *  This means that reset() will be called twice, but this should
	 *  be harmless.
	 */
        stoplistening(); /* remove from p3proxy listenqueue */
	reset(); 

	if (udpproxy)
	{
		delete udpproxy;
	}

	if (tou_bio) // this should be in the reset?
	{
		BIO_free(tou_bio);
	}
	return;
}

int pqissludp::reset()
{
	/* and reset the udpproxy */
	if (udpproxy)
	{
		udpproxy->reset();
	}
	/* reset for next time.*/
	stun_attempts = 0;
	return pqissl::reset();
}


void *pqissludp::generate_stun_pkt(struct sockaddr_in *stun_addr, int *len)
{
	/* just the header */
	void *stun_pkt = malloc(20);
	((uint16_t *) stun_pkt)[0] = 0x0001;
	((uint16_t *) stun_pkt)[1] = 0x0020; /* only header */
	/* transaction id - should be random */
	((uint32_t *) stun_pkt)[1] = 0x0020; 
	((uint32_t *) stun_pkt)[2] = 0x0121; 
	((uint32_t *) stun_pkt)[3] = 0x0111; 
	((uint32_t *) stun_pkt)[4] = 0x1010; 
	*len = 20;
	return stun_pkt;
}


int pqissludp::getStunReturnedAddr(void *stun_pkt, int len, struct sockaddr_in *fw_addr)
{
	if (((uint16_t *) stun_pkt)[0] != 0x0101)
	{
		/* not a response */
		return 0;
	}

	/* iterate through the packet */
	/* for now assume the address follows the header directly */
	fw_addr->sin_family = AF_INET;
	fw_addr->sin_addr.s_addr = ((uint32_t *) stun_pkt)[6];
	fw_addr->sin_port = ((uint16_t *) stun_pkt)[11];

	return 1;
}


// Udp Proxy service is started by the pqiudpproxy.
// connection is attempted by searching for common neighbours.
// Proxy Pkts are exchanged and an exchange of external addresses
// is done. these are passed to pqissludp, through a series
// of function calls. should all work perfectly.
//

int 	pqissludp::Reattempt_Connection()
{
  	 pqioutput(PQL_DEBUG_BASIC, pqissludpzone, 
		  "pqissludp::Reattempt_Connection() Failed Doing nothing");
	// notify the parent.
	waiting = WAITING_NOT;
	return -1;
}

	/* <===================== UDP Difference *******************/
	// The Proxy Version takes a few more step
	//
	// connectInterface is sent via message from the proxy.
	// and is set here.
	/* <===================== UDP Difference *******************/

int	pqissludp::attach(struct sockaddr_in &addr)
{
	if (waiting != WAITING_PROXY_CONNECT)
	{
  		pqioutput(PQL_WARNING, pqissludpzone, 
		  "pqissludp::attach() not WAITING_PROXY_CONNECT");
		/* should be attached already! */
		return 1;
	}

	sockfd = tou_socket(0,0,0);
	if (0 > sockfd)
	{
  		pqioutput(PQL_WARNING, pqissludpzone, 
		  "pqissludp::attach() failed to create a socket");
		return -1;
	}

	/* just fix the address to start with */
	int err = -1;
	int fails = 0;
	while(0 > (err = tou_bind(sockfd, (struct sockaddr *) &addr, sizeof(addr))))
	{
		addr.sin_port = htons(ntohs(addr.sin_port) + 1);

		std::ostringstream out;
		out << "pqissludp::attach() Changing udp bind address to: ";
		out << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port);
  		pqioutput(PQL_WARNING, pqissludpzone, out.str());
		if (fails++ > 20)
		{
  			pqioutput(PQL_WARNING, pqissludpzone, 
			  "pqissludp::attach() Too many tou_bind attempts");
			net_internal_close(sockfd);
			sockfd = -1;
			return 0;
		}
	}

	// setup remote address
  	pqioutput(PQL_WARNING, pqissludpzone, 
		  "pqissludp::attach() opening Local Udp Socket");
	
	return 1;
}


// find a suitable STUN client.
// This is choosen from a list of neighbours. (AutoDiscovery)
// and failing a suitable STUN, uses googles STUN servers.
// send out message.

int 	pqissludp::Request_Proxy_Connection()
{
	/* to start the whole thing rolling we need, to be connected
	 * (this don't match the fn name)
	 */

	/* so start a connection */
	waiting = WAITING_PROXY_CONNECT;
	proxy_timeout = time(NULL) + PQI_SSLUDP_PROXY_TIMEOUT;
  	pqioutput(PQL_DEBUG_BASIC, pqissludpzone, 
		  "pqissludp::Request_Proxy_Connection() connectattempt!");
	return udpproxy -> connectattempt();
}

int 	pqissludp::Check_Proxy_Connection()
{
	int mode;
	if (udpproxy -> isConnected(mode))
	{
  		pqioutput(PQL_WARNING, pqissludpzone, 
		  "pqissludp::Check_Proxy_Connection() isConnected!");
		sslmode = mode;
		waiting = WAITING_PROXY_CONNECT;
		/* This will switch into WAITING_LOCAL_ADDR */
		return Request_Local_Address();
	}
	if (udpproxy -> hasFailed())
	{
  		pqioutput(PQL_WARNING, pqissludpzone, 
		  "pqissludp::Check_Proxy_Connection() hasFailed!");

		/* failure */
		if (parent())
		{

			/* only notify of failure, if its an active connect attempt.
			 * ie not if waiting == WAITING_NOT and we are listening...
			 */

			if (waiting == WAITING_PROXY_CONNECT)
			{
  				pqioutput(PQL_WARNING, pqissludpzone, 
		  			"pqissludp::Check_Proxy_Connection() notifying parent - hasFailed!");

				udpproxy->reset();
				if (parent())
				{
					parent() -> notifyEvent(this, NET_CONNECT_FAILED);
				}
			}
			else
			{
  				pqioutput(PQL_WARNING, pqissludpzone, 
		  			"pqissludp::Check_Proxy_Connection() listen - not Connected....Ignore");
			}
		}
		else
		{
  			pqioutput(PQL_WARNING, pqissludpzone, 
		  		"pqissludp::Check_Proxy_Connection() no parent to notify - hasFailed!");
		}
		waiting = WAITING_FAIL_INTERFACE;
		return -1;
	}

	/* finally a proxy timeout - shouldn't be needed, but you don't know....
	 */
	if (waiting == WAITING_PROXY_CONNECT)
	{
		if (proxy_timeout < time(NULL))
		{
  			pqioutput(PQL_ALERT, pqissludpzone, 
		  		"pqissludp::Check_Proxy_Connection() Proxy Connect Timed Out!");

			udpproxy->reset();
			if (parent())
			{
				parent() -> notifyEvent(this, NET_CONNECT_FAILED);
			}
			waiting = WAITING_FAIL_INTERFACE;
			return -1;
		}
	}

	return 0;
}

int 	pqissludp::Request_Local_Address()
{
	// reset of stun_addr, restarts attempts.
	if (stun_addr.sin_addr.s_addr == 0)
	{
		stun_attempts = 0;
	}

	udpproxy->requestStunServer(stun_addr);

	/* check its valid */
	if (!isValidNet((&(stun_addr.sin_addr))))
	{
  		pqioutput(PQL_WARNING, pqissludpzone, 
			  "pqissludp::Request_Local_Address() No StunServers!");


		/* must do this manually - as we don't know if ::reset() will send notification.
		 */

		udpproxy->reset();
		if (sockfd > 0) /* might be valid - might not */
		{
			net_internal_close(sockfd);
		}
		waiting = WAITING_FAIL_INTERFACE;
		if (parent())
		{
			// not enough stun attempts. FAILED (not UNREACHABLE)
			parent() -> notifyEvent(this, NET_CONNECT_FAILED);
		}
		return -1;
	}

	/* add +1 to get correct stun addr */
	stun_addr.sin_port = htons(ntohs(stun_addr.sin_port) + 1);

	/*
	 * send of pkt.
	 */

	net_attempt = PQISSL_UDP_FLAG;

  	pqioutput(PQL_WARNING, pqissludpzone, 
		  "pqissludp::Request_Local_Address() Opening Local Socket");

	/* only if the first time */
	if (waiting == WAITING_PROXY_CONNECT)
	{
	  /* open socket. */
	  local_addr = sslccr -> getOwnCert() -> localaddr;
	  if (0 > attach(local_addr))
	  {
  		pqioutput(PQL_WARNING, pqissludpzone, 
		  "pqissludp::Request_Local_Address() Failed to Attach");

		udpproxy->reset();
		waiting = WAITING_FAIL_INTERFACE;
		parent() -> notifyEvent(this, NET_CONNECT_FAILED);
		return -1;
	  }
	}

	/* then send packet */
	int len;
	void *stun_pkt = generate_stun_pkt(&stun_addr, &len);
	if (!tou_sendto(sockfd, stun_pkt, len, 0, 
		(const sockaddr *) &stun_addr, sizeof(stun_addr)))
	{	
  		pqioutput(PQL_WARNING, pqissludpzone, 
		  "pqissludp::Request_Local_Address() Failed to Stun");
		free(stun_pkt);
		return -1;
	}

	{
		std::ostringstream out;
		out << "pqissludp::Request_Local_Address() Sent StunPkt to: ";
		out << inet_ntoa(stun_addr.sin_addr);
		out << ":" << ntohs(stun_addr.sin_port);

  		pqioutput(PQL_WARNING, pqissludpzone, out.str());
	}

	free(stun_pkt);
	waiting = WAITING_LOCAL_ADDR;
	stun_timeout = time(NULL) + PQI_SSLUDP_STUN_TIMEOUT;
	return 0;
}


int 	pqissludp::Determine_Local_Address()
{
	/* have we recieved anything from the Stun server? */
        /* now find the listenUdp port */
	struct sockaddr_in stun_addr_in;
	socklen_t addrlen = sizeof(stun_addr_in);
	int size = 1000;
	char data[size];

  	pqioutput(PQL_DEBUG_BASIC, pqissludpzone, 
			  "pqissludp::Determine_Local_Address() Waiting Local Addr - Can't do nothing,!");

	size = tou_recvfrom(sockfd, data, size, 0,
		(struct sockaddr *) &stun_addr_in, &addrlen);
		
	if (0 < size)
	{
		{
		std::ostringstream out;
		out << "pqissludp::Determine_Local_Address() Received + Decoding Stun Reply: " << size;
		pqioutput(PQL_DEBUG_BASIC, pqissludpzone, out.str());
		}
		
		/* get the firewall_address out.
		 */

		if (getStunReturnedAddr(data, size, &firewall_addr))
		{
			std::ostringstream out;
			out << "pqissludp::Listen_for_Proxy() Got ExtAddr: ";
			out << inet_ntoa(firewall_addr.sin_addr) << ":";
			out << ntohs(firewall_addr.sin_port) << std::endl;

			pqioutput(PQL_DEBUG_BASIC, pqissludpzone, out.str());

			/* initiate (or complete) a connection via the proxy */

			/* now tell the udpproxy our address */
			udpproxy -> sendExternalAddress(firewall_addr);
			waiting = WAITING_REMOTE_ADDR;
			remote_timeout = time(NULL) + PQI_SSLUDP_REMOTE_TIMEOUT;
			return Determine_Remote_Address();
		}
		else
		{
			std::ostringstream out;
			out << "pqissludp::Listen_for_Proxy() Failed to Get StunRtn Addr";
			pqioutput(PQL_DEBUG_BASIC, pqissludpzone, out.str());
		}
	}
	
	// switched to time based timeout.
	//if (stun_timeout++ > PQI_SSLUDP_STUN_TIMEOUT)
	if (stun_timeout < time(NULL))
	{
		/* too many times? */
		if (stun_attempts++ > PQI_SSLUDP_STUN_ATTEMPTS)
		{
			std::ostringstream out;
			out << "pqissludp::Listen_for_Proxy() Too Many Stun Attempts";
			out << std::endl;
			out << "Will notifyEvent(NET_CONNECT_UNREACHABLE)";
			out << std::endl;
			pqioutput(PQL_ALERT, pqissludpzone, out.str());

			reset();
			return -1;
		}

		/* retry.
		 */
		return Request_Local_Address();
	}
	return 0;
}


int 	pqissludp::Determine_Remote_Address()
{
	int ret;
	int cMode;
	if (waiting != WAITING_REMOTE_ADDR)
	{
  		pqioutput(PQL_WARNING, pqissludpzone, 
			  "pqissludp::Determine_Remote_Address() ERROR: waiting != REMOTE_ADDR!");
		return -1;
	}
		
	if (0 > (ret = udpproxy -> gotRemoteAddress(remote_addr, cMode)))
	{
  		pqioutput(PQL_WARNING, pqissludpzone, 
			  "pqissludp::Determine_Remote_Address() FAILURE to Get Remote Address");

		reset();
		waiting = WAITING_FAIL_INTERFACE;
		return -1;

	}
	if (ret == 0)
	{
  		pqioutput(PQL_DEBUG_BASIC, pqissludpzone, 
		 "pqissludp::Determine_Remote_Address() Remote Address Acquistion in progress");
		/* inprogress */

		/* check if its timed out */
		if (remote_timeout < time(NULL))
		{
  			pqioutput(PQL_WARNING, pqissludpzone, 
		 		"pqissludp::Determine_Remote_Address() Timed out: NOTIFY of FAILURE");

			reset();
			waiting = WAITING_FAIL_INTERFACE;
			return -1;
		}
		return 0;
	}

	/* success */

	sslmode = cMode; /* set the connect mode */

	std::ostringstream out;
	out << "pqissludp::Determine_Remote_Address() Success:";
	out << "Remote Address: " << inet_ntoa(remote_addr.sin_addr) << ":";
	out << ntohs(remote_addr.sin_port);
	out << " sslMode: " << (int) sslmode;
  	pqioutput(PQL_DEBUG_BASIC, pqissludpzone, out.str());

	return Initiate_Connection();
}


int 	pqissludp::Initiate_Connection()
{
	int err;
	remote_addr.sin_family = AF_INET;

  	pqioutput(PQL_DEBUG_BASIC, pqissludpzone, 
	  "pqissludp::Initiate_Connection() Attempting Outgoing Connection....");

	if (waiting != WAITING_REMOTE_ADDR)
	{
  		pqioutput(PQL_WARNING, pqissludpzone, 
		 "pqissludp::Initiate_Connection() Already Attempt in Progress!");
		return -1;
	}

  	pqioutput(PQL_DEBUG_BASIC, pqissludpzone, 
	  "pqissludp::Initiate_Connection() Opening Socket");

	{
		std::ostringstream out;
		out << "pqissludp::Initiate_Connection() ";
		out << "Connecting To: " << inet_ntoa(remote_addr.sin_addr) << ":";
		out << ntohs(remote_addr.sin_port) << std::endl;
  		pqioutput(PQL_WARNING, pqissludpzone, out.str());
	}

	if (remote_addr.sin_addr.s_addr == 0)
	{
		std::ostringstream out;
		out << "pqissludp::Initiate_Connection() ";
		out << "Invalid (0.0.0.0) Remote Address,";
		out << " Aborting Connect.";
		out << std::endl;
  		pqioutput(PQL_WARNING, pqissludpzone, out.str());
		waiting = WAITING_FAIL_INTERFACE;

		reset();
		return -1;
	}

	{ 
		std::ostringstream out;
		out << "Connecting to ";
		out << sslcert -> Name() << " via ";
		out << inet_ntoa(remote_addr.sin_addr);
		out << ":" << ntohs(remote_addr.sin_port);
  		pqioutput(PQL_DEBUG_BASIC, pqissludpzone, out.str());
	}

	udp_connect_timeout = time(NULL) + PQI_SSLUDP_CONNECT_TIMEOUT;
	/* <===================== UDP Difference *******************/
	if (0 != (err = tou_connect(sockfd, (struct sockaddr *) &remote_addr, sizeof(remote_addr))))
	/* <===================== UDP Difference *******************/
	{
		int tou_err = tou_errno(sockfd);
		std::cerr << "pqissludp::Initiate_Connection() connect returns:";
		std::cerr << err << " -> errno: " << tou_err << " error: ";
		std::cerr << socket_errorType(tou_err) << std::endl;
		
		std::ostringstream out;
		out << "pqissludp::Initiate_Connection()";

		if ((tou_err == EINPROGRESS) || (tou_err == EAGAIN))
		{
			// set state to waiting.....
			waiting = WAITING_SOCK_CONNECT;

			out << " EINPROGRESS Waiting for Socket Connection";
  		        pqioutput(PQL_WARNING, pqissludpzone, out.str());
  
			return 0;
		}
		else if ((tou_err == ENETUNREACH) || (tou_err == ETIMEDOUT))
		{
			out << "ENETUNREACHABLE: cert" << sslcert -> Name();
			out << std::endl;

			// Then send unreachable message.
			waiting = WAITING_FAIL_INTERFACE;
			net_unreachable |= net_attempt;
		}

		out << "Error: Connection Failed: " << tou_err;
		out << " - " << socket_errorType(tou_err) << std::endl;

		reset();

  		pqioutput(PQL_WARNING, pqissludpzone, out.str());
		return -1;
	}
	else
	{
  		pqioutput(PQL_WARNING, pqissludpzone,
		 "pqissludp::Init_Connection() connect returned 0");
	}

	waiting = WAITING_SOCK_CONNECT;

	pqioutput(PQL_DEBUG_BASIC, pqissludpzone, 
	  "pqissludp::Initiate_Connection() Waiting for Socket Connect");

	return 1;
}

/********* VERY DIFFERENT **********/
int 	pqissludp::Basic_Connection_Complete()
{
	pqioutput(PQL_DEBUG_BASIC, pqissludpzone, 
	  "pqissludp::Basic_Connection_Complete()...");


	if (time(NULL) > udp_connect_timeout)
	{
		pqioutput(PQL_DEBUG_BASIC, pqissludpzone, 
	  		"pqissludp::Basic_Connection_Complete() Connectoin Timed Out!");
		/* as sockfd is valid, this should close it all up */
		reset();
	}


	if (waiting != WAITING_SOCK_CONNECT)
	{
		pqioutput(PQL_DEBUG_BASIC, pqissludpzone, 
	  		"pqissludp::Basic_Connection_Complete() Wrong Mode");
		return -1;
	}

	


	/* new approach is to check for an error */
	/* check for an error */
	int err;
	if (0 != (err = tou_errno(sockfd)))
	{
		if (err == EINPROGRESS)
		{

			std::ostringstream out;
	  	  	out << "pqissludp::Basic_Connection_Complete()";
			out << "EINPROGRESS: cert" << sslcert -> Name();
  		        pqioutput(PQL_WARNING, pqissludpzone, out.str());

		}
		else if ((err == ENETUNREACH) || (err == ETIMEDOUT))
		{
			std::ostringstream out;
	  	  	out << "pqissludp::Basic_Connection_Complete()";
			out << "ENETUNREACH/ETIMEDOUT: cert";
			out << sslcert -> Name() << std::endl;

			net_unreachable |= net_attempt;

			reset();

			// Then send unreachable message.
			waiting = WAITING_FAIL_INTERFACE;

		  	out << "pqissludp::Basic_Connection_Complete()";
			out << "Error: Connection Failed: " << err;
			out << " - " << socket_errorType(err);
	  		pqioutput(PQL_WARNING, pqissludpzone, out.str());
			return -1;
		}
	}

	/* <===================== UDP Difference *******************/
	if (tou_connected(sockfd))
	/* <===================== UDP Difference *******************/
	{
  		pqioutput(PQL_WARNING, pqissludpzone, 
	  	  "pqissludp::Basic_Connection_Complete() Connection Complete!");
		return 1;
	}
	else
	{
		// not ready return -1;
  		pqioutput(PQL_WARNING, pqissludpzone, 
	  	  "pqissludp::Basic_Connection_Complete() Not Yet Ready!");
		return 0;
	}

	return -1;
}


/* New Internal Functions required to generalise tcp/udp version 
 * of the programs
 */

// used everywhere
int pqissludp::net_internal_close(int fd)
{
  	pqioutput(PQL_ALERT, pqissludpzone, 
	  	  "pqissludp::net_internal_close() -> tou_close()");
	return tou_close(fd);
}

// install udp BIO. 
int pqissludp::net_internal_SSL_set_fd(SSL *ssl, int fd)
{
  	pqioutput(PQL_DEBUG_BASIC, pqissludpzone, 
	  	  "pqissludp::net_internal_SSL_set_fd()");

	/* create the bio's */
	tou_bio =BIO_new(BIO_s_tou_socket());

	/* attach the fd's to the BIO's */
	BIO_set_fd(tou_bio, fd, BIO_NOCLOSE);
        SSL_set_bio(ssl, tou_bio, tou_bio);
	return 1;
}

int pqissludp::net_internal_fcntl_nonblock(int fd)
{
  	pqioutput(PQL_DEBUG_BASIC, pqissludpzone, 
	  	  "pqissludp::net_internal_fcntl_nonblock()");
	return 0;
}


/* These are identical to pqinetssl version */
//int 	pqissludp::status()

int	pqissludp::tick()
{
	/* check the udpproxy, if we should be listening */
	if ((waiting == WAITING_NOT) && (sslcert -> Listening()))
	{
		if (time(NULL) > listen_checktime)
		{
			listen_checktime = time(NULL) + 
				PQI_SSLUDP_PROXY_CHECK_TIME;

			/* check the Proxy anyway (every xxx secs) */
			Check_Proxy_Connection();
		}

	}

	pqissl::tick();
	udpproxy->tick();
	return 1;
}

        // listen fns call the udpproxy.
int pqissludp::listen()
{
	{
		std::ostringstream out;
		out << "pqissludp::listen()";
		pqioutput(PQL_ALERT, pqissludpzone, out.str());
	}
	return udpproxy->listen();
}

int pqissludp::stoplistening()
{
	{
		std::ostringstream out;
		out << "pqissludp::stoplistening()";
		pqioutput(PQL_ALERT, pqissludpzone, out.str());
	}
	return udpproxy->stoplistening();
}



/********** PQI STREAMER OVERLOADING *********************************/

bool 	pqissludp::moretoread()
{
	{
		std::ostringstream out;
		out << "pqissludp::moretoread()";
		out << "  polling socket (" << sockfd << ")";
		pqioutput(PQL_DEBUG_ALL, pqissludpzone, out.str());
	}

	/* check for more to read first ... if nothing... check error
	 */
	/* <===================== UDP Difference *******************/
	if (tou_maxread(sockfd))
	/* <===================== UDP Difference *******************/
	{
		pqioutput(PQL_DEBUG_BASIC, pqissludpzone, 
			"pqissludp::moretoread() Data to Read!");
		return 1;
	}

	/* else check the error */
	pqioutput(PQL_DEBUG_ALL, pqissludpzone, 
		"pqissludp::moretoread() No Data to Read!");

	int err;
	if (0 != (err = tou_errno(sockfd)))
	{
		if ((err == EAGAIN) || (err == EINPROGRESS))
		{

			std::ostringstream out;
	  	  	out << "pqissludp::moretoread() ";
			out << "EAGAIN/EINPROGRESS: cert" << sslcert -> Name();
  		        pqioutput(PQL_WARNING, pqissludpzone, out.str());
			return 0;

		}
		else if ((err == ENETUNREACH) || (err == ETIMEDOUT))
		{
			std::ostringstream out;
	  	  	out << "pqissludp::moretoread() ";
			out << "ENETUNREACH/ETIMEDOUT: cert";
			out << sslcert -> Name();
  		        pqioutput(PQL_WARNING, pqissludpzone, out.str());

		}
		else if (err == EBADF)
		{
			std::ostringstream out;
	  	  	out << "pqissludp::moretoread() ";
			out << "EBADF: cert";
			out << sslcert -> Name();
  		        pqioutput(PQL_WARNING, pqissludpzone, out.str());

		}
		else 
		{
			std::ostringstream out;
	  	  	out << "pqissludp::moretoread() ";
			out << " Unknown ERROR: " << err << ": cert";
			out << sslcert -> Name();
  		        pqioutput(PQL_WARNING, pqissludpzone, out.str());

		}

		reset();
		return 0;
	}

	/* otherwise - not error - strange! */
	pqioutput(PQL_DEBUG_BASIC, pqissludpzone, 
		"pqissludp::moretoread() No Data + No Error (really nothing)");

	return 0;


}

bool 	pqissludp::cansend()
{
	pqioutput(PQL_DEBUG_ALL, pqissludpzone, 
		"pqissludp::cansend() polling socket!");

	/* <===================== UDP Difference *******************/
	return (0 < tou_maxwrite(sockfd));
	/* <===================== UDP Difference *******************/

}



pqiudplistener::pqiudplistener(p3udpproxy *p, struct sockaddr_in addr)
	:p3u(p), sockfd(-1), active(false), laststun(0), firstattempt(0), 
	lastattempt(0)
{
	pqioutput(PQL_DEBUG_BASIC, pqissludpzone, 
		"pqiudplistener::pqiudplistener()");

	setListenAddr(addr);
	setuplisten();

	stunpkt = malloc(1024); /* 20 = size of a stun request */
	stunpktlen = 1024;
}


int     pqiudplistener::setListenAddr(struct sockaddr_in addr)
{
	laddr = addr;
	// increment the port by 1. (can't open on same)
	laddr.sin_port = htons(ntohs(laddr.sin_port) + 1);

	return 1;
}

int     pqiudplistener::resetlisten()
{
	if (sockfd > -1)
	{
		/* close it down */
		tou_close(sockfd);
		sockfd = -1;
	}
	active = false;
	return 1;
}

int     pqiudplistener::setuplisten()
{
	sockfd = tou_socket(0,0,0);
	if (0 == tou_bind(sockfd, (struct sockaddr *) &laddr, sizeof(laddr)))
	{
		active = true;
		pqioutput(PQL_DEBUG_BASIC, pqissludpzone, "pqiudplistener::setuplisten Succeeded!");
		return 1;
	}
        active = false;
	pqioutput(PQL_DEBUG_BASIC, pqissludpzone, "pqiudplistener::setuplisten Failed!");

	std::cerr << "pqiudplistener failed to bind to :" << inet_ntoa(laddr.sin_addr);
	std::cerr << ":" << ntohs(laddr.sin_port) << std::endl;
	return 0;
}

int     pqiudplistener::status()
{

	return 1;
}

int     pqiudplistener::tick()
{
	int dsize = 1024;
	char data[dsize];
	struct sockaddr_in addr, pot_ext_addr;

	/* must check if address is okay. 
	 */
        if (!active)
	{
		/* can't do much ... */
		return 1;
	}

	serverStun();

	if (recvfrom(data, &dsize, addr))
	{
		if (response(data, dsize, pot_ext_addr))
		{
			checkExtAddr(addr, data, dsize, pot_ext_addr);
		}
		else
		{
			reply(data, dsize, addr);
		}
	}

	return 1;
}

const int MIN_STUN_PERIOD = 24 * 60 * 60; /* 1 day */
const int MIN_STUN_GAP = 10; /* seconds. */
const int MAX_STUN_ATTEMPTS = 60; /* 6 * 10 sec attempts */
const int STUN_RETRY_PERIOD = 10; // * 60; /* 20 minutes */

int     pqiudplistener::serverStun()
{
	pqioutput(PQL_DEBUG_BASIC, pqissludpzone, "pqiudplistener::serverStun()");
	/* get the list from p3disc() */
	int ts = time(NULL);
	if (!firstattempt) firstattempt = ts;

	/* if not stunned */
	if ((!laststun) && (ts - firstattempt > MAX_STUN_ATTEMPTS))
	{
		pqioutput(PQL_DEBUG_BASIC, pqissludpzone, "pqiudplistener::Stun Paused!");
		/* will fail until RETRY_PERIOD */
		if (ts - firstattempt > STUN_RETRY_PERIOD)
		{
			firstattempt = ts;
		}

		return 0;
	}

	if (ts - laststun < MIN_STUN_PERIOD)
	{
		pqioutput(PQL_DEBUG_BASIC, pqissludpzone, "pqiudplistener::Stun Paused! 2");
		return 0;
	}

	if (ts - lastattempt < MIN_STUN_GAP)
	{
		pqioutput(PQL_DEBUG_BASIC, pqissludpzone, "pqiudplistener::Stun Paused! 3");
		return 0;
	}

	lastattempt = ts;

	/* stun someone */
	if (!p3u -> requestStunServer(stun_addr))
	{
		pqioutput(PQL_DEBUG_BASIC, pqissludpzone, 
			"pqiudplistener::serverStun() No Stun Server");
		/* failed, wait a bit */
		return 0;
	}

	/* send out a stun packet -> save in the local variable */

	int tmplen = stunpktlen;
	bool done = generate_stun_pkt(stunpkt, &tmplen);
	if (!done)
		return 0;

	/* increment the port +1 */
	stun_addr.sin_port = htons(ntohs(stun_addr.sin_port) + 1);
	/* and send it off */
 	// int sentlen = 
	tou_sendto(sockfd, stunpkt, tmplen, 0, 
		(const struct sockaddr *) &stun_addr, sizeof(stun_addr));

	std::ostringstream out;
	out << "pqiudplistener::serverStun() Sent Stun Packet to:";
	out << inet_ntoa(stun_addr.sin_addr) << ":" << ntohs(stun_addr.sin_port);
	pqioutput(PQL_DEBUG_BASIC, pqissludpzone, out.str());

	return 1;
}



bool    pqiudplistener::response(void *stun_pkt, int size, struct sockaddr_in &addr)
{
	/* check what type it is */
	if (size < 28)
	{
		return false;
	}

	if (((uint16_t *) stun_pkt)[0] != 0x0101)
	{
		/* not a response */
		return false;
	}

	/* iterate through the packet */
	/* for now assume the address follows the header directly */
	/* all stay in netbyteorder! */
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ((uint32_t *) stun_pkt)[6];
	addr.sin_port = ((uint16_t *) stun_pkt)[11];


	std::ostringstream out;
	out << "pqiudplistener::response() Recvd a Stun Response, ext_addr: ";
	out << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port);
	pqioutput(PQL_ALERT, pqissludpzone, out.str());

	return true;

}

int    pqiudplistener::checkExtAddr(struct sockaddr_in &src_addr,
			void *data, int size, struct sockaddr_in &ext_addr)
{

	/* display it */
	{
	  std::ostringstream out;
	  out << "pqiudplistener::checkExtAddr(): received Stun Pkt" << std::endl;
	  out << "\t\text_addr: ";
	  out << inet_ntoa(ext_addr.sin_addr) << ":" << ntohs(ext_addr.sin_port);
	  out << std::endl;
	  out << "\t\tsrc_addr: ";
	  out << inet_ntoa(src_addr.sin_addr) << ":" << ntohs(src_addr.sin_port);
	  pqioutput(PQL_WARNING, pqissludpzone, out.str());
	}

	/* decide if its better than the current external address */

	cert *own = getSSLRoot() -> getOwnCert();


	/* if src is same network as local, or it is not external ... then ignore */
	if ((sameNet(&(src_addr.sin_addr), &(own->localaddr.sin_addr))) ||
		(!isExternalNet(&(src_addr.sin_addr))))
	{
		std::ostringstream out;
		out << "Cannot use returned StunAddr:";
		out << " srcAddr !Ext || sameNet(srcAddr, localAddr)";
		pqioutput(PQL_WARNING, pqissludpzone, out.str());
		return 0;
	}

	/* if extAddr is not ext, then don't accept */
	if (!isExternalNet(&(ext_addr.sin_addr)))
	{
		std::ostringstream out;
		out << "Cannot use returned StunAddr:";
		out << " returnedAddr is !External";
		pqioutput(PQL_WARNING, pqissludpzone, out.str());
		return 0;
	}

	/* port is harder....
	 * there are a couple of cases.
	 * --- NOT Firewalled ---------------------
	 * (1) ext = local, port should = local.port.
	 * --- Firewalled    ---------------------
	 * (2) own->Forwarded(), keep old port number.
	 * (3) behind firewall -> port is irrelevent, can't connect. put 0.
	 */

	/* default is case (3) */
	unsigned short server_port = 0;
	/* incase they have actually forwarded a port, set it correct! */
	server_port = ntohs(ext_addr.sin_port) - 1;

	if (isValidNet(&(own->serveraddr.sin_addr)))
	{
		/* Final extra check....
		 * if src Address is same network as current server addr...
		 * then its not far enough away 
		 */

		/*************************** 
		 * ... but until network is well established we wont switch this
		 * rule on...
		 *
		if (isSameNet(src_addr, own->serveraddr))
		{
			std::ostringstream out;
			out << "Cannot use returned StunAddr:";
			out << " sameNet(srcAddr, current srvAddr)";
			pqioutput(PQL_WARNING, pqissludpzone, out.str());
			return 0;
		}
		 *
		 *
		 **************************/

		/* port case (2) */
		if (own->Forwarded())
		{
			/* this *MUST* have been set by the user, so
			 * keep the port number....
			 */
			server_port = ntohs(own->serveraddr.sin_port);

			std::ostringstream out;
			out << "We are marked as Forwarded(), using existing port: ";
			out << server_port;
			pqioutput(PQL_WARNING, pqissludpzone, out.str());
		}
	}

	/* port case (1) */
	if (0 == inaddr_cmp(ext_addr, own->localaddr))
	{
		server_port = ntohs(own->localaddr.sin_port);
		{
		  std::ostringstream out;
		  out << "extAddr == localAddr, Not Firewalled() + using local port: ";
		  out << server_port;
		  pqioutput(PQL_WARNING, pqissludpzone, out.str());
		}

		/* if it is the same as local address -> then not firewalled */
		if (own->Firewalled())
		{
		  std::ostringstream out;
		  out << "We are currently Marked as Firewalled, changing to NotFirewalled()";
		  own->Firewalled(false);
		  pqioutput(PQL_WARNING, pqissludpzone, out.str());
		}
	}

	/* otherwise we will take the addr */
	own->serveraddr = ext_addr;

	/* set the port number finally */
	own->serveraddr.sin_port = htons(server_port);

	/* save timestamp of successful stun */
	laststun = time(NULL);

	return 1;
}


/************************** Basic Functionality ******************/
int     pqiudplistener::recvfrom(void *data, int *size, struct sockaddr_in &addr)
{
	/* check the socket */
	socklen_t addrlen = sizeof(addr);

	int rsize = tou_recvfrom(sockfd, data, *size, 0, (struct sockaddr *) &addr, &addrlen);
	if (rsize > 0)
	{
		std::ostringstream out;
		out << "pqiudplistener::recvfrom() Recved a Pkt from: ";
		out << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port);
		pqioutput(PQL_ALERT, pqissludpzone, out.str());

		*size = rsize;
		return 1;
	}
	return 0;
}

int     pqiudplistener::reply(void *data, int size, struct sockaddr_in &addr)
{
	/* so we design a new packet with the external address in it */
	int pktlen = 0;
	void *pkt = generate_stun_reply(&addr, &pktlen);

	/* and send it off */
 	int sentlen = tou_sendto(sockfd, pkt, pktlen, 0, 
			(const struct sockaddr *) &addr, sizeof(addr));

	free(pkt);

	/* display status */
	std::ostringstream out;
	out << "pqiudplistener::reply() Responding to a Stun Req, ext_addr: ";
	out << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port);
	pqioutput(PQL_ALERT, pqissludpzone, out.str());

	return sentlen;
}

bool pqiudplistener::generate_stun_pkt(void *stun_pkt, int *len)
{
	if (*len < 20)
	{
		return false;
	}

	/* just the header */
	((uint16_t *) stun_pkt)[0] = 0x0001;
	((uint16_t *) stun_pkt)[1] = 0x0020; /* only header */
	/* transaction id - should be random */
	((uint32_t *) stun_pkt)[1] = 0x0020; 
	((uint32_t *) stun_pkt)[2] = 0x0121; 
	((uint32_t *) stun_pkt)[3] = 0x0111; 
	((uint32_t *) stun_pkt)[4] = 0x1010; 
	*len = 20;
	return true;
}


void *pqiudplistener::generate_stun_reply(struct sockaddr_in *stun_addr, int *len)
{
	/* just the header */
	void *stun_pkt = malloc(28);
	((uint16_t *) stun_pkt)[0] = 0x0101;
	((uint16_t *) stun_pkt)[1] = 0x0028; /* only header + 8 byte addr */
	/* transaction id - should be random */
	((uint32_t *) stun_pkt)[1] = 0x0020; 
	((uint32_t *) stun_pkt)[2] = 0x0121; 
	((uint32_t *) stun_pkt)[3] = 0x0111; 
	((uint32_t *) stun_pkt)[4] = 0x1010; 
	/* now add address
	 *  0  1    2  3
	 * <INET>  <port>
	 * <inet address>
	 */

	((uint32_t *) stun_pkt)[6] =  stun_addr->sin_addr.s_addr;
	((uint16_t *) stun_pkt)[11] = stun_addr->sin_port;

	*len = 28;
	return stun_pkt;
}


