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




#include "pqi/pqinetwork.h"
#include "util/rsnet.h"

#include <errno.h>
#include <iostream>
#include <stdio.h>

#include "util/rsdebug.h"
#include <sstream>
#include <iomanip>
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

std::ostream &showSocketError(std::ostream &out)
{
	int err = errno;
	out << "\tSocket Error(" << err << ") : ";
	out << socket_errorType(err) << std::endl;
	return out;
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

	return std::string("UNKNOWN ERROR CODE");
}

#include <net/if.h> 
#include <sys/ioctl.h> 

bool getLocalInterfaces(std::list<struct in_addr> &addrs)
{
	int sock = 0;
	struct ifreq ifreq;

	struct if_nameindex *iflist = if_nameindex();
	struct if_nameindex *ifptr = iflist;

	//need a socket for ioctl()
	if( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		pqioutput(PQL_ALERT, pqinetzone, 
			"Cannot Determine Local Addresses!");
		return false;
	}

	if (!ifptr)
	{
		pqioutput(PQL_ALERT, pqinetzone, 
			"getLocalInterfaces(): ERROR if_nameindex == NULL");
		return false;
	}

	// loop through the interfaces.
	for(; ifptr->if_index != 0; ifptr++)
	{
		//copy in the interface name to look up address of
		strncpy(ifreq.ifr_name, ifptr->if_name, IF_NAMESIZE);

		if(ioctl(sock, SIOCGIFADDR, &ifreq) != 0)
		{
			std::ostringstream out;
			out << "Cannot Determine Address for Iface: ";
			out << ifptr -> if_name << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqinetzone, out.str());
		}
		else
		{
			struct sockaddr_in *aptr = 
				(struct sockaddr_in *) &ifreq.ifr_addr;
			std::string astr =rs_inet_ntoa(aptr -> sin_addr);

			std::ostringstream out;
			out << "Iface: ";
			out << ifptr -> if_name << std::endl;
			out << " Address: " << astr;
			out << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqinetzone, out.str());

			// Now check wether the interface is up and running. If not, we don't use it!!
			//
			if(ioctl(sock,SIOCGIFFLAGS,&ifreq) != 0)
			{
				std::cerr << "Could not get flags from interface " << ifptr -> if_name << std::endl ;
				continue ;
			}
#ifdef NET_DEBUG
			std::cout << out.str() ;
			std::cout << "flags = " << ifreq.ifr_flags << std::endl ;
#endif
			if((ifreq.ifr_flags & IFF_UP) == 0) continue ;
			if((ifreq.ifr_flags & IFF_RUNNING) == 0) continue ;

			addrs.push_back(aptr->sin_addr);
		}
	}
	// free socket -> or else run out of fds.
	close(sock);

	if_freenameindex(iflist);
	return (addrs.size() > 0);
}

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else

std::ostream &showSocketError(std::ostream &out)
{
	int err = WSAGetLastError();
	out << "\tSocket Error(" << err << ") : ";
	out << socket_errorType(err) << std::endl;
	return out;
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
bool getLocalInterfaces(std::list<struct in_addr> &addrs)
{

	/* USE MIB IPADDR Interface */
	PMIB_IPADDRTABLE iptable =  NULL;
	DWORD dwSize = 0;

	if (GetIpAddrTable(iptable, &dwSize, 0) != 
				ERROR_INSUFFICIENT_BUFFER)
	{
		pqioutput(PQL_ALERT, pqinetzone, 
			"Cannot Find Windoze Interfaces!");
		exit(0);
	}

	iptable = (MIB_IPADDRTABLE *) malloc(dwSize);
	GetIpAddrTable(iptable, &dwSize, 0);

	struct in_addr addr;

	for(unsigned int i = 0; i < iptable -> dwNumEntries; i++)
	{
		std::ostringstream out;

		out << "Iface(" << iptable->table[i].dwIndex << ") ";
		addr.s_addr = iptable->table[i].dwAddr;
		out << " => " << rs_inet_ntoa(addr);
		out << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqinetzone, out.str());

		addrs.push_back(addr);
	}

	free (iptable);

	return (addrs.size() > 0);
}

// implement the improved unix inet address fn.
// using old one.
int inet_aton(const char *name, struct in_addr *addr)
{
	return (((*addr).s_addr = inet_addr(name)) != INADDR_NONE);
}


// This returns in Net Byte Order. 
// NB: Linux man page claims it is in Host Byte order, but
// this is blatantly wrong!..... (for Debian anyway)
// Making this consistent with the Actual behavior (rather than documented).
in_addr_t inet_netof(struct in_addr addr)
{
	return pqi_inet_netof(addr);
}

// This returns in Host Byte Order. (as the man page says)
// Again, to be consistent with Linux.
in_addr_t inet_network(const char *inet_name)
{
	struct in_addr addr;
	if (inet_aton(inet_name, &addr))
	{
#ifdef NET_DEBUG
//		std::cerr << "inet_network(" << inet_name << ") : ";
//		std::cerr << rs_inet_ntoa(addr) << std::endl;
#endif
		return ntohl(inet_netof(addr));
	}
	return 0xffffffff;
	//return -1;
}


#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

#include <iostream>


// This returns in Net Byte Order. 
// NB: Linux man page claims it is in Host Byte order, but
// this is blatantly wrong!..... (for Debian anyway)
// Making this consistent with the Actual behavior (rather than documented).
in_addr_t pqi_inet_netof(struct in_addr addr)
{
	// decide if A class address.
	unsigned long haddr = ntohl(addr.s_addr);
	unsigned long abit = haddr & 0xff000000UL;
	unsigned long bbit = haddr & 0xffff0000UL;
	unsigned long cbit = haddr & 0xffffff00UL;

#ifdef NET_DEBUG
	std::cerr << "inet_netof(" << rs_inet_ntoa(addr) << ") ";
#endif

	if (!((haddr >> 31) | 0x0UL)) // MSB = 0
	{
#ifdef NET_DEBUG
		std::cerr << " Type A " << std::endl;
		std::cerr << "\tShifted(31): " << (haddr >> 31);
		std::cerr << " Xord(0x0UL): " <<
			!((haddr >> 31) | 0x0UL) << std::endl;
#endif

		return htonl(abit);
	}
	else if (!((haddr >> 30) ^ 0x2UL)) // 2MSBs = 10
	{
#ifdef NET_DEBUG
		std::cerr << " Type B " << std::endl;
		std::cerr << "\tShifted(30): " << (haddr >> 30);
		std::cerr << " Xord(0x2UL): " <<
			!((haddr >> 30) | 0x2UL) << std::endl;
#endif

		return htonl(bbit);
	}
	else if (!((haddr >> 29) ^ 0x6UL)) // 3MSBs = 110
	{
#ifdef NET_DEBUG
		std::cerr << " Type C " << std::endl;
		std::cerr << "\tShifted(29): " << (haddr >> 29);
		std::cerr << " Xord(0x6UL): " <<
			!((haddr >> 29) | 0x6UL) << std::endl;
#endif

		return htonl(cbit);
	}
	else if (!((haddr >> 28) ^ 0xeUL)) // 4MSBs = 1110
	{
#ifdef NET_DEBUG
		std::cerr << " Type Multicast " << std::endl;
		std::cerr << "\tShifted(28): " << (haddr >> 28);
		std::cerr << " Xord(0xeUL): " <<
			!((haddr >> 29) | 0xeUL) << std::endl;
#endif

		return addr.s_addr; // return full address.
	}
	else if (!((haddr >> 27) ^ 0x1eUL)) // 5MSBs = 11110
	{
#ifdef NET_DEBUG
		std::cerr << " Type Reserved " << std::endl;
		std::cerr << "\tShifted(27): " << (haddr >> 27);
		std::cerr << " Xord(0x1eUL): " <<
			!((haddr >> 27) | 0x1eUL) << std::endl;
#endif

		return addr.s_addr; // return full address.
	}
	return htonl(abit);
}

int sockaddr_cmp(struct sockaddr_in &addr1, struct sockaddr_in &addr2 )
{
        if (addr1.sin_family != addr2.sin_family)
		return addr1.sin_family - addr2.sin_family;
        if (addr1.sin_addr.s_addr != addr2.sin_addr.s_addr)
        	return (addr1.sin_addr.s_addr - addr2.sin_addr.s_addr);
	if (addr1.sin_port != addr2.sin_port)
		return (addr1.sin_port - addr2.sin_port);
	return 0;
}

int inaddr_cmp(struct sockaddr_in addr1, struct sockaddr_in addr2 )
{
#ifdef NET_DEBUG
	std::ostringstream out;
	out << "inaddr_cmp(" << rs_inet_ntoa(addr1.sin_addr);
	out << "-" << addr1.sin_addr.s_addr;
	out << "," << rs_inet_ntoa(addr2.sin_addr);
	out << "-" << addr2.sin_addr.s_addr << ")" << std::endl;
	pqioutput(PQL_DEBUG_BASIC, pqinetzone, out.str());
#endif


	if (addr1.sin_addr.s_addr == addr2.sin_addr.s_addr)
	{
		return 0;
	}
	if (addr1.sin_addr.s_addr < addr2.sin_addr.s_addr)
		return -1;
	return 1;
}

int inaddr_cmp(struct sockaddr_in addr1, unsigned long addr2)
{
#ifdef NET_DEBUG
	struct in_addr inaddr_tmp;
	inaddr_tmp.s_addr = addr2;

	std::ostringstream out;
	out << "inaddr_cmp2(" << rs_inet_ntoa(addr1.sin_addr);
	out << " vs " << rs_inet_ntoa(inaddr_tmp);
	out << " /or/ ";
	out << std::hex << std::setw(10) << addr1.sin_addr.s_addr;
	out << " vs "   << std::setw(10) << addr2 << ")" << std::endl;
	pqioutput(PQL_DEBUG_BASIC, pqinetzone, out.str());
#endif

	if (addr1.sin_addr.s_addr == addr2)
	{
		return 0;
	}
	if (addr1.sin_addr.s_addr < addr2)
		return -1;
	return 1;
}


bool 	getPreferredInterface(struct in_addr &prefAddr) // returns best addr.
{
	std::list<struct in_addr> addrs;
	std::list<struct in_addr>::iterator it;
	struct in_addr addr_zero, addr_loop, addr_priv, addr_ext;

#ifdef NET_DEBUG
	struct in_addr addr;
#endif

	bool found_zero = false;
	bool found_loopback = false;
	bool found_priv = false;
	bool found_ext = false;

	if (!getLocalInterfaces(addrs))
	{
		return false;
	}

	memset(&addr_zero, 0, sizeof(addr_zero));
	memset(&addr_loop, 0, sizeof(addr_loop));
	memset(&addr_priv, 0, sizeof(addr_priv));
	memset(&addr_ext, 0, sizeof(addr_ext));

#ifdef NET_DEBUG
	memset(&addr, 0, sizeof(addr));
#endif

	// find the first of each of these.
	// if ext - take first.
	// if no ext -> first priv
	// if no priv -> first loopback.
	
#ifdef NET_DEBUG
	std::cerr << "getPreferredInterface() " << addrs.size() << " interfaces." << std::endl;
#endif

	for(it = addrs.begin(); it != addrs.end(); it++)
	{
		struct in_addr addr = *it;

#ifdef NET_DEBUG
		std::cerr << "Examining addr: " << rs_inet_ntoa(addr);
		std::cerr << " => " << (uint32_t) addr.s_addr << std::endl ;
#endif

		// for windows silliness (returning 0.0.0.0 as valid addr!).
		if (addr.s_addr == 0)
		{
			if (!found_zero)
			{
#ifdef NET_DEBUG
				std::cerr << "\tFound Zero Address" << std::endl ;
#endif

				found_zero = true;
				addr_zero = addr;
			}
		}
		else if (isLoopbackNet(&addr))
		{
			if (!found_loopback)
			{
#ifdef NET_DEBUG
				std::cerr << "\tFound Loopback Address" << std::endl ;
#endif

				found_loopback = true;
				addr_loop = addr;
			}
		}
		else if (isPrivateNet(&addr))
		{
			if (!found_priv)
			{
#ifdef NET_DEBUG
				std::cerr << "\tFound Private Address" << std::endl ;
#endif

				found_priv = true;
				addr_priv = addr;
			}
		}
		else
		{
			if (!found_ext)
			{
#ifdef NET_DEBUG
				std::cerr << "\tFound Other Address (Ext?) " << std::endl ;
#endif
				found_ext = true;
				addr_ext = addr;
			}
		}
	}

	if(found_ext)					// external address is best.
	{
		prefAddr = addr_ext;
		return true;
	}

	if (found_priv)
	{
		prefAddr = addr_priv;
		return true;
	}

	// next bit can happen under windows, 
	// a general address is still
	// preferable to a loopback device.
	if (found_zero)
	{
		prefAddr = addr_zero;
		return true;
	}

	if (found_loopback)
	{
		prefAddr = addr_loop;
		return true;
	}

	// shound be 255.255.255.255 (error).
	prefAddr.s_addr = 0xffffffff;
	return false;
}

bool    sameNet(struct in_addr *addr, struct in_addr *addr2)
{
#ifdef NET_DEBUG
	std::cerr << "sameNet: " << rs_inet_ntoa(*addr);
	std::cerr << " VS " << rs_inet_ntoa(*addr2);
	std::cerr << std::endl;
#endif
	struct in_addr addrnet, addrnet2;

	addrnet.s_addr = inet_netof(*addr);
	addrnet2.s_addr = inet_netof(*addr2);

#ifdef NET_DEBUG
	std::cerr << " (" << rs_inet_ntoa(addrnet);
	std::cerr << " =?= " << rs_inet_ntoa(addrnet2);
	std::cerr << ")" << std::endl;
#endif

	in_addr_t address1 = htonl(addr->s_addr);
	in_addr_t address2 = htonl(addr2->s_addr);

	// handle case for private net: 172.16.0.0/12
	if (address1>>20 == (172<<4 | 16>>4))
	{
		return (address1>>20 == address2>>20);
	}

	return (inet_netof(*addr) == inet_netof(*addr2));
}


bool 	isSameSubnet(struct in_addr *addr1, struct in_addr *addr2)
{
	/* 
	 * check that the (addr1 & 255.255.255.0) == (addr2 & 255.255.255.0)
	 */

	unsigned long a1 = ntohl(addr1->s_addr);
	unsigned long a2 = ntohl(addr2->s_addr);

	return ((a1 & 0xffffff00) == (a2 & 0xffffff00));
}

/* This just might be portable!!! will see!!!
 * Unfortunately this is usable on winXP+, determined by: (_WIN32_WINNT >= 0x0501)
 * but not older platforms.... which must use gethostbyname.
 *
 * include it for now..... 
 */

bool LookupDNSAddr(std::string name, struct sockaddr_in &addr)
{

#if 1  
	char service[100];
	struct addrinfo hints_st;
	struct addrinfo *hints = &hints_st;
	struct addrinfo *res;
	
	hints -> ai_flags = 0; // (cygwin don;t like these) AI_ADDRCONFIG | AI_NUMERICSERV;
	hints -> ai_family = AF_INET;
	hints -> ai_socktype = 0;
	hints -> ai_protocol = 0;
	hints -> ai_addrlen = 0;
	hints -> ai_addr = NULL;
	hints -> ai_canonname = NULL;
	hints -> ai_next = NULL;

	/* get the port number */
	sprintf(service, "%d", ntohs(addr.sin_port));

	/* set it to IPV4 */
#ifdef NET_DEBUG
	std::cerr << "LookupDNSAddr() name: " << name << " service: " << service << std::endl;
#endif

 	int err = 0;
	if (0 != (err = getaddrinfo(name.c_str(), service, hints, &res)))
	{
#ifdef NET_DEBUG
		std::cerr << "LookupDNSAddr() getaddrinfo failed!" << std::endl;
		std::cerr << "Error: " << gai_strerror(err)  << std::endl;
#endif
		return false;
	}
	
	if ((res) && (res->ai_family == AF_INET)) 
	{
		addr = *((struct sockaddr_in *) res->ai_addr);
		freeaddrinfo(res); 

#ifdef NET_DEBUG
		std::cerr << "LookupDNSAddr() getaddrinfo found address" << std::endl;
		std::cerr << "addr: " << rs_inet_ntoa(addr.sin_addr) << std::endl;
		std::cerr << "port: " << ntohs(addr.sin_port) << std::endl;
#endif
		return true;
	}
	
#ifdef NET_DEBUG
	std::cerr << "getaddrinfo failed - no address" << std::endl;
#endif

#endif
#ifdef NET_DEBUG
	//std::cerr << "getaddrinfo disabled" << std::endl;
#endif
	return false;
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
	int osock = socket(PF_INET, SOCK_STREAM, 0);

/******************* WINDOWS SPECIFIC PART ******************/
#ifdef WINDOWS_SYS  // WINDOWS

#ifdef NET_DEBUG
	std::cerr << "unix_socket()" << std::endl;
#endif

	if ((unsigned) osock == INVALID_SOCKET)
	{
		// Invalidate socket Unix style.
		osock = -1;
		errno = WinToUnixError(WSAGetLastError());
	}
#endif
/******************* WINDOWS SPECIFIC PART ******************/
	return osock;
}


int unix_fcntl_nonblock(int fd)
{
        int ret;

/******************* WINDOWS SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX
	ret = fcntl(fd, F_SETFL, O_NONBLOCK);

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


int unix_connect(int fd, const struct sockaddr *serv_addr, socklen_t addrlen)
{
	int ret = connect(fd, serv_addr, addrlen);

/******************* WINDOWS SPECIFIC PART ******************/
#ifdef WINDOWS_SYS // WINDOWS

#ifdef NET_DEBUG
	std::cerr << "unix_connect()" << std::endl;
#endif
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
