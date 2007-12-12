/*
 * "$Id: pqissl.cc,v 1.28 2007-03-17 19:32:59 rmf24 Exp $"
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





#include "pqi/pqissl.h"
#include "pqi/pqinetwork.h"

#include <errno.h>
#include <openssl/err.h>

#include "pqi/pqidebug.h"
#include <sstream>

#include "pqi/pqissllistener.h"

const int pqisslzone = 37714;

/*********
#define WAITING_NOT            0
#define WAITING_LOCAL_ADDR     1  
#define WAITING_REMOTE_ADDR    2  
#define WAITING_SOCK_CONNECT   3
#define WAITING_SSL_CONNECTION 4
#define WAITING_SSL_AUTHORISE  5
#define WAITING_FAIL_INTERFACE 6


#define PQISSL_PASSIVE  0x00
#define PQISSL_ACTIVE   0x01

const int PQISSL_LOCAL_FLAG = 0x01;
const int PQISSL_REMOTE_FLAG = 0x02;
const int PQISSL_UDP_FLAG = 0x02;
***********/

static const int PQISSL_MAX_READ_ZERO_COUNT = 20;
static const int PQISSL_SSL_CONNECT_TIMEOUT = 30;

/********** PQI SSL STUFF ******************************************
 *
 * A little note on the notifyEvent(FAILED)....
 *
 * this is called from 
 * (1) reset if needed!
 * (2) Determine_Remote_Address (when all options have failed).
 *
 * reset() is only called when a TCP/SSL connection has been
 * established, and there is an error. If there is a failed TCP 
 * connection, then an alternative address can be attempted.
 *
 * reset() is called from
 * (1) destruction.
 * (2) disconnect()
 * (3) bad waiting state.
 *
 * // TCP/or SSL connection already established....
 * (5) pqissl::SSL_Connection_Complete() <- okay -> cos we made a TCP connection already.
 * (6) pqissl::accept() <- okay cos something went wrong.
 * (7) moretoread()/cansend() <- okay cos 
 *
 */

pqissl::pqissl(cert *c, pqissllistener *l, PQInterface *parent)
	//:NetBinInterface(parent, c), 
	:NetBinInterface(parent, parent->PeerId()), 
	waiting(WAITING_NOT), active(false), certvalid(false), 
	sslmode(PQISSL_ACTIVE), ssl_connection(NULL), sockfd(-1), 
	sslcert(c), sslccr(NULL), pqil(l),  // no init for remote_addr.
	readpkt(NULL), pktlen(0), 
	attempt_ts(0),
	net_attempt(0), net_failure(0), net_unreachable(0), 
	sameLAN(false), n_read_zero(0)

{
	sslccr = getSSLRoot();

  	{
	  std::ostringstream out;
	  out << "pqissl for PeerId: " << PeerId();
	  pqioutput(PQL_ALERT, pqisslzone, out.str());
	}

	// check certificate
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	sslccr -> validateCertificateXPGP(sslcert);
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	sslccr -> validateCertificate(sslcert);
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	if (!(sslcert -> Valid()))
	{
  	  pqioutput(PQL_ALERT, pqisslzone, 
	    "pqissl::Warning Certificate Not Approved!");

  	  pqioutput(PQL_ALERT, pqisslzone, 
	    "\t pqissl will not initialise....");

	}

	return;
}

	pqissl::~pqissl()
{ 
  	pqioutput(PQL_ALERT, pqisslzone, 
	    "pqissl::~pqissl -> destroying pqissl");
	stoplistening(); /* remove from pqissllistener only */
	reset(); 
	return;
}


/********** Implementation of NetInterface *************************/

int	pqissl::connectattempt()
{
	// reset failures
	net_failure = 0;
	return ConnectAttempt();
}

// tells pqilistener to listen for us.
int	pqissl::listen()
{
	if (pqil)
	{
		return pqil -> addlistenaddr(sslcert, this);
	}
	return 0;
}

int 	pqissl::stoplistening()
{
	if (pqil)
	{
		pqil -> removeListenPort(sslcert);
	}
	return 1;
}

int 	pqissl::disconnect()
{
	return reset();
}

// put back on the listening queue.
int 	pqissl::reset()
{
	std::ostringstream out;

	/* a reset shouldn't cause us to stop listening 
	 * only reasons for stoplistening() are;
	 *
	 * (1) destruction.
	 * (2) connection.
	 * (3) WillListen state change
	 *
	 */

	out << "pqissl::reset():" << sslcert -> Name() << std::endl;

	out << "pqissl::reset() State Before Reset:" << std::endl;
	out << "\tActive: " << (int) active << std::endl;
	out << "\tsockfd: " << sockfd << std::endl;
	out << "\twaiting: " << waiting << std::endl;
	out << "\tssl_con: " << ssl_connection << std::endl;
	out << std::endl;

	bool neededReset = false;

	if (ssl_connection != NULL)
	{
		out << "pqissl::reset() Shutting down SSL Connection";
		out << std::endl;
		SSL_shutdown(ssl_connection);

		neededReset = true;	
	}

	if (sockfd > 0)
	{
		out << "pqissl::reset() Shutting down (active) socket";
		out << std::endl;
		net_internal_close(sockfd);
		sockfd = -1;
		neededReset = true;	
	}
	active = false;
	sockfd = -1;
	waiting = WAITING_NOT;
	ssl_connection = NULL;
	sameLAN = false;
	n_read_zero = 0;

	if (neededReset)
	{
		out << "pqissl::reset() Reset Required!" << std::endl;
		out << "pqissl::reset() Will Attempt notifyEvent(FAILED)";
		out << std::endl;
	}

	out << "pqissl::reset() Complete!" << std::endl;
	pqioutput(PQL_ALERT, pqisslzone, out.str());

	// notify people of problem!
	// but only if we really shut something down.
	if (neededReset)
	{
		// clean up the streamer 
		if (parent())
		{
		  parent() -> notifyEvent(this, NET_CONNECT_FAILED);
		}
	}
	return 1;
}


/********** End of Implementation of NetInterface ******************/
/********** Implementation of BinInterface **************************
 * Only status() + tick() are here ... as they are really related
 * to the NetInterface, and not the BinInterface,
 *
 */

/* returns ...
 * -1 if inactive.
 *  0 if connecting.
 *  1 if connected.
 */

int 	pqissl::status()
{
	int alg;

	std::ostringstream out;
	out << "pqissl::status()";
	if (active)
	{
		out << " active: " << std::endl;
		// print out connection.
		out << "Connected TO : ";
		sslccr -> printCertificate(sslcert, out);
		
		// print out cipher.
		out << "\t\tSSL Cipher:" << SSL_get_cipher(ssl_connection);
		out << " (" << SSL_get_cipher_bits(ssl_connection, &alg);
		out << ":" << alg << ") ";
		out << "Vers:" << SSL_get_cipher_version(ssl_connection);
		out << std::endl;
		out << std::endl;

	}
	else
	{
		out << " Waiting for connection!" << std::endl;
	}

	out << "pqissl::cert status : "  << sslcert -> Status() << std::endl;
	pqioutput(PQL_DEBUG_BASIC, pqisslzone, out.str());

	if (active)
	{
		return 1;
	}
	else if (waiting > 0)
	{
		return 0;
	}
	return -1;
}

	// tick......
int	pqissl::tick()
{
	//pqistreamer::tick();

	// continue existing connection attempt.
	if (!active)
	{
		// if we are waiting.. continue the connection (only)
		if (waiting > 0)
		{
			std::ostringstream out;
			out << "pqissl::tick() ";
			out << "Continuing Connection Attempt!";
			pqioutput(PQL_DEBUG_BASIC, pqisslzone, out.str());

			ConnectAttempt();
			return 1;
		}
	}
	return 1;
}

/********** End of Implementation of BinInterface ******************/
/********** Internals of SSL Connection ****************************/


int 	pqissl::ConnectAttempt()
{
	switch(waiting)
	{
		case WAITING_NOT:

  	  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::ConnectAttempt() STATE = Not Waiting, starting connection");
	
			sslmode = PQISSL_ACTIVE; /* we're starting this one */
			return Request_Proxy_Connection();

			break;
		case WAITING_PROXY_CONNECT:

  	  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::ConnectAttempt() STATE = Proxy Wait.");
	
			return Check_Proxy_Connection();

			break;
		case WAITING_LOCAL_ADDR:

  	  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::ConnectAttempt() STATE = Waiting Local Addr");

			return Determine_Local_Address();

		case WAITING_REMOTE_ADDR:

  	  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::ConnectAttempt() STATE = Waiting Remote Addr");

			return Determine_Remote_Address();

		case WAITING_SOCK_CONNECT:

  	  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::ConnectAttempt() STATE = Waiting Sock Connect");

			return Initiate_SSL_Connection();
			break;

		case WAITING_SSL_CONNECTION:

  	  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::ConnectAttempt() STATE = Waiting SSL Connection");

			return Authorise_SSL_Connection();
			break;

		case WAITING_SSL_AUTHORISE:

  	  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::ConnectAttempt() STATE = Waiting SSL Authorise");

			return Authorise_SSL_Connection();
			break;
		case WAITING_FAIL_INTERFACE:

  	  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::ConnectAttempt() Failed - Retrying");

			return Reattempt_Connection();
			break;


		default:
  	  		pqioutput(PQL_ALERT, pqisslzone, 
		 "pqissl::ConnectAttempt() STATE = Unknown - Reset");

			reset();
			break;
	}
  	pqioutput(PQL_ALERT, pqisslzone, "pqissl::ConnectAttempt() Unknown");

	return -1;
}

/****************************** REQUEST LOCAL ADDR **************************
 * Start Transaction. 
 *
 * Specifics:
 * TCP / UDP
 * TCP - null interface.
 * UDP - Proxy Connection and Exchange of Stunned addresses.
 *
 * X509 / XPGP - Same.
 *
 */
int 	pqissl::Request_Proxy_Connection()
{
	waiting = WAITING_REMOTE_ADDR;
	return Determine_Remote_Address();
}

int 	pqissl::Check_Proxy_Connection()
{
	waiting = WAITING_REMOTE_ADDR;
	return Determine_Remote_Address();
}

int 	pqissl::Request_Local_Address()
{
	waiting = WAITING_REMOTE_ADDR;
	return Determine_Remote_Address();
}

int 	pqissl::Determine_Local_Address()
{
	waiting = WAITING_REMOTE_ADDR;
	return Determine_Remote_Address();
	return -1;
}

/****************************** DETERMINE ADDR ******************************
 * Determine the Remote Address.
 *
 * Specifics:
 * TCP / UDP
 * TCP - check for which interface to use.
 * UDP - start proxy request....
 *
 * X509 / XPGP - Same.
 *
 */

int 	pqissl::Determine_Remote_Address()
{
	struct sockaddr_in addr;
    	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
		  "pqissl::Determine_Remote_Address() Finding Interface");
	if (0 < connectInterface(addr))
	{
		remote_addr = addr;
		remote_addr.sin_family = AF_INET;
		// jump unneccessary state.
		waiting = WAITING_REMOTE_ADDR;
		return Initiate_Connection();
	}
	// cannot connect!.
    	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
		  "pqissl::Request_Remote_Address() No Avail Interfaces");

	// This is one of only place that notifies of failure...
	if (parent())
	{
		parent() -> notifyEvent(this, NET_CONNECT_UNREACHABLE);
	}
	//waiting = WAITING_NOT;
	return -1;
}


/****************************** FAILED ATTEMPT ******************************
 * Determine the Remote Address.
 *
 * Specifics:
 * TCP / UDP
 * TCP - check for which interface to use.
 * UDP - check for request proxies....
 *
 * X509 / XPGP - Same.
 *
 */

int 	pqissl::Reattempt_Connection()
{
	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
		"pqissl::ConnectAttempt() Failed - Retrying");
	// flag last attempt as a failure.
	net_failure |= net_attempt;
	waiting = WAITING_NOT;

	return Request_Proxy_Connection(); /* start of the chain */
}

/****************************** MAKE CONNECTION *****************************
 * Open Socket and Initiate Connection.
 *
 * Specifics:
 * TCP / UDP
 * TCP - socket()/connect()
 * UDP - tou_socket()/tou_connect()
 *
 * X509 / XPGP - Same.
 *
 */

int 	pqissl::Initiate_Connection()
{
	int err;
	struct sockaddr_in addr = remote_addr;

  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Initiate_Connection() Attempting Outgoing Connection....");

	if (waiting != WAITING_REMOTE_ADDR)
	{
  		pqioutput(PQL_WARNING, pqisslzone, 
		 "pqissl::Initiate_Connection() Already Attempt in Progress!");
		return -1;
	}

  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Initiate_Connection() Opening Socket");

	// open socket connection to addr.
	int osock = unix_socket(PF_INET, SOCK_STREAM, 0);

	{
		std::ostringstream out;
		out << "pqissl::Initiate_Connection() osock = " << osock;
  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, out.str());
	}

	if (osock < 0)
	{
		std::ostringstream out;
		out << "pqissl::Initiate_Connection()";
		out << "Failed to open socket!" << std::endl;
		out << "Socket Error:" << socket_errorType(errno) << std::endl;
  		pqioutput(PQL_WARNING, pqisslzone, out.str());

		net_internal_close(osock);
		waiting = WAITING_FAIL_INTERFACE;
		return -1;
	}

  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Initiate_Connection() Making Non-Blocking");

        err = unix_fcntl_nonblock(osock);
	if (err < 0)
	{
		std::ostringstream out;
		out << "pqissl::Initiate_Connection()";
		out << "Error: Cannot make socket NON-Blocking: ";
		out << err << std::endl;
  		pqioutput(PQL_WARNING, pqisslzone, out.str());

		waiting = WAITING_FAIL_INTERFACE;
		net_internal_close(osock);
		return -1;
	}


	{
		std::ostringstream out;
		out << "pqissl::Initiate_Connection() ";
		out << "Connecting To: " << inet_ntoa(addr.sin_addr) << ":";
		out << ntohs(addr.sin_port) << std::endl;
  		pqioutput(PQL_WARNING, pqisslzone, out.str());
	}

	if (addr.sin_addr.s_addr == 0)
	{
		std::ostringstream out;
		out << "pqissl::Initiate_Connection() ";
		out << "Invalid (0.0.0.0) Remote Address,";
		out << " Aborting Connect.";
		out << std::endl;
  		pqioutput(PQL_WARNING, pqisslzone, out.str());
		waiting = WAITING_FAIL_INTERFACE;
		net_internal_close(osock);
		return -1;
	}

	{ 
		std::ostringstream out;
		out << "Connecting to ";
		out << sslcert -> Name() << " via ";
		out << inet_ntoa(addr.sin_addr);
		out << ":" << ntohs(addr.sin_port);
  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, out.str());
	}

	if (0 != (err = unix_connect(osock, (struct sockaddr *) &addr, sizeof(addr))))
	{
		std::ostringstream out;
		out << "pqissl::Initiate_Connection() connect returns:";
		out << err << " -> errno: " << errno << " error: ";
		out << socket_errorType(errno) << std::endl;
		
		if (errno == EINPROGRESS)
		{
			// set state to waiting.....
			waiting = WAITING_SOCK_CONNECT;
			sockfd = osock;

			out << " EINPROGRESS Waiting for Socket Connection";
  		        pqioutput(PQL_WARNING, pqisslzone, out.str());
  
			return 0;
		}
		else if ((errno == ENETUNREACH) || (errno == ETIMEDOUT))
		{
			out << "ENETUNREACHABLE: cert" << sslcert -> Name();
  		        pqioutput(PQL_WARNING, pqisslzone, out.str());

			// Then send unreachable message.
			net_internal_close(osock);
			osock=-1;
			//reset();

			waiting = WAITING_FAIL_INTERFACE;
			// removing unreachables...
			//net_unreachable |= net_attempt;

			return -1;
		}

		/* IF we get here ---- we Failed for some other reason. 
                 * Should abandon this interface 
		 * Known reasons to get here: EINVAL (bad address)
		 */

		out << "Error: Connection Failed: " << errno;
		out << " - " << socket_errorType(errno) << std::endl;

		net_internal_close(osock);
		osock=-1;
		waiting = WAITING_FAIL_INTERFACE;

  		pqioutput(PQL_WARNING, pqisslzone, out.str());
		// extra output for the moment.
		std::cerr << out.str();

		return -1;
	}
	else
	{
  		pqioutput(PQL_WARNING, pqisslzone,
		 "pqissl::Init_Connection() connect returned 0");
	}

	waiting = WAITING_SOCK_CONNECT;
	sockfd = osock;

	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Initiate_Connection() Waiting for Socket Connect");

	return 1;
}


/******************************  CHECK SOCKET   *****************************
 * Check the Socket.
 *
 * select() and getsockopt().
 *
 * Specifics:
 * TCP / UDP
 * TCP - select()/getsockopt()
 * UDP - tou_error()
 *
 * X509 / XPGP - Same.
 *
 */

int 	pqissl::Basic_Connection_Complete()
{
	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Basic_Connection_Complete()...");

	if (waiting != WAITING_SOCK_CONNECT)
	{
		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  		"pqissl::Basic_Connection_Complete() Wrong Mode");
		return -1;
	}
	// use select on the opened socket.
	// Interestingly - This code might be portable....
	
	fd_set ReadFDs, WriteFDs, ExceptFDs;
	FD_ZERO(&ReadFDs);
	FD_ZERO(&WriteFDs);
	FD_ZERO(&ExceptFDs);

	FD_SET(sockfd, &ReadFDs);
	FD_SET(sockfd, &WriteFDs);
	FD_SET(sockfd, &ExceptFDs);

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Basic_Connection_Complete() Selecting ....");

	int sr = 0;
	if (0 > (sr = select(sockfd + 1, 
			&ReadFDs, &WriteFDs, &ExceptFDs, &timeout))) 
	{
		// select error.
  		pqioutput(PQL_WARNING, pqisslzone, 
	  	  "pqissl::Basic_Connection_Complete() Select ERROR(1)");
		
		net_internal_close(sockfd);
		sockfd=-1;
		//reset();
		waiting = WAITING_FAIL_INTERFACE;
		return -1;
	}

	{
		std::ostringstream out;
		out << "pqissl::Basic_Connection_Complete() Select ";
		out << " returned " << sr;
  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, out.str());
	}
		

	if (FD_ISSET(sockfd, &ExceptFDs))
	{
		// error - reset socket.
		// this is a definite bad socket!.
		
  		pqioutput(PQL_WARNING, pqisslzone, 
	  	  "pqissl::Basic_Connection_Complete() Select ERROR(2)");
		
		net_internal_close(sockfd);
		sockfd=-1;
		//reset();
		waiting = WAITING_FAIL_INTERFACE;
		return -1;
	}

	if (FD_ISSET(sockfd, &WriteFDs))
	{
  		pqioutput(PQL_WARNING, pqisslzone, 
	  	  "pqissl::Basic_Connection_Complete() Can Write!");
	}
	else
	{
		// not ready return -1;
  		pqioutput(PQL_WARNING, pqisslzone, 
	  	  "pqissl::Basic_Connection_Complete() Not Yet Ready!");
		return 0;
	}

	if (FD_ISSET(sockfd, &ReadFDs))
	{
  		pqioutput(PQL_WARNING, pqisslzone, 
	  	  "pqissl::Basic_Connection_Complete() Can Read!");
	}
	else
	{
		// not ready return -1;
  		pqioutput(PQL_WARNING, pqisslzone, 
	  	  "pqissl::Basic_Connection_Complete() Cannot Read!");
	}

	int err = 1;
	if (0==unix_getsockopt_error(sockfd, &err))
	{
		if (err == 0)
		{

			{
			std::ostringstream out;
	  	  	out << "pqissl::Basic_Connection_Complete()";
			out << "TCP Connection Complete: cert: ";
			out << sslcert -> Name();
			out << " on osock: " << sockfd;
  		        pqioutput(PQL_WARNING, pqisslzone, out.str());
			}
			return 1;
		}
		else if (err == EINPROGRESS)
		{

			std::ostringstream out;
	  	  	out << "pqissl::Basic_Connection_Complete()";
			out << "EINPROGRESS: cert" << sslcert -> Name();
  		        pqioutput(PQL_WARNING, pqisslzone, out.str());

			return 0;
		}
		else if ((err == ENETUNREACH) || (err == ETIMEDOUT))
		{
			std::ostringstream out;
	  	  	out << "pqissl::Basic_Connection_Complete()";
			out << "ENETUNREACH/ETIMEDOUT: cert";
			out << sslcert -> Name();
  		        pqioutput(PQL_WARNING, pqisslzone, out.str());

			// Then send unreachable message.
			net_internal_close(sockfd);
			sockfd=-1;
			//reset();
			
			waiting = WAITING_FAIL_INTERFACE;
			// removing unreachables...
			//net_unreachable |= net_attempt;

			return -1;
		}
		else if ((err == EHOSTUNREACH) || (err == EHOSTDOWN))
		{
			std::ostringstream out;
	  	  	out << "pqissl::Basic_Connection_Complete()";
			out << "EHOSTUNREACH/EHOSTDOWN: cert";
			out << sslcert -> Name();
  		        pqioutput(PQL_WARNING, pqisslzone, out.str());

			// Then send unreachable message.
			net_internal_close(sockfd);
			sockfd=-1;
			//reset();
			waiting = WAITING_FAIL_INTERFACE;

			return -1;
		}
		else if ((err == ECONNREFUSED))
		{
			std::ostringstream out;
	  	  	out << "pqissl::Basic_Connection_Complete()";
			out << "ECONNREFUSED: cert";
			out << sslcert -> Name();
  		        pqioutput(PQL_WARNING, pqisslzone, out.str());

			// Then send unreachable message.
			net_internal_close(sockfd);
			sockfd=-1;
			//reset();
			waiting = WAITING_FAIL_INTERFACE;

			return -1;
		}
			
		std::ostringstream out;
		out << "Error: Connection Failed UNKNOWN ERROR: " << err;
		out << " - " << socket_errorType(err);
  		pqioutput(PQL_WARNING, pqisslzone, out.str());

		net_internal_close(sockfd);
		sockfd=-1;
		//reset(); // which will send Connect Failed,
		return -1;
	}

  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Basic_Connection_Complete() BAD GETSOCKOPT!");
	waiting = WAITING_FAIL_INTERFACE;

	return -1;
}


int 	pqissl::Initiate_SSL_Connection()
{
	int err;

  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Initiate_SSL_Connection() Checking Basic Connection");

	if (0 >= (err = Basic_Connection_Complete()))
	{
		return err;
	}

  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Initiate_SSL_Connection() Basic Connection Okay");

	// setup timeout value.
	ssl_connect_timeout = time(NULL) + PQISSL_SSL_CONNECT_TIMEOUT;

	// Perform SSL magic.
	// library already inited by sslroot().
	SSL *ssl = SSL_new(sslccr -> getCTX());
	if (ssl == NULL)
	{
  		pqioutput(PQL_ALERT, pqisslzone, 
		  "pqissl::Initiate_SSL_Connection() SSL_new failed!");

		exit(1);
		return -1;
	}
	
  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Initiate_SSL_Connection() SSL Connection Okay");

	ssl_connection = ssl;

	net_internal_SSL_set_fd(ssl, sockfd);
	if (err < 1)
	{
		std::ostringstream out;
		out << "pqissl::Initiate_SSL_Connection() SSL_set_fd failed!";
		out << std::endl;
		printSSLError(ssl, err, SSL_get_error(ssl, err), 
				ERR_get_error(), out);

  		pqioutput(PQL_ALERT, pqisslzone, out.str());
	}

  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Initiate_SSL_Connection() Waiting for SSL Connection");

	waiting = WAITING_SSL_CONNECTION;
	return 1;
}

int 	pqissl::SSL_Connection_Complete()
{
  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::SSL_Connection_Complete()??? ... Checking");

	if (waiting == WAITING_SSL_AUTHORISE)
	{
  		pqioutput(PQL_ALERT, pqisslzone, 
		  "pqissl::SSL_Connection_Complete() Waiting = W_SSL_AUTH");

		return 1;
	}
	if (waiting != WAITING_SSL_CONNECTION)
	{
  		pqioutput(PQL_ALERT, pqisslzone, 
		  "pqissl::SSL_Connection_Complete() Still Waiting..");

		return -1;
	}

  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::SSL_Connection_Complete() Attempting SSL_connect");

	/* if we are passive - then accept! */
	int err;

	if (sslmode)
	{
  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, "--------> Active Connect!");
		err = SSL_connect(ssl_connection);
	}
	else
	{
  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, "--------> Passive Accept!");
		err = SSL_accept(ssl_connection);
	}

	if (err != 1)
	{
		int serr = SSL_get_error(ssl_connection, err);
		if ((serr == SSL_ERROR_WANT_READ) 
				|| (serr == SSL_ERROR_WANT_WRITE))
		{
  			pqioutput(PQL_WARNING, pqisslzone, 
			  "Waiting for SSL handshake!");

			waiting = WAITING_SSL_CONNECTION;
			return 0;
		}


		std::ostringstream out;
		out << "pqissl::SSL_Connection_Complete()" << std::endl;
		out << "Issues with SSL Connect(" << err << ")!" << std::endl;
		printSSLError(ssl_connection, err, serr, 
				ERR_get_error(), out);

  		pqioutput(PQL_WARNING, pqisslzone, 
			out.str());

		// attempt real error.
		Extract_Failed_SSL_Certificate();

		reset();
		waiting = WAITING_FAIL_INTERFACE;

		return -1;
	}
	// if we get here... success v quickly.
	
  	pqioutput(PQL_WARNING, pqisslzone, 
		"\tAttempted Connect... Success!");

	waiting = WAITING_SSL_AUTHORISE;
	return 1;
}

int 	pqissl::Extract_Failed_SSL_Certificate()
{
  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Extract_Failed_SSL_Certificate()");

	// Get the Peer Certificate....
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	XPGP *peercert = SSL_get_peer_pgp_certificate(ssl_connection);
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	X509 *peercert = SSL_get_peer_certificate(ssl_connection);
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	if (peercert == NULL)
	{
  		pqioutput(PQL_WARNING, pqisslzone, 
		  "pqissl::Extract_Failed_SSL_Certificate() Peer Didnt Give Cert");
		return -1;
	}

  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Extract_Failed_SSL_Certificate() Have Peer Cert - Registering");

	// save certificate... (and ip locations)
	// false for outgoing....
	// we actually connected to remote_addr, 
	// 	which could be 
	//      (pqissl's case) sslcert->serveraddr or sslcert->localaddr.
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	sslccr -> registerCertificateXPGP(peercert, remote_addr, false);
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	sslccr -> registerCertificate(peercert, remote_addr, false);
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	return 1;
}




int 	pqissl::Authorise_SSL_Connection()
{
  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Authorise_SSL_Connection()");
	ssl_connect_timeout = time(NULL) + PQISSL_SSL_CONNECT_TIMEOUT;

        if (time(NULL) > ssl_connect_timeout)
        {
		pqioutput(PQL_DEBUG_BASIC, pqisslzone,
			"pqissl::Authorise_SSL_Connection() Connectoin Timed Out!");
	        /* as sockfd is valid, this should close it all up */
	        reset();
	}

	int err;
	if (0 >= (err = SSL_Connection_Complete()))
	{
		return err;
	}

  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Authorise_SSL_Connection() SSL_Connection_Complete");

	// reset switch.
	waiting = WAITING_NOT;

	// Get the Peer Certificate....
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	XPGP *peercert = SSL_get_peer_pgp_certificate(ssl_connection);
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	X509 *peercert = SSL_get_peer_certificate(ssl_connection);
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	if (peercert == NULL)
	{
  		pqioutput(PQL_WARNING, pqisslzone, 
		  "pqissl::Authorise_SSL_Connection() Peer Didnt Give Cert");


		//SSL_shutdown(ssl_connection);
		//net_internal_close(sockfd);
		//waiting = WAITING_FAIL_INTERFACE;
		//
		// Failed completely
		reset();
		return -1;
	}

  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Authorise_SSL_Connection() Have Peer Cert");

	// save certificate... (and ip locations)
	// false for outgoing....
	// we actually connected to remote_addr, 
	// 	which could be 
	//      (pqissl's case) sslcert->serveraddr or sslcert->localaddr.
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	//cert *npc = sslccr -> registerCertificateXPGP(peercert, sslcert -> serveraddr, false);
	cert *npc = sslccr -> registerCertificateXPGP(peercert, remote_addr, false);
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	//cert *npc = sslccr -> registerCertificate(peercert, sslcert -> serveraddr, false);
	cert *npc = sslccr -> registerCertificate(peercert, remote_addr, false);
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	// check it's the right one.
	if ((npc != NULL) && (!(npc -> Connected())) && 
				(sslccr -> compareCerts(sslcert, npc) == 0))
	{
		// then okay...

		// timestamp.
		npc -> lc_timestamp = time(NULL);
		// pass to accept...
		// and notify that we're likely to succeed.
	
  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  		"pqissl::Authorise_SSL_Connection() Accepting Conn");

		accept(ssl_connection, sockfd, remote_addr);
		return 1;
	}

  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Authorise_SSL_Connection() Something Wrong ... Shutdown ");

	// else shutdown ssl connection.
	//
	// failed completely...
	//SSL_shutdown(ssl_connection);
	//net_internal_close(sockfd);
	//sockfd = -1;
	//waiting = WAITING_FAIL_INTERFACE;
	reset();
	return 0;
}

int	pqissl::accept(SSL *ssl, int fd, struct sockaddr_in foreign_addr) // initiate incoming connection.
{
	if (waiting != WAITING_NOT)
	{
  	  	pqioutput(PQL_WARNING, pqisslzone, 
			"pqissl::accept() - Two connections in progress - Shut 1 down!");

		// outgoing connection in progress.
		// shut this baby down.
		//
		// Thought I should shut down one in progress, and continue existing one!
		// But the existing one might be broke.... take second.
		// all we need is to never stop listening.
		
		switch(waiting)
		{
		case WAITING_LOCAL_ADDR:

  	  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::accept() STATE = Waiting Local Addr - Nothing to close");

		case WAITING_REMOTE_ADDR:

  	  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::accept() STATE = Waiting Remote Addr - Nothing to close");

		case WAITING_SOCK_CONNECT:

  	  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::accept() STATE = Waiting Sock Connect - close the socket");

			break;

		case WAITING_SSL_CONNECTION:

  	  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::accept() STATE = Waiting SSL Connection - close sockfd + ssl_conn");

			break;

		case WAITING_SSL_AUTHORISE:

  	  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::accept() STATE = Waiting SSL Authorise - close sockfd + ssl_conn");

			break;

		case WAITING_FAIL_INTERFACE:

  	  		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::accept() STATE = Failed, ignore?");

			break;


		default:
  	  		pqioutput(PQL_ALERT, pqisslzone, 
		 		"pqissl::accept() STATE = Unknown - ignore?");

			reset();
			break;
		}

		//waiting = WAITING_FAIL_INTERFACE;
		//return -1;
	}

	/* shutdown existing - in all cases use the new one */
	if ((ssl_connection) && (ssl_connection != ssl))
	{
  	 	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
		  "pqissl::accept() closing Previous/Existing ssl_connection");
		SSL_shutdown(ssl_connection);
	}

	if ((sockfd > -1) && (sockfd != fd))
	{
  	 	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
		  "pqissl::accept() closing Previous/Existing sockfd");
		net_internal_close(sockfd);
	}


	// save ssl + sock.
	
	ssl_connection = ssl;
	sockfd = fd;

	/* if we connected - then just writing the same over, 
	 * but if from ssllistener then we need to save the address.
	 */
	remote_addr = foreign_addr; 

	/* check whether it is on the same LAN */
	cert *own = sslccr -> getOwnCert();
	struct sockaddr_in own_laddr = own -> localaddr;

	sameLAN = isSameSubnet(&(remote_addr.sin_addr), &(own_laddr.sin_addr));

	{
	  std::ostringstream out;
	  out << "pqissl::accept() checking for same LAN";
	  out << std::endl;
	  out << "\t localaddr: " << inet_ntoa(remote_addr.sin_addr);
	  out << std::endl;
	  out << "\tremoteaddr: " << inet_ntoa(own_laddr.sin_addr);
	  out << std::endl;
	  if (sameLAN)
	  {
	  	out << "\tSAME LAN - no bandwidth restrictions!";
	  }
	  else
	  {
	  	out << "\tDifferent LANs - bandwidth restrictions!";
	  }
	  out << std::endl;

  	  pqioutput(PQL_WARNING, pqisslzone, out.str());
	}

	// establish the ssl details.
	// cipher name.
	int alg;
	int err;

	{
	  std::ostringstream out;
	  out << "SSL Cipher:" << SSL_get_cipher(ssl) << std::endl;
	  out << "SSL Cipher Bits:" << SSL_get_cipher_bits(ssl, &alg);
	  out << " - " << alg << std::endl;
	  out << "SSL Cipher Version:" << SSL_get_cipher_version(ssl) << std::endl;
  	  pqioutput(PQL_DEBUG_BASIC, pqisslzone, out.str());
	}

	// make non-blocking / or check.....
        if ((err = net_internal_fcntl_nonblock(sockfd)) < 0)
	{
  	  	pqioutput(PQL_ALERT, pqisslzone, "Error: Cannot make socket NON-Blocking: ");

		active = false;
		waiting = WAITING_FAIL_INTERFACE;
		// failed completely.
		reset();
		return -1;
	}
	else
	{
  	  	pqioutput(PQL_ALERT, pqisslzone, "pqissl::accept() Socket Made Non-Blocking!");
	}

	// remove form listening set.
	// no - we want to continue listening - incase this socket is crap, and they try again.
	//stoplistening();

	active = true;
	waiting = WAITING_NOT;

	// Notify the pqiperson.... (Both Connect/Receive)
	if (parent())
	{
	  parent() -> notifyEvent(this, NET_CONNECT_SUCCESS);
	}
	return 1;
}

/********** Implementation of BinInterface **************************
 * All the rest of the BinInterface.
 *
 */

int 	pqissl::senddata(void *data, int len)
{
	int tmppktlen = SSL_write(ssl_connection, data, len);
	if (len != tmppktlen)
	{
		std::ostringstream out;
		out << "pqissl::senddata()";
		out << " Full Packet Not Sent!" << std::endl; 
		out << " -> Expected len(" << len << ") actually sent(";
		out << tmppktlen << ")" << std::endl;
	
		int err = SSL_get_error(ssl_connection, tmppktlen);
		// incomplete operations - to repeat....
		// handled by the pqistreamer...
		if (err == SSL_ERROR_SYSCALL)
		{
			out << "SSL_write() SSL_ERROR_SYSCALL";
			out << std::endl;
	        	out << "Socket Closed Abruptly.... Resetting PQIssl";
			out << std::endl;
			pqioutput(PQL_ALERT, pqisslzone, out.str());
			reset();
			return -1;
		}
		else if (err == SSL_ERROR_WANT_WRITE)
		{
			out << "SSL_write() SSL_ERROR_WANT_WRITE";
			out << std::endl;
			pqioutput(PQL_ALERT, pqisslzone, out.str());
			return -1;
		}
		else if (err == SSL_ERROR_WANT_READ)
		{
			out << "SSL_write() SSL_ERROR_WANT_READ";
			out << std::endl;
			pqioutput(PQL_ALERT, pqisslzone, out.str());
			return -1;
		}
		else
		{
			out << "SSL_write() UNKNOWN ERROR: " << err;
			out << std::endl;
			printSSLError(ssl_connection, tmppktlen, err, ERR_get_error(), out);
			out << std::endl;
	        	out << "\tResetting!";
			out << std::endl;
			pqioutput(PQL_ALERT, pqisslzone, out.str());

			reset();
			return -1;
		}
	}
	return tmppktlen;
}

int 	pqissl::readdata(void *data, int len)
{
	int tmppktlen = SSL_read(ssl_connection, data, len);
	if (len != tmppktlen)
	{
		std::ostringstream out;
		out << "pqissl::readdata()";
		out << " Full Packet Not read!" << std::endl; 
		out << " -> Expected len(" << len << ") actually read(";
		out << tmppktlen << ")" << std::endl;
		pqioutput(PQL_WARNING, pqisslzone, out.str());
	}
	// need to catch errors.....
	if (tmppktlen <= 0) // probably needs a reset.
	{
		std::ostringstream out;
		out << "pqissl::readdata()";
		out << " No Data Read ... Probably a Bad Connection" << std::endl; 
		int error = SSL_get_error(ssl_connection, tmppktlen);
		unsigned long err2 =  ERR_get_error();

		printSSLError(ssl_connection, tmppktlen, error, err2, out);

		if ((error == SSL_ERROR_ZERO_RETURN) && (err2 == 0))
		{
			/* this code will be called when
			 * (1) moretoread -> returns true. +
			 * (2) SSL_read fails.
			 *
			 * There are two ways this can happen:
			 * (1) there is a little data on the socket, but not enough
			 * for a full SSL record, so there legimitately is no error, and the moretoread()
			 * was correct, but the read fails.
			 *
			 * (2) the socket has been closed correctly. this leads to moretoread() -> true, 
			 * and ZERO error.... we catch this case by counting how many times
			 * it occurs in a row (cos the other one will not).
			 */

			++n_read_zero;
	        	out << "SSL_ERROR_ZERO_RETURN -- ";
			out << std::endl;
			out << " Has socket closed been properly closed? nReadZero: " << n_read_zero;
			out << std::endl;

			if (PQISSL_MAX_READ_ZERO_COUNT < n_read_zero)
			{
				out << "Count passed Limit, shutting down!";
				reset();
			}
				
			pqioutput(PQL_ALERT, pqisslzone, out.str());
			return 0;
		}

		/* the only real error we expect */
		if (error == SSL_ERROR_SYSCALL)
		{
			out << "SSL_read() SSL_ERROR_SYSCALL";
			out << std::endl;
	        	out << "Socket Closed Abruptly.... Resetting PQIssl";
			out << std::endl;
			pqioutput(PQL_ALERT, pqisslzone, out.str());
			reset();
			return -1;
		}
		else if (error == SSL_ERROR_WANT_WRITE)
		{
			out << "SSL_read() SSL_ERROR_WANT_WRITE";
			out << std::endl;
			pqioutput(PQL_ALERT, pqisslzone, out.str());
			return -1;
		}
		else if (error == SSL_ERROR_WANT_READ)
		{
			out << "SSL_read() SSL_ERROR_WANT_READ";
			out << std::endl;
			pqioutput(PQL_ALERT, pqisslzone, out.str());
			return -1;
		}
		else
		{
			out << "SSL_read() UNKNOWN ERROR: " << error;
			out << std::endl;
	        	out << "\tResetting!";
			pqioutput(PQL_ALERT, pqisslzone, out.str());
			reset();
			return -1;
		}

		pqioutput(PQL_ALERT, pqisslzone, out.str());
		//exit(1);
	}
	n_read_zero = 0;
	return tmppktlen;
}


// dummy function currently.
int 	pqissl::netstatus()
{
	return 1;
}

int 	pqissl::isactive()
{
	return active;
}

bool 	pqissl::bandwidthLimited()
{
	return (!sameLAN);
}

bool 	pqissl::moretoread()
{
	{
		std::ostringstream out;
		out << "pqissl::moretoread()";
		out << "  polling socket (" << sockfd << ")";
		pqioutput(PQL_DEBUG_ALL, pqisslzone, out.str());
	}

	fd_set ReadFDs, WriteFDs, ExceptFDs;
	FD_ZERO(&ReadFDs);
	FD_ZERO(&WriteFDs);
	FD_ZERO(&ExceptFDs);

	FD_SET(sockfd, &ReadFDs);
	FD_SET(sockfd, &WriteFDs);
	FD_SET(sockfd, &ExceptFDs);

	struct timeval timeout;
	timeout.tv_sec = 0; 
	timeout.tv_usec = 0; 

	if (select(sockfd + 1, &ReadFDs, &WriteFDs, &ExceptFDs, &timeout) < 0) 
	{
		pqioutput(PQL_ALERT, pqisslzone, 
			"pqissl::moretoread() Select ERROR!");
		return 0;
	}

	if (FD_ISSET(sockfd, &ExceptFDs))
	{
		// error - reset socket.
		pqioutput(PQL_ALERT, pqisslzone, 
			"pqissl::moretoread() Select Exception ERROR!");

		// this is a definite bad socket!.
		// reset.
		reset();
		return 0;
	}

	if (FD_ISSET(sockfd, &WriteFDs))
	{
		// write can work.
		pqioutput(PQL_DEBUG_ALL, pqisslzone, 
			"pqissl::moretoread() Can Write!");
	}
	else
	{
		// write can work.
		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
			"pqissl::moretoread() Can *NOT* Write!");
	}

	if (FD_ISSET(sockfd, &ReadFDs))
	{
		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
			"pqissl::moretoread() Data to Read!");
		return 1;
	}
	else
	{
		pqioutput(PQL_DEBUG_ALL, pqisslzone, 
			"pqissl::moretoread() No Data to Read!");
		return 0;
	}

}

bool 	pqissl::cansend()
{
	pqioutput(PQL_DEBUG_ALL, pqisslzone, 
		"pqissl::cansend() polling socket!");

	// Interestingly - This code might be portable....

	fd_set ReadFDs, WriteFDs, ExceptFDs;
	FD_ZERO(&ReadFDs);
	FD_ZERO(&WriteFDs);
	FD_ZERO(&ExceptFDs);

	FD_SET(sockfd, &ReadFDs);
	FD_SET(sockfd, &WriteFDs);
	FD_SET(sockfd, &ExceptFDs);

	struct timeval timeout;
	timeout.tv_sec = 0; 
	timeout.tv_usec = 0; 

	if (select(sockfd + 1, &ReadFDs, &WriteFDs, &ExceptFDs, &timeout) < 0) 
	{
		// select error.
		pqioutput(PQL_ALERT, pqisslzone, 
			"pqissl::cansend() Select Error!");

		return 0;
	}

	if (FD_ISSET(sockfd, &ExceptFDs))
	{
		// error - reset socket.
		pqioutput(PQL_ALERT, pqisslzone, 
			"pqissl::cansend() Select Exception!");

		// this is a definite bad socket!.
		// reset.
		reset();
		return 0;
	}

	if (FD_ISSET(sockfd, &WriteFDs))
	{
		// write can work.
		pqioutput(PQL_DEBUG_ALL, pqisslzone, 
			"pqissl::cansend() Can Write!");
		return 1;
	}
	else
	{
		// write can work.
		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
			"pqissl::cansend() Can *NOT* Write!");

		return 0;
	}

}


/********** End of Implementation of BinInterface ******************/







/******************* PQI NET SSL INTERFACE CHOOSER ************************
 * PQI NET SSL 
 *
 * This is the part that selects which network address the
 * SSL connection will attempt to connect to.
 *
 */


/********************* CHOOSE TARGET ********************/
int	pqissl::connectInterface(struct sockaddr_in &addr)
{
	// choose the interface to connect via.
	// either local  addr. (if same localnet)
	// or     server addr. (if not firewalled).

	// says local, local is valid, and matches a local address.
	// sounds like a lot of work, but only happens max 1 per hour / ssl.
	
	// attempt to connect to localaddr.
	// don't worry about the local flag -> try anyway. (set it afterwards)
	//              local  server  dns
	//net_attempt;    0x01   0x02  0x04
	//net_failure;    0x01   0x02  0x04
        //net_unreachable;
	
	{
		std::ostringstream out;
		out << "pqissl::connectInterface() for: " << sslcert->Name();
  		pqioutput(PQL_WARNING, pqisslzone, out.str());
	}

	if ((net_unreachable & PQISSL_LOCAL_FLAG) ||
		(net_failure & PQISSL_LOCAL_FLAG) ||
		!isValidNet(&(sslcert->localaddr.sin_addr)))
	{
  		pqioutput(PQL_WARNING, pqisslzone, 
		  "pqissl::connectInterface() Not Local");
		// not local...
		net_failure |= PQISSL_LOCAL_FLAG;
		sslcert -> Local(false);
	}
	else
	{
  		pqioutput(PQL_WARNING, pqisslzone, 
		  "pqissl::connectInterface() Using Local Address: ");


		std::list<std::string> addrs = getLocalInterfaces();
		std::list<std::string>::iterator it;
		for(it = addrs.begin(); it != addrs.end(); it++)
		{
			struct in_addr local;
			inet_aton(it -> c_str(), &local);
			if (sameNet(&local, &(sslcert -> localaddr.sin_addr)))
			{
				net_attempt = PQISSL_LOCAL_FLAG;
		 		addr = sslcert -> localaddr;
				return 1;
			}
		}
	}

	// if we get here the local failed....
	if ((net_unreachable & PQISSL_REMOTE_FLAG) ||
		(net_failure & PQISSL_REMOTE_FLAG) ||
		!isValidNet(&(sslcert->serveraddr.sin_addr)))
	{
		std::ostringstream out;
		out << "pqissl::connectInterface()";
		out << " Failure to Connect via SSL (u:";
		out << net_unreachable << ", f:";
		out << net_failure << ")";

  		pqioutput(PQL_WARNING, pqisslzone, out.str());

		// fails server test.
		net_failure |= PQISSL_REMOTE_FLAG;
		// fall through...
	}
	else if ((sslcert->Firewalled()) && (!sslcert->Forwarded()))
	{
		// setup remote address
  		pqioutput(PQL_WARNING, pqisslzone, 
		  "pqissl::connectInterface() Server Firewalled - set u");
  		pqioutput(PQL_WARNING, pqisslzone, 
		  "pqissl::connectInterface() Destination Unreachable");
		net_failure |= PQISSL_REMOTE_FLAG;


		// shouldn't flag as unreachable - as if flags change, 
		// we want to connect! -> only failure....
		//net_unreachable |= PQISSL_REMOTE_FLAG;
		
		// dont' fall through... (name resolution wont help).
		//return -1;
	}
	else
	{
		// setup remote address
  		pqioutput(PQL_WARNING, pqisslzone, 
		  "pqissl::connectInterface() Using Server Address: ");

		net_attempt = PQISSL_REMOTE_FLAG;
		addr = sslcert -> serveraddr;
		return 1;
	}

	/* Finally we attempt the dns name lookup thingy */
  	pqioutput(PQL_WARNING, pqisslzone, 
		  "pqissl::connectInterface() Attempting DNS Address lookup: ");
	

	/* The initial attempt should have cached the dns request locally.
	 * This is made at ...
	 */
	if ((net_unreachable & PQISSL_DNS_FLAG) ||
		(net_failure & PQISSL_DNS_FLAG))
	{

		/* failed already */
  		pqioutput(PQL_WARNING, pqisslzone, 
			  "pqissl::connectInterface() DNS Address Failed Already");

	}
	else /* go for it! */
	{
		/* we also set it to the server addr, as it's
		 * the best guess
		 */

		if (sslcert->hasDHT())
		{
			sslcert->serveraddr = sslcert->dhtaddr;
			std::ostringstream out;
			out << "pqissl::connectInterface() DHT:";
			out << " " << inet_ntoa(sslcert->serveraddr.sin_addr) << std::endl;

			net_attempt = PQISSL_DNS_FLAG;
		 	addr = sslcert -> serveraddr;

  			pqioutput(PQL_WARNING, pqisslzone, out.str());
			return 1;
		}

#if  (0) /* DNS name resolution */
		if (sslcert->dynDNSaddr.length () > 0)
		{
			if (LookupDNSAddr(sslcert->dynDNSaddr, sslcert->serveraddr))
			{
				/* success, load DNS */
				net_attempt = PQISSL_DNS_FLAG;
		 		addr = sslcert -> serveraddr;

				std::ostringstream out;
				out << "pqissl::connectInterface() DNS:" << sslcert->dynDNSaddr;
				out << " " << inet_ntoa(sslcert->serveraddr.sin_addr) << std::endl;

  				pqioutput(PQL_WARNING, pqisslzone, out.str());
				return 1;
			}
		}
#endif

		/* otherwise we failed */
	}

  	pqioutput(PQL_WARNING, pqisslzone, 
		  "pqissl::connectInterface() DNS Address Failed");
	/* the end of the road */
	net_failure |= PQISSL_DNS_FLAG;
	waiting = WAITING_NOT;
	return -1;
}


/************************ PQI NET SSL LISTENER ****************************
 * PQI NET SSL LISTENER
 *
 * to be worked out
 */



#if 0 /* NO LISTENER */

pqissllistener::pqissllistener(struct sockaddr_in addr)
	:laddr(addr), active(false)
{
	sslccr = getSSLRoot();
	if (!(sslccr -> active()))
	{
		pqioutput(PQL_ALERT, pqisslzone, 
			"SSL-CTX-CERT-ROOT not initialised!");

		exit(1);
	}

	setuplisten();
	return;
}

int 	pqissllistener::tick()
{
	status();
	// check listen port.
	acceptconnection();
	return continueaccepts();
}

int 	pqissllistener::status()
{
	// print certificates we are listening for.
	std::map<cert *, pqissl *>::iterator it;

	std::ostringstream out;
	out << "pqissllistener::status(): ";
	out << " Listening (" << ntohs(laddr.sin_port) << ") for Certs:" << std::endl;
	for(it = listenaddr.begin(); it != listenaddr.end(); it++)
	{
		sslccr -> printCertificate(it -> first, out);
	}
	pqioutput(PQL_DEBUG_ALL, pqisslzone, out.str());

	return 1;
}

int 	pqissllistener::addlistenaddr(cert *c, pqissl *acc)
{
	std::map<cert *, pqissl *>::iterator it;

	std::ostringstream out;

	out << "Adding to Cert Listening Addresses:" << std::endl;
	sslccr -> printCertificate(c, out);
	out << "Current Certs:" << std::endl;
	for(it = listenaddr.begin(); it != listenaddr.end(); it++)
	{
		sslccr -> printCertificate(it -> first, out);
		if (sslccr -> compareCerts(c, it -> first) == 0)
		{
			out << "pqissllistener::addlistenaddr()";
			out << "Already listening for Certificate!";
			out << std::endl;
			
			pqioutput(PQL_DEBUG_ALERT, pqisslzone, out.str());
			return -1;

		}
	}

	pqioutput(PQL_DEBUG_BASIC, pqisslzone, out.str());

	// not there can accept it!
	listenaddr[c] = acc;
	return 1;
}




int	pqissllistener::setuplisten()
{
        int err;
	if (active)
		return -1;

        lsock = socket(PF_INET, SOCK_STREAM, 0);
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX
        if (lsock < 0)
        {
		pqioutput(PQL_ALERT, pqisslzone, 
		 "pqissllistener::setuplisten() Cannot Open Socket!");

		return -1;
	}

        err = fcntl(lsock, F_SETFL, O_NONBLOCK);
	if (err < 0)
	{
		std::ostringstream out;
		out << "Error: Cannot make socket NON-Blocking: ";
		out << err << std::endl;
		pqioutput(PQL_ERROR, pqisslzone, out.str());

		return -1;
	}

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else //WINDOWS_SYS 
        if ((unsigned) lsock == INVALID_SOCKET)
        {
		std::ostringstream out;
		out << "pqissllistener::setuplisten()";
		out << " Cannot Open Socket!" << std::endl;
		out << "Socket Error:";
		out  << socket_errorType(WSAGetLastError()) << std::endl;
		pqioutput(PQL_ALERT, pqisslzone, out.str());

		return -1;
	}

	// Make nonblocking.
	unsigned long int on = 1;
	if (0 != (err = ioctlsocket(lsock, FIONBIO, &on)))
	{
		std::ostringstream out;
		out << "pqissllistener::setuplisten()";
		out << "Error: Cannot make socket NON-Blocking: ";
		out << err << std::endl;
		out << "Socket Error:";
		out << socket_errorType(WSAGetLastError()) << std::endl;
		pqioutput(PQL_ALERT, pqisslzone, out.str());

		return -1;
	}
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

	// setup listening address.

	// fill in fconstant bits.

	laddr.sin_family = AF_INET;

	{
		std::ostringstream out;
		out << "pqissllistener::setuplisten()";
		out << "\tAddress Family: " << (int) laddr.sin_family;
		out << std::endl;
		out << "\tSetup Address: " << inet_ntoa(laddr.sin_addr);
		out << std::endl;
		out << "\tSetup Port: " << ntohs(laddr.sin_port);

		pqioutput(PQL_DEBUG_BASIC, pqisslzone, out.str());
	}

	if (0 != (err = bind(lsock, (struct sockaddr *) &laddr, sizeof(laddr))))
	{
		std::ostringstream out;
		out << "pqissllistener::setuplisten()";
		out << " Cannot Bind to Local Address!" << std::endl;
		showSocketError(out);
		pqioutput(PQL_ALERT, pqisslzone, out.str());

		exit(1); 
		return -1;
	}
	else
	{
		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
		  "pqissllistener::setuplisten() Bound to Address.");
	}

	if (0 != (err = listen(lsock, 100)))
	{
		std::ostringstream out;
		out << "pqissllistener::setuplisten()";
		out << "Error: Cannot Listen to Socket: ";
		out << err << std::endl;
		showSocketError(out);
		pqioutput(PQL_ALERT, pqisslzone, out.str());

		exit(1); 
		return -1;
	}
	else
	{
		pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
		  "pqissllistener::setuplisten() Listening to Socket");
	}
	active = true;
	return 1;
}

int	pqissllistener::setListenAddr(struct sockaddr_in addr)
{
	laddr = addr;
	return 1;
}

int	pqissllistener::resetlisten()
{
	if (active)
	{
		// close ports etc.
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX
		shutdown(lsock, SHUT_RDWR);	
		close(lsock);
#else //WINDOWS_SYS 
		closesocket(lsock);
#endif 
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

		active = false;
		return 1;
	}

	return 0;
}


int	pqissllistener::acceptconnection()
{
	if (!active)
		return 0;
	// check port for any socets...
	pqioutput(PQL_DEBUG_ALL, pqisslzone, "pqissllistener::accepting()");

	// These are local but temp variables...
	// can't be arsed making them all the time.
	addrlen = sizeof(raddr);
	int fd = accept(lsock, (struct sockaddr *) &raddr, &addrlen);
	int err = 0;

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX
        if (fd < 0)
        {
		pqioutput(PQL_DEBUG_ALL, pqisslzone, 
		 "pqissllistener::acceptconnnection() Nothing to Accept!");
		return 0;
	}

        err = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (err < 0)
	{
		std::ostringstream out;
		out << "pqissllistener::acceptconnection()";
		out << "Error: Cannot make socket NON-Blocking: ";
		out << err << std::endl;
		pqioutput(PQL_ERROR, pqisslzone, out.str());

		close(fd);
		return -1;
	}

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else //WINDOWS_SYS 
        if ((unsigned) fd == INVALID_SOCKET)
        {
		pqioutput(PQL_DEBUG_ALL, pqisslzone, 
		 "pqissllistener::acceptconnnection() Nothing to Accept!");
		return 0;
	}

	// Make nonblocking.
	unsigned long int on = 1;
	if (0 != (err = ioctlsocket(fd, FIONBIO, &on)))
	{
		std::ostringstream out;
		out << "pqissllistener::acceptconnection()";
		out << "Error: Cannot make socket NON-Blocking: ";
		out << err << std::endl;
		out << "Socket Error:";
		out << socket_errorType(WSAGetLastError()) << std::endl;
		pqioutput(PQL_ALERT, pqisslzone, out.str());

		closesocket(fd);
		return 0;
	}
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

	{
	  std::ostringstream out;
	  out << "Accepted Connection from ";
	  out << inet_ntoa(raddr.sin_addr) << ":" << ntohs(raddr.sin_port);
	  pqioutput(PQL_DEBUG_BASIC, pqisslzone, out.str());
	}

	// Negotiate certificates. SSL stylee.
	// Allow negotiations for secure transaction.
	
	SSL *ssl = SSL_new(sslccr -> getCTX());
	SSL_set_fd(ssl, fd);

	return continueSSL(ssl, true); // continue and save if incomplete.
}

int	pqissllistener::continueSSL(SSL *ssl, bool addin)
{
	// attempt the accept again.
	int fd =  SSL_get_fd(ssl);
	int err = SSL_accept(ssl);
	if (err <= 0)
	{
		int ssl_err = SSL_get_error(ssl, err);
		int err_err = ERR_get_error();

		{
	  	  std::ostringstream out;
	  	  out << "pqissllistener::continueSSL() ";
		  out << "Issues with SSL Accept(" << err << ")!" << std::endl;
		  printSSLError(ssl, err, ssl_err, err_err, out);
	  	  pqioutput(PQL_DEBUG_BASIC, pqisslzone, out.str());
		}

		if ((ssl_err == SSL_ERROR_WANT_READ) || 
		   (ssl_err == SSL_ERROR_WANT_WRITE))
		{
	  		std::ostringstream out;
	  	        out << "pqissllistener::continueSSL() ";
			out << " Connection Not Complete!";
			out << std::endl;

			if (addin)
			{
	  	        	out << "pqissllistener::continueSSL() ";
				out << "Adding SSL to incoming!";

				// add to incomingqueue.
				incoming_ssl.push_back(ssl);
			}

	  	        pqioutput(PQL_DEBUG_BASIC, pqisslzone, out.str());

			// zero means still continuing....
			return 0;
		}

		/* we have failed -> get certificate if possible */
		Extract_Failed_SSL_Certificate(ssl, &raddr);

		// other wise delete ssl connection.
		// kill connection....
		// so it will be removed from cache.
		SSL_shutdown(ssl);

		// close socket???
/************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX
		shutdown(fd, SHUT_RDWR);	
		close(fd);
#else //WINDOWS_SYS 
		closesocket(fd);
#endif 
/************************** WINDOWS/UNIX SPECIFIC PART ******************/
		// free connection.
		SSL_free(ssl);

		std::ostringstream out;
		out << "Read Error on the SSL Socket";
		out << std::endl;
		out << "Shutting it down!" << std::endl;
  	        pqioutput(PQL_WARNING, pqisslzone, out.str());

		// failure -1, pending 0, sucess 1.
		return -1;
	}
	

	// if it succeeds

	// Get the Peer Certificate....
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	XPGP *peercert = SSL_get_peer_pgp_certificate(ssl);
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	X509 *peercert = SSL_get_peer_certificate(ssl);
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	if (peercert == NULL)
	{
  	        pqioutput(PQL_WARNING, pqisslzone, 
		 "pqissl::connectattempt() Peer Did Not Provide Cert!");

		// delete ssl connection.
		SSL_shutdown(ssl);

		// close socket???
/************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX
		shutdown(fd, SHUT_RDWR);	
		close(fd);
#else //WINDOWS_SYS 
		closesocket(fd);
#endif 
/************************** WINDOWS/UNIX SPECIFIC PART ******************/
		// free connection.
		SSL_free(ssl);

		std::ostringstream out;
		out << "Shutting it down!" << std::endl;
  	        pqioutput(PQL_WARNING, pqisslzone, out.str());

		// failure -1, pending 0, sucess 1.
		return -1;
	}


	// save certificate... (and ip locations)
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	cert *npc = sslccr -> registerCertificateXPGP(peercert, raddr, true);
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	cert *npc = sslccr -> registerCertificate(peercert, raddr, true);
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	bool found = false;
	std::map<cert *, pqissl *>::iterator it;

	if ((npc == NULL) || (npc -> Connected()))
	{
		// bad - shutdown.
	}
	else
	{
		std::ostringstream out;

		out << "pqissllistener::continueSSL()" << std::endl;
		out << "checking: " << npc -> Name() << std::endl;
		// check if cert is in our list.....
		for(it = listenaddr.begin();(found!=true) && (it!=listenaddr.end());)
		{
		 	out << "\tagainst: " << it->first->Name() << std::endl;
			if (sslccr -> compareCerts(it -> first, npc) == 0)
			{
		 	        out << "\t\tMatch!";
				found = true;
			}
			else
			{
				it++;
			}
		}

  	        pqioutput(PQL_DEBUG_BASIC, pqisslzone, out.str());
	}
	
	if (found == false)
	{
		// kill connection....
		// so it will be removed from cache.
		SSL_shutdown(ssl);

		// close socket???
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX
		shutdown(fd, SHUT_RDWR);	
		close(fd);
#else //WINDOWS_SYS 
		closesocket(fd);
#endif 
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

		// free connection.
		SSL_free(ssl);

		std::ostringstream out;
		out << "No Matching Certificate/Already Connected";
		out << " for Connection:" << inet_ntoa(raddr.sin_addr);
		out << std::endl;
		out << "Shutting it down!" << std::endl;
  	        pqioutput(PQL_WARNING, pqisslzone, out.str());
		return -1;
	}

	pqissl *pqis = it -> second;

	// remove from the list of certificates.
	listenaddr.erase(it);

	// timestamp
	// done in sslroot... npc -> lr_timestamp = time(NULL);

	// hand off ssl conection.
	pqis -> accept(ssl, fd);
	return 1;
}


int 	pqissllistener::Extract_Failed_SSL_Certificate(SSL *ssl, struct sockaddr_in *inaddr)
{
  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissllistener::Extract_Failed_SSL_Certificate()");

	// Get the Peer Certificate....
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	XPGP *peercert = SSL_get_peer_pgp_certificate(ssl);
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	X509 *peercert = SSL_get_peer_certificate(ssl);
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	if (peercert == NULL)
	{
  		pqioutput(PQL_WARNING, pqisslzone, 
		  "pqissllistener::Extract_Failed_SSL_Certificate() Peer Didnt Give Cert");
		return -1;
	}

  	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
	  "pqissllistener::Extract_Failed_SSL_Certificate() Have Peer Cert - Registering");

	// save certificate... (and ip locations)
	// false for outgoing....
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	cert *npc = sslccr -> registerCertificateXPGP(peercert, *inaddr, true);
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	cert *npc = sslccr -> registerCertificate(peercert, *inaddr, true);
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	return 1;
}


int pqissllistener::continueSocket(int fd, bool addin)
{
	// this does nothing, as sockets
	// cannot (haven't) blocked yet!
	return 0;
}


int	pqissllistener::continueaccepts()
{

	// for each of the incoming sockets.... call continue.
	std::list<int>::iterator it;
	std::list<SSL *>::iterator its;

	for(it = incoming_skts.begin(); it != incoming_skts.end();)
	{
		if (continueSocket(*it, false))
		{
			it = incoming_skts.erase(it);
		}
		else
		{
			it++;
		}
	}
	
	for(its = incoming_ssl.begin(); its != incoming_ssl.end();)
	{
  	        pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
		  "pqissllistener::continueaccepts() Continuing SSL");
		if (0 != continueSSL(*its, false))
		{
  	        	pqioutput(PQL_DEBUG_ALERT, pqisslzone, 
			  "pqissllistener::continueaccepts() SSL Complete/Dead!");

			its = incoming_ssl.erase(its);
		}
		else
		{
			its++;
		}
	}
	return 1;
}


	

int	pqissllistener::removeListenPort(cert *c)
{
	// check where the connection is coming from.
	// if in list of acceptable addresses, 
	//
	// check if in list.
	std::map<cert *, pqissl *>::iterator it;
	for(it = listenaddr.begin();it!=listenaddr.end();it++)
	{
		if (sslccr -> compareCerts(it -> first, c) == 0)
		{
			listenaddr.erase(it);

  	        	pqioutput(PQL_DEBUG_BASIC, pqisslzone, 
			  "pqissllisten::removeListenPort() Success!");
			return 1;
		}
	}

  	pqioutput(PQL_WARNING, pqisslzone, 
	  "pqissllistener::removeListenPort() Failed to Find a Match");

	return -1;
}

#endif /* NO LISTENER */
