/*******************************************************************************
 * libretroshare/src/pqi: pqissl.h                                             *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2006  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2015-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

// operating system specific network header.
#include "pqi/pqinetwork.h"

#include <string>
#include <map>

#include "pqi/pqi_base.h"
#include "pqi/authssl.h"
#include "util/rsdebug.h"
#include "retroshare/rsevents.h"

#ifndef RS_DEBUG_PQISSL
#	define RS_DEBUG_PQISSL 1
#endif

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

struct RemotePeerRefusedConnectionEvent : RsEvent
{
	RemotePeerRefusedConnectionEvent();

	RsPeerId mPeerId;
	std::string errorMessage;

	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx) override
	{
		RsEvent::serial_process(j,ctx);
		RS_SERIAL_PROCESS(mPeerId);
		RS_SERIAL_PROCESS(errorMessage);
	}
};


class pqissl;
class cert;

class pqissllistener;
class p3LinkMgr;
struct RsPeerCryptoParams;

/**
 * pqi Net SSL Interface
 * This provides the base SSL interface class,
 * and handles most of the required functionality.
 *
 * there are a series of small fn's that can be overloaded
 * to provide alternative behaviour....
 */
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
virtual bool bandwidthLimited() { return true ; }

public:

/// initiate incoming connection.
int accept(SSL *ssl, int fd, const struct sockaddr_storage &foreign_addr);

void getCryptoParams(RsPeerCryptoParams& params) ;
bool actAsServer();

protected:


	/* no mutex protection for these ones */

	p3LinkMgr *mLinkMgr;
	pqissllistener *pqil; 

	RsMutex mSslMtx; /**** MUTEX protects data and fn below ****/

virtual int reset_locked();

	/// initiate incoming connection.
	int accept_locked( SSL *ssl, int fd,
	                   const sockaddr_storage& foreign_addr );

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

	// check connection timeout.
bool  	CheckConnectionTimeout();


	/* Do we really need this ?
	 * It is very specific TCP+SSL stuff and unlikely to be reused.
	 * In fact we are overloading them in pqissludp case where they do different things or nothing.
	 */
	virtual int net_internal_close(int fd);
	virtual int net_internal_SSL_set_fd(SSL *ssl, int fd);
	virtual int net_internal_fcntl_nonblock(int fd);


	/* data */
	bool active;
	bool certvalid;

	int waiting; 

	// addition for udp (tcp version == ACTIVE).
	int sslmode;     

	SSL* ssl_connection;
	int sockfd;

	sockaddr_storage remote_addr;

	void *readpkt;
	int pktlen;
	int total_len ; // saves the reading state accross successive calls.

	rstime_t attempt_ts;

	int n_read_zero; /* a counter to determine if the connection is really dead */
	rstime_t mReadZeroTS; /* timestamp of first READ_ZERO occurance */

	rstime_t ssl_connect_timeout; /* timeout to ensure that we don't get stuck (can happen on udp!) */

	uint32_t mConnectDelay;
	rstime_t   mConnectTS;
	uint32_t mConnectTimeout;
	rstime_t   mTimeoutTS;

private:
	// ssl only fns.
	int connectInterface(const struct sockaddr_storage &addr);

protected:
#if defined(RS_DEBUG_PQISSL) && RS_DEBUG_PQISSL == 1
	using Dbg1 = RsDbg;
	using Dbg2 = RsNoDbg;
	using Dbg3 = RsNoDbg;
#elif defined(RS_DEBUG_PQISSL) && RS_DEBUG_PQISSL == 2
	using Dbg1 = RsDbg;
	using Dbg2 = RsDbg;
	using Dbg3 = RsNoDbg;
#elif defined(RS_DEBUG_PQISSL) && RS_DEBUG_PQISSL >= 3
	using Dbg1 = RsDbg;
	using Dbg2 = RsDbg;
	using Dbg3 = RsDbg;
#else // RS_DEBUG_PQISSL
	using Dbg1 = RsNoDbg;
	using Dbg2 = RsNoDbg;
	using Dbg3 = RsNoDbg;
#endif // RS_DEBUG_PQISSL
};
