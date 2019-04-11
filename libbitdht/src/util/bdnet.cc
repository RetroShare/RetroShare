/*******************************************************************************
 * util/bdnet.cc                                                               *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright 2010 by Robert Fernie <bitdht@lunamutt.com>                       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "bdnet.h"
#include "bdstring.h"

#include <iostream>
#include <stdlib.h>
#include <string.h>

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#if defined(_WIN32) || defined(__MINGW32__)

//#define BDNET_DEBUG

/* error handling */
int bdnet_int_errno;

int bdnet_errno() 
{
	return bdnet_int_errno;
}

int bdnet_init() 
{ 
	std::cerr << "bdnet_init()" << std::endl;
	bdnet_int_errno = 0;

        // Windows Networking Init.
        WORD wVerReq = MAKEWORD(2,2);
        WSADATA wsaData;

        if (0 != WSAStartup(wVerReq, &wsaData))
        {
                std::cerr << "Failed to Startup Windows Networking";
                std::cerr << std::endl;
        }
        else
        {
                std::cerr << "Started Windows Networking";
                std::cerr << std::endl;
        }

	return 0; 
}

/* check if we can modify the TTL on a UDP packet */
int bdnet_checkTTL(int fd) 
{
	std::cerr << "bdnet_checkTTL()" << std::endl;
	int optlen = 4;
	char optval[optlen];

	int ret = getsockopt(fd, IPPROTO_IP, IP_TTL, optval, &optlen);
	//int ret = getsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, optval, &optlen);
	if (ret == SOCKET_ERROR)
	{
		ret = -1;
		bdnet_int_errno = bdnet_w2u_errno(WSAGetLastError());
		std::cerr << "bdnet_checkTTL() Failed!";
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "bdnet_checkTTL() :";
		std::cerr << (int) optval[0] << ":";
		std::cerr << (int) optval[1] << ":";
		std::cerr << (int) optval[2] << ":";
		std::cerr << (int) optval[3] << ": RET: ";
		std::cerr << ret << ":";
		std::cerr << std::endl;
	}
	return ret;
}

int bdnet_close(int fd) 
{ 
	std::cerr << "bdnet_close()" << std::endl;
	return closesocket(fd); 
}

int bdnet_socket(int domain, int type, int protocol)
{
        int osock = socket(domain, type, protocol);
	std::cerr << "bdnet_socket()" << std::endl;

	if ((unsigned) osock == INVALID_SOCKET)
	{
		// Invalidate socket Unix style.
		osock = -1;
	        bdnet_int_errno = bdnet_w2u_errno(WSAGetLastError());
	}
	bdnet_checkTTL(osock);
	return osock;
}

int bdnet_bind(int  sockfd,  const  struct  sockaddr  *my_addr,  socklen_t addrlen)
{
	std::cerr << "bdnet_bind()" << std::endl;
	int ret = bind(sockfd,my_addr,addrlen);
	if (ret != 0)
	{
		/* store unix-style error
		 */

		ret = -1;
		bdnet_int_errno = bdnet_w2u_errno(WSAGetLastError());
	}
	return ret;
}

int bdnet_fcntl(int fd, int cmd, long arg)
{
        int ret;
	
	unsigned long int on = 1;
	std::cerr << "bdnet_fcntl()" << std::endl;

	/* can only do NONBLOCK at the moment */
	if ((cmd != F_SETFL) || (arg != O_NONBLOCK))
	{
		std::cerr << "bdnet_fcntl() limited to fcntl(fd, F_SETFL, O_NONBLOCK)";
		std::cerr << std::endl;
		bdnet_int_errno =  EOPNOTSUPP;
		return -1;
	}

	ret = ioctlsocket(fd, FIONBIO, &on);

	if (ret != 0)
	{
		/* store unix-style error
		 */

		ret = -1;
		bdnet_int_errno = bdnet_w2u_errno(WSAGetLastError());
	}
	return ret;
}

int bdnet_setsockopt(int s, int level, int optname, 
				const void *optval, socklen_t optlen)
{
	std::cerr << "bdnet_setsockopt() val:" << *((int *) optval) << std::endl;
	std::cerr << "bdnet_setsockopt() len:" << optlen << std::endl;
	if ((level != IPPROTO_IP) || (optname != IP_TTL))
	{
		std::cerr << "bdnet_setsockopt() limited to ";
		std::cerr << "setsockopt(fd, IPPROTO_IP, IP_TTL, ....)";
		std::cerr << std::endl;
		bdnet_int_errno =  EOPNOTSUPP;
		return -1;
	}

	int ret = setsockopt(s, level, optname, (const char *) optval, optlen);
	//int ret = setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (const char *) optval, optlen);
	if (ret == SOCKET_ERROR)
	{
		ret = -1;
		bdnet_int_errno = bdnet_w2u_errno(WSAGetLastError());
	}
	bdnet_checkTTL(s);
	return ret;
}

ssize_t bdnet_recvfrom(int s, void *buf, size_t len, int flags,
                              struct sockaddr *from, socklen_t *fromlen)
{
#ifdef BDNET_DEBUG
	std::cerr << "bdnet_recvfrom()" << std::endl;
#endif
	int ret = recvfrom(s, (char *) buf, len, flags, from, fromlen);
	if (ret == SOCKET_ERROR)
	{
		ret = -1;
		bdnet_int_errno = bdnet_w2u_errno(WSAGetLastError());
	}
	return ret;
}

ssize_t bdnet_sendto(int s, const void *buf, size_t len, int flags, 
 				const struct sockaddr *to, socklen_t tolen)
{
#ifdef BDNET_DEBUG
	std::cerr << "bdnet_sendto()" << std::endl;
#endif
	int ret = sendto(s, (const char *) buf, len, flags, to, tolen);
	if (ret == SOCKET_ERROR)
	{
		ret = -1;
		bdnet_int_errno = bdnet_w2u_errno(WSAGetLastError());
	}
	return ret;
}

int bdnet_w2u_errno(int err)
{
	/* switch */
	std::cerr << "tou_net_w2u_errno(" << err << ")" << std::endl;
	switch(err)
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
		case WSAEADDRINUSE:
			return EADDRINUSE;
			break;
		case WSAEUSERS:
			return EUSERS;
			break;
		/* This one is returned for UDP recvfrom, when nothing there
		 * but not a real error... translate into EINPROGRESS
		 */
		case WSAECONNRESET:
			std::cerr << "tou_net_w2u_errno(" << err << ")";
			std::cerr << " = WSAECONNRESET ---> EINPROGRESS";
	 		std::cerr << std::endl;
			return EINPROGRESS;
			break;
		/***
		 *
		case WSAECONNRESET:
			return ECONNRESET;
			break;
		 *
		 ***/
	
		case WSANOTINITIALISED:	
			std::cerr << "tou_net_w2u_errno(" << err << ") WSANOTINITIALISED. Fix Your Code!";
			std::cerr << std::endl;
			break;
		default:
			std::cerr << "tou_net_w2u_errno(" << err << ") Unknown";
			std::cerr << std::endl;
			break;
	}

	return ECONNREFUSED; /* sensible default? */
}

int bdnet_inet_aton(const char *name, struct in_addr *addr)
{
        return (((*addr).s_addr = inet_addr(name)) != INADDR_NONE);
}

#ifndef __MINGW64_VERSION_MAJOR
int sleep(unsigned int sec)
{ 
	Sleep(sec * 1000); 
	return 0;
}
#endif

int usleep(unsigned int usec)
{ 
	Sleep(usec / 1000); 
	return 0;
}

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else // UNIX

/* Unix Version is easy -> just call the unix fn
 */

#include <unistd.h> /* for close definition */

/* the universal interface */
int bdnet_init() { return 0; }
int bdnet_errno() { return errno; }


/* check if we can modify the TTL on a UDP packet */
int bdnet_checkTTL(int /*fd*/) { return 1;}


int bdnet_inet_aton(const char *name, struct in_addr *addr)
{
        return inet_aton(name, addr);
}

int bdnet_close(int fd) { return close(fd); }

int bdnet_socket(int domain, int type, int protocol)
{
	return socket(domain, type, protocol);
}

int bdnet_bind(int  sockfd,  const  struct  sockaddr  *my_addr,  socklen_t addrlen)
{
	return bind(sockfd,my_addr,addrlen);
}

int bdnet_fcntl(int fd, int cmd, long arg)
{
	return fcntl(fd, cmd, arg);
}

int bdnet_setsockopt(int s, int level, int optname, 
				const void *optval, socklen_t optlen)
{
	return setsockopt(s, level, optname,  optval, optlen);
}

ssize_t bdnet_recvfrom(int s, void *buf, size_t len, int flags,
                              struct sockaddr *from, socklen_t *fromlen)
{
	return recvfrom(s, buf, len, flags, from, fromlen);
}

ssize_t bdnet_sendto(int s, const void *buf, size_t len, int flags, 
 				const struct sockaddr *to, socklen_t tolen)
{
	return sendto(s, buf, len, flags, to, tolen);
}


#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/


void    bdsockaddr_clear(struct sockaddr_in *addr)
{
	memset(addr, 0, sizeof(*addr));
}

/* thread-safe version of inet_ntoa */

std::string bdnet_inet_ntoa(struct in_addr in)
{
	std::string str;
	uint8_t *bytes = (uint8_t *) &(in.s_addr);
	bd_sprintf(str, "%u.%u.%u.%u", (int) bytes[0], (int) bytes[1], (int) bytes[2], (int) bytes[3]);
	return str;
}
