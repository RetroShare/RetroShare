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
#include "pqi/sslfns.h"

#include "util/rsnet.h"
#include "util/rsdebug.h"
#include "util/rsstring.h"

#include <unistd.h>
#include <errno.h>
#include <openssl/err.h>

#include "pqi/pqissllistener.h"

#include "pqi/p3linkmgr.h"
#include <retroshare/rspeers.h>
#include <retroshare/rsdht.h>
#include <retroshare/rsbanlist.h>

#include "rsserver/p3face.h"

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

#define PQISSL_DEBUG 		1
#define PQISSL_LOG_DEBUG 	1

const int PQISSL_LOCAL_FLAG = 0x01;
const int PQISSL_REMOTE_FLAG = 0x02;
const int PQISSL_UDP_FLAG = 0x02;
***********/

static const int PQISSL_MAX_READ_ZERO_COUNT = 20;
static const time_t PQISSL_MAX_READ_ZERO_TIME = 15; // 15 seconds of no data => reset. (atm HeartBeat pkt sent 5 secs)

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

pqissl::pqissl(pqissllistener *l, PQInterface *parent, p3LinkMgr *lm)
	:NetBinInterface(parent, parent->PeerId()), 
	mLinkMgr(lm), pqil(l), 
	mSslMtx("pqissl"),
	active(false), certvalid(false), waiting(WAITING_NOT), 
	sslmode(PQISSL_ACTIVE), ssl_connection(NULL), sockfd(-1), 
	readpkt(NULL), pktlen(0), total_len(0),
	attempt_ts(0),
	n_read_zero(0), mReadZeroTS(0), ssl_connect_timeout(0),
	mConnectDelay(0), mConnectTS(0),
	mConnectTimeout(0), mTimeoutTS(0)
{
	RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/

	/* set address to zero */
        sockaddr_storage_clear(remote_addr);

#ifdef PQISSL_LOG_DEBUG 
    rslog(RSL_DEBUG_BASIC, pqisslzone, "pqissl for PeerId: " + PeerId());
#endif

#if 0
	if (!(AuthSSL::getAuthSSL()->isAuthenticated(PeerId())))
	{
  	  rslog(RSL_ALERT, pqisslzone, 
	    "pqissl::Warning Certificate Not Approved!");

  	  rslog(RSL_ALERT, pqisslzone, 
	    "\t pqissl will not initialise....");

	}
#else
  	  rslog(RSL_DEBUG_BASIC, pqisslzone, 
	    "pqissl::Warning SSL Certificate Approval Not CHECKED??");
#endif

	return;
}

	pqissl::~pqissl()
{ 
  	rslog(RSL_ALERT, pqisslzone, 
	    "pqissl::~pqissl -> destroying pqissl");
	stoplistening(); /* remove from pqissllistener only */

	rslog(RSL_ALERT, pqisslzone, "pqissl::~pqissl() -> calling reset()");
	reset(); 
	return;
}


/********** Implementation of NetInterface *************************/

int	pqissl::connect(const struct sockaddr_storage &raddr)
{
	RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/

	// reset failures
	remote_addr = raddr;

	return ConnectAttempt();
}

// tells pqilistener to listen for us.
int	pqissl::listen()
{
	if (pqil)
	{
		return pqil -> addlistenaddr(PeerId(), this);
	}
	return 0;
}

int 	pqissl::stoplistening()
{
	if (pqil)
	{
		pqil -> removeListenPort(PeerId());
	}
	return 1;
}

int 	pqissl::disconnect()
{
	rslog(RSL_ALERT, pqisslzone, "pqissl::disconnect() -> calling reset()");
	return reset();
}

int pqissl::getConnectAddress(struct sockaddr_storage &raddr) 
{
	RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/

	raddr = remote_addr;
	// TODO.
	return (!sockaddr_storage_isnull(remote_addr));
}

/* BinInterface version of reset() for pqistreamer */
int 	pqissl::close() 
{
	rslog(RSL_ALERT, pqisslzone, "pqissl::close() -> calling reset()");
	return reset();
}

// put back on the listening queue.
int 	pqissl::reset()
{
	RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/

	return reset_locked();
}

int 	pqissl::reset_locked()
{
	std::string outLog;
	bool neededReset = false;

	
	/* a reset shouldn't cause us to stop listening 
	 * only reasons for stoplistening() are;
	 *
	 * (1) destruction.
	 * (2) connection.
	 * (3) WillListen state change
	 *
	 */

	outLog += "pqissl::reset():" + PeerId().toStdString();
	rs_sprintf_append(outLog, " (A: %d", (int) active);
	rs_sprintf_append(outLog, " FD: %d", sockfd);
	rs_sprintf_append(outLog, " W: %d",  waiting);
	rs_sprintf_append(outLog, " SSL: %p) ", ssl_connection);
#ifdef PQISSL_LOG_DEBUG 
	outLog += "\n";
#endif


	if (ssl_connection != NULL)
	{
		//outLog << "pqissl::reset() Shutting down SSL Connection";
		//outLog << std::endl;
		SSL_shutdown(ssl_connection);
		SSL_free (ssl_connection);

		neededReset = true;	
	}

	if (sockfd > 0)
	{
#ifdef PQISSL_LOG_DEBUG 
		outLog += "pqissl::reset() Shutting down (active) socket\n";
#endif
		net_internal_close(sockfd);
		sockfd = -1;
		neededReset = true;	
	}
	active = false;
	sockfd = -1;
	waiting = WAITING_NOT;
	ssl_connection = NULL;
	n_read_zero = 0;
	mReadZeroTS = 0;
	total_len = 0 ;
	mTimeoutTS = 0;

	if (neededReset)
	{
#ifdef PQISSL_LOG_DEBUG 
		outLog += "pqissl::reset() Reset Required!\n";
		outLog += "pqissl::reset() Will Attempt notifyEvent(FAILED)\n";
#endif
	}

	rslog(RSL_ALERT, pqisslzone, outLog);

	// notify people of problem!
	// but only if we really shut something down.
	if (neededReset)
	{
		// clean up the streamer 
		if (parent())
		{
			struct sockaddr_storage addr;
			sockaddr_storage_clear(addr);
			parent() -> notifyEvent(this, NET_CONNECT_FAILED, addr);
		}
	}
	return 1;
}

bool pqissl::connect_parameter(uint32_t type, uint32_t value)
{
#ifdef PQISSL_LOG_DEBUG
	std::cerr << "pqissl::connect_parameter() Peer: " << PeerId();
#endif

	switch(type)
	{
	case NET_PARAM_CONNECT_DELAY:
	{
#ifdef PQISSL_LOG_DEBUG
		std::cerr << " DELAY: " << value << std::endl;
#endif
		RS_STACK_MUTEX(mSslMtx);
		mConnectDelay = value;
		return true;
	}
	case NET_PARAM_CONNECT_TIMEOUT:
	{
#ifdef PQISSL_LOG_DEBUG
		std::cerr << " TIMEOUT: " << value << std::endl;
#endif
		RS_STACK_MUTEX(mSslMtx);
		mConnectTimeout = value;
		return true;
	}
	default:
	{
#ifdef PQISSL_LOG_DEBUG
		std::cerr << " type: " << type << " value: " << value << std::endl;
#endif
		return false;
	}
	}
}


/********** End of Implementation of NetInterface ******************/
/********** Implementation of BinInterface **************************
 * Only status() + tick() are here ... as they are really related
 * to the NetInterface, and not the BinInterface,
 *
 */

void pqissl::getCryptoParams(RsPeerCryptoParams& params)
{
	RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/

	if(active)
	{
		params.connexion_state = 1 ;
		params.cipher_name = std::string( SSL_get_cipher(ssl_connection));

		int alg ;
		int al2 = SSL_get_cipher_bits(ssl_connection,&alg);

		params.cipher_bits_1 = alg ;
		params.cipher_bits_2 = al2 ;

		char *desc = SSL_CIPHER_description(SSL_get_current_cipher(ssl_connection), NULL, 0);
		params.cipher_version = std::string(desc).find("TLSv1.2") != std::string::npos ? std::string("TLSv1.2") : std::string("TLSv1");
		OPENSSL_free(desc);
	}
	else
	{
		params.connexion_state = 0 ;
		params.cipher_name.clear() ;
		params.cipher_bits_1 = 0 ;
		params.cipher_bits_2 = 0 ;
		params.cipher_version.clear() ;
	}
}

bool pqissl::actAsServer()
{
	return (bool)ssl_connection->server;
}

/* returns ...
 * -1 if inactive.
 *  0 if connecting.
 *  1 if connected.
 */

int 	pqissl::status()
{
	RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/

#ifdef PQISSL_LOG_DEBUG 
	std::string out = "pqissl::status()";

	if (active)
	{
		int alg;

		out += " active: \n";
		// print out connection.
		out += "Connected TO : " + PeerId() + "\n";
		// print out cipher.
		rs_sprintf_append(out, "\t\tSSL Cipher:%s", SSL_get_cipher(ssl_connection));
		rs_sprintf_append(out, " (%d:%d)", SSL_get_cipher_bits(ssl_connection, &alg), alg);
		rs_sprintf_append(out, "Vers:%s\n\n", SSL_get_cipher_version(ssl_connection));
	}
	else
	{
		out += " Waiting for connection!\n";
	}

	rslog(RSL_DEBUG_BASIC, pqisslzone, out);
#endif

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
	RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/

	//pqistreamer::tick();

	// continue existing connection attempt.
	if (!active)
	{
		// if we are waiting.. continue the connection (only)
		if (waiting > 0)
		{
#ifdef PQISSL_LOG_DEBUG 
			rslog(RSL_DEBUG_BASIC, pqisslzone, "pqissl::tick() Continuing Connection Attempt!");
#endif

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

			sslmode = PQISSL_ACTIVE; /* we're starting this one */

#ifdef PQISSL_LOG_DEBUG 
  	  		rslog(RSL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::ConnectAttempt() STATE = Not Waiting, starting connection");
#endif

		case WAITING_DELAY:

#ifdef PQISSL_LOG_DEBUG 
  	  		rslog(RSL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::ConnectAttempt() STATE = Waiting Delay, starting connection");
#endif
	
			return Delay_Connection();
			//return Initiate_Connection(); /* now called by Delay_Connection() */

			break;

		case WAITING_SOCK_CONNECT:

#ifdef PQISSL_LOG_DEBUG 
  	  		rslog(RSL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::ConnectAttempt() STATE = Waiting Sock Connect");
#endif

			return Initiate_SSL_Connection();
			break;

		case WAITING_SSL_CONNECTION:

#ifdef PQISSL_LOG_DEBUG 
  	  		rslog(RSL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::ConnectAttempt() STATE = Waiting SSL Connection");
#endif

			return Authorise_SSL_Connection();
			break;

		case WAITING_SSL_AUTHORISE:

#ifdef PQISSL_LOG_DEBUG 
  	  		rslog(RSL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::ConnectAttempt() STATE = Waiting SSL Authorise");
#endif

			return Authorise_SSL_Connection();
			break;
		case WAITING_FAIL_INTERFACE:

#ifdef PQISSL_LOG_DEBUG 
  	  		rslog(RSL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::ConnectAttempt() Failed - Retrying");
#endif

			return Failed_Connection();
			break;


		default:
  	  		rslog(RSL_ALERT, pqisslzone, 
		 "pqissl::ConnectAttempt() STATE = Unknown - calling reset()");

			reset_locked();
			break;
	}
  	rslog(RSL_ALERT, pqisslzone, "pqissl::ConnectAttempt() Unknown");

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

int 	pqissl::Failed_Connection()
{
#ifdef PQISSL_LOG_DEBUG 
	rslog(RSL_DEBUG_BASIC, pqisslzone, 
		"pqissl::ConnectAttempt() Failed - Notifying");
#endif

	if (parent())
	{
		struct sockaddr_storage addr;
		sockaddr_storage_clear(addr);
		parent() -> notifyEvent(this, NET_CONNECT_UNREACHABLE, addr);
	}

	waiting = WAITING_NOT;

	return 1;
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

int 	pqissl::Delay_Connection()
{
#ifdef PQISSL_LOG_DEBUG 
  	rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Delay_Connection() Attempting Outgoing Connection....");
#endif

	if (waiting == WAITING_NOT)
	{
		waiting = WAITING_DELAY;

		/* we cannot just jump to Initiate_Connection, 
  		 * but must switch to WAITING_DELAY for at least one cycle.
		 * to avoid deadlock between threads....
		 * ie. so the connection stuff is called from tick(), rather than connect()
		 */

		/* set Connection TS.
		 */
#ifdef PQISSL_LOG_DEBUG 
		{ 
			std::string out;
			rs_sprintf(out, "pqissl::Delay_Connection()  Delaying Connection to %s for %lu seconds", PeerId().c_str(), mConnectDelay);
			rslog(RSL_DEBUG_BASIC, pqisslzone, out);
		}
#endif

		mConnectTS = time(NULL) + mConnectDelay;
		return 0;
	}
	else if (waiting == WAITING_DELAY)
	{
#ifdef PQISSL_LOG_DEBUG 
		{ 
			std::string out;
			rs_sprintf(out, "pqissl::Delay_Connection() Connection to %s starting in %ld seconds", PeerId().c_str(), mConnectTS - time(NULL));
			rslog(RSL_DEBUG_BASIC, pqisslzone, out);
		}
#endif
		if (time(NULL) >= mConnectTS)
		{
			return Initiate_Connection();
		}
		return 0;
	}

  	rslog(RSL_WARNING, pqisslzone, 
		 "pqissl::Initiate_Connection() Already Attempt in Progress!");
	return -1;
}


int 	pqissl::Initiate_Connection()
{
	int err;
	struct sockaddr_storage addr = remote_addr;

#ifdef PQISSL_LOG_DEBUG 
  	rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Initiate_Connection() Attempting Outgoing Connection....");
#endif

	if (waiting != WAITING_DELAY)
	{
  		rslog(RSL_WARNING, pqisslzone, 
		 "pqissl::Initiate_Connection() Already Attempt in Progress!");
		return -1;
	}

#ifdef PQISSL_LOG_DEBUG 
  	rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Initiate_Connection() Opening Socket");
#endif

	// open socket connection to addr.
	int osock = unix_socket(PF_INET, SOCK_STREAM, 0);

#ifdef PQISSL_LOG_DEBUG 
	{
		std::string out;
		rs_sprintf(out, "pqissl::Initiate_Connection() osock = %d", osock);
		rslog(RSL_DEBUG_BASIC, pqisslzone, out);
	}
#endif

	if (osock < 0)
	{
		std::string out = "pqissl::Initiate_Connection() Failed to open socket!\n";
		out += "Socket Error:" + socket_errorType(errno);
		rslog(RSL_WARNING, pqisslzone, out);

		net_internal_close(osock);
		waiting = WAITING_FAIL_INTERFACE;
		return -1;
	}

#ifdef PQISSL_LOG_DEBUG 
  	rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Initiate_Connection() Making Non-Blocking");
#endif

        err = unix_fcntl_nonblock(osock);
	if (err < 0)
	{
		std::string out;
		rs_sprintf(out, "pqissl::Initiate_Connection() Error: Cannot make socket NON-Blocking: %d", err);
		rslog(RSL_WARNING, pqisslzone, out);

		waiting = WAITING_FAIL_INTERFACE;
		net_internal_close(osock);
		return -1;
	}

	{ 
		std::string out;
		rs_sprintf(out, "pqissl::Initiate_Connection() Connecting To: %s via: ", PeerId().toStdString().c_str());
		out += sockaddr_storage_tostring(addr);
		rslog(RSL_WARNING, pqisslzone, out);
	}

	if (sockaddr_storage_isnull(addr))
	{
		rslog(RSL_WARNING, pqisslzone, "pqissl::Initiate_Connection() Invalid (0.0.0.0) Remote Address, Aborting Connect.");
		waiting = WAITING_FAIL_INTERFACE;
		net_internal_close(osock);
		return -1;
	}

#ifdef WINDOWS_SYS
	/* Set TCP buffer size for Windows systems */

	int sockbufsize = 0;
	int size = sizeof(int);

	err = getsockopt(osock, SOL_SOCKET, SO_RCVBUF, (char *)&sockbufsize, &size);
#ifdef PQISSL_DEBUG
	if (err == 0) {
		std::cerr << "pqissl::Initiate_Connection: Current TCP receive buffer size " << sockbufsize << std::endl;
	} else {
		std::cerr << "pqissl::Initiate_Connection: Error getting TCP receive buffer size. Error " << err << std::endl;
	}
#endif

	sockbufsize = 0;

	err = getsockopt(osock, SOL_SOCKET, SO_SNDBUF, (char *)&sockbufsize, &size);
#ifdef PQISSL_DEBUG
	if (err == 0) {
		std::cerr << "pqissl::Initiate_Connection: Current TCP send buffer size " << sockbufsize << std::endl;
	} else {
		std::cerr << "pqissl::Initiate_Connection: Error getting TCP send buffer size. Error " << err << std::endl;
	}
#endif

	sockbufsize = WINDOWS_TCP_BUFFER_SIZE;

	err = setsockopt(osock, SOL_SOCKET, SO_RCVBUF, (char *)&sockbufsize, sizeof(sockbufsize));
#ifdef PQISSL_DEBUG
	if (err == 0) {
		std::cerr << "pqissl::Initiate_Connection: TCP receive buffer size set to " << sockbufsize << std::endl;
	} else {
		std::cerr << "pqissl::Initiate_Connection: Error setting TCP receive buffer size. Error " << err << std::endl;
	}
#endif

	err = setsockopt(osock, SOL_SOCKET, SO_SNDBUF, (char *)&sockbufsize, sizeof(sockbufsize));
#ifdef PQISSL_DEBUG
	if (err == 0) {
		std::cerr << "pqissl::Initiate_Connection: TCP send buffer size set to " << sockbufsize << std::endl;
	} else {
		std::cerr << "pqissl::Initiate_Connection: Error setting TCP send buffer size. Error " << err << std::endl;
	}
#endif
#endif // WINDOWS_SYS

	mTimeoutTS = time(NULL) + mConnectTimeout;
	//std::cerr << "Setting Connect Timeout " << mConnectTimeout << " Seconds into Future " << std::endl;

	if (0 != (err = unix_connect(osock, (struct sockaddr *) &addr, sizeof(addr))))
	{
		std::string out;
		rs_sprintf(out, "pqissl::Initiate_Connection() connect returns:%d -> errno: %d error: %s\n", err, errno, socket_errorType(errno).c_str());
		
		if (errno == EINPROGRESS)
		{
			// set state to waiting.....
			waiting = WAITING_SOCK_CONNECT;
			sockfd = osock;

#ifdef PQISSL_LOG_DEBUG 
			out += " EINPROGRESS Waiting for Socket Connection";
			rslog(RSL_DEBUG_BASIC, pqisslzone, out);
#endif
  
			return 0;
		}
		else if ((errno == ENETUNREACH) || (errno == ETIMEDOUT))
		{
			out += "ENETUNREACHABLE: cert: " + PeerId().toStdString();
			rslog(RSL_WARNING, pqisslzone, out);

			// Then send unreachable message.
			net_internal_close(osock);
			osock=-1;
			//reset();

			waiting = WAITING_FAIL_INTERFACE;

			return -1;
		}

		/* IF we get here ---- we Failed for some other reason. 
                 * Should abandon this interface 
		 * Known reasons to get here: EINVAL (bad address)
		 */

		rs_sprintf_append(out, "Error: Connection Failed: %d - %s", errno, socket_errorType(errno).c_str());

		net_internal_close(osock);
		osock=-1;
		waiting = WAITING_FAIL_INTERFACE;

		rslog(RSL_WARNING, pqisslzone, out);

		// extra output for the moment.
		std::cerr << out;

		return -1;
	}
	else
	{
#ifdef PQISSL_LOG_DEBUG 
  		rslog(RSL_DEBUG_BASIC, pqisslzone,
		 "pqissl::Init_Connection() connect returned 0");
#endif
	}

	waiting = WAITING_SOCK_CONNECT;
	sockfd = osock;

#ifdef PQISSL_LOG_DEBUG 
	rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Initiate_Connection() Waiting for Socket Connect");
#endif

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

bool  	pqissl::CheckConnectionTimeout()
{
	/* new TimeOut code. */
	if (time(NULL) > mTimeoutTS)
	{
		std::string out;
		rs_sprintf(out, "pqissl::Basic_Connection_Complete() Connection Timed Out. Peer: %s Period: %lu", PeerId().toStdString().c_str(), mConnectTimeout);

		rslog(RSL_WARNING, pqisslzone, out);
		/* as sockfd is valid, this should close it all up */
		
		rslog(RSL_ALERT, pqisslzone, "pqissl::Basic_Connection_Complete() -> calling reset()");
		reset_locked();
		return true;
	}
	return false;
}



int 	pqissl::Basic_Connection_Complete()
{
#ifdef PQISSL_LOG_DEBUG 
	rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Basic_Connection_Complete()...");
#endif

	if (CheckConnectionTimeout())
	{
		// calls reset.
		return -1;
	}

	if (waiting != WAITING_SOCK_CONNECT)
	{
		rslog(RSL_ALERT, pqisslzone, 
	  		"pqissl::Basic_Connection_Complete() Wrong Mode");
		return -1;
	}

        if (sockfd == -1)
        {
                rslog(RSL_ALERT, pqisslzone,
                        "pqissl::Basic_Connection_Complete() problem with the socket descriptor. Aborting");
		rslog(RSL_ALERT, pqisslzone, "pqissl::Basic_Connection_Complete() -> calling reset()");
                reset_locked();
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

#ifdef PQISSL_LOG_DEBUG 
  	rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Basic_Connection_Complete() Selecting ....");
#endif

	int sr = 0;
	if (0 > (sr = select(sockfd + 1, 
			&ReadFDs, &WriteFDs, &ExceptFDs, &timeout))) 
	{
		// select error.
  		rslog(RSL_WARNING, pqisslzone, 
	  	  "pqissl::Basic_Connection_Complete() Select ERROR(1)");
		
		net_internal_close(sockfd);
		sockfd=-1;
		//reset();
		waiting = WAITING_FAIL_INTERFACE;
		return -1;
	}

#ifdef PQISSL_LOG_DEBUG 
	{
		std::string out;
		rs_sprintf(out, "pqissl::Basic_Connection_Complete() Select returned %d", sr);
		rslog(RSL_DEBUG_BASIC, pqisslzone, out);
	}
#endif
		

	if (FD_ISSET(sockfd, &ExceptFDs))
	{
		// error - reset socket.
		// this is a definite bad socket!.
		
  		rslog(RSL_WARNING, pqisslzone, 
	  	  "pqissl::Basic_Connection_Complete() Select ERROR(2)");
		
		net_internal_close(sockfd);
		sockfd=-1;
		//reset();
		waiting = WAITING_FAIL_INTERFACE;
		return -1;
	}

	if (FD_ISSET(sockfd, &WriteFDs))
	{
#ifdef PQISSL_LOG_DEBUG 
  		rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  	  "pqissl::Basic_Connection_Complete() Can Write!");
#endif
	}
	else
	{
#ifdef PQISSL_LOG_DEBUG 
		// happens frequently so switched to debug msg.
  		rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  	  "pqissl::Basic_Connection_Complete() Not Yet Ready!");
#endif
		return 0;
	}

	if (FD_ISSET(sockfd, &ReadFDs))
	{
#ifdef PQISSL_LOG_DEBUG 
  		rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  	  "pqissl::Basic_Connection_Complete() Can Read!");
#endif
	}
	else
	{
#ifdef PQISSL_LOG_DEBUG 
		// not ready return -1;
  		rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  	  "pqissl::Basic_Connection_Complete() Cannot Read!");
#endif
	}

	int err = 1;
	if (0==unix_getsockopt_error(sockfd, &err))
	{
		if (err == 0)
		{
			{
				std::string out;
				rs_sprintf(out, "pqissl::Basic_Connection_Complete() TCP Connection Complete: cert: %s on osock: ", PeerId().toStdString().c_str(), sockfd);
				rslog(RSL_WARNING, pqisslzone, out);
			}
			return 1;
		}
		else if (err == EINPROGRESS)
		{
			rslog(RSL_WARNING, pqisslzone, "pqissl::Basic_Connection_Complete() EINPROGRESS: cert: " + PeerId().toStdString());

			return 0;
		}
		else if ((err == ENETUNREACH) || (err == ETIMEDOUT))
		{
			rslog(RSL_WARNING, pqisslzone, "pqissl::Basic_Connection_Complete() ENETUNREACH/ETIMEDOUT: cert: " + PeerId().toStdString());

			// Then send unreachable message.
			net_internal_close(sockfd);
			sockfd=-1;
			//reset();
			
			waiting = WAITING_FAIL_INTERFACE;

			return -1;
		}
		else if ((err == EHOSTUNREACH) || (err == EHOSTDOWN))
		{
			rslog(RSL_WARNING, pqisslzone, "pqissl::Basic_Connection_Complete() EHOSTUNREACH/EHOSTDOWN: cert: " + PeerId().toStdString());

			// Then send unreachable message.
			net_internal_close(sockfd);
			sockfd=-1;
			//reset();
			waiting = WAITING_FAIL_INTERFACE;

			return -1;
		}
		else if ((err == ECONNREFUSED))
		{
			rslog(RSL_WARNING, pqisslzone, "pqissl::Basic_Connection_Complete() ECONNREFUSED: cert: " + PeerId().toStdString());

			// Then send unreachable message.
			net_internal_close(sockfd);
			sockfd=-1;
			//reset();
			waiting = WAITING_FAIL_INTERFACE;

			return -1;
		}
			
		std::string out;
		rs_sprintf(out, "Error: Connection Failed UNKNOWN ERROR: %d - %s", err, socket_errorType(err).c_str());
		rslog(RSL_WARNING, pqisslzone, out);

		net_internal_close(sockfd);
		sockfd=-1;
		//reset(); // which will send Connect Failed,
		return -1;
	}

  	rslog(RSL_ALERT, pqisslzone, 
	  "pqissl::Basic_Connection_Complete() BAD GETSOCKOPT!");
	waiting = WAITING_FAIL_INTERFACE;

	return -1;
}


int 	pqissl::Initiate_SSL_Connection()
{
	int err;

#ifdef PQISSL_LOG_DEBUG 
  	rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Initiate_SSL_Connection() Checking Basic Connection");
#endif

	if (0 >= (err = Basic_Connection_Complete()))
	{
		return err;
	}

#ifdef PQISSL_LOG_DEBUG 
  	rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Initiate_SSL_Connection() Basic Connection Okay");
#endif

	// setup timeout value.
	ssl_connect_timeout = time(NULL) + PQISSL_SSL_CONNECT_TIMEOUT;

	// Perform SSL magic.
	// library already inited by sslroot().
	SSL *ssl = SSL_new(AuthSSL::getAuthSSL()->getCTX());
	if (ssl == NULL)
	{
  		rslog(RSL_ALERT, pqisslzone, 
		  "pqissl::Initiate_SSL_Connection() SSL_new failed!");

		exit(1);
		return -1;
	}
	
#ifdef PQISSL_LOG_DEBUG 
  	rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Initiate_SSL_Connection() SSL Connection Okay");
#endif

	ssl_connection = ssl;

	net_internal_SSL_set_fd(ssl, sockfd);
	if (err < 1)
	{
		std::string out = "pqissl::Initiate_SSL_Connection() SSL_set_fd failed!\n";
		printSSLError(ssl, err, SSL_get_error(ssl, err), ERR_get_error(), out);

		rslog(RSL_ALERT, pqisslzone, out);
	}

#ifdef PQISSL_LOG_DEBUG 
  	rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Initiate_SSL_Connection() Waiting for SSL Connection");
#endif

	waiting = WAITING_SSL_CONNECTION;
	return 1;
}

int 	pqissl::SSL_Connection_Complete()
{
#ifdef PQISSL_LOG_DEBUG 
  	rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::SSL_Connection_Complete()??? ... Checking");
#endif

	if (waiting == WAITING_SSL_AUTHORISE)
	{
  		rslog(RSL_ALERT, pqisslzone, 
		  "pqissl::SSL_Connection_Complete() Waiting = W_SSL_AUTH");

		return 1;
	}
	if (waiting != WAITING_SSL_CONNECTION)
	{
  		rslog(RSL_ALERT, pqisslzone, 
		  "pqissl::SSL_Connection_Complete() Still Waiting..");

		return -1;
	}

#ifdef PQISSL_LOG_DEBUG 
  	rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::SSL_Connection_Complete() Attempting SSL_connect");
#endif

	/* if we are passive - then accept! */
	int err;

	if (sslmode)
	{
#ifdef PQISSL_LOG_DEBUG 
        rslog(RSL_DEBUG_BASIC, pqisslzone, "--------> Active Connect! Client side.");
#endif
        err = SSL_connect(ssl_connection);
	}
	else
	{
#ifdef PQISSL_LOG_DEBUG 
        rslog(RSL_DEBUG_BASIC, pqisslzone, "--------> Passive Accept! Server side.");
#endif
		err = SSL_accept(ssl_connection);
	}

	if (err != 1)
	{
		int serr = SSL_get_error(ssl_connection, err);
		if ((serr == SSL_ERROR_WANT_READ) 
				|| (serr == SSL_ERROR_WANT_WRITE))
		{
#ifdef PQISSL_LOG_DEBUG 
  			rslog(RSL_DEBUG_BASIC, pqisslzone, 
			  "Waiting for SSL handshake!");
#endif

			waiting = WAITING_SSL_CONNECTION;
			return 0;
		}


		std::string out;
		rs_sprintf(out, "pqissl::SSL_Connection_Complete()\nIssues with SSL Connect(%d)!\n", err);
		printSSLError(ssl_connection, err, serr, ERR_get_error(), out);

		rslog(RSL_WARNING, pqisslzone, out);

		// attempt real error.
		Extract_Failed_SSL_Certificate();

		rslog(RSL_ALERT, pqisslzone, "pqissl::SSL_Connection_Complete() -> calling reset()");
		reset_locked();
		waiting = WAITING_FAIL_INTERFACE;

		return -1;
	}
	// if we get here... success v quickly.

	rslog(RSL_WARNING, pqisslzone, "pqissl::SSL_Connection_Complete() Success!: Peer: " + PeerId().toStdString());

	waiting = WAITING_SSL_AUTHORISE;
	return 1;
}

int 	pqissl::Extract_Failed_SSL_Certificate()
{
	std::cerr << "pqissl::Extract_Failed_SSL_Certificate() FAILED Connection due to Security Issues";
	std::cerr << std::endl;

#ifdef PQISSL_LOG_DEBUG 
  	rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Extract_Failed_SSL_Certificate()");
#endif

	// Get the Peer Certificate....
	X509 *peercert = SSL_get_peer_certificate(ssl_connection);

	if (peercert == NULL)
	{
  		rslog(RSL_WARNING, pqisslzone, 
		  "pqissl::Extract_Failed_SSL_Certificate() Peer Didnt Give Cert");

		std::cerr << "pqissl::Extract_Failed_SSL_Certificate() ERROR Peer Didn't Give Us Certificate";
		std::cerr << std::endl;

		return -1;
	}

#ifdef PQISSL_LOG_DEBUG 
  	rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Extract_Failed_SSL_Certificate() Have Peer Cert - Registering");
#endif

	std::cerr << "pqissl::Extract_Failed_SSL_Certificate() Passing FAILED Cert to AuthSSL for analysis";
	std::cerr << std::endl;

	// save certificate... (and ip locations)
	// false for outgoing....
	// we actually connected to remote_addr, 
	// 	which could be 
	//      (pqissl's case) sslcert->serveraddr or sslcert->localaddr.

	RsPeerId sslid ;
	getX509id(peercert, sslid) ;

	RsPgpId gpgid(getX509CNString(peercert->cert_info->issuer));
	std::string sslcn = getX509CNString(peercert->cert_info->subject);

	AuthSSL::getAuthSSL()->FailedCertificate(peercert, gpgid,sslid,sslcn,remote_addr, false);
	mLinkMgr->notifyDeniedConnection(gpgid, sslid, sslcn, remote_addr, false);

	return 1;
}




int 	pqissl::Authorise_SSL_Connection()
{
#ifdef PQISSL_LOG_DEBUG 
  	rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Authorise_SSL_Connection()");
#endif

        if (time(NULL) > ssl_connect_timeout)
        {
		rslog(RSL_WARNING, pqisslzone,
			"pqissl::Authorise_SSL_Connection() Connection Timed Out!");
	        /* as sockfd is valid, this should close it all up */
		rslog(RSL_ALERT, pqisslzone, "pqissl::Authorise_Connection_Complete() -> calling reset()");
	        reset_locked();
	}

	int err;
	if (0 >= (err = SSL_Connection_Complete()))
	{
		return err;
	}

#ifdef PQISSL_LOG_DEBUG 
  	rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Authorise_SSL_Connection() SSL_Connection_Complete");
#endif

	// reset switch.
	waiting = WAITING_NOT;

	X509 *peercert = SSL_get_peer_certificate(ssl_connection);

	if (peercert == NULL)
	{
  		rslog(RSL_WARNING, pqisslzone, 
		  "pqissl::Authorise_SSL_Connection() Peer Didnt Give Cert");

		rslog(RSL_ALERT, pqisslzone, "pqissl::Authorise_Connection_Complete() -> calling reset()");
		// Failed completely
		reset_locked();
		return -1;
	}

        RsPeerId certPeerId;
        getX509id(peercert, certPeerId);
        if (RsPeerId(certPeerId) != PeerId()) {
                rslog(RSL_WARNING, pqisslzone,
                  "pqissl::Authorise_SSL_Connection() the cert Id doesn't match the Peer id we're trying to connect to.");

		rslog(RSL_ALERT, pqisslzone, "pqissl::Authorise_Connection_Complete() -> calling reset()");
                // Failed completely
                reset_locked();
                return -1;
        }

#ifdef PQISSL_LOG_DEBUG 
  	rslog(RSL_DEBUG_BASIC, pqisslzone, 
	  "pqissl::Authorise_SSL_Connection() Have Peer Cert");
#endif

	// save certificate... (and ip locations)
	// false for outgoing....
	// we actually connected to remote_addr, 
	// 	which could be 
	//      (pqissl's case) sslcert->serveraddr or sslcert->localaddr.

    bool res = AuthSSL::getAuthSSL()->CheckCertificate(PeerId(), peercert);
	bool certCorrect = true; /* WE know it okay already! */

    uint32_t check_result ;
    uint32_t checking_flags = RSBANLIST_CHECKING_FLAGS_BLACKLIST;
    if (rsPeers->servicePermissionFlags(PeerId()) & RS_NODE_PERM_REQUIRE_WL)
        checking_flags |= RSBANLIST_CHECKING_FLAGS_WHITELIST;

    if(!rsBanList->isAddressAccepted(remote_addr,checking_flags,&check_result))
    {
	std::cerr << "(SS) refusing connection attempt from IP address " << sockaddr_storage_iptostring(remote_addr) << ". Reason: " <<
        ((check_result == RSBANLIST_CHECK_RESULT_NOT_WHITELISTED)?"not whitelisted (peer requires whitelist)":"blacklisted") << std::endl;
            
        RsServer::notify()->AddFeedItem(RS_FEED_ITEM_SEC_IP_BLACKLISTED, PeerId().toStdString(), sockaddr_storage_iptostring(remote_addr), "", "", check_result);
    reset_locked();
    return 0 ;
    }
	// check it's the right one.
	if (certCorrect)
	{
		// then okay...
		rslog(RSL_WARNING, pqisslzone, "pqissl::Authorise_SSL_Connection() Accepting Conn. Peer: " + PeerId().toStdString());

        //std::cerr << "pqissl::Authorise_SSL_Connection(): accepting connection from " << sockaddr_storage_iptostring(remote_addr) << std::endl;
		accept_locked(ssl_connection, sockfd, remote_addr);
		return 1;
	}

	rslog(RSL_WARNING, pqisslzone, "pqissl::Authorise_SSL_Connection() Something Wrong ... Shutdown. Peer: " + PeerId().toStdString());

	// else shutdown ssl connection.
	rslog(RSL_ALERT, pqisslzone, "pqissl::Authorise_Connection_Complete() -> calling reset()");

	reset_locked();
	return 0;
}


/* This function is public, and callable from pqilistener - so must be mutex protected */
int	pqissl::accept(SSL *ssl, int fd, const struct sockaddr_storage &foreign_addr) // initiate incoming connection.
{
#ifdef PQISSL_DEBUG
	std::cerr << "pqissl::accept()";
    std::cerr << std::endl;
#endif

	RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/

	return accept_locked(ssl, fd, foreign_addr);
}

int	pqissl::accept_locked(SSL *ssl, int fd, const struct sockaddr_storage &foreign_addr) // initiate incoming connection.
{
    uint32_t check_result;
    uint32_t checking_flags = RSBANLIST_CHECKING_FLAGS_BLACKLIST;
    if (rsPeers->servicePermissionFlags(PeerId()) & RS_NODE_PERM_REQUIRE_WL)
        checking_flags |= RSBANLIST_CHECKING_FLAGS_WHITELIST;

    if(!rsBanList->isAddressAccepted(foreign_addr,checking_flags,&check_result))
    {
        std::cerr << "(SS) refusing incoming SSL connection from blacklisted foreign address " << sockaddr_storage_iptostring(foreign_addr)
              << ". Reason: " << check_result << "." << std::endl;
        RsServer::notify()->AddFeedItem(RS_FEED_ITEM_SEC_IP_BLACKLISTED, PeerId().toStdString(), sockaddr_storage_iptostring(foreign_addr), "", "", check_result);
            reset_locked();
        return -1;
    }
	if (waiting != WAITING_NOT)
	{
		rslog(RSL_WARNING, pqisslzone, "pqissl::accept() Peer: " + PeerId().toStdString() + " - Two connections in progress - Shut 1 down!");

		// outgoing connection in progress.
		// shut this baby down.
		//
		// Thought I should shut down one in progress, and continue existing one!
		// But the existing one might be broke.... take second.
		// all we need is to never stop listening.
		
		switch(waiting)
		{

		case WAITING_SOCK_CONNECT:

#ifdef PQISSL_LOG_DEBUG 
  	  		rslog(RSL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::accept() STATE = Waiting Sock Connect - close the socket");
#endif

			break;

		case WAITING_SSL_CONNECTION:

#ifdef PQISSL_LOG_DEBUG 
  	  		rslog(RSL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::accept() STATE = Waiting SSL Connection - close sockfd + ssl_conn");
#endif

			break;

		case WAITING_SSL_AUTHORISE:

#ifdef PQISSL_LOG_DEBUG 
  	  		rslog(RSL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::accept() STATE = Waiting SSL Authorise - close sockfd + ssl_conn");
#endif

			break;

		case WAITING_FAIL_INTERFACE:

#ifdef PQISSL_LOG_DEBUG 
  	  		rslog(RSL_DEBUG_BASIC, pqisslzone, 
			  "pqissl::accept() STATE = Failed, ignore?");
#endif

			break;


		default:
  	  		rslog(RSL_ALERT, pqisslzone, 
		 		"pqissl::accept() STATE = Unknown - ignore?");

			rslog(RSL_ALERT, pqisslzone, "pqissl::accept() -> calling reset()");
			reset_locked();
			break;
		}

		//waiting = WAITING_FAIL_INTERFACE;
		//return -1;
	}

	/* shutdown existing - in all cases use the new one */
	if ((ssl_connection) && (ssl_connection != ssl))
	{
  	 	rslog(RSL_ALERT, pqisslzone, 
		  "pqissl::accept() closing Previous/Existing ssl_connection");
		SSL_shutdown(ssl_connection);
	}

	if ((sockfd > -1) && (sockfd != fd))
	{
  	 	rslog(RSL_ALERT, pqisslzone, 
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

	struct sockaddr_storage localaddr;
	mLinkMgr->getLocalAddress(localaddr);

	{
		std::string out = "pqissl::accept() SUCCESSFUL connection to: " + PeerId().toStdString();
		out += " localaddr: " + sockaddr_storage_iptostring(localaddr);
		out += " remoteaddr: " + sockaddr_storage_iptostring(remote_addr);

		rslog(RSL_WARNING, pqisslzone, out);
	}

	// establish the ssl details.
	// cipher name.
	int err;

#ifdef PQISSL_LOG_DEBUG 
	{
	  int alg;
	  std::string out;
	  rs_sprintf(out, "SSL Cipher:%s\n", SSL_get_cipher(ssl));
	  rs_sprintf_append(out, "SSL Cipher Bits:%d - %d\n", SSL_get_cipher_bits(ssl, &alg), alg);
	  rs_sprintf_append(out, "SSL Cipher Version:%s\n", SSL_get_cipher_version(ssl));
	  rslog(RSL_DEBUG_BASIC, pqisslzone, out);
	}
#endif

	// make non-blocking / or check.....
        if ((err = net_internal_fcntl_nonblock(sockfd)) < 0)
	{
  	  	rslog(RSL_ALERT, pqisslzone, "Error: Cannot make socket NON-Blocking: ");

		active = false;
		waiting = WAITING_FAIL_INTERFACE;
		// failed completely.
		rslog(RSL_ALERT, pqisslzone, "pqissl::accept() -> calling reset()");
		reset_locked();
		return -1;
	}
	else
	{
#ifdef PQISSL_LOG_DEBUG 
  	  	rslog(RSL_DEBUG_BASIC, pqisslzone, "pqissl::accept() Socket Made Non-Blocking!");
#endif
	}

	// we want to continue listening - incase this socket is crap, and they try again.
	//stoplistening();

	active = true;
	waiting = WAITING_NOT;

#ifdef PQISSL_DEBUG
	std::cerr << "pqissl::accept_locked() connection complete - notifying parent";
    std::cerr << std::endl;
#endif

	// Notify the pqiperson.... (Both Connect/Receive)
	if (parent())
	{
		struct sockaddr_storage addr = remote_addr;
		parent() -> notifyEvent(this, NET_CONNECT_SUCCESS, addr);
	}
	return 1;
}

/********** Implementation of BinInterface **************************
 * All the rest of the BinInterface.
 * This functions much be Mutex protected.
 *
 */

int 	pqissl::senddata(void *data, int len)
{
	RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/

	int tmppktlen ;

	// safety check.  Apparently this avoids some SIGSEGV.
	//
	if(ssl_connection == NULL)
		return -1;

#ifdef PQISSL_DEBUG
	std::cout << "Sending data thread=" << pthread_self() << ", ssl=" << (void*)this << ", size=" << len << std::endl ;
#endif
	tmppktlen = SSL_write(ssl_connection, data, len) ;

	if (len != tmppktlen)
	{
		std::string out = "pqissl::senddata() " + PeerId().toStdString();
		rs_sprintf_append(out, " Partial Send: len: %d sent: %d ", len, tmppktlen);
	
		int err = SSL_get_error(ssl_connection, tmppktlen);
		// incomplete operations - to repeat....
		// handled by the pqistreamer...
		if (err == SSL_ERROR_SYSCALL)
		{
			rs_sprintf_append(out, "SSL_write() SSL_ERROR_SYSCALL SOCKET_DEAD -> calling reset() errno: %d ", errno);
			out += socket_errorType(errno);
			std::cerr << out << std::endl;
			rslog(RSL_ALERT, pqisslzone, out);

			/* extra debugging - based on SSL_get_error() man page */
			{
				int errsys = errno;
				int sslerr = 0;
				std::string out2;
				rs_sprintf(out2, "SSL_ERROR_SYSCALL, ret == %d errno: %d %s\n", tmppktlen, errsys, socket_errorType(errsys).c_str());

				while(0 != (sslerr = ERR_get_error()))
				{
					rs_sprintf_append(out2, "SSLERR:%d:", sslerr);

					char sslbuf[256] = {0};
					out2 += ERR_error_string(sslerr, sslbuf);
					out2 += "\n";
				}
				rslog(RSL_ALERT, pqisslzone, out2);
			}

			rslog(RSL_ALERT, pqisslzone, "pqissl::senddata() -> calling reset()");
			reset_locked();
			return -1;
		}
		else if (err == SSL_ERROR_WANT_WRITE)
		{
			out += "SSL_write() SSL_ERROR_WANT_WRITE";
			rslog(RSL_WARNING, pqisslzone, out);
			return -1;
		}
		else if (err == SSL_ERROR_WANT_READ)
		{
			out += "SSL_write() SSL_ERROR_WANT_READ";
			rslog(RSL_WARNING, pqisslzone, out);
			//std::cerr << out << std::endl;
			return -1;
		}
		else
		{
			rs_sprintf_append(out, "SSL_write() UNKNOWN ERROR: %d\n", err);
			printSSLError(ssl_connection, tmppktlen, err, ERR_get_error(), out);
			out += "\n\tResetting!";
			std::cerr << out << std::endl;
			rslog(RSL_ALERT, pqisslzone, out);

			rslog(RSL_ALERT, pqisslzone, "pqissl::senddata() -> calling reset()");
			reset_locked();
			return -1;
		}
	}
	return tmppktlen;
}

int 	pqissl::readdata(void *data, int len)
{
	RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/

#ifdef PQISSL_DEBUG
	std::cout << "Reading data thread=" << pthread_self() << ", ssl=" << (void*)this << std::endl ;
#endif

	// There is a do, because packets can be splitted into multiple ssl buffers
	// when they are larger than 16384 bytes. Such packets have to be read in 
	// multiple slices.
	do
	{
		int tmppktlen  ;

#ifdef PQISSL_DEBUG
		std::cerr << "calling SSL_read. len=" << len << ", total_len=" << total_len << std::endl ;
#endif
		tmppktlen = SSL_read(ssl_connection, (void*)( &(((uint8_t*)data)[total_len])), len-total_len) ;
#ifdef PQISSL_DEBUG
		std::cerr << "have read " << tmppktlen << " bytes" << std::endl ;
		std::cerr << "data[0] = " 
			<< (int)((uint8_t*)data)[total_len+0] << " "
			<< (int)((uint8_t*)data)[total_len+1] << " "
			<< (int)((uint8_t*)data)[total_len+2] << " "
			<< (int)((uint8_t*)data)[total_len+3] << " "
			<< (int)((uint8_t*)data)[total_len+4] << " "
			<< (int)((uint8_t*)data)[total_len+5] << " "
			<< (int)((uint8_t*)data)[total_len+6] << " "
			<< (int)((uint8_t*)data)[total_len+7] << std::endl ;
#endif

		// Need to catch errors.....
		//
		if (tmppktlen <= 0) // probably needs a reset.
		{
			std::string out;

			int error = SSL_get_error(ssl_connection, tmppktlen);
			unsigned long err2 =  ERR_get_error();

			//printSSLError(ssl_connection, tmppktlen, error, err2, out);

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
				if (n_read_zero == 0)
				{
					/* first read_zero */
					mReadZeroTS = time(NULL);
				}

				++n_read_zero;
				out += "pqissl::readdata() " + PeerId().toStdString();
				rs_sprintf_append(out, " SSL_read() SSL_ERROR_ZERO_RETURN : nReadZero: %d", n_read_zero);

				if ((PQISSL_MAX_READ_ZERO_COUNT < n_read_zero)
					&& (time(NULL) - mReadZeroTS > PQISSL_MAX_READ_ZERO_TIME)) 
				{
					out += " Count passed Limit, shutting down!";
					rs_sprintf_append(out, " ReadZero Age: %ld", time(NULL) - mReadZeroTS);

					rslog(RSL_ALERT, pqisslzone, "pqissl::readdata() -> calling reset()");
					reset_locked();
				}

				rslog(RSL_ALERT, pqisslzone, out);
				//std::cerr << out << std::endl ;
				return -1;
			}

			/* the only real error we expect */
			if (error == SSL_ERROR_SYSCALL)
			{
				out += "pqissl::readdata() " + PeerId().toStdString();
				out += " SSL_read() SSL_ERROR_SYSCALL";
				out += " SOCKET_DEAD -> calling reset()";
				rs_sprintf_append(out, " errno: %d", errno);
				out += " " + socket_errorType(errno);
				rslog(RSL_ALERT, pqisslzone, out);

				/* extra debugging - based on SSL_get_error() man page */
				{
					int syserr = errno;
					int sslerr = 0;
					std::string out2;
					rs_sprintf(out2, "SSL_ERROR_SYSCALL, ret == %d errno: %d %s\n", tmppktlen, syserr, socket_errorType(syserr).c_str());
	
					while(0 != (sslerr = ERR_get_error()))
					{
						rs_sprintf_append(out2, "SSLERR:%d : ", sslerr);
	
						char sslbuf[256] = {0};
						out2 += ERR_error_string(sslerr, sslbuf);
						out2 += "\n";
					}
					rslog(RSL_ALERT, pqisslzone, out2);
				}

				rslog(RSL_ALERT, pqisslzone, "pqissl::readdata() -> calling reset()");
				reset_locked();
				std::cerr << out << std::endl ;
				return -1;
			}
			else if (error == SSL_ERROR_WANT_WRITE)
			{
				out += "SSL_read() SSL_ERROR_WANT_WRITE";
				rslog(RSL_WARNING, pqisslzone, out);
				std::cerr << out << std::endl ;
				return -1;
			}
			else if (error == SSL_ERROR_WANT_READ)				
			{							
				// SSL_WANT_READ is not a crittical error. It's just a sign that
				// the internal SSL buffer is not ready to accept more data. So -1 
				// is returned, and the connection will be retried as is on next
				// call of readdata().

#ifdef PQISSL_DEBUG
				out += "SSL_read() SSL_ERROR_WANT_READ";
				rslog(RSL_DEBUG_BASIC, pqisslzone, out);
#endif
				return -1;
			}
			else
			{
				rs_sprintf_append(out, "SSL_read() UNKNOWN ERROR: %d Resetting!", error);
				rslog(RSL_ALERT, pqisslzone, out);
				std::cerr << out << std::endl ;

				rslog(RSL_ALERT, pqisslzone, "pqissl::readdata() -> calling reset()");
				reset_locked();
				return -1;
			}

			rslog(RSL_ALERT, pqisslzone, out);
			//exit(1);
		}
		else
			total_len+=tmppktlen ;
	} while(total_len < len) ;

#ifdef PQISSL_DEBUG
	std::cerr << "pqissl: have read data of length " << total_len << ", expected is " << len << std::endl ;
#endif

	if (len != total_len)
	{
		std::string out;
		rs_sprintf(out, "pqissl::readdata() Full Packet Not read!\n -> Expected len(%d) actually read(%d)", len, total_len);
		std::cerr << out << std::endl;
		rslog(RSL_WARNING, pqisslzone, out);
	}
	total_len = 0 ;		// reset the packet pointer as we have finished a packet.
	n_read_zero = 0;
	return len;//tmppktlen;
}


// dummy function currently.
int 	pqissl::netstatus()
{
	return 1;
}

int 	pqissl::isactive()
{
    return active;	// no need to mutex this. It's atomic.
}

bool 	pqissl::moretoread(uint32_t usec)
{
	RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/

#ifdef PQISSL_DEBUG
	{
		std::string out;
		rs_sprintf(out, "pqissl::moretoread() polling socket (%d)", sockfd);
		rslog(RSL_DEBUG_ALL, pqisslzone, out);
	}
#endif

	if(sockfd == -1)
	{
	   std::cerr << "pqissl::moretoread(): socket is invalid or closed." << std::endl;
	   return 0 ;
	}

	fd_set ReadFDs, WriteFDs, ExceptFDs;
	FD_ZERO(&ReadFDs);
	FD_ZERO(&WriteFDs);
	FD_ZERO(&ExceptFDs);

	FD_SET(sockfd, &ReadFDs);
	// Dont set WriteFDs.
	FD_SET(sockfd, &ExceptFDs);

	struct timeval timeout;
	timeout.tv_sec = 0; 
	timeout.tv_usec = usec; 

	if (select(sockfd + 1, &ReadFDs, &WriteFDs, &ExceptFDs, &timeout) < 0) 
	{
		rslog(RSL_ALERT, pqisslzone, 
			"pqissl::moretoread() Select ERROR!");
		return 0;
	}

	if (FD_ISSET(sockfd, &ExceptFDs))
	{
		// error - reset socket.
		rslog(RSL_ALERT, pqisslzone, 
			"pqissl::moretoread() Select Exception ERROR!");

		// this is a definite bad socket!.
		// reset.
		rslog(RSL_ALERT, pqisslzone, "pqissl::moretoread() -> calling reset()");
		reset_locked();
		return 0;
	}


	if (FD_ISSET(sockfd, &ReadFDs))
	{
#ifdef PQISSL_LOG_DEBUG 
		rslog(RSL_DEBUG_BASIC, pqisslzone, 
			"pqissl::moretoread() Data to Read!");
#endif
		return 1;
	}
	else
	{
#ifdef PQISSL_DEBUG 
		rslog(RSL_DEBUG_ALL, pqisslzone, 
			"pqissl::moretoread() No Data to Read!");
#endif
		return 0;
	}

}

bool 	pqissl::cansend(uint32_t usec)
{
	RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/

#ifdef PQISSL_DEBUG
	rslog(RSL_DEBUG_ALL, pqisslzone, 
		"pqissl::cansend() polling socket!");
#endif

	if(sockfd == -1)
	{
	   std::cerr << "pqissl::cansend(): socket is invalid or closed." << std::endl;
	   return 0 ;
	}

	// Interestingly - This code might be portable....

	fd_set ReadFDs, WriteFDs, ExceptFDs;
	FD_ZERO(&ReadFDs);
	FD_ZERO(&WriteFDs);
	FD_ZERO(&ExceptFDs);

	// Dont Set ReadFDs.
	FD_SET(sockfd, &WriteFDs);
	FD_SET(sockfd, &ExceptFDs);

	struct timeval timeout;
	timeout.tv_sec = 0; 
	timeout.tv_usec = usec; 
	

	if (select(sockfd + 1, &ReadFDs, &WriteFDs, &ExceptFDs, &timeout) < 0) 
	{
		// select error.
		rslog(RSL_ALERT, pqisslzone, 
			"pqissl::cansend() Select Error!");

		return 0;
	}

	if (FD_ISSET(sockfd, &ExceptFDs))
	{
		// error - reset socket.
		rslog(RSL_ALERT, pqisslzone, 
			"pqissl::cansend() Select Exception!");

		// this is a definite bad socket!.
		// reset.
		rslog(RSL_ALERT, pqisslzone, "pqissl::cansend() -> calling reset()");
		reset_locked();
		return 0;
	}

	if (FD_ISSET(sockfd, &WriteFDs))
	{
#ifdef PQISSL_DEBUG 
		// write can work.
		rslog(RSL_DEBUG_ALL, pqisslzone, 
			"pqissl::cansend() Can Write!");
#endif
		return 1;
	}
	else
	{
#ifdef PQISSL_DEBUG 
		// write can work.
		rslog(RSL_DEBUG_BASIC, pqisslzone, 
			"pqissl::cansend() Can *NOT* Write!");
#endif

		return 0;
	}

}

RsFileHash pqissl::gethash()
{
	return RsFileHash() ;
}

/********** End of Implementation of BinInterface ******************/


int pqissl::net_internal_close(int fd)
{
	return unix_close(fd);
}

int pqissl::net_internal_SSL_set_fd(SSL *ssl, int fd)
{
	return SSL_set_fd(ssl, fd);
}

int pqissl::net_internal_fcntl_nonblock(int fd)
{
	return unix_fcntl_nonblock(fd);
}
