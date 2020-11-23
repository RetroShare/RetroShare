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

#include <string>
#include <map>

#include "pqi/pqi_base.h"
#include "pqi/pqilistener.h"
#include "pqi/authssl.h"
#include "util/rsdebug.h"
#include "pqi/pqinetwork.h"

#define RS_PQISSL_AUTH_DOUBLE_CHECK 1


//  This is a simple overview of how the listener is setup, ticked (calling accept) and peers added to it.
//  On the highest level (RsServer) the listener lives inside the pqisslpersongrp class (variable: "pqih").
//  Inside pqisslpersongrp the listener is stored in "pqil".
//
//  The listener has an internal list with incoming connections that are handled in a similar fashion to pqissl.
//  (Mainly setting up the socket (non-blocking) and establisching the ssl handshake.)
//  When everything went fine the connection is passed to pqissl in finaliseConnection()
//
//  This is how the listener is initialized during start up:
//
//   RsServer::StartupRetroShare()
//      |
//      +----- pqih = new pqisslpersongrp(serviceCtrl, flags, mPeerMgr);
//      +----- pqih->init_listener();
//                  |
//                  +----- pqil = locked_createListener(laddr);
//                              |
//                              +----- return new pqissllistener(laddr, mPeerMgr);
//
//
//  This is how the listener is ticked to call accept:
//
//   RsServer::StartupRetroShare()
//      |
//      +----- pqih->tick();
//                  |
//                  +----- pqil->tick();
//                              |
//                              +----- acceptconnection();
//                              |           |
//                              |           +----- accecpt()
//                              |
//                              +----- continueaccepts();
//                              +----- finaliseAccepts();
//                                          |
//                                          +----- finaliseConnection()
//                                                      |
//                                                      +----- pqis->accept()
//
//
//  This is how peers (their id) are registered to the listener:
//  (This is only used to tell if a connection peer is known or a new one (which is then added))
//
//   pqipersongrp::addPeer()
//      |
//      +----- pqiperson *pqip = locked_createPerson(id, pqil);
//      |           |
//      |           +----- pqiperson *pqip = new pqiperson(id, this);
//      |           +----- pqissl *pqis = new pqissl((pqissllistener *) listener, pqip, mLinkMgr);
//      |           +----- pqiconnect *pqisc = new pqiconnect(pqip, rss, pqis);
//      |           +----- pqip->addChildInterface(PQI_CONNECT_TCP, pqisc);
//      |                       |
//      |                       +-- sets kids[type] = pqisc;
//      |
//      +----- pqip->reset();
//      +----- pqip->listen();
//                  |
//                  +-- for all kids[]
//                              |
//                              +----- listen() ( of class pqiconnect )
//                                          |
//                                          +----- listen() ( of class pqissl )
//                                                      |
//                                                      +----- pqil->addlistenaddr(PeerId(), this);


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

	bool active;
	int lsock;
	std::list<IncomingSSLInfo> incoming_ssl ;
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

	RS_SET_CONTEXT_DEBUG_LEVEL(2)
};
