/*******************************************************************************
 * libretroshare/src/pqi: pqinetwork.cc                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2006  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2015-2021  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>       *
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
#ifdef WINDOWS_SYS
#	include "util/rswin.h"
#	include "util/rsmemory.h"
#	include <ws2tcpip.h>
#endif // WINDOWS_SYS

#ifdef __ANDROID__
#	include <android/api-level.h>
#endif // def __ANDROID__

#include <cerrno>
#include <iostream>
#include <cstdio>
#include <unistd.h>

#include "pqi/pqinetwork.h"
#include "util/rsnet.h"
#include "util/rsdebug.h"
#include "util/rsstring.h"
#include "util/rsnet.h"
#include "util/stacktrace.h"

static struct RsLog::logInfo pqinetzoneInfo = {RsLog::Default, "pqinet"};
#define pqinetzone &pqinetzoneInfo

/*****
 * #define NET_DEBUG 1
 ****/

#ifdef WINDOWS_SYS   /* Windows - define errno */

int errno;

#else /* Windows - define errno */

#include <netdb.h>

#endif               

#ifdef __HAIKU__
 #include <sys/sockio.h>
 #define IFF_RUNNING 0x0001
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

// implement the improved unix inet address fn.
// using old one.
int inet_aton(const char *name, struct in_addr *addr)
{
	return (((*addr).s_addr = inet_addr(name)) != INADDR_NONE);
}

#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/


#include <sys/types.h>
#ifdef WINDOWS_SYS
#	include <winsock2.h>
#	include <iphlpapi.h>
#	pragma comment(lib, "IPHLPAPI.lib")
#elif defined(__ANDROID__) && __ANDROID_API__ < 24
#	include <string>
#	include <QString>
#	include <QHostAddress>
#	include <QNetworkInterface>
#else // not __ANDROID__ nor WINDOWS => Linux and other unixes
#	include <ifaddrs.h>
#	include <net/if.h>
#endif // WINDOWS_SYS

bool getLocalAddresses(std::vector<sockaddr_storage>& addrs)
{
	addrs.clear();

#ifdef WINDOWS_SYS
	// Seems strange to me but M$ documentation suggests to allocate this way...
	DWORD bf_size = 16000;
	IP_ADAPTER_ADDRESSES* adapter_addresses = (IP_ADAPTER_ADDRESSES*) rs_malloc(bf_size);
    
    	if(adapter_addresses == NULL)
            return false ;
        
	DWORD error = GetAdaptersAddresses(AF_UNSPEC,
									   GAA_FLAG_SKIP_MULTICAST |
									   GAA_FLAG_SKIP_DNS_SERVER |
									   GAA_FLAG_SKIP_FRIENDLY_NAME,
									   NULL,
									   adapter_addresses,
									   &bf_size);
	if (error != ERROR_SUCCESS)
	{
	   std::cerr << "FATAL ERROR: getLocalAddresses failed!" << std::endl;
	   return false ;
	}

	IP_ADAPTER_ADDRESSES* adapter(NULL);
	for(adapter = adapter_addresses; NULL != adapter; adapter = adapter->Next)
	{
		IP_ADAPTER_UNICAST_ADDRESS* address;
		for ( address = adapter->FirstUnicastAddress; address; address = address->Next)
		{
			sockaddr_storage tmp;
			sockaddr_storage_clear(tmp);
			if (sockaddr_storage_copyip(tmp, * reinterpret_cast<sockaddr_storage*>(address->Address.lpSockaddr)))
				addrs.push_back(tmp);
		}
	}
	free(adapter_addresses);
#elif defined(__ANDROID__) && __ANDROID_API__ < 24
	for(auto& qAddr: QNetworkInterface::allAddresses())
	{
		sockaddr_storage tmpAddr;
		sockaddr_storage_clear(tmpAddr);
		if(sockaddr_storage_ipv4_aton(tmpAddr, qAddr.toString().toStdString().c_str()))
			addrs.push_back(tmpAddr);
	}
#else // not  WINDOWS_SYS not ANDROID => Linux and other unixes
	struct ifaddrs *ifsaddrs, *ifa;
	if(getifaddrs(&ifsaddrs) != 0) 
	{
		std::cerr << __PRETTY_FUNCTION__ << " FATAL ERROR: " << errno << " "
		          << strerror(errno) << std::endl;
		print_stacktrace();
		return false;
	}
	for ( ifa = ifsaddrs; ifa; ifa = ifa->ifa_next )
		if ( ifa->ifa_addr && (ifa->ifa_flags & IFF_UP) )
		{
			sockaddr_storage tmp;
			sockaddr_storage_clear(tmp);
			if (sockaddr_storage_copyip(tmp, *reinterpret_cast<sockaddr_storage*>(ifa->ifa_addr)))
				addrs.push_back(tmp);
		}
	freeifaddrs(ifsaddrs);
#endif // WINDOWS_SYS

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

/******************* OS SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX
	int flags = fcntl(fd, F_GETFL);
	ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);

#ifdef NET_DEBUG
	std::cerr << "unix_fcntl_nonblock():" << ret << " errno:" << errno << std::endl;
#endif

#else
	unsigned long int on = 1;
	ret = ioctlsocket(fd, FIONBIO, &on);

#ifdef NET_DEBUG
	std::cerr << "unix_fcntl_nonblock()" << std::endl;
#endif
	if (ret != 0)
	{
		/* store unix-style error
		 */
		ret = -1;
		errno = WinToUnixError(WSAGetLastError());
	}
#endif
/******************* WINDOWS SPECIFIC PART ******************/
	return ret;
}


int unix_connect(int fd, const sockaddr_storage &serv_addr)
{
#ifdef NET_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif

	int ret = connect( fd, (const struct sockaddr *) &serv_addr,
	                   sizeof(struct sockaddr_in6) );

/******************* WINDOWS SPECIFIC PART ******************/
#ifdef WINDOWS_SYS // WINDOWS
	if (ret != 0)
	{
		errno = WinToUnixError(WSAGetLastError());
		ret = -1;
	}
#endif
/******************* WINDOWS SPECIFIC PART ******************/
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
