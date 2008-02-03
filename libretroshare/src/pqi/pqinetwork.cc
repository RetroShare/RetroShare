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

#include <errno.h>
#include <iostream>

#include "pqi/pqidebug.h"
#include <sstream>
#include <iomanip>
static const int pqinetzone = 96184;

#ifdef WINDOWS_SYS   /* Windows - define errno */

int errno;

#else /* Windows - define errno */

#include <netdb.h>

#endif               

void sockaddr_clear(struct sockaddr_in *addr)
{
	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
}


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

std::list<std::string> getLocalInterfaces()
{
	std::list<std::string> addrs;
	
	int sock = 0;
	struct ifreq ifreq;

	struct if_nameindex *iflist = if_nameindex();
	struct if_nameindex *ifptr = iflist;

	//need a socket for ioctl()
	if( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		pqioutput(PQL_ALERT, pqinetzone, 
			"Cannot Determine Local Addresses!");
		exit(1);
	}

	if (!ifptr)
	{
		pqioutput(PQL_ALERT, pqinetzone, 
			"getLocalInterfaces(): ERROR if_nameindex == NULL");
	}

	// loop through the interfaces.
	//for(; *(char *)ifptr != 0; ifptr++)
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
			const char *astr=inet_ntoa(aptr -> sin_addr);

			std::ostringstream out;
			out << "Iface: ";
			out << ifptr -> if_name << std::endl;
			out << " Address: " << astr;
			out << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqinetzone, out.str());

			addrs.push_back(astr);
		}
	}
	// free socket -> or else run out of fds.
	close(sock);

	if_freenameindex(iflist);
	return addrs;
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
std::list<std::string> getLocalInterfaces()
{
	std::list<std::string> addrs;


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
		out << " => " << inet_ntoa(addr);
		out << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqinetzone, out.str());

		addrs.push_back(inet_ntoa(addr));
	}

	return addrs;
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
in_addr_t inet_network(char *inet_name)
{
	struct in_addr addr;
	if (inet_aton(inet_name, &addr))
	{
//		std::cerr << "inet_network(" << inet_name << ") : ";
//		std::cerr << inet_ntoa(addr) << std::endl;
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

	std::cerr << "inet_netof(" << inet_ntoa(addr) << ") ";
	if (!((haddr >> 31) | 0x0UL)) // MSB = 0
	{
		std::cerr << " Type A " << std::endl;
		std::cerr << "\tShifted(31): " << (haddr >> 31);
		std::cerr << " Xord(0x0UL): " <<
			!((haddr >> 31) | 0x0UL) << std::endl;

		return htonl(abit);
		//return abit;
	}
	else if (!((haddr >> 30) ^ 0x2UL)) // 2MSBs = 10
	{
		std::cerr << " Type B " << std::endl;
		std::cerr << "\tShifted(30): " << (haddr >> 30);
		std::cerr << " Xord(0x2UL): " <<
			!((haddr >> 30) | 0x2UL) << std::endl;

		return htonl(bbit);
		//return bbit;
	}
	else if (!((haddr >> 29) ^ 0x6UL)) // 3MSBs = 110
	{
		std::cerr << " Type C " << std::endl;
		std::cerr << "\tShifted(29): " << (haddr >> 29);
		std::cerr << " Xord(0x6UL): " <<
			!((haddr >> 29) | 0x6UL) << std::endl;

		return htonl(cbit);
		//return cbit;
	}
	else if (!((haddr >> 28) ^ 0xeUL)) // 4MSBs = 1110
	{
		std::cerr << " Type Multicast " << std::endl;
		std::cerr << "\tShifted(28): " << (haddr >> 28);
		std::cerr << " Xord(0xeUL): " <<
			!((haddr >> 29) | 0xeUL) << std::endl;

		return addr.s_addr; // return full address.
		//return haddr;
	}
	else if (!((haddr >> 27) ^ 0x1eUL)) // 5MSBs = 11110
	{
		std::cerr << " Type Reserved " << std::endl;
		std::cerr << "\tShifted(27): " << (haddr >> 27);
		std::cerr << " Xord(0x1eUL): " <<
			!((haddr >> 27) | 0x1eUL) << std::endl;

		return addr.s_addr; // return full address.
		//return haddr;
	}
	return htonl(abit);
	//return abit;
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
	std::ostringstream out;
	out << "inaddr_cmp(" << inet_ntoa(addr1.sin_addr);
	out << "-" << addr1.sin_addr.s_addr;
	out << "," << inet_ntoa(addr2.sin_addr);
	out << "-" << addr2.sin_addr.s_addr << ")" << std::endl;
	pqioutput(PQL_DEBUG_BASIC, pqinetzone, out.str());


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
	struct in_addr inaddr_tmp;
	inaddr_tmp.s_addr = addr2;
	std::ostringstream out;
	out << "inaddr_cmp2(" << inet_ntoa(addr1.sin_addr);
	out << " vs " << inet_ntoa(inaddr_tmp);
	out << " /or/ ";
	out << std::hex << std::setw(10) << addr1.sin_addr.s_addr;
	out << " vs "   << std::setw(10) << addr2 << ")" << std::endl;
	pqioutput(PQL_DEBUG_BASIC, pqinetzone, out.str());

	if (addr1.sin_addr.s_addr == addr2)
	{
		return 0;
	}
	if (addr1.sin_addr.s_addr < addr2)
		return -1;
	return 1;
}

bool    isLoopbackNet(struct in_addr *addr)
{
	if (0 == strcmp(inet_ntoa(*addr), "127.0.0.1"))
	{
		return true;
	}
	return false;
}

// returns all possible addrs.
// all processing is done in network byte order.
bool    isPrivateNet(struct in_addr *addr)
{
	bool ret = false;
	std::ostringstream out;

	in_addr_t taddr = inet_netof(*addr);

	out << "isPrivateNet(" << inet_ntoa(*addr) << ") ? ";
	out << std::endl;

	struct in_addr addr2;

	addr2.s_addr = taddr;
	out << "Checking (" << inet_ntoa(addr2) << ") Against: " << std::endl;
	// shouldn't need to rotate these...h -> n (in network already.
	// check text output....
	addr2.s_addr = htonl(inet_network("10.0.0.0"));
	out << "\t(" << inet_ntoa(addr2) << ")" << std::endl;
	addr2.s_addr = htonl(inet_network("172.16.0.0"));
	out << "\t(" << inet_ntoa(addr2) << ")" << std::endl;
	addr2.s_addr = htonl(inet_network("192.168.0.0"));
	out << "\t(" << inet_ntoa(addr2) << ")" << std::endl;
	addr2.s_addr = htonl(inet_network("169.254.0.0"));
	out << "\t(" << inet_ntoa(addr2) << ")" << std::endl;

	if ((taddr == htonl(inet_network("10.0.0.0"))) ||
 		(taddr == htonl(inet_network("172.16.0.0"))) ||
 		(taddr == htonl(inet_network("192.168.0.0"))) ||
 		(taddr == htonl(inet_network("169.254.0.0"))))
	{
		out << " True" << std::endl;
		ret = true;
	}
	else
	{
		out << " False" << std::endl;
		ret = false;
	}
	pqioutput(PQL_DEBUG_BASIC, pqinetzone, out.str());
	return ret;
}

bool    isExternalNet(struct in_addr *addr)
{
	if (!isValidNet(addr))
	{
		return false;
	}
	if (isLoopbackNet(addr))
	{
		return false;
	}
	if (isPrivateNet(addr))
	{
		return false;
	}
	return true;
}


struct in_addr getPreferredInterface() // returns best addr.
{

	std::list<std::string> addrs =  getLocalInterfaces();
	std::list<std::string>::iterator it;
	struct in_addr addr_zero = {0}, addr_loop = {0}, addr_priv = {0}, addr_ext = {0}, addr = {0};

	bool found_zero = false;
	bool found_loopback = false;
	bool found_priv = false;
	bool found_ext = false;

	// find the first of each of these.
	// if ext - take first.
	// if no ext -> first priv
	// if no priv -> first loopback.
	
	for(it = addrs.begin(); it != addrs.end(); it++)
	{
		inet_aton((*it).c_str(), &addr);
		// for windows silliness (returning 0.0.0.0 as valid addr!).
		if (addr.s_addr == 0)
		{
			if (!found_zero)
			{
				found_zero = true;
				addr_zero = addr;
			}
		}
		else if (isLoopbackNet(&addr))
		{
			if (!found_loopback)
			{
				found_loopback = true;
				addr_loop = addr;
			}
		}
		else if (isPrivateNet(&addr))
		{
			if (!found_priv)
			{
				found_priv = true;
				addr_priv = addr;
			}
		}
		else
		{
			if (!found_ext)
			{
				found_ext = true;
				addr_ext = addr;
				return addr_ext;
			}
		}
	}

	if (found_priv)
		return addr_priv;

	// next bit can happen under windows, 
	// a general address is still
	// preferable to a loopback device.
	if (found_zero)
		return addr_zero;

	if (found_loopback)
		return addr_loop;

	// shound be 255.255.255.255 (error).
	addr.s_addr = 0xffffffff;
	return addr;
}

bool    sameNet(struct in_addr *addr, struct in_addr *addr2)
{
	return (inet_netof(*addr) == inet_netof(*addr2));
}


bool    isValidNet(struct in_addr *addr)
{
	// invalid address.
	if((*addr).s_addr == INADDR_NONE)
		return false;
	if((*addr).s_addr == 0)
		return false;
	// should do more tests.
	return true;
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
	std::cerr << "LookupDNSAddr() name: " << name << " service: " << service << std::endl;

 	int err = 0;
	if (0 != (err = getaddrinfo(name.c_str(), service, hints, &res)))
	{
		std::cerr << "LookupDNSAddr() getaddrinfo failed!" << std::endl;
		std::cerr << "Error: " << gai_strerror(err)  << std::endl;
		return false;
	}
	
	if ((res) && (res->ai_family == AF_INET)) 
	{
		std::cerr << "LookupDNSAddr() getaddrinfo found address" << std::endl;
		addr = *((struct sockaddr_in *) res->ai_addr);
		std::cerr << "addr: " << inet_ntoa(addr.sin_addr) << std::endl;
		std::cerr << "port: " << ntohs(addr.sin_port) << std::endl;
		freeaddrinfo(res); 
		return true;
	}
	
	std::cerr << "getaddrinfo failed - no address" << std::endl;

#endif
	std::cerr << "getaddrinfo disabled" << std::endl;
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
	std::cerr << "unix_close()" << std::endl;
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
	std::cerr << "unix_socket()" << std::endl;
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
	std::cerr << "unix_fcntl_nonblock():" << ret << " errno:" << errno << std::endl;
#else
	unsigned long int on = 1;
	ret = ioctlsocket(fd, FIONBIO, &on);
	std::cerr << "unix_fcntl_nonblock()" << std::endl;
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
	std::cerr << "unix_connect()" << std::endl;
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
	std::cerr << "unix_getsockopt_error() returned: " << (int) err << std::endl;
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
	std::cerr << "WinToUnixError(" << error << ")" << std::endl;
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
			std::cerr << "WinToUnixError(" << error << ") Code Unknown!";
			std::cerr << std::endl;
			break;
	}
	return ECONNREFUSED; /* sensible default? */
}

#endif
/******************* WINDOWS SPECIFIC PART ******************/

