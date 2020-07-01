/*
 * "$Id: pqinetwork.cc,v 1.18 2007-04-15 18:45:18 rmf24 Exp $"
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

// is the order here impotant? otherwise this can be moved to pqinetwork_win.cc
#ifdef WINDOWS_SYS
#	include "util/rswin.h"
#	include "util/rsmemory.h"
#	include <ws2tcpip.h>
#endif // WINDOWS_SYS

#include "pqi/pqinetwork.h"
#include "util/rsnet.h"

#include <errno.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "util/rsdebug.h"
#include "util/rsstring.h"
#include "util/rsnet.h"

static struct RsLog::logInfo pqinetzoneInfo = {RsLog::Default, "pqinet"};
#define pqinetzone &pqinetzoneInfo

/*****
 * #define NET_DEBUG 1
 ****/

#ifdef __HAIKU__
 #include <sys/sockio.h>
 #define IFF_RUNNING 0x0001
#endif

#include <sys/types.h>

#ifdef WINDOWS_SYS
// moved to pqinetwork_win.cc
#elif defined(__ANDROID__)
#	include <string>
#	include <QString>
#	include <QHostAddress>
#	include <QNetworkInterface>
#else // not __ANDROID__ nor WINDOWS => Linux and other unixes
// moved to pqinetwork_linux.cc
#endif // WINDOWS_SYS

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
#include "pqinetwork_unix.cc"
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else
#include "pqinetwork_win.cc"
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/


bool getLocalAddresses(std::list<sockaddr_storage> & addrs)
{
	bool ret;

	addrs.clear();

	// currently there is no dedicated netOps for android
#ifdef __ANDROID__
	foreach(QHostAddress qAddr, QNetworkInterface::allAddresses())
	{
		sockaddr_storage tmpAddr;
		sockaddr_storage_clear(tmpAddr);
		if(sockaddr_storage_ipv4_aton(tmpAddr, qAddr.toString().toStdString().c_str()))
			addrs.push_back(tmpAddr);
	}
#else
	ret = _getLocalAddresses(addrs);

	// the functions have internal checks that may result in "return false"
	// handle it here
	if(!ret)
		return false;
#endif

#ifdef NET_DEBUG
	std::list<sockaddr_storage>::iterator it;
	std::cout << "getLocalAddresses(...) returning: <" ;
	for(it = addrs.begin(); it != addrs.end(); ++it)
			std::cout << sockaddr_storage_iptostring(*it) << ", ";
	std::cout << ">" << std::endl;
#endif

	return !addrs.empty();
}


/*************************************************************
 * Socket Library Wrapper Functions 
 * to get over the crapness of the windows.
 *
 */
int unix_close(int fd)
{
#ifdef NET_DEBUG
	std::cerr << "unix_close()" << std::endl;
#endif

	return _unix_close(fd);
}

int unix_socket(int domain, int type, int protocol)
{
#ifdef NET_DEBUG
	std::cerr << "unix_socket()" << std::endl;
#endif // NET_DEBUG

	int osock = socket(domain, type, protocol);

	_unix_socket(osock);

	return osock;
}

int unix_fcntl_nonblock(int fd)
{
	return _unix_fcntl_nonblock(fd);
}

int unix_connect(int fd, const struct sockaddr *serv_addr, socklen_t socklen)
{
#ifdef NET_DEBUG
	std::cerr << "unix_connect()";
	std::cerr << std::endl;
#endif

	const struct sockaddr_storage *ss_addr = (struct sockaddr_storage *) serv_addr;
	socklen_t len = socklen;

	switch (ss_addr->ss_family)
	{
		case AF_INET:
			len = sizeof(struct sockaddr_in);
			break;
		case AF_INET6:
			len = sizeof(struct sockaddr_in6);
			break;
	}

	if (len > socklen)
	{
		std::cerr << "unix_connect() ERROR len > socklen";
		std::cerr << std::endl;

		len = socklen;
		//return EINVAL;
	}

	int ret = connect(fd, serv_addr, len);

	ret = _unix_connect(ret);

	return ret;
}

int unix_getsockopt_error(int sockfd, int *err)
{
	*err = 1;

	return _unix_getsockopt_error(sockfd, err);
}
