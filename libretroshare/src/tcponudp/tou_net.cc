/*
 * "$Id: tou_net.cc,v 1.3 2007-02-18 21:46:50 rmf24 Exp $"
 *
 * TCP-on-UDP (tou) network interface for RetroShare.
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




#include "tou_net.h"


/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS

/* Unix Version is easy -> just call the unix fn
 */

#include <unistd.h> /* for close definition */

/* the universal interface */
int tounet_init() { return 0; }
int tounet_errno() { return errno; }


/* check if we can modify the TTL on a UDP packet */
int tounet_checkTTL(int fd) { return 1;}


int tounet_inet_aton(const char *name, struct in_addr *addr)
{
        return inet_aton(name, addr);
}

int tounet_close(int fd) { return close(fd); }

int tounet_socket(int domain, int type, int protocol)
{
	return socket(domain, type, protocol);
}

int tounet_bind(int  sockfd,  const  struct  sockaddr  *my_addr,  socklen_t addrlen)
{
	return bind(sockfd,my_addr,addrlen);
}

int tounet_fcntl(int fd, int cmd, long arg)
{
	return fcntl(fd, cmd, arg);
}

int tounet_setsockopt(int s, int level, int optname, 
				const void *optval, socklen_t optlen)
{
	return setsockopt(s, level, optname,  optval, optlen);
}

ssize_t tounet_recvfrom(int s, void *buf, size_t len, int flags,
                              struct sockaddr *from, socklen_t *fromlen)
{
	return recvfrom(s, buf, len, flags, from, fromlen);
}

ssize_t tounet_sendto(int s, const void *buf, size_t len, int flags, 
 				const struct sockaddr *to, socklen_t tolen)
{
	return sendto(s, buf, len, flags, to, tolen);
}


/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else /* WINDOWS OS */

#include <iostream>

/* error handling */
int tounet_int_errno;

int tounet_errno() 
{
	return tounet_int_errno;
}

int tounet_init() 
{ 
	std::cerr << "tounet_init()" << std::endl;
	tounet_int_errno = 0;

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
int tounet_checkTTL(int fd) 
{
	std::cerr << "tounet_checkTTL()" << std::endl;
	int optlen = 4;
	char optval[optlen];

	int ret = getsockopt(fd, IPPROTO_IP, IP_TTL, optval, &optlen);
	//int ret = getsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, optval, &optlen);
	if (ret == SOCKET_ERROR)
	{
		ret = -1;
		tounet_int_errno = tounet_w2u_errno(WSAGetLastError());
		std::cerr << "tounet_checkTTL() Failed!";
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "tounet_checkTTL() :";
		std::cerr << (int) optval[0] << ":";
		std::cerr << (int) optval[1] << ":";
		std::cerr << (int) optval[2] << ":";
		std::cerr << (int) optval[3] << ": RET: ";
		std::cerr << ret << ":";
		std::cerr << std::endl;
	}
	return ret;
}

int tounet_close(int fd) 
{ 
	std::cerr << "tounet_close()" << std::endl;
	return closesocket(fd); 
}

int tounet_socket(int domain, int type, int protocol)
{
        int osock = socket(domain, type, protocol);
	std::cerr << "tounet_socket()" << std::endl;

	if ((unsigned) osock == INVALID_SOCKET)
	{
		// Invalidate socket Unix style.
		osock = -1;
	        tounet_int_errno = tounet_w2u_errno(WSAGetLastError());
	}
	tounet_checkTTL(osock);
	return osock;
}

int tounet_bind(int  sockfd,  const  struct  sockaddr  *my_addr,  socklen_t addrlen)
{
	std::cerr << "tounet_bind()" << std::endl;
	int ret = bind(sockfd,my_addr,addrlen);
	if (ret != 0)
	{
		/* store unix-style error
		 */

		ret = -1;
		tounet_int_errno = tounet_w2u_errno(WSAGetLastError());
	}
	return ret;
}

int tounet_fcntl(int fd, int cmd, long arg)
{
        int ret;
	
	unsigned long int on = 1;
	std::cerr << "tounet_fcntl()" << std::endl;

	/* can only do NONBLOCK at the moment */
	if ((cmd != F_SETFL) || (arg != O_NONBLOCK))
	{
		std::cerr << "tounet_fcntl() limited to fcntl(fd, F_SETFL, O_NONBLOCK)";
		std::cerr << std::endl;
		tounet_int_errno =  EOPNOTSUPP;
		return -1;
	}

	ret = ioctlsocket(fd, FIONBIO, &on);

	if (ret != 0)
	{
		/* store unix-style error
		 */

		ret = -1;
		tounet_int_errno = tounet_w2u_errno(WSAGetLastError());
	}
	return ret;
}

int tounet_setsockopt(int s, int level, int optname, 
				const void *optval, socklen_t optlen)
{
	std::cerr << "tounet_setsockopt() val:" << *((int *) optval) << std::endl;
	std::cerr << "tounet_setsockopt() len:" << optlen << std::endl;
	if ((level != IPPROTO_IP) || (optname != IP_TTL))
	{
		std::cerr << "tounet_setsockopt() limited to ";
		std::cerr << "setsockopt(fd, IPPROTO_IP, IP_TTL, ....)";
		std::cerr << std::endl;
		tounet_int_errno =  EOPNOTSUPP;
		return -1;
	}

	int ret = setsockopt(s, level, optname, (const char *) optval, optlen);
	//int ret = setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (const char *) optval, optlen);
	if (ret == SOCKET_ERROR)
	{
		ret = -1;
		tounet_int_errno = tounet_w2u_errno(WSAGetLastError());
	}
	tounet_checkTTL(s);
	return ret;
}

ssize_t tounet_recvfrom(int s, void *buf, size_t len, int flags,
                              struct sockaddr *from, socklen_t *fromlen)
{
	std::cerr << "tounet_recvfrom()" << std::endl;
	int ret = recvfrom(s, (char *) buf, len, flags, from, fromlen);
	if (ret == SOCKET_ERROR)
	{
		ret = -1;
		tounet_int_errno = tounet_w2u_errno(WSAGetLastError());
	}
	return ret;
}

ssize_t tounet_sendto(int s, const void *buf, size_t len, int flags, 
 				const struct sockaddr *to, socklen_t tolen)
{
	std::cerr << "tounet_sendto()" << std::endl;
	int ret = sendto(s, (const char *) buf, len, flags, to, tolen);
	if (ret == SOCKET_ERROR)
	{
		ret = -1;
		tounet_int_errno = tounet_w2u_errno(WSAGetLastError());
	}
	return ret;
}

int tounet_w2u_errno(int err)
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
		
		default:
			std::cerr << "tou_net_w2u_errno(" << err << ") Unknown";
			std::cerr << std::endl;
			break;
	}

	return ECONNREFUSED; /* sensible default? */
}

int tounet_inet_aton(const char *name, struct in_addr *addr)
{
        return (((*addr).s_addr = inet_addr(name)) != INADDR_NONE);
}



void sleep(int sec) 
{ 
	Sleep(sec * 1000); 
}

void usleep(int usec) 
{ 
	Sleep(usec / 1000); 
}

#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

