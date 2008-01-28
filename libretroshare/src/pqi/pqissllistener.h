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

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
#include "pqi/authxpgp.h"
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
//#include "pqi/sslcert.h"
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

/***************************** pqi Net SSL Interface *********************************
 */

class pqissl;

class pqissllistenbase: public pqilistener
{
	public:


	pqissllistenbase(struct sockaddr_in addr, p3AuthMgr *am, p3ConnectMgr *cm);
virtual ~pqissllistenbase();

/*************************************/
/*       LISTENER INTERFACE         **/

virtual int 	tick();
virtual int 	status();
virtual int     setListenAddr(struct sockaddr_in addr);
virtual int	setuplisten();
virtual int     resetlisten();

/*************************************/

int	acceptconnection();
int	continueaccepts();
int	continueSSL(SSL *ssl, struct sockaddr_in remote_addr, bool);


virtual int completeConnection(int sockfd, SSL *in_connection, struct sockaddr_in &raddr) = 0;

	protected:

	struct sockaddr_in laddr;

	private:

	// fn to get cert, anyway
int     Extract_Failed_SSL_Certificate(SSL *ssl, struct sockaddr_in *inaddr);

	bool active;
	int lsock;

	std::map<SSL *, struct sockaddr_in> incoming_ssl;

	protected:

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)

	AuthXPGP *mAuthMgr;

#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	p3AuthMgr *mAuthMgr;

#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	p3ConnectMgr *mConnMgr;

};


class pqissllistener: public pqissllistenbase
{
	public:

	pqissllistener(struct sockaddr_in addr, p3AuthMgr *am, p3ConnectMgr *cm);
virtual ~pqissllistener();

int 	addlistenaddr(std::string id, pqissl *acc);
int	removeListenPort(std::string id);

//virtual int 	tick();
virtual int 	status();

virtual int completeConnection(int sockfd, SSL *in_connection, struct sockaddr_in &raddr);

	private:

	std::map<std::string, pqissl *> listenaddr;
};


#endif // MRK_PQI_SSL_LISTEN_HEADER
