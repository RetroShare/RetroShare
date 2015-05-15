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

#ifdef WINDOWS_SYS
#include "util/rswin.h"
#include <ws2tcpip.h>
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

static const int pqinetzone = 96184;

/*****
 * #define NET_DEBUG 1
 ****/

#ifdef WINDOWS_SYS   /* Windows - define errno */

int errno;

#else /* Windows - define errno */

#include <netdb.h>

#endif               

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS

void showSocketError(std::string &out)
{
	int err = errno;
	rs_sprintf_append(out, "\tSocket Error(%d) : %s\n", err, socket_errorType(err).c_str());
}

std::string socket_errorType(int err)
{
	if (err == EBADF)
	{
		return std::string("EBADF");
	}
	else if (err == EINVAL)
	{
		return std::string("EINVAL");
	}
	else if (err == EFAULT)
	{
		return std::string("EFAULT");
	}
	else if (err == ENOTSOCK)
	{
		return std::string("ENOTSOCK");
	}
	else if (err == EISCONN)
	{
		return std::string("EISCONN");
	}
	else if (err == ECONNREFUSED)
	{
		return std::string("ECONNREFUSED");
	}
	else if (err == ETIMEDOUT)
	{
		return std::string("ETIMEDOUT");
	}
	else if (err == ENETUNREACH)
	{
		return std::string("ENETUNREACH");
	}
	else if (err == EADDRINUSE)
	{
		return std::string("EADDRINUSE");
	}
	else if (err == EINPROGRESS)
	{
		return std::string("EINPROGRESS");
	}
	else if (err == EALREADY)
	{
		return std::string("EALREADY");
	}
	else if (err == EAGAIN)
	{
		return std::string("EAGAIN");
	}
	else if (err == EISCONN)
	{
		return std::string("EISCONN");
	}
	else if (err == ENOTCONN)
	{
		return std::string("ENOTCONN");
	}
	// These ones have been turning up in SSL CONNECTION FAILURES.
	else if (err == EPIPE)
	{
		return std::string("EPIPE");
	}
	else if (err == ECONNRESET)
	{
		return std::string("ECONNRESET");
	}
	else if (err == EHOSTUNREACH)
	{
		return std::string("EHOSTUNREACH");
	}
	else if (err == EADDRNOTAVAIL)
	{
		return std::string("EADDRNOTAVAIL");
	}
	//

	return std::string("UNKNOWN ERROR CODE - ASK RS-DEVS TO ADD IT!");
}

#include <sys/types.h>
#include <ifaddrs.h>
#include <net/if.h>

bool getLocalAddresses(std::list<struct sockaddr_storage> & addrs)
{
	struct ifaddrs *ifsaddrs, *ifa;
	if(getifaddrs(&ifsaddrs) != 0)
	{
		freeifaddrs(ifsaddrs);
		return false;
	}

	addrs.clear();
	for ( ifa = ifsaddrs; ifa; ifa = ifa->ifa_next )
		if ( (ifa->ifa_flags & IFF_UP) && !(ifa->ifa_flags & IFF_LOOPBACK) )
		{
			const sockaddr_storage & ifaaddr = * (const struct sockaddr_storage *) ifa->ifa_addr;
			if (sockaddr_storage_isLinkLocal(ifaaddr)) continue;
			sockaddr_storage tmp;
			if (sockaddr_storage_copyip(tmp, ifaaddr)) addrs.push_back(tmp);
		}


	freeifaddrs(ifsaddrs);
	return !addrs.empty();
}

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else

void showSocketError(std::string &out)
{
	int err = WSAGetLastError();
	rs_sprintf_append(out, "\tSocket Error(%d) : %s\n", err, socket_errorType(err).c_str());
}


std::string socket_errorType(int err)
{
	if (err == WSAEBADF)
	{
		return std::string("WSABADF");
	}


	else if (err == WSAEINTR)
	{
		return std::string("WSAEINTR");
	}
	else if (err == WSAEACCES)
	{
		return std::string("WSAEACCES");
	}
	else if (err == WSAEFAULT)
	{
		return std::string("WSAEFAULT");
	}
	else if (err == WSAEINVAL)
	{
		return std::string("WSAEINVAL");
	}
	else if (err == WSAEMFILE)
	{
		return std::string("WSAEMFILE");
	}
	else if (err == WSAEWOULDBLOCK)
	{
		return std::string("WSAEWOULDBLOCK");
	}
	else if (err == WSAEINPROGRESS)
	{
		return std::string("WSAEINPROGRESS");
	}
	else if (err == WSAEALREADY)
	{
		return std::string("WSAEALREADY");
	}
	else if (err == WSAENOTSOCK)
	{
		return std::string("WSAENOTSOCK");
	}
	else if (err == WSAEDESTADDRREQ)
	{
		return std::string("WSAEDESTADDRREQ");
	}
	else if (err == WSAEMSGSIZE)
	{
		return std::string("WSAEMSGSIZE");
	}
	else if (err == WSAEPROTOTYPE)
	{
		return std::string("WSAEPROTOTYPE");
	}
	else if (err == WSAENOPROTOOPT)
	{
		return std::string("WSAENOPROTOOPT");
	}
	else if (err == WSAENOTSOCK)
	{
		return std::string("WSAENOTSOCK");
	}
	else if (err == WSAEISCONN)
	{
		return std::string("WSAISCONN");
	}
	else if (err == WSAECONNREFUSED)
	{
		return std::string("WSACONNREFUSED");
	}
	else if (err == WSAECONNRESET)
	{
		return std::string("WSACONNRESET");
	}
	else if (err == WSAETIMEDOUT)
	{
		return std::string("WSATIMEDOUT");
	}
	else if (err == WSAENETUNREACH)
	{
		return std::string("WSANETUNREACH");
	}
	else if (err == WSAEADDRINUSE)
	{
		return std::string("WSAADDRINUSE");
	}
	else if (err == WSAEAFNOSUPPORT)
	{
		return std::string("WSAEAFNOSUPPORT (normally UDP related!)");
	}

	return std::string("----WINDOWS OPERATING SYSTEM FAILURE----");
}

#include <iphlpapi.h>
//#include <iprtrmib.h>

// A function to determine the interfaces on your computer....
// No idea of how to do this in windows....
// see if it compiles.
bool getLocalInterfaces_ipv4(std::list<struct in_addr> &addrs)
{
	/* USE MIB IPADDR Interface */
	PMIB_IPADDRTABLE iptable =  NULL;
	DWORD dwSize = 0;

	if (GetIpAddrTable(iptable, &dwSize, 0) != ERROR_INSUFFICIENT_BUFFER)
	{
		pqioutput(PQL_ALERT, pqinetzone, "Cannot Find Windoze Interfaces!");
		exit(0);
	}

	iptable = (MIB_IPADDRTABLE *) malloc(dwSize);
	GetIpAddrTable(iptable, &dwSize, 0);

	struct in_addr addr;

	for (unsigned int i = 0; i < iptable -> dwNumEntries; i++)
	{
		MIB_IPADDRROW &ipaddr = iptable->table[i];

		std::string out;

		addr.s_addr = ipaddr.dwAddr;
		rs_sprintf(out, "Iface(%ld) => %s\n", ipaddr.dwIndex, rs_inet_ntoa(addr).c_str());

#if __MINGW_MAJOR_VERSION <= 3 && !defined(__MINGW64_VERSION_MAJOR)
		unsigned short wType = ipaddr.unused2; // should be wType
#else
		unsigned short wType = ipaddr.wType;
#endif
		if (wType & MIB_IPADDR_DISCONNECTED)
		{
			pqioutput(PQL_DEBUG_BASIC, pqinetzone, "Interface disconnected, " + out);
			continue;
		}

		if (wType & MIB_IPADDR_DELETED)
		{
			pqioutput(PQL_DEBUG_BASIC, pqinetzone, "Interface deleted, " + out);
			continue;
		}

		pqioutput(PQL_DEBUG_BASIC, pqinetzone, out);
		addrs.push_back(addr);
	}

	free (iptable);

	return (addrs.size() > 0);
}

// temporary fix
// wrapper for getLocalInterfaces_ipv4
bool getLocalAddresses(std::list<struct sockaddr_storage> &addrs)
{
	addrs.clear();
	std::list<struct in_addr> addrs2;
	std::list<struct in_addr>::iterator addrs2_it;
	getLocalInterfaces_ipv4(addrs2);
	for(addrs2_it = addrs2.begin(); addrs2_it != addrs2.end(); ++addrs2_it) {
		struct sockaddr_storage *ss = new sockaddr_storage;
		sockaddr_in *ss_ptr = (sockaddr_in *) ss;
		ss_ptr->sin_family = AF_INET;
		ss_ptr->sin_addr = *addrs2_it;
		addrs.push_back(*ss);
	}
	return !addrs.empty();
}

// implement the improved unix inet address fn.
// using old one.
int inet_aton(const char *name, struct in_addr *addr)
{
	return (((*addr).s_addr = inet_addr(name)) != INADDR_NONE);
}

int inet_pton(int af, const char *src, void *dst)
{
	struct sockaddr_storage ss;
	int size = sizeof(ss);
	char src_copy[INET6_ADDRSTRLEN+1];

	ZeroMemory(&ss, sizeof(ss));
	/* stupid non-const API */
	strncpy (src_copy, src, INET6_ADDRSTRLEN+1);
	src_copy[INET6_ADDRSTRLEN] = 0;

	if (WSAStringToAddressA(src_copy, af, NULL, (struct sockaddr *)&ss, &size) == 0) {
		switch(af) {
		case AF_INET:
			*(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
			return 1;
		case AF_INET6:
			*(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
			return 1;
		}
	}
	return 0;
}

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
	struct sockaddr_storage ss;
	unsigned long s = size;

	ZeroMemory(&ss, sizeof(ss));
	ss.ss_family = af;

	switch(af) {
	case AF_INET:
		((struct sockaddr_in *)&ss)->sin_addr = *(struct in_addr *)src;
		break;
	case AF_INET6:
		((struct sockaddr_in6 *)&ss)->sin6_addr = *(struct in6_addr *)src;
		break;
	default:
		return NULL;
	}
	/* cannot direclty use &size because of strict aliasing rules */
	int ret = WSAAddressToStringA((struct sockaddr *)&ss, sizeof(ss), NULL, dst, &s);
	return (ret == 0) ? dst : NULL;
}

#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/


/*************************************************************
 * Socket Library Wrapper Functions 
 * to get over the crapness of the windows.
 *
 */

int unix_close(int fd)
{
	int ret;
/******************* WINDOWS SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX
	ret = close(fd);
#else

#ifdef NET_DEBUG
	std::cerr << "unix_close()" << std::endl;
#endif
	ret = closesocket(fd);
	/* translate error */
#endif
/******************* WINDOWS SPECIFIC PART ******************/
	return ret;
}

int unix_socket(int domain, int type, int protocol)
{
	int osock = socket(domain, type, protocol);

#ifdef WINDOWS_SYS
#ifdef NET_DEBUG
	std::cerr << "unix_socket()" << std::endl;
#endif // NET_DEBUG

	if ((unsigned) osock == INVALID_SOCKET)
	{
		// Invalidate socket Unix style.
		osock = -1;
		errno = WinToUnixError(WSAGetLastError());
	}
#endif // WINDOWS_SYS

	return osock;
}

int unix_fcntl_nonblock(int fd)
{
	int ret;

#ifdef WINDOWS_SYS
	unsigned long int on = 1;
	ret = ioctlsocket(fd, FIONBIO, &on);

	if (ret != 0)
	{
		ret = -1;
		errno = WinToUnixError(WSAGetLastError());
	}
#else // ! WINDOWS_SYS => is UNIX !
	ret = fcntl(fd, F_SETFL, O_NONBLOCK);
#endif // WINDOWS_SYS

#ifdef NET_DEBUG
	std::cerr << "unix_fcntl_nonblock():" << ret << " errno:" << errno << std::endl;
#endif // NET_DEBUG

	return ret;
}

int unix_connect(int fd, const sockaddr_storage *serv_addr)
{
#ifdef NET_DEBUG
	std::cerr << "unix_connect()";
	std::cerr << std::endl;
#endif // NET_DEBUG

	int ret = connect(fd, (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in6));

#ifdef WINDOWS_SYS
#ifdef NET_DEBUG
	std::cerr << "unix_connect()" << std::endl;
#endif // NET_DEBUG
	if (ret != 0)
	{
		errno = WinToUnixError(WSAGetLastError());
		ret = -1;
	}
#endif // WINDOWS_SYS

	return ret;
}

int unix_getsockopt_error(int sockfd, int *err)
{
	int ret;
	*err = 1;
/******************* WINDOWS SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX
	socklen_t optlen = 4;
	ret=getsockopt(sockfd, SOL_SOCKET, SO_ERROR, err, &optlen);
#else // WINDOWS_SYS
	int optlen = 4;
	ret=getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char *) err, &optlen);
	/* translate */
#ifdef NET_DEBUG
	std::cerr << "unix_getsockopt_error() returned: " << (int) err << std::endl;
#endif
	if (*err != 0)
	{
		*err = WinToUnixError(*err);
	}
#endif
/******************* WINDOWS SPECIFIC PART ******************/
	return ret;
}

/******************* WINDOWS SPECIFIC PART ******************/
#ifdef WINDOWS_SYS // ie WINDOWS.

int	WinToUnixError(int error)
{
#ifdef NET_DEBUG
	std::cerr << "WinToUnixError(" << error << ")" << std::endl;
#endif
	switch(error)
	{
		case WSAEINPROGRESS:
			return EINPROGRESS;
			break;
		case WSAEWOULDBLOCK:
			return EINPROGRESS;
			break;
		case WSAENETUNREACH:
			return ENETUNREACH;
			break;
		case WSAETIMEDOUT:
			return ETIMEDOUT;
			break;
		case WSAEHOSTDOWN:
			return EHOSTDOWN;
			break;
		case WSAECONNREFUSED:
			return ECONNREFUSED;
			break;
		case WSAECONNRESET:
			return ECONNRESET;
			break;
		default:
#ifdef NET_DEBUG
			std::cerr << "WinToUnixError(" << error << ") Code Unknown!";
			std::cerr << std::endl;
#endif
			break;
	}
	return ECONNREFUSED; /* sensible default? */
}

#endif
/******************* WINDOWS SPECIFIC PART ******************/
