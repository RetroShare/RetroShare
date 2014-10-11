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
#include <openssl/ssl.h>


#include "util/rsdebug.h"
#include "util/rsnet.h"
#include "util/rsstring.h"

#include "pqi/p3linkmgr.h"
#include <unistd.h>

const int pqissludpzone = 3144;

	/* a final timeout, to ensure this never blocks completely
	 * 300 secs to complete udp/tcp/ssl connection.
	 * This is long as the udp connect can take some time.
	 */

static const uint32_t PQI_SSLUDP_DEF_CONN_PERIOD = 300;  /* 5  minutes? */

/********** PQI SSL UDP STUFF **************************************/

pqissludp::pqissludp(PQInterface *parent, p3LinkMgr *lm)
        :pqissl(NULL, parent, lm), tou_bio(NULL),
	listen_checktime(0), mConnectPeriod(PQI_SSLUDP_DEF_CONN_PERIOD)
{
	RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/

	sockaddr_storage_clear(remote_addr);
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

	RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/

	if (tou_bio) // this should be in the reset?
	{
		BIO_free(tou_bio);
	}
	return;
}

int pqissludp::reset_locked()
{
	/* reset for next time.*/
	mConnectFlags = 0;
	mConnectPeriod = PQI_SSLUDP_DEF_CONN_PERIOD;

	return pqissl::reset_locked();
}


	/* <===================== UDP Difference *******************/
	// The Proxy Version takes a few more step
	//
	// connectInterface is sent via message from the proxy.
	// and is set here.
	/* <===================== UDP Difference *******************/

int	pqissludp::attach()
{
	// IN THE IMPROVED TOU LIBRARY, we need to be careful with the tou_socket PARAMETERS.
	// For now, this should do!
	sockfd = -1;

	if (mConnectFlags & RS_CB_FLAG_MODE_UDP_DIRECT)
	{
		std::cerr << "pqissludp::attach() Opening DIRECT Socket";
		std::cerr << std::endl;
		sockfd = tou_socket(RSUDP_TOU_RECVER_DIRECT_IDX,TOU_RECEIVER_TYPE_UDPPEER,0);
	}
	else if (mConnectFlags & RS_CB_FLAG_MODE_UDP_PROXY)
	{
		std::cerr << "pqissludp::attach() Opening PROXY Socket";
		std::cerr << std::endl;
		sockfd = tou_socket(RSUDP_TOU_RECVER_PROXY_IDX,TOU_RECEIVER_TYPE_UDPPEER,0);
	}
	else if (mConnectFlags & RS_CB_FLAG_MODE_UDP_RELAY)
	{
		std::cerr << "pqissludp::attach() Opening RELAY Socket";
		std::cerr << std::endl;
		sockfd = tou_socket(RSUDP_TOU_RECVER_RELAY_IDX,TOU_RECEIVER_TYPE_UDPRELAY,0);
	}
	else 
	{
		std::cerr << "pqissludp::attach() ERROR unknown Connect Mode" << std::endl;
		std::cerr << "pqissludp::attach() mConnectFlags: " << std::hex << mConnectFlags << std::dec;
		std::cerr << std::endl;
		sockfd = -1;
	}

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
	//remote_addr.sin_family = AF_INET;

  	rslog(RSL_DEBUG_BASIC, pqissludpzone, 
	  "pqissludp::Initiate_Connection() Attempting Outgoing Connection....");

	/* decide if we're active or passive */
	if (mConnectFlags & RS_CB_FLAG_ORDER_ACTIVE)
	{
		sslmode = PQISSL_ACTIVE;
	}
	else if (mConnectFlags & RS_CB_FLAG_ORDER_PASSIVE)
	{
		sslmode = PQISSL_PASSIVE;
	}
	else // likely UNSPEC - use old method to decide.
	{
		if (PeerId() < mLinkMgr->getOwnId())
		{
			sslmode = PQISSL_ACTIVE;
		}
		else
		{
			sslmode = PQISSL_PASSIVE;
		}
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
		std::string out = "pqissludp::Initiate_Connection() Connecting To: " + PeerId().toStdString();
		out += " via: ";
		out += sockaddr_storage_tostring(remote_addr);
		out += " ";

		if (sslmode)
		{
			out += "ACTIVE Connect (SSL_Connect)";
		}
		else
		{
			out += "PASSIVE Connect (SSL_Accept)";
		}
		rslog(RSL_WARNING, pqissludpzone, out);
	}

	if (sockaddr_storage_isnull(remote_addr))
	{
		rslog(RSL_WARNING, pqissludpzone, "pqissludp::Initiate_Connection() Invalid (0.0.0.0) Remote Address, Aborting Connect.");
		waiting = WAITING_FAIL_INTERFACE;

		reset_locked();
		return -1;
	}

	mTimeoutTS = time(NULL) + mConnectTimeout;
	//std::cerr << "Setting Connect Timeout " << mConnectTimeout << " Seconds into Future " << std::endl;
	//std::cerr << " Connect Period is:" << mConnectPeriod <<  std::endl;

	/* <===================== UDP Difference *******************/

	if (mConnectFlags & RS_CB_FLAG_MODE_UDP_DIRECT)
	{
		err = tou_connect(sockfd, (struct sockaddr *) &remote_addr, sizeof(remote_addr), mConnectPeriod);
	}
	else if (mConnectFlags & RS_CB_FLAG_MODE_UDP_PROXY)
	{
		err = tou_connect(sockfd, (struct sockaddr *) &remote_addr, sizeof(remote_addr), mConnectPeriod);
	}
	else if (mConnectFlags & RS_CB_FLAG_MODE_UDP_RELAY)
	{
		std::cerr << "Calling tou_connect_via_relay(";
		std::cerr << sockaddr_storage_tostring(mConnectSrcAddr) << ",";
		std::cerr << sockaddr_storage_tostring(mConnectProxyAddr) << ",";
		std::cerr << sockaddr_storage_tostring(remote_addr) << ")" << std::endl;

		
		{
			std::cerr << "CONVERTING ALL ADDRESSES TO IPV4: TODO make IPV6";
			std::cerr << std::endl;
			
			struct sockaddr_in srcaddr;
			struct sockaddr_in proxyaddr;
			struct sockaddr_in remoteaddr;
			
			if ((mConnectSrcAddr.ss_family != AF_INET) ||
					(mConnectProxyAddr.ss_family != AF_INET) ||
					(remote_addr.ss_family != AF_INET))
			{
				std::cerr << "Error One Address is not IPv4. aborting";
				std::cerr << std::endl;
				abort();
			}
			
			struct sockaddr_in *rap = (struct sockaddr_in *) &remote_addr;
			struct sockaddr_in *pap = (struct sockaddr_in *) &mConnectProxyAddr;
			struct sockaddr_in *sap = (struct sockaddr_in *) &mConnectSrcAddr;
			
			srcaddr.sin_family = AF_INET;
			proxyaddr.sin_family = AF_INET;
			remoteaddr.sin_family = AF_INET;

			srcaddr.sin_addr = sap->sin_addr;
			proxyaddr.sin_addr = pap->sin_addr;
			remoteaddr.sin_addr = rap->sin_addr;
			
			srcaddr.sin_port = sap->sin_port;
			proxyaddr.sin_port = pap->sin_port;
			remoteaddr.sin_port = rap->sin_port;
			
			tou_connect_via_relay(sockfd, &srcaddr, &proxyaddr, &remoteaddr);
			
		}
		

/*** It seems that the UDP Layer sees x 1.2 the traffic of the SSL layer.
 * We need to compensate somewhere... we drop the maximum traffic to 75% of limit
 * to allow for extra lost packets etc.
 * NB: If we have a lossy UDP transmission - re-transmission could cause excessive data to 
 * exceed the limit... This is difficult to account for without hacking the TcpOnUdp layer.
 * If it is noticed as a problem - we'll deal with it then
 */
#define UDP_RELAY_TRANSPORT_OVERHEAD_FACTOR (0.7)

        parent()->setRateCap( UDP_RELAY_TRANSPORT_OVERHEAD_FACTOR * mConnectBandwidth / 1000.0,
				UDP_RELAY_TRANSPORT_OVERHEAD_FACTOR * mConnectBandwidth / 1000.0); // Set RateCap.
	}

	if (0 != err)
	/* <===================== UDP Difference *******************/
	{
		int tou_err = tou_errno(sockfd);
		
		std::string out = "pqissludp::Initiate_Connection()";

		if ((tou_err == EINPROGRESS) || (tou_err == EAGAIN))
		{
			// set state to waiting.....
			waiting = WAITING_SOCK_CONNECT;

			out += " EINPROGRESS Waiting for Socket Connection";
			rslog(RSL_WARNING, pqissludpzone, out);
  
			return 0;
		}
		else if ((tou_err == ENETUNREACH) || (tou_err == ETIMEDOUT))
		{
			out += "ENETUNREACHABLE: cert: " + PeerId().toStdString() + "\n";

			// Then send unreachable message.
			waiting = WAITING_FAIL_INTERFACE;
		}

		rs_sprintf_append(out, "Error: Connection Failed: %d - %s", tou_err, socket_errorType(tou_err).c_str());

		rslog(RSL_WARNING, pqissludpzone, out);

		reset_locked();

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

	if (CheckConnectionTimeout())
	{
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
			rslog(RSL_DEBUG_BASIC, pqissludpzone, "pqissludp::Basic_Connection_Complete() EINPROGRESS: cert: " + PeerId().toStdString());
		}
		else if ((err == ENETUNREACH) || (err == ETIMEDOUT))
		{
			rslog(RSL_WARNING, pqissludpzone, "pqissludp::Basic_Connection_Complete() ENETUNREACH/ETIMEDOUT: cert: " + PeerId().toStdString());

			/* is the second one needed? */
			std::string out = "pqissludp::Basic_Connection_Complete() ";
			rs_sprintf_append(out, "Error: Connection Failed: %d - %s", err, socket_errorType(err).c_str());
			rslog(RSL_DEBUG_BASIC, pqissludpzone, out);

			reset_locked();

			// Then send unreachable message.
			waiting = WAITING_FAIL_INTERFACE;

			return -1;
		}
	}

	/* <===================== UDP Difference *******************/
	if (tou_connected(sockfd))
	/* <===================== UDP Difference *******************/
	{
		rslog(RSL_WARNING, pqissludpzone, "pqissludp::Basic_Connection_Complete() Connection Complete: cert: " + PeerId().toStdString());

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

int pqissludp::net_internal_fcntl_nonblock(int /*fd*/)
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
	rslog(RSL_DEBUG_BASIC, pqissludpzone, "pqissludp::listen() (NULLOP)");

	return 1; //udpproxy->listen();
}

int pqissludp::stoplistening()
{
	rslog(RSL_DEBUG_BASIC, pqissludpzone, "pqissludp::stoplistening() (NULLOP)");

	return 1; //udpproxy->stoplistening();
}


bool 	pqissludp::connect_parameter(uint32_t type, uint32_t value)
{
	{
		RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/
	
		//std::cerr << "pqissludp::connect_parameter() type: " << type << "value: " << value << std::endl;
		if (type == NET_PARAM_CONNECT_PERIOD)
		{
			std::string out;
			rs_sprintf(out, "pqissludp::connect_parameter() Peer: %s PERIOD: %lu", PeerId().toStdString().c_str(), value);
			rslog(RSL_WARNING, pqissludpzone, out);
	
			mConnectPeriod = value;
			std::cerr << out << std::endl;
			return true;
		}
		else if (type == NET_PARAM_CONNECT_FLAGS)
		{
			std::string out;
			rs_sprintf(out, "pqissludp::connect_parameter() Peer: %s FLAGS: %lu", PeerId().toStdString().c_str(), value);
			rslog(RSL_WARNING, pqissludpzone, out);
	
			mConnectFlags = value;
			std::cerr << out<< std::endl;
			return true;
		}
		else if (type == NET_PARAM_CONNECT_BANDWIDTH)
		{
			std::string out;
			rs_sprintf(out, "pqissludp::connect_parameter() Peer: %s BANDWIDTH: %lu", PeerId().toStdString().c_str(), value);
			rslog(RSL_WARNING, pqissludpzone, out);
	
			mConnectBandwidth = value;
			std::cerr << out << std::endl;
			return true;
		}
	}

	return pqissl::connect_parameter(type, value);
}

bool pqissludp::connect_additional_address(uint32_t type, const struct sockaddr_storage &addr)
{
	{
		RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/
		
		if (type == NET_PARAM_CONNECT_PROXY)
		{
			std::string out;
			rs_sprintf(out, "pqissludp::connect_additional_address() Peer: %s PROXYADDR: ", PeerId().toStdString().c_str());
			out += sockaddr_storage_tostring(addr);
			rslog(RSL_WARNING, pqissludpzone, out);
	
			mConnectProxyAddr = addr;
	
			std::cerr << out << std::endl;
			return true;
		}
		else if (type == NET_PARAM_CONNECT_SOURCE)
		{
			std::string out;
			rs_sprintf(out, "pqissludp::connect_additional_address() Peer: %s SRCADDR: ", PeerId().toStdString().c_str());
			out += sockaddr_storage_tostring(addr);
			rslog(RSL_WARNING, pqissludpzone, out);
	
			mConnectSrcAddr = addr;
	
			std::cerr << out << std::endl;
			return true;
		}
	}
	return pqissl::connect_additional_address(type, addr);
}

/********** PQI STREAMER OVERLOADING *********************************/

bool 	pqissludp::moretoread(uint32_t usec)
{
	RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/
		
	{
		std::string out = "pqissludp::moretoread()";
		rs_sprintf_append(out, "  polling socket (%d)", sockfd);
		rslog(RSL_DEBUG_ALL, pqissludpzone, out);
	}

	if (usec)
	{
		//std::cerr << "pqissludp::moretoread() usec parameter: " << usec;
		//std::cerr << std::endl;

		if (0 < tou_maxread(sockfd))
		{
			return true;
		}
		usleep(usec);
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
			rslog(RSL_DEBUG_BASIC, pqissludpzone, "pqissludp::moretoread() EAGAIN/EINPROGRESS: cert " + PeerId().toStdString());
			return 0;

		}
		else if ((err == ENETUNREACH) || (err == ETIMEDOUT))
		{
			rslog(RSL_WARNING, pqissludpzone, "pqissludp::moretoread() ENETUNREACH/ETIMEDOUT: cert " + PeerId().toStdString());
		}
		else if (err == EBADF)
		{
			rslog(RSL_WARNING, pqissludpzone, "pqissludp::moretoread() EBADF: cert " + PeerId().toStdString());
		}
		else 
		{
			std::string out = "pqissludp::moretoread() ";
			rs_sprintf_append(out, " Unknown ERROR: %d: cert ", err, PeerId().toStdString().c_str());
			rslog(RSL_WARNING, pqissludpzone, out);
		}

		reset_locked();
		return 0;
	}

	/* otherwise - not error - strange! */
	rslog(RSL_DEBUG_BASIC, pqissludpzone, 
		"pqissludp::moretoread() No Data + No Error (really nothing)");

	return 0;


}

bool 	pqissludp::cansend(uint32_t usec)
{
	RsStackMutex stack(mSslMtx); /**** LOCKED MUTEX ****/

	if (usec)
	{
		std::cerr << "pqissludp::cansend() usec parameter: " << usec;
		std::cerr << std::endl;

		if (0 < tou_maxwrite(sockfd))
		{
			return true;
		}

		usleep(usec);
	}

	rslog(RSL_DEBUG_ALL, pqissludpzone, 
		"pqissludp::cansend() polling socket!");

	/* <===================== UDP Difference *******************/
	return (0 < tou_maxwrite(sockfd));
	/* <===================== UDP Difference *******************/

}



