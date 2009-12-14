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
#include "pqi/pqinetwork.h"

#include "tcponudp/tou.h"
#include "tcponudp/bio_tou.h"

#include <errno.h>
#include <openssl/err.h>

#include <sstream>

#include "util/rsdebug.h"
#include "util/rsnet.h"

const int pqissludpzone = 3144;

	/* a final timeout, to ensure this never blocks completely
	 * 300 secs to complete udp/tcp/ssl connection.
	 * This is long as the udp connect can take some time.
	 */

static const uint32_t PQI_SSLUDP_DEF_CONN_PERIOD = 300;  /* 5  minutes? */

/********** PQI SSL UDP STUFF **************************************/

pqissludp::pqissludp(PQInterface *parent, p3AuthMgr *am, p3ConnectMgr *cm)
	:pqissl(NULL, parent, am, cm), tou_bio(NULL), 
	listen_checktime(0), mConnectPeriod(PQI_SSLUDP_DEF_CONN_PERIOD)
{
	sockaddr_clear(&remote_addr);
	return;
}


pqissludp::~pqissludp()
{ 
        rslog(RSL_ALERT, pqissludpzone,  
            "pqissludp::~pqissludp -> destroying pqissludp");

	/* must call reset from here, so that the
	 * virtual functions will still work.
	 * -> as they stop working in base class destructor.
	 *
	 *  This means that reset() will be called twice, but this should
	 *  be harmless.
	 */
        stoplistening(); /* remove from p3proxy listenqueue */
	reset(); 

	if (tou_bio) // this should be in the reset?
	{
		BIO_free(tou_bio);
	}
	return;
}

int pqissludp::reset()
{
	/* reset for next time.*/
	return pqissl::reset();
}


	/* <===================== UDP Difference *******************/
	// The Proxy Version takes a few more step
	//
	// connectInterface is sent via message from the proxy.
	// and is set here.
	/* <===================== UDP Difference *******************/

int	pqissludp::attach()
{
	sockfd = tou_socket(0,0,0);
	if (0 > sockfd)
	{
  		rslog(RSL_WARNING, pqissludpzone, 
		  "pqissludp::attach() failed to create a socket");
		return -1;
	}

	// setup remote address
  	rslog(RSL_WARNING, pqissludpzone, 
		  "pqissludp::attach() Opened Local Udp Socket");
	
	return 1;
}


// The Address determination is done centrally
int 	pqissludp::Initiate_Connection()
{
	int err;

	attach(); /* open socket */
	remote_addr.sin_family = AF_INET;

  	rslog(RSL_DEBUG_BASIC, pqissludpzone, 
	  "pqissludp::Initiate_Connection() Attempting Outgoing Connection....");

	/* decide if we're active or passive */
	if (PeerId() < mConnMgr->getOwnId())
	{
		sslmode = PQISSL_ACTIVE;
	}
	else
	{
		sslmode = PQISSL_PASSIVE;
	}

	if (waiting != WAITING_DELAY)
	{
  		rslog(RSL_WARNING, pqissludpzone, 
		 "pqissludp::Initiate_Connection() Already Attempt in Progress!");
		return -1;
	}

	if (sockfd < 0)
	{
  		rslog(RSL_ALERT, pqissludpzone, 
		 "pqissludp::Initiate_Connection() Socket Creation Failed!");
		waiting = WAITING_FAIL_INTERFACE;
		return -1;
	}

  	rslog(RSL_DEBUG_BASIC, pqissludpzone, 
	  "pqissludp::Initiate_Connection() Opening Socket");

	{
		std::ostringstream out;
		out << "pqissludp::Initiate_Connection() ";
		out << "Connecting To: " << PeerId();
		out << " via: " << inet_ntoa(remote_addr.sin_addr) << ":";
		out << ntohs(remote_addr.sin_port) << " ";
		if (sslmode)
		{
			out << "ACTIVE Connect (SSL_Connect)";
		}
		else
		{
			out << "PASSIVE Connect (SSL_Accept)";
		}
  		rslog(RSL_WARNING, pqissludpzone, out.str());
	}

	if (remote_addr.sin_addr.s_addr == 0)
	{
		std::ostringstream out;
		out << "pqissludp::Initiate_Connection() ";
		out << "Invalid (0.0.0.0) Remote Address,";
		out << " Aborting Connect.";
		out << std::endl;
  		rslog(RSL_WARNING, pqissludpzone, out.str());
		waiting = WAITING_FAIL_INTERFACE;

		reset();
		return -1;
	}

	mTimeoutTS = time(NULL) + mConnectTimeout;
	//std::cerr << "Setting Connect Timeout " << mConnectTimeout << " Seconds into Future " << std::endl;
	//std::cerr << " Connect Period is:" << mConnectPeriod <<  std::endl;

	/* <===================== UDP Difference *******************/
	if (0 != (err = tou_connect(sockfd, (struct sockaddr *) &remote_addr, 
						sizeof(remote_addr), mConnectPeriod)))
	/* <===================== UDP Difference *******************/
	{
		int tou_err = tou_errno(sockfd);
		
		std::ostringstream out;
		out << "pqissludp::Initiate_Connection()";

		if ((tou_err == EINPROGRESS) || (tou_err == EAGAIN))
		{
			// set state to waiting.....
			waiting = WAITING_SOCK_CONNECT;

			out << " EINPROGRESS Waiting for Socket Connection";
  		        rslog(RSL_WARNING, pqissludpzone, out.str());
  
			return 0;
		}
		else if ((tou_err == ENETUNREACH) || (tou_err == ETIMEDOUT))
		{
			out << "ENETUNREACHABLE: cert: " << PeerId();
			out << std::endl;

			// Then send unreachable message.
			waiting = WAITING_FAIL_INTERFACE;
			net_unreachable |= net_attempt;
		}

		out << "Error: Connection Failed: " << tou_err;
		out << " - " << socket_errorType(tou_err) << std::endl;

  		rslog(RSL_WARNING, pqissludpzone, out.str());

		reset();

		return -1;
	}
	else
	{
  		rslog(RSL_DEBUG_BASIC, pqissludpzone,
		 "pqissludp::Init_Connection() connect returned 0");
	}

	waiting = WAITING_SOCK_CONNECT;

	rslog(RSL_DEBUG_BASIC, pqissludpzone, 
	  "pqissludp::Initiate_Connection() Waiting for Socket Connect");

	return 1;
}

/********* VERY DIFFERENT **********/
int 	pqissludp::Basic_Connection_Complete()
{
	rslog(RSL_DEBUG_BASIC, pqissludpzone, 
	  "pqissludp::Basic_Connection_Complete()...");


	if (time(NULL) > mTimeoutTS)
	{
		std::ostringstream out;
	  	out << "pqissludp::Basic_Connection_Complete() Connection Timed Out. ";
                out << "Peer: " << PeerId() << " Timeout: ";
		out << mConnectTimeout;

		rslog(RSL_WARNING, pqissludpzone, out.str());

		/* as sockfd is valid, this should close it all up */

		reset();
		return -1;
	}


	if (waiting != WAITING_SOCK_CONNECT)
	{
		rslog(RSL_DEBUG_BASIC, pqissludpzone, 
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
	  	  	out << "pqissludp::Basic_Connection_Complete() ";
			out << "EINPROGRESS: cert: " << PeerId();
  		        rslog(RSL_DEBUG_BASIC, pqissludpzone, out.str());

		}
		else if ((err == ENETUNREACH) || (err == ETIMEDOUT))
		{
			std::ostringstream out;
	  	  	out << "pqissludp::Basic_Connection_Complete() ";
			out << "ENETUNREACH/ETIMEDOUT: cert: ";
			out << PeerId();
	  		rslog(RSL_WARNING, pqissludpzone, out.str());

			/* is the second one needed? */
			std::ostringstream out2;
		  	out2 << "pqissludp::Basic_Connection_Complete() ";
			out2 << "Error: Connection Failed: " << err;
			out2 << " - " << socket_errorType(err);
	  		rslog(RSL_DEBUG_BASIC, pqissludpzone, out2.str());

			net_unreachable |= net_attempt;

			reset();

			// Then send unreachable message.
			waiting = WAITING_FAIL_INTERFACE;

			return -1;
		}
	}

	/* <===================== UDP Difference *******************/
	if (tou_connected(sockfd))
	/* <===================== UDP Difference *******************/
	{
		std::ostringstream out;
	  	out << "pqissludp::Basic_Connection_Complete() ";
		out << "Connection Complete: cert: ";
		out << PeerId();
	  	rslog(RSL_WARNING, pqissludpzone, out.str());

		return 1;
	}
	else
	{
		// not ready return -1;
  		rslog(RSL_DEBUG_BASIC, pqissludpzone, 
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
  	rslog(RSL_ALERT, pqissludpzone, 
	  	  "pqissludp::net_internal_close() -> tou_close()");
	return tou_close(fd);
}

// install udp BIO. 
int pqissludp::net_internal_SSL_set_fd(SSL *ssl, int fd)
{
  	rslog(RSL_DEBUG_BASIC, pqissludpzone, 
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
  	rslog(RSL_DEBUG_BASIC, pqissludpzone, 
	  	  "pqissludp::net_internal_fcntl_nonblock()");
	return 0;
}


/* These are identical to pqinetssl version */
//int 	pqissludp::status()

int	pqissludp::tick()
{
	pqissl::tick();
	return 1;
}

        // listen fns call the udpproxy.
int pqissludp::listen()
{
	{
		std::ostringstream out;
		out << "pqissludp::listen() (NULLOP)";
		rslog(RSL_DEBUG_BASIC, pqissludpzone, out.str());
	}
	return 1; //udpproxy->listen();
}

int pqissludp::stoplistening()
{
	{
		std::ostringstream out;
		out << "pqissludp::stoplistening() (NULLOP)";
		rslog(RSL_DEBUG_BASIC, pqissludpzone, out.str());
	}
	return 1; //udpproxy->stoplistening();
}


bool 	pqissludp::connect_parameter(uint32_t type, uint32_t value)
{
	//std::cerr << "pqissludp::connect_parameter() type: " << type << "value: " << value << std::endl;
	if (type == NET_PARAM_CONNECT_PERIOD)
	{
		std::ostringstream out;
		out << "pqissludp::connect_parameter() Peer: " << PeerId() << " PERIOD: " << value;
		rslog(RSL_WARNING, pqissludpzone, out.str());
		mConnectPeriod = value;
		return true;
        }else if (type == NET_PARAM_CONNECT_DELAY)
        {
                std::ostringstream out;
                out << "pqissludp::connect_parameter() Peer: " << PeerId() << " DELAY: " << value;
                rslog(RSL_WARNING, pqissludpzone, out.str());
                mConnectDelay = value;
                return true;
        }
        else if (type == NET_PARAM_CONNECT_TIMEOUT)
        {
                std::ostringstream out;
                out << "pqissludp::connect_parameter() Peer: " << PeerId() << " TIMEOUT: " << value;
                rslog(RSL_WARNING, pqissludpzone, out.str());
                mConnectTimeout = value;
                return true;
        }
        return pqissl::connect_parameter(type, value);
}

/********** PQI STREAMER OVERLOADING *********************************/

bool 	pqissludp::moretoread()
{
	{
		std::ostringstream out;
		out << "pqissludp::moretoread()";
		out << "  polling socket (" << sockfd << ")";
		rslog(RSL_DEBUG_ALL, pqissludpzone, out.str());
	}

	/* check for more to read first ... if nothing... check error
	 */
	/* <===================== UDP Difference *******************/
	if (tou_maxread(sockfd))
	/* <===================== UDP Difference *******************/
	{
		rslog(RSL_DEBUG_BASIC, pqissludpzone, 
			"pqissludp::moretoread() Data to Read!");
		return 1;
	}

	/* else check the error */
	rslog(RSL_DEBUG_ALL, pqissludpzone, 
		"pqissludp::moretoread() No Data to Read!");

	int err;
	if (0 != (err = tou_errno(sockfd)))
	{
		if ((err == EAGAIN) || (err == EINPROGRESS))
		{

			std::ostringstream out;
	  	  	out << "pqissludp::moretoread() ";
			out << "EAGAIN/EINPROGRESS: cert " << PeerId();
  		        rslog(RSL_DEBUG_BASIC, pqissludpzone, out.str());
			return 0;

		}
		else if ((err == ENETUNREACH) || (err == ETIMEDOUT))
		{
			std::ostringstream out;
	  	  	out << "pqissludp::moretoread() ";
			out << "ENETUNREACH/ETIMEDOUT: cert ";
			out << PeerId();
  		        rslog(RSL_WARNING, pqissludpzone, out.str());

		}
		else if (err == EBADF)
		{
			std::ostringstream out;
	  	  	out << "pqissludp::moretoread() ";
			out << "EBADF: cert ";
			out << PeerId();
  		        rslog(RSL_WARNING, pqissludpzone, out.str());

		}
		else 
		{
			std::ostringstream out;
	  	  	out << "pqissludp::moretoread() ";
			out << " Unknown ERROR: " << err << ": cert ";
			out << PeerId();
  		        rslog(RSL_WARNING, pqissludpzone, out.str());

		}

		reset();
		return 0;
	}

	/* otherwise - not error - strange! */
	rslog(RSL_DEBUG_BASIC, pqissludpzone, 
		"pqissludp::moretoread() No Data + No Error (really nothing)");

	return 0;


}

bool 	pqissludp::cansend()
{
	rslog(RSL_DEBUG_ALL, pqissludpzone, 
		"pqissludp::cansend() polling socket!");

	/* <===================== UDP Difference *******************/
	return (0 < tou_maxwrite(sockfd));
	/* <===================== UDP Difference *******************/

}



