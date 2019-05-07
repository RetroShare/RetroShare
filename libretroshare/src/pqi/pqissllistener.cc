/*******************************************************************************
 * libretroshare/src/pqi: pqissllistener.cc                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie <retroshare@lunamutt.com>              *
 * Copyright (C) 2015-2018  Gioacchino Mazzurco <gio@eigenlab.org>             *
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

#include "pqi/pqissl.h"
#include "pqi/pqissllistener.h"
#include "pqi/pqinetwork.h"
#include "pqi/sslfns.h"
#include "pqi/p3peermgr.h"
#include "util/rsdebug.h"
#include "util/rsstring.h"
#include "retroshare/rsbanlist.h"
#include "pqi/authgpg.h"

#include <unistd.h>
#include <errno.h>
#include <openssl/err.h>

static struct RsLog::logInfo pqissllistenzoneInfo = {RsLog::Default, "p3peermgr"};
#define pqissllistenzone &pqissllistenzoneInfo

/* NB: This #define makes the listener open 0.0.0.0:X port instead
 * of a specific port - this might help retroshare work on PCs with
 * multiple interfaces or unique network setups.
 * #define OPEN_UNIVERSAL_PORT 1
 */

//#define DEBUG_LISTENNER
#define OPEN_UNIVERSAL_PORT 1

/************************ PQI SSL LISTEN BASE ****************************
 *
 * This provides all of the basic connection stuff, 
 * and calls completeConnection afterwards...
 *
 */


pqissllistenbase::pqissllistenbase(const sockaddr_storage &addr, p3PeerMgr *pm)
    : mPeerMgr(pm), active(false)
{
	sockaddr_storage_copy(addr, laddr);

	if (!(AuthSSL::getAuthSSL()-> active()))
	{
		pqioutput(PQL_ALERT, pqissllistenzone,
				  "SSL-CTX-CERT-ROOT not initialised!");
		exit(1);
	}

	setuplisten();
}

pqissllistenbase::~pqissllistenbase()
{
    if(lsock != -1)
    {
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX
        shutdown(lsock, SHUT_RDWR);
        close(lsock);
#else //WINDOWS_SYS
        closesocket(lsock);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
    }
}

int 	pqissllistenbase::tick()
{
	status();
	// check listen port.
	acceptconnection();
	continueaccepts();
	return finaliseAccepts();
}

int 	pqissllistenbase::status()
{
	std::string out;
	rs_sprintf(out, "pqissllistenbase::status(): Listening on port: %u", sockaddr_storage_port(laddr));
	pqioutput(PQL_DEBUG_ALL, pqissllistenzone, out);
	return 1;
}

int pqissllistenbase::setuplisten()
{
	int err;
	if (active) return -1;

	lsock = socket(PF_INET6, SOCK_STREAM, 0);

#ifdef IPV6_V6ONLY
	int no = 0;
	err = rs_setsockopt(lsock, IPPROTO_IPV6, IPV6_V6ONLY,
	                    reinterpret_cast<uint8_t*>(&no), sizeof(no));
	if (err) std::cerr << __PRETTY_FUNCTION__
	                   << ": Error setting IPv6 socket dual stack" << std::endl;
#ifdef DEBUG_LISTENNER
	else std::cerr << __PRETTY_FUNCTION__
	               << ": Success setting IPv6 socket dual stack" << std::endl;
#endif
#endif // IPV6_V6ONLY

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX
	if (lsock < 0)
	{
		pqioutput(PQL_ALERT, pqissllistenzone, 
		 "pqissllistenbase::setuplisten() Cannot Open Socket!");

		return -1;
	}

        err = fcntl(lsock, F_SETFL, O_NONBLOCK);
	if (err < 0)
	{
        		shutdown(lsock,SHUT_RDWR) ;
                	close(lsock) ;
                    	lsock = -1 ;
                        
		std::string out;
		rs_sprintf(out, "Error: Cannot make socket NON-Blocking: %d", err);
		pqioutput(PQL_ERROR, pqissllistenzone, out);

		return -1;
	}

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else //WINDOWS_SYS 
        if ((unsigned) lsock == INVALID_SOCKET)
        {
        std::string out = "pqissllistenbase::setuplisten() Cannot Open Socket!\n";
        out += "Socket Error: "+ socket_errorType(WSAGetLastError());
        pqioutput(PQL_ALERT, pqissllistenzone, out);

		return -1;
	}

	// Make nonblocking.
	unsigned long int on = 1;
	if (0 != (err = ioctlsocket(lsock, FIONBIO, &on)))
	{
        		closesocket(lsock) ;
                	lsock = -1 ;
                    
		std::string out;
		rs_sprintf(out, "pqissllistenbase::setuplisten() Error: Cannot make socket NON-Blocking: %d\n", err);
		out += "Socket Error: " + socket_errorType(WSAGetLastError());
		pqioutput(PQL_ALERT, pqissllistenzone, out);

		return -1;
	}
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

	// setup listening address.

	{
		std::string out = "pqissllistenbase::setuplisten()\n";
		out += "\t FAMILY: " + sockaddr_storage_familytostring(laddr);
		out += "\t ADDRESS: " + sockaddr_storage_tostring(laddr);
		
        pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, out);
                //std::cerr << out.str() << std::endl;
	}
	
	/* added a call to REUSEADDR, so that we can re-open an existing socket
	 * when we restart_listener.
	 */

    	{
      		int on = 1;

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX
      		if (setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
#else //WINDOWS_SYS 
      		if (setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on)) < 0)
#endif 
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
        	{
            std::string out = "pqissllistenbase::setuplisten() Cannot setsockopt SO_REUSEADDR!\n";
			showSocketError(out);
			pqioutput(PQL_ALERT, pqissllistenzone, out);
			std::cerr << out << std::endl;

			exit(1); 
        	}
    	}

	struct sockaddr_storage tmpaddr;
	sockaddr_storage_copy(laddr, tmpaddr);
	sockaddr_storage_ipv4_to_ipv6(tmpaddr);
	if (!mPeerMgr->isHidden()) sockaddr_storage_zeroip(tmpaddr);

	if (0 != (err = rs_bind(lsock, tmpaddr)))
	{
		std::string out = "pqissllistenbase::setuplisten()  Cannot Bind to Local Address!\n";
		showSocketError(out);
		pqioutput(PQL_ALERT, pqissllistenzone, out);
		std::cerr << out << std::endl
		          << "tmpaddr: " << sockaddr_storage_tostring(tmpaddr)
		          << std::endl;
		print_stacktrace();

		return -1;
	}
	else
	{
		pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, 
		  "pqissllistenbase::setuplisten() Bound to Address.");
	}

#ifdef WINDOWS_SYS
	/* Set TCP buffer size for Windows systems */

	int sockbufsize = 0;
	int size = sizeof(int);

	err = getsockopt(lsock, SOL_SOCKET, SO_RCVBUF, (char *)&sockbufsize, &size);
	if (err == 0) {
		std::cerr << "pqissllistenbase::setuplisten: Current TCP receive buffer size " << sockbufsize << std::endl;
	} else {
		std::cerr << "pqissllistenbase::setuplisten: Error getting TCP receive buffer size. Error " << err << std::endl;
	}

	sockbufsize = 0;

	err = getsockopt(lsock, SOL_SOCKET, SO_SNDBUF, (char *)&sockbufsize, &size);

	if (err == 0) {
		std::cerr << "pqissllistenbase::setuplisten: Current TCP send buffer size " << sockbufsize << std::endl;
	} else {
		std::cerr << "pqissllistenbase::setuplisten: Error getting TCP send buffer size. Error " << err << std::endl;
	}

	sockbufsize = WINDOWS_TCP_BUFFER_SIZE;

	err = setsockopt(lsock, SOL_SOCKET, SO_RCVBUF, (char *)&sockbufsize, sizeof(sockbufsize));

	if (err == 0) {
		std::cerr << "pqissllistenbase::setuplisten: TCP receive buffer size set to " << sockbufsize << std::endl;
	} else {
		std::cerr << "pqissllistenbase::setuplisten: Error setting TCP receive buffer size. Error " << err << std::endl;
	}

	err = setsockopt(lsock, SOL_SOCKET, SO_SNDBUF, (char *)&sockbufsize, sizeof(sockbufsize));

	if (err == 0) {
		std::cerr << "pqissllistenbase::setuplisten: TCP send buffer size set to " << sockbufsize << std::endl;
	} else {
		std::cerr << "pqissllistenbase::setuplisten: Error setting TCP send buffer size. Error " << err << std::endl;
	}
#endif

	if (0 != (err = listen(lsock, 100)))
	{
		std::string out;
		rs_sprintf(out, "pqissllistenbase::setuplisten() Error: Cannot Listen to Socket: %d\n", err);
		showSocketError(out);
		pqioutput(PQL_ALERT, pqissllistenzone, out);
		std::cerr << out << std::endl;

		exit(1); 
		return -1;
	}
	else
	{
		pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, 
		  "pqissllistenbase::setuplisten() Listening to Socket");
	}
	active = true;
	return 1;
}

int	pqissllistenbase::setListenAddr(const struct sockaddr_storage &addr)
{
	laddr = addr;
	return 1;
}

int	pqissllistenbase::resetlisten()
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
        lsock = -1;
		active = false;
		return 1;
	}

	return 0;
}


int	pqissllistenbase::acceptconnection()
{
	if (!active)
		return 0;
	// check port for any socets...
	pqioutput(PQL_DEBUG_ALL, pqissllistenzone, "pqissllistenbase::accepting()");

	// These are local but temp variables...
	// can't be arsed making them all the time.
	struct sockaddr_storage remote_addr;
	socklen_t addrlen = sizeof(remote_addr);

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX
	int fd = accept(lsock, (struct sockaddr *) &remote_addr, &addrlen);
	int err = 0;
    
        if (fd < 0)
        {
		pqioutput(PQL_DEBUG_ALL, pqissllistenzone, 
		 "pqissllistenbase::acceptconnnection() Nothing to Accept!");
		return 0;
	}

        err = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (err < 0)
	{
		std::string out;
		rs_sprintf(out, "pqissllistenbase::acceptconnection() Error: Cannot make socket NON-Blocking: %d", err);
		pqioutput(PQL_ERROR, pqissllistenzone, out);

		close(fd);
		return -1;
	}

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else //WINDOWS_SYS 
	SOCKET fd = accept(lsock, (struct sockaddr *) &remote_addr, &addrlen);
	int err = 0;
    
        if (fd == INVALID_SOCKET)
        {
		pqioutput(PQL_DEBUG_ALL, pqissllistenzone, 
		 "pqissllistenbase::acceptconnnection() Nothing to Accept!");
		return 0;
	}

	// Make nonblocking.
	unsigned long int on = 1;
	if (0 != (err = ioctlsocket(fd, FIONBIO, &on)))
	{
		std::string out;
		rs_sprintf(out, "pqissllistenbase::acceptconnection() Error: Cannot make socket NON-Blocking: %d\n", err);
		out += "Socket Error:" + socket_errorType(WSAGetLastError());
		pqioutput(PQL_ALERT, pqissllistenzone, out);

		closesocket(fd);
		return 0;
	}
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

    std::cerr << "(II) Checking incoming connection address: " << sockaddr_storage_iptostring(remote_addr) ;
        if(rsBanList != NULL && !rsBanList->isAddressAccepted(remote_addr, RSBANLIST_CHECKING_FLAGS_BLACKLIST))
        {
            std::cerr << " => early rejected at this point, because of blacklist." << std::endl;
#ifndef WINDOWS_SYS
            close(fd);
#else
            closesocket(fd);
#endif
            return false ;
        }
        else
            std::cerr << " => Accepted (i.e. whitelisted, or not blacklisted)." << std::endl;
        
    {
      std::string out;
      out += "Accepted Connection from ";
      out += sockaddr_storage_tostring(remote_addr);
      pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, out);
    }

	// Negotiate certificates. SSL stylee.
	// Allow negotiations for secure transaction.
	
	IncomingSSLInfo incoming_connexion_info ;

	incoming_connexion_info.ssl   = SSL_new(AuthSSL::getAuthSSL() -> getCTX());
	incoming_connexion_info.addr  = remote_addr ;
	incoming_connexion_info.gpgid.clear() ;
	incoming_connexion_info.sslid.clear() ;
	incoming_connexion_info.sslcn = "" ;

	SSL_set_fd(incoming_connexion_info.ssl, fd);

    return continueSSL(incoming_connexion_info, true); // continue and save if incomplete.
}

int	pqissllistenbase::continueSSL(IncomingSSLInfo& incoming_connexion_info, bool addin)
{
	// attempt the accept again.
    int fd =  SSL_get_fd(incoming_connexion_info.ssl);

    AuthSSL::getAuthSSL()->setCurrentConnectionAttemptInfo(RsPgpId(),RsPeerId(),std::string()) ;
    int err = SSL_accept(incoming_connexion_info.ssl);

    // Now grab the connection info that was filled in by the callback.
    // In the case the callback did not succeed the SSL certificate will not be accessible
    // from SSL_get_peer_certificate, so we need to get it from the callback system.
    //
    AuthSSL::getAuthSSL()->getCurrentConnectionAttemptInfo(incoming_connexion_info.gpgid,incoming_connexion_info.sslid,incoming_connexion_info.sslcn) ;

#ifdef DEBUG_LISTENNER
    std::cerr << "Info from callback: " << std::endl;
        std::cerr << "  Got PGP Id = " << incoming_connexion_info.gpgid << std::endl;
        std::cerr << "  Got SSL Id = " << incoming_connexion_info.sslid << std::endl;
        std::cerr << "  Got SSL CN = " << incoming_connexion_info.sslcn << std::endl;
#endif

    if (err <= 0)
	{
		int ssl_err = SSL_get_error(incoming_connexion_info.ssl, err);
		int err_err = ERR_get_error();

		{
			std::string out;
			rs_sprintf(out, "pqissllistenbase::continueSSL() Issues with SSL Accept(%d)!\n", err);
			printSSLError(incoming_connexion_info.ssl, err, ssl_err, err_err, out);
			pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, out);
		}

		switch (ssl_err) {
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
			{
				std::string out = "pqissllistenbase::continueSSL() Connection Not Complete!\n";

				if (addin)
				{
					out += "pqissllistenbase::continueSSL() Adding SSL to incoming!";

					// add to incomingqueue.
					incoming_ssl.push_back(incoming_connexion_info) ;
				}

				pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, out);

				// zero means still continuing....
				return 0;
			}
			break;
			case SSL_ERROR_SYSCALL:
			{
				std::string out = "pqissllistenbase::continueSSL() Connection failed!\n";
				pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, out);

				closeConnection(fd, incoming_connexion_info.ssl);

				// basic-error while connecting, no security message needed
				return -1;
			}
			break;
		}

		closeConnection(fd, incoming_connexion_info.ssl) ;

		pqioutput(PQL_WARNING, pqissllistenzone, "Read Error on the SSL Socket\nShutting it down!");

		// failure -1, pending 0, sucess 1.
		return -1;
    }

    // Now grab the connection info from the SSL itself, because the callback info might be
    // tempered due to multiple connection attempts at once.
    //
    X509 *x509 = SSL_get_peer_certificate(incoming_connexion_info.ssl) ;

	if(x509)
	{
		incoming_connexion_info.gpgid = RsX509Cert::getCertIssuer(*x509);
		incoming_connexion_info.sslcn = RsX509Cert::getCertName(*x509);
		incoming_connexion_info.sslid = RsX509Cert::getCertSslId(*x509);

#ifdef DEBUG_LISTENNER
        std::cerr << "  Got PGP Id = " << incoming_connexion_info.gpgid << std::endl;
        std::cerr << "  Got SSL Id = " << incoming_connexion_info.sslid << std::endl;
        std::cerr << "  Got SSL CN = " << incoming_connexion_info.sslcn << std::endl;
#endif
    }
#ifdef DEBUG_LISTENNER
    else
        std::cerr << "  no info." << std::endl;
#endif


	// if it succeeds
	if (0 < completeConnection(fd, incoming_connexion_info))
	{
		return 1;
	}

	/* else we shut it down! */
  	pqioutput(PQL_WARNING, pqissllistenzone, 
	 	"pqissllistenbase::completeConnection() Failed!");

	closeConnection(fd, incoming_connexion_info.ssl) ;

	pqioutput(PQL_WARNING, pqissllistenzone, "Shutting it down!");

	// failure -1, pending 0, sucess 1.
	return -1;
}


int pqissllistenbase::closeConnection(int fd, SSL *ssl)
{
	/* else we shut it down! */
  	pqioutput(PQL_WARNING, pqissllistenzone, "pqissllistenbase::closeConnection() Shutting it Down!");

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

	return 1;
}

int	pqissllistenbase::continueaccepts()
{

	// for each of the incoming sockets.... call continue.

	for(std::list<IncomingSSLInfo>::iterator it = incoming_ssl.begin(); it != incoming_ssl.end();)
	{
		pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, "pqissllistenbase::continueaccepts() Continuing SSL");

		if (0 != continueSSL( *it, false))
		{
			pqioutput(PQL_DEBUG_ALERT, pqissllistenzone, 
					"pqissllistenbase::continueaccepts() SSL Complete/Dead!");

			/* save and increment -> so we can delete */
			std::list<IncomingSSLInfo>::iterator itd = it++;
			incoming_ssl.erase(itd);
		}
		else
			++it;
	}
	return 1;
}

#define ACCEPT_WAIT_TIME 30	

int	pqissllistenbase::finaliseAccepts()
{

	// for each of the incoming sockets.... call continue.
	std::list<AcceptedSSL>::iterator it;

	rstime_t now = time(NULL);
	for(it = accepted_ssl.begin(); it != accepted_ssl.end();)
	{
  	        pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, 
		  "pqissllistenbase::finalisedAccepts() Continuing SSL Accept");

		/* check that the socket is still active - how? */
		int active = isSSLActive(it->mFd, it->mSSL);
		if (active > 0)
		{
  	        	pqioutput(PQL_WARNING, pqissllistenzone, 
		  		"pqissllistenbase::finaliseAccepts() SSL Connection Ok => finaliseConnection");

			if (0 > finaliseConnection(it->mFd, it->mSSL, it->mPeerId, it->mAddr))
			{
				closeConnection(it->mFd, it->mSSL);
			}
			it = accepted_ssl.erase(it);
		}
		else if (active < 0)
		{
          		pqioutput(PQL_WARNING, pqissllistenzone, 
		  		"pqissllistenbase::finaliseAccepts() SSL Connection Dead => closeConnection");

			closeConnection(it->mFd, it->mSSL);
			it = accepted_ssl.erase(it);
		}
		else if (now - it->mAcceptTS > ACCEPT_WAIT_TIME)
		{
          		pqioutput(PQL_WARNING, pqissllistenzone, 
		  		"pqissllistenbase::finaliseAccepts() SSL Connection Timed Out => closeConnection");
			closeConnection(it->mFd, it->mSSL);
			it = accepted_ssl.erase(it);
		}
		else
		{
          		pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, 
		  		"pqissllistenbase::finaliseAccepts() SSL Connection Status Unknown");
			++it;
		}
	}
	return 1;
}

int pqissllistenbase::isSSLActive(int /*fd*/, SSL *ssl)
{

	/* can we just get error? */
	int bufsize = 8; /* just a little look */
	uint8_t buf[bufsize];
	int err = SSL_peek(ssl, buf, bufsize);
	if (err <= 0)
	{
		int ssl_err = SSL_get_error(ssl, err);
		int err_err = ERR_get_error();

		{
		  std::string out;
		  rs_sprintf(out, "pqissllistenbase::isSSLActive() Issues with SSL_Accept(%d)!\n", err);
		  printSSLError(ssl, err, ssl_err, err_err, out);
		  pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, out);
		}

		if (ssl_err == SSL_ERROR_ZERO_RETURN)
		{
			pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, "pqissllistenbase::isSSLActive() SSL_ERROR_ZERO_RETURN Connection state unknown");

			// zero means still continuing....
			return 0;
		}
		if ((ssl_err == SSL_ERROR_WANT_READ) || 
		   (ssl_err == SSL_ERROR_WANT_WRITE))
		{
			pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, "pqissllistenbase::isSSLActive() SSL_ERROR_WANT_READ || SSL_ERROR_WANT_WRITE Connection state unknown");

			// zero means still continuing....
			return 0;
		}
		else
		{
			std::string out;
			rs_sprintf(out, "pqissllistenbase::isSSLActive() Issues with SSL Peek(%d) Likely the Connection was killed by Peer\n", err);
			printSSLError(ssl, err, ssl_err, err_err, out);
			pqioutput(PQL_ALERT, pqissllistenzone, out);

			return -1;
		}
	}

	pqioutput(PQL_WARNING, pqissllistenzone, "pqissllistenbase::isSSLActive() Successful Peer -> Connection Okay");

	return 1;
}

/************************ PQI SSL LISTENER ****************************
 *
 * This is the standard pqissl listener interface....
 *
 * this class only allows connections from 
 * specific certificates, which are pre specified.
 *
 */

int pqissllistener::addlistenaddr(const RsPeerId& id, pqissl *acc)
{
	std::map<RsPeerId, pqissl *>::iterator it;

	std::string out = "Adding to Cert Listening Addresses Id: " + id.toStdString() + "\nCurrent Certs:\n";
	for(it = listenaddr.begin(); it != listenaddr.end(); ++it)
	{
		out += id.toStdString() + "\n";
		if (it -> first == id)
		{
			out += "pqissllistener::addlistenaddr() Already listening for Certificate!\n";
			
			pqioutput(PQL_DEBUG_ALERT, pqissllistenzone, out);
			return -1;
		}
	}

	pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, out);

	// not there can accept it!
	listenaddr[id] = acc;
	return 1;
}

int	pqissllistener::removeListenPort(const RsPeerId& id)
{
	// check where the connection is coming from.
	// if in list of acceptable addresses, 
	//
	// check if in list.
	std::map<RsPeerId, pqissl *>::iterator it;
	for(it = listenaddr.begin();it!=listenaddr.end(); ++it)
	{
		if (it->first == id)
		{
			listenaddr.erase(it);

  	        	pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, 
			  "pqissllisten::removeListenPort() Success!");
			return 1;
		}
	}

  	pqioutput(PQL_WARNING, pqissllistenzone, 
	  "pqissllistener::removeListenPort() Failed to Find a Match");

	return -1;
}


int pqissllistener::status()
{
	pqissllistenbase::status();
	// print certificates we are listening for.
	std::map<RsPeerId, pqissl *>::iterator it;

	std::string out = "pqissllistener::status(): Listening (";
	out += sockaddr_storage_tostring(laddr);
	out += ") for Certs:";
	for(it = listenaddr.begin(); it != listenaddr.end(); ++it)
	{
		out += "\n" + it -> first.toStdString() ;
	}
	pqioutput(PQL_DEBUG_ALL, pqissllistenzone, out);

	return 1;
}

int pqissllistener::completeConnection(int fd, IncomingSSLInfo& info)
{
	constexpr int failure = -1;
	constexpr int success = 1;

	// Get the Peer Certificate....
	X509* peercert = SSL_get_peer_certificate(info.ssl);
	if(!peercert)
	{
		RsFatal() << __PRETTY_FUNCTION__ << " failed to retrieve peer "
		          << "certificate at this point this should never happen!"
		          << std::endl;
		print_stacktrace();
		exit(failure);
	}

	RsPgpId pgpId = RsX509Cert::getCertIssuer(*peercert);
	RsPeerId newPeerId = RsX509Cert::getCertSslId(*peercert);

#ifdef RS_PQISSL_AUTH_REDUNDANT_CHECK
	/* At this point the actual connection authentication has already been
	 * performed in AuthSSL::VerifyX509Callback, any furter authentication check
	 * like the following two are redundant. */

	uint32_t authErrCode = 0;
	if(!AuthSSL::instance().AuthX509WithGPG(peercert, authErrCode))
	{
		RsFatal() << __PRETTY_FUNCTION__ << " failure verifying peer "
		          << "certificate signature. This should never happen at this "
		          << "point!" << std::endl;
		print_stacktrace();

		X509_free(peercert); // not needed but just in case we change to return
		exit(failure);
	}

	if( !AuthGPG::getAuthGPG()->isGPGAccepted(pgpId) )
	{
		RsFatal() << __PRETTY_FUNCTION__ << " pgpId: " << pgpId
		          << " is not friend. It is very unlikely to happen at this "
		          << "point! Either the user must have been so fast to deny "
		          << "friendship just after VerifyX509Callback have returned "
		          << "success and just before this code being executed, or "
		          << "something really fishy is happening! Share the full log "
		          << "with developers." << std::endl;
		print_stacktrace();

		X509_free(peercert); // not needed but just in case we change to return
		exit(failure);
	}
#endif //def RS_PQISSL_AUTH_REDUNDANT_CHECK

	bool found = false;
	for(auto it = listenaddr.begin(); !found && it != listenaddr.end(); )
	{
		if (it -> first == newPeerId) found = true;
		else ++it;
	}

	if (found == false)
	{
		Dbg1() << __PRETTY_FUNCTION__ << " got secure connection from address: "
		       << info.addr << " with previously unknown SSL certificate: "
		       << newPeerId << " signed by PGP friend: " << pgpId
		       << ". Adding the new location as SSL friend." << std::endl;

		mPeerMgr->addFriend(newPeerId, pgpId);
	}

	// Cleanup cert.
	X509_free(peercert);

	// Pushback into Accepted List.
	AcceptedSSL as;
	as.mFd = fd;
	as.mSSL = info.ssl;
	as.mPeerId = newPeerId;
	as.mAddr = info.addr;
	as.mAcceptTS = time(nullptr);

	accepted_ssl.push_back(as);

	Dbg1() << __PRETTY_FUNCTION__ << "Successful Connection with: "
	       << newPeerId << " with address: " << info.addr << std::endl;

	return success;
}

int pqissllistener::finaliseConnection(int fd, SSL *ssl, const RsPeerId& peerId, const struct sockaddr_storage &remote_addr)
{ 
	std::map<RsPeerId, pqissl *>::iterator it;

	std::string out = "pqissllistener::finaliseConnection()\n";
	out += "checking: " + peerId.toStdString() + "\n";
	// check if cert is in the list.....

	it = listenaddr.find(peerId);
	if (it == listenaddr.end())
	{
		out += "No Matching Peer for Connection:";
		out += sockaddr_storage_tostring(remote_addr);
		out += "\npqissllistener => Shutting Down!";
		pqioutput(PQL_WARNING, pqissllistenzone, out);
		return -1;
	}

	out += "Found Matching Peer for Connection:";
	out += sockaddr_storage_tostring(remote_addr);
	out += "\npqissllistener => Passing to pqissl module!";
	pqioutput(PQL_WARNING, pqissllistenzone, out);

    std::cerr << "pqissllistenner::finaliseConnection() connected to " << sockaddr_storage_tostring(remote_addr) << std::endl;

	// hand off ssl conection.
	pqissl *pqis = it -> second;
	pqis -> accept(ssl, fd, remote_addr);

	return 1;
}
