/*
 * "$Id: pqissllistener.cc,v 1.3 2007-02-18 21:46:49 rmf24 Exp $"
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
#include "pqi/pqissllistener.h"
#include "pqi/pqinetwork.h"
#include "pqi/sslfns.h"

#include "pqi/p3peermgr.h"

#include <errno.h>
#include <openssl/err.h>

#include "util/rsdebug.h"
#include <sstream>
#include <unistd.h>

const int pqissllistenzone = 49787;

/* NB: This #define makes the listener open 0.0.0.0:X port instead
 * of a specific port - this might help retroshare work on PCs with
 * multiple interfaces or unique network setups.
 * #define OPEN_UNIVERSAL_PORT 1
 */

#define OPEN_UNIVERSAL_PORT 1

/************************ PQI SSL LISTEN BASE ****************************
 *
 * This provides all of the basic connection stuff, 
 * and calls completeConnection afterwards...
 *
 */


pqissllistenbase::pqissllistenbase(struct sockaddr_in addr, p3PeerMgr *pm)
        :laddr(addr), active(false), mPeerMgr(pm)

{
        if (!(AuthSSL::getAuthSSL()-> active())) {
		pqioutput(PQL_ALERT, pqissllistenzone, 
			"SSL-CTX-CERT-ROOT not initialised!");

		exit(1);
	}

	setuplisten();
	return;
}

pqissllistenbase::~pqissllistenbase()
{
	return;
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
	std::ostringstream out;
	out << "pqissllistenbase::status(): ";
	out << " Listening on port: " << ntohs(laddr.sin_port) << std::endl;
	pqioutput(PQL_DEBUG_ALL, pqissllistenzone, out.str());
	return 1;
}



int	pqissllistenbase::setuplisten()
{
        int err;
	if (active)
		return -1;

        lsock = socket(PF_INET, SOCK_STREAM, 0);
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
		std::ostringstream out;
		out << "Error: Cannot make socket NON-Blocking: ";
		out << err << std::endl;
		pqioutput(PQL_ERROR, pqissllistenzone, out.str());

		return -1;
	}

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else //WINDOWS_SYS 
        if ((unsigned) lsock == INVALID_SOCKET)
        {
		std::ostringstream out;
		out << "pqissllistenbase::setuplisten()";
		out << " Cannot Open Socket!" << std::endl;
		out << "Socket Error:";
		out  << socket_errorType(WSAGetLastError()) << std::endl;
		pqioutput(PQL_ALERT, pqissllistenzone, out.str());

		return -1;
	}

	// Make nonblocking.
	unsigned long int on = 1;
	if (0 != (err = ioctlsocket(lsock, FIONBIO, &on)))
	{
		std::ostringstream out;
		out << "pqissllistenbase::setuplisten()";
		out << "Error: Cannot make socket NON-Blocking: ";
		out << err << std::endl;
		out << "Socket Error:";
		out << socket_errorType(WSAGetLastError()) << std::endl;
		pqioutput(PQL_ALERT, pqissllistenzone, out.str());

		return -1;
	}
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

	// setup listening address.

	// fill in fconstant bits.

	laddr.sin_family = AF_INET;

	{
		std::ostringstream out;
		out << "pqissllistenbase::setuplisten()";
		out << "\tAddress Family: " << (int) laddr.sin_family;
		out << std::endl;
		out << "\tSetup Address: " << rs_inet_ntoa(laddr.sin_addr);
		out << std::endl;
		out << "\tSetup Port: " << ntohs(laddr.sin_port);

		pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, out.str());
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
			std::ostringstream out;
			out << "pqissllistenbase::setuplisten()";
			out << " Cannot setsockopt SO_REUSEADDR!" << std::endl;
			showSocketError(out);
			pqioutput(PQL_ALERT, pqissllistenzone, out.str());
			std::cerr << out.str() << std::endl;

			exit(1); 
        	}
    	}

#ifdef OPEN_UNIVERSAL_PORT
	struct sockaddr_in tmpaddr = laddr;
	tmpaddr.sin_addr.s_addr = 0;
	if (0 != (err = bind(lsock, (struct sockaddr *) &tmpaddr, sizeof(tmpaddr))))
#else
	if (0 != (err = bind(lsock, (struct sockaddr *) &laddr, sizeof(laddr))))
#endif
	{
		std::ostringstream out;
		out << "pqissllistenbase::setuplisten()";
		out << " Cannot Bind to Local Address!" << std::endl;
		showSocketError(out);
		pqioutput(PQL_ALERT, pqissllistenzone, out.str());
                std::cerr << out.str() << std::endl;

		exit(1); 
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
		std::ostringstream out;
		out << "pqissllistenbase::setuplisten()";
		out << "Error: Cannot Listen to Socket: ";
		out << err << std::endl;
		showSocketError(out);
		pqioutput(PQL_ALERT, pqissllistenzone, out.str());
		std::cerr << out.str() << std::endl;

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

int	pqissllistenbase::setListenAddr(struct sockaddr_in addr)
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
	struct sockaddr_in remote_addr;
	socklen_t addrlen = sizeof(remote_addr);
	int fd = accept(lsock, (struct sockaddr *) &remote_addr, &addrlen);
	int err = 0;

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX
        if (fd < 0)
        {
		pqioutput(PQL_DEBUG_ALL, pqissllistenzone, 
		 "pqissllistenbase::acceptconnnection() Nothing to Accept!");
		return 0;
	}

        err = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (err < 0)
	{
		std::ostringstream out;
		out << "pqissllistenbase::acceptconnection()";
		out << "Error: Cannot make socket NON-Blocking: ";
		out << err << std::endl;
		pqioutput(PQL_ERROR, pqissllistenzone, out.str());

		close(fd);
		return -1;
	}

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else //WINDOWS_SYS 
        if ((unsigned) fd == INVALID_SOCKET)
        {
		pqioutput(PQL_DEBUG_ALL, pqissllistenzone, 
		 "pqissllistenbase::acceptconnnection() Nothing to Accept!");
		return 0;
	}

	// Make nonblocking.
	unsigned long int on = 1;
	if (0 != (err = ioctlsocket(fd, FIONBIO, &on)))
	{
		std::ostringstream out;
		out << "pqissllistenbase::acceptconnection()";
		out << "Error: Cannot make socket NON-Blocking: ";
		out << err << std::endl;
		out << "Socket Error:";
		out << socket_errorType(WSAGetLastError()) << std::endl;
		pqioutput(PQL_ALERT, pqissllistenzone, out.str());

		closesocket(fd);
		return 0;
	}
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

	{
	  std::ostringstream out;
	  out << "Accepted Connection from ";
	  out << rs_inet_ntoa(remote_addr.sin_addr) << ":" << ntohs(remote_addr.sin_port);
	  pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, out.str());
	}

	// Negotiate certificates. SSL stylee.
	// Allow negotiations for secure transaction.
	
        SSL *ssl = SSL_new(AuthSSL::getAuthSSL() -> getCTX());
	SSL_set_fd(ssl, fd);

	return continueSSL(ssl, remote_addr, true); // continue and save if incomplete.
}

int	pqissllistenbase::continueSSL(SSL *ssl, struct sockaddr_in remote_addr, bool addin)
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
	  	  out << "pqissllistenbase::continueSSL() ";
		  out << "Issues with SSL Accept(" << err << ")!" << std::endl;
		  printSSLError(ssl, err, ssl_err, err_err, out);
	  	  pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, out.str());
		}

		if ((ssl_err == SSL_ERROR_WANT_READ) || 
		   (ssl_err == SSL_ERROR_WANT_WRITE))
		{
	  		std::ostringstream out;
	  	        out << "pqissllistenbase::continueSSL() ";
			out << " Connection Not Complete!";
			out << std::endl;

			if (addin)
			{
	  	        	out << "pqissllistenbase::continueSSL() ";
				out << "Adding SSL to incoming!";

				// add to incomingqueue.
				incoming_ssl[ssl] = remote_addr;
			}

	  	        pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, out.str());

			// zero means still continuing....
			return 0;
		}

		/* we have failed -> get certificate if possible */
		Extract_Failed_SSL_Certificate(ssl, &remote_addr);

		closeConnection(fd, ssl);

		std::ostringstream out;
		out << "Read Error on the SSL Socket";
		out << std::endl;
		out << "Shutting it down!" << std::endl;
  	        pqioutput(PQL_WARNING, pqissllistenzone, out.str());

		// failure -1, pending 0, sucess 1.
		return -1;
	}
	
	// if it succeeds
	if (0 < completeConnection(fd, ssl, remote_addr))
	{
		return 1;
	}

	/* else we shut it down! */
  	pqioutput(PQL_WARNING, pqissllistenzone, 
	 	"pqissllistenbase::completeConnection() Failed!");

	closeConnection(fd, ssl);

	std::ostringstream out;
	out << "Shutting it down!" << std::endl;
  	pqioutput(PQL_WARNING, pqissllistenzone, out.str());

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




int 	pqissllistenbase::Extract_Failed_SSL_Certificate(SSL *ssl, struct sockaddr_in *addr)
{
  	pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, 
	  "pqissllistenbase::Extract_Failed_SSL_Certificate()");

	std::cerr << "pqissllistenbase::Extract_Failed_SSL_Certificate() FAILED CONNECTION due to security!";
	std::cerr << std::endl;

	// Get the Peer Certificate....
	X509 *peercert = SSL_get_peer_certificate(ssl);

	if (peercert == NULL)
	{
		std::ostringstream out;
		out << "pqissllistenbase::Extract_Failed_SSL_Certificate() from: ";
		out << rs_inet_ntoa(addr->sin_addr) << ":" << ntohs(addr->sin_port); 
		out << " ERROR Peer didn't give Cert!";
		std::cerr << out.str() << std::endl;

  		pqioutput(PQL_WARNING, pqissllistenzone, out.str());
		return -1;
	}

  	pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, 
	  "pqissllistenbase::Extract_Failed_SSL_Certificate() Have Peer Cert - Registering");

	{
		std::ostringstream out;

		out << "pqissllistenbase::Extract_Failed_SSL_Certificate() from: ";
		out << rs_inet_ntoa(addr->sin_addr) << ":" << ntohs(addr->sin_port); 
		out << " Passing Cert to AuthSSL() for analysis";
		out << std::endl;
		std::cerr << out.str() << std::endl;

  		pqioutput(PQL_WARNING, pqissllistenzone, out.str());
	}

	// save certificate... (and ip locations)
	// false for outgoing....
        AuthSSL::getAuthSSL()->FailedCertificate(peercert, *addr, true);

	return 1;
}


int	pqissllistenbase::continueaccepts()
{

	// for each of the incoming sockets.... call continue.
	std::map<SSL *, struct sockaddr_in>::iterator it, itd;

	for(it = incoming_ssl.begin(); it != incoming_ssl.end();)
	{
  	        pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, 
		  "pqissllistenbase::continueaccepts() Continuing SSL");
		if (0 != continueSSL(it->first, it->second, false))
		{
  	        	pqioutput(PQL_DEBUG_ALERT, pqissllistenzone, 
			  "pqissllistenbase::continueaccepts() SSL Complete/Dead!");

			/* save and increment -> so we can delete */
			itd = it++;
			incoming_ssl.erase(itd);
		}
		else
		{
			it++;
		}
	}
	return 1;
}

#define ACCEPT_WAIT_TIME 30	

int	pqissllistenbase::finaliseAccepts()
{

	// for each of the incoming sockets.... call continue.
	std::list<AcceptedSSL>::iterator it;

	time_t now = time(NULL);
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
			it++;
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
	  	  std::ostringstream out;
	  	  out << "pqissllistenbase::isSSLActive() ";
		  out << "Issues with SSL_Accept(" << err << ")!" << std::endl;
		  printSSLError(ssl, err, ssl_err, err_err, out);
	  	  pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, out.str());
		}

		if (ssl_err == SSL_ERROR_ZERO_RETURN)
		{
	  		std::ostringstream out;
	  	        out << "pqissllistenbase::isSSLActive() SSL_ERROR_ZERO_RETURN ";
			out << " Connection state unknown";
			out << std::endl;

	  	        pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, out.str());

			// zero means still continuing....
			return 0;
		}
		if ((ssl_err == SSL_ERROR_WANT_READ) || 
		   (ssl_err == SSL_ERROR_WANT_WRITE))
		{
	  		std::ostringstream out;
	  	        out << "pqissllistenbase::isSSLActive() SSL_ERROR_WANT_READ || SSL_ERROR_WANT_WRITE ";
			out << " Connection state unknown";
			out << std::endl;

	  	        pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, out.str());

			// zero means still continuing....
			return 0;
		}
		else
		{
			std::ostringstream out;
	  		out << "pqissllistenbase::isSSLActive() ";
			out << "Issues with SSL Peek(" << err << ") Likely the Connection was killed by Peer" << std::endl;
			printSSLError(ssl, err, ssl_err, err_err, out);
	  		pqioutput(PQL_ALERT, pqissllistenzone, out.str());

			return -1;
		}
	}

	std::ostringstream out;
	out << "pqissllistenbase::isSSLActive() Successful Peer -> Connection Okay";
	pqioutput(PQL_WARNING, pqissllistenzone, out.str());

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

pqissllistener::pqissllistener(struct sockaddr_in addr, p3PeerMgr *lm)
        :pqissllistenbase(addr, lm)
{
	return;
}

pqissllistener::~pqissllistener()
{
	return;
}

int 	pqissllistener::addlistenaddr(std::string id, pqissl *acc)
{
	std::map<std::string, pqissl *>::iterator it;

	std::ostringstream out;

	out << "Adding to Cert Listening Addresses Id: " << id << std::endl;
	out << "Current Certs:" << std::endl;
	for(it = listenaddr.begin(); it != listenaddr.end(); it++)
	{
		out << id << std::endl;
		if (it -> first == id)
		{
			out << "pqissllistener::addlistenaddr()";
			out << "Already listening for Certificate!";
			out << std::endl;
			
			pqioutput(PQL_DEBUG_ALERT, pqissllistenzone, out.str());
			return -1;

		}
	}

	pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, out.str());

	// not there can accept it!
	listenaddr[id] = acc;
	return 1;
}

int	pqissllistener::removeListenPort(std::string id)
{
	// check where the connection is coming from.
	// if in list of acceptable addresses, 
	//
	// check if in list.
	std::map<std::string, pqissl *>::iterator it;
	for(it = listenaddr.begin();it!=listenaddr.end();it++)
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


int 	pqissllistener::status()
{
	pqissllistenbase::status();
	// print certificates we are listening for.
	std::map<std::string, pqissl *>::iterator it;

	std::ostringstream out;
	out << "pqissllistener::status(): ";
	out << " Listening (" << ntohs(laddr.sin_port) << ") for Certs:" << std::endl;
	for(it = listenaddr.begin(); it != listenaddr.end(); it++)
	{
		out << it -> first << std::endl;
	}
	pqioutput(PQL_DEBUG_ALL, pqissllistenzone, out.str());

	return 1;
}

int pqissllistener::completeConnection(int fd, SSL *ssl, struct sockaddr_in &remote_addr)
{ 

	// Get the Peer Certificate....
	X509 *peercert = SSL_get_peer_certificate(ssl);

	if (peercert == NULL)
	{
  	        pqioutput(PQL_WARNING, pqissllistenzone, 
		 "pqissllistener::completeConnection() Peer Did Not Provide Cert!");

		// failure -1, pending 0, sucess 1.
		// pqissllistenbase will shutdown!
		return -1;
	}

	// Check cert.
	std::string newPeerId;


	/****
	 * As the validation is actually done before this...
	 * we should only need to call CheckCertificate here!
	 ****/

        bool certOk = AuthSSL::getAuthSSL()->ValidateCertificate(peercert, newPeerId);

	bool found = false;
	std::map<std::string, pqissl *>::iterator it;

	// Let connected one through as well! if ((npc == NULL) || (npc -> Connected()))
	if (!certOk)
	{
  	        pqioutput(PQL_WARNING, pqissllistenzone, 
		 "pqissllistener::completeConnection() registerCertificate Failed!");

		// bad - shutdown.
		// pqissllistenbase will shutdown!
		X509_free(peercert);

		return -1;
	}
	else
	{
		std::ostringstream out;

		out << "pqissllistener::continueSSL()" << std::endl;
		out << "checking: " << newPeerId << std::endl;
		// check if cert is in our list.....
		for(it = listenaddr.begin();(found!=true) && (it!=listenaddr.end());)
		{
		 	out << "\tagainst: " << it->first << std::endl;
			if (it -> first == newPeerId)
			{
				// accept even if already connected.
		 	        out << "\t\tMatch!";
                                found = true;
			}
			else
			{
				it++;
			}
		}

  	        pqioutput(PQL_DEBUG_BASIC, pqissllistenzone, out.str());
	}
	
	if (found == false)
	{
		std::ostringstream out;
		out << "No Matching Certificate";
		out << " for Connection:" << rs_inet_ntoa(remote_addr.sin_addr);
		out << std::endl;
		out << "pqissllistenbase: Will shut it down!" << std::endl;
  	        pqioutput(PQL_WARNING, pqissllistenzone, out.str());

		// but as it passed the authentication step, 
		// we can add it into the AuthSSL, and mConnMgr.

		AuthSSL::getAuthSSL()->CheckCertificate(newPeerId, peercert);

		/* now need to get GPG id too */
		std::string pgpid = getX509CNString(peercert->cert_info->issuer);
		mPeerMgr->addFriend(newPeerId, pgpid);
	
		X509_free(peercert);
		return -1;
	}

	// Cleanup cert.
	X509_free(peercert);

	// Pushback into Accepted List.
	AcceptedSSL as;
	as.mFd = fd;
	as.mSSL = ssl;
	as.mPeerId = newPeerId;
	as.mAddr = remote_addr;
	as.mAcceptTS = time(NULL);

	accepted_ssl.push_back(as);

	std::ostringstream out;

	out << "pqissllistener::completeConnection() Successful Connection with: " << newPeerId;
	out << " for Connection:" << rs_inet_ntoa(remote_addr.sin_addr) << " Adding to WAIT-ACCEPT Queue";
	out << std::endl;
  	pqioutput(PQL_WARNING, pqissllistenzone, out.str());

	return 1;
}





int pqissllistener::finaliseConnection(int fd, SSL *ssl, std::string peerId, struct sockaddr_in &remote_addr)
{ 
	std::map<std::string, pqissl *>::iterator it;

	std::ostringstream out;

	out << "pqissllistener::finaliseConnection()" << std::endl;
	out << "checking: " << peerId << std::endl;
	// check if cert is in the list.....

	it = listenaddr.find(peerId);
	if (it == listenaddr.end())
	{
		out << "No Matching Peer";
		out << " for Connection:" << rs_inet_ntoa(remote_addr.sin_addr);
		out << std::endl;
		out << "pqissllistener => Shutting Down!" << std::endl;
  	        pqioutput(PQL_WARNING, pqissllistenzone, out.str());
		return -1;
	}

	out << "Found Matching Peer";
	out << " for Connection:" << rs_inet_ntoa(remote_addr.sin_addr);
	out << std::endl;
	out << "pqissllistener => Passing to pqissl module!" << std::endl;
  	pqioutput(PQL_WARNING, pqissllistenzone, out.str());

	// hand off ssl conection.
	pqissl *pqis = it -> second;
	pqis -> accept(ssl, fd, remote_addr);

	return 1;
}




