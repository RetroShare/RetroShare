/*******************************************************************************
 * libretroshare/src/pqi: pqissllistener.h                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie.                                       *
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

#include <openssl/ssl.h>

// operating system specific network header.
#include "pqi/pqinetwork.h"

#include <string>
#include <map>

#include "pqi/pqi_base.h"
#include "pqi/pqilistener.h"

#include "pqi/authssl.h"

#ifndef RS_DEBUG_PQISSLLISTENER
#	define RS_DEBUG_PQISSLLISTENER 1
#endif

/***************************** pqi Net SSL Interface *********************************
 */

class pqissl;
class p3PeerMgr;

class AcceptedSSL
{
public:
	int mFd;
	SSL *mSSL;
	RsPeerId mPeerId;

	sockaddr_storage mAddr;
	rstime_t mAcceptTS;
};




class pqissllistenbase: public pqilistener
{
public:
	pqissllistenbase(const struct sockaddr_storage &addr, p3PeerMgr *pm);
	virtual ~pqissllistenbase();

	/*************************************/
	/*       LISTENER INTERFACE          */
	virtual int tick();
	virtual int status();
	virtual int setListenAddr(const struct sockaddr_storage &addr);
	virtual int setuplisten();
	virtual int resetlisten();
	/*************************************/

	int acceptconnection();
	int continueaccepts();
	int finaliseAccepts();

	struct IncomingSSLInfo
	{
		SSL *ssl ;
		sockaddr_storage addr ;
		RsPgpId gpgid ;
		RsPeerId sslid ;
		std::string sslcn ;
	};

	// fn to get cert, anyway
	int	continueSSL(IncomingSSLInfo&, bool);
	int closeConnection(int fd, SSL *ssl);
	int isSSLActive(int fd, SSL *ssl);

	virtual int completeConnection(int sockfd, IncomingSSLInfo&) = 0;
	virtual int finaliseConnection(int fd, SSL *ssl, const RsPeerId& peerId,
								   const sockaddr_storage &raddr) = 0;

protected:
	struct sockaddr_storage laddr;
	std::list<AcceptedSSL> accepted_ssl;
	p3PeerMgr *mPeerMgr;

private:
	/**
	 * @deprecated When this function get called the information is usually not
	 * available anymore because the authentication is completely handled by
	 * OpenSSL through @see AuthSSL::VerifyX509Callback, so debug messages and
	 * notifications generated from this functions are most of the time
	 * misleading saying that the certificate wasn't provided by the peer, while
	 * it have been provided but already deleted by OpenSSL.
	 */
	RS_DEPRECATED
	int Extract_Failed_SSL_Certificate(const IncomingSSLInfo&);

	bool active;
	int lsock;
	std::list<IncomingSSLInfo> incoming_ssl;

protected:
#if defined(RS_DEBUG_PQISSLLISTENER) && RS_DEBUG_PQISSLLISTENER == 1
	using Dbg1 = RsDbg;
	using Dbg2 = RsNoDbg;
	using Dbg3 = RsNoDbg;
#elif defined(RS_DEBUG_PQISSLLISTENER) && RS_DEBUG_PQISSLLISTENER == 2
	using Dbg1 = RsDbg;
	using Dbg2 = RsDbg;
	using Dbg3 = RsNoDbg;
#elif defined(RS_DEBUG_PQISSLLISTENER) && RS_DEBUG_PQISSLLISTENER >= 3
	using Dbg1 = RsDbg;
	using Dbg2 = RsDbg;
	using Dbg3 = RsDbg;
#else // RS_DEBUG_PQISSLLISTENER
	using Dbg1 = RsNoDbg;
	using Dbg2 = RsNoDbg;
	using Dbg3 = RsNoDbg;
#endif // RS_DEBUG_PQISSLLISTENER
};


class pqissllistener: public pqissllistenbase
{
public:
	pqissllistener(const struct sockaddr_storage &addr, p3PeerMgr *pm) :
		pqissllistenbase(addr, pm) {}
	virtual ~pqissllistener() {}

	int addlistenaddr(const RsPeerId& id, pqissl *acc);
	int removeListenPort(const RsPeerId& id);

	virtual int status();
	virtual int completeConnection(int sockfd, IncomingSSLInfo&);
	virtual int finaliseConnection(int fd, SSL *ssl, const RsPeerId& peerId,
								   const sockaddr_storage &raddr);

private:
	std::map<RsPeerId, pqissl*> listenaddr;
};
