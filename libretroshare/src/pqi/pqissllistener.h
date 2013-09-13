/*
 * "$Id: pqissllistener.h,v 1.2 2007-02-18 21:46:49 rmf24 Exp $"
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



#ifndef MRK_PQI_SSL_LISTEN_HEADER
#define MRK_PQI_SSL_LISTEN_HEADER

#include <openssl/ssl.h>

// operating system specific network header.
#include "pqi/pqinetwork.h"

#include <string>
#include <map>

#include "pqi/pqi_base.h"
#include "pqi/pqilistener.h"

#include "pqi/authssl.h"

/***************************** pqi Net SSL Interface *********************************
 */

class pqissl;
class p3PeerMgr;

class AcceptedSSL
{
	public:
	
	int mFd;
	SSL *mSSL;
	std::string mPeerId;
	
	struct sockaddr_storage mAddr;
	time_t mAcceptTS;
};




class pqissllistenbase: public pqilistener
{
	public:


        pqissllistenbase(const struct sockaddr_storage &addr, p3PeerMgr *pm);
virtual ~pqissllistenbase();

/*************************************/
/*       LISTENER INTERFACE         **/

virtual int 	tick();
virtual int 	status();
virtual int     setListenAddr(const struct sockaddr_storage &addr);
virtual int	setuplisten();
virtual int     resetlisten();

/*************************************/

int	acceptconnection();
int	continueaccepts();
int	finaliseAccepts();

	struct IncomingSSLInfo
	{
		SSL *ssl ;
		sockaddr_storage addr ;
		std::string gpgid ;
		std::string sslid ;
		std::string sslcn ;
	};

	// fn to get cert, anyway
int	continueSSL(IncomingSSLInfo&, bool);
int 	closeConnection(int fd, SSL *ssl);
int 	isSSLActive(int fd, SSL *ssl);

virtual int completeConnection(int sockfd, IncomingSSLInfo&) = 0;
virtual int finaliseConnection(int fd, SSL *ssl, std::string peerId, const struct sockaddr_storage &raddr) = 0;
	protected:

	struct sockaddr_storage laddr;
	std::list<AcceptedSSL> accepted_ssl;

	private:

	int Extract_Failed_SSL_Certificate(const IncomingSSLInfo&);

	bool active;
	int lsock;

	std::list<IncomingSSLInfo> incoming_ssl ;

	protected:

	p3PeerMgr *mPeerMgr;

};


class pqissllistener: public pqissllistenbase
{
	public:

        pqissllistener(const struct sockaddr_storage &addr, p3PeerMgr *pm);
virtual ~pqissllistener();

int 	addlistenaddr(std::string id, pqissl *acc);
int	removeListenPort(std::string id);

//virtual int 	tick();
virtual int 	status();

virtual int completeConnection(int sockfd, IncomingSSLInfo&);
virtual int finaliseConnection(int fd, SSL *ssl, std::string peerId, const struct sockaddr_storage &raddr);

	private:

	std::map<std::string, pqissl *> listenaddr;
};


#endif // MRK_PQI_SSL_LISTEN_HEADER
