/*
 * "$Id: pqinetwork.h,v 1.15 2007-04-15 18:45:18 rmf24 Exp $"
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



#ifndef MRK_PQI_NETWORKING_HEADER
#define MRK_PQI_NETWORKING_HEADER


/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/poll.h>

//socket blocking/options.
#include <fcntl.h>
#include <inttypes.h>

#else


#include "util/rsnet.h" /* more generic networking header */

#include <winsock2.h>
typedef int socklen_t;
//typedef unsigned long in_addr_t;

// Some Network functions that are missing from windows.

in_addr_t inet_netof(struct in_addr addr);
in_addr_t inet_network(const char *inet_name);
int inet_aton(const char *name, struct in_addr *addr);

extern int errno; /* Define extern errno, to duplicate unix behaviour */

/* define the Unix Error Codes that we use...
 * NB. we should make the same, but not necessary
 */
#define EAGAIN          11
#define EWOULDBLOCK     EAGAIN

#define EUSERS          87
#define ENOTSOCK        88

#define EOPNOTSUPP      95

#define EADDRINUSE      98
#define EADDRNOTAVAIL   99
#define ENETDOWN        100
#define ENETUNREACH     101

#define ECONNRESET      104

#define ETIMEDOUT       10060 // value from pthread.h
#define ECONNREFUSED    111
#define EHOSTDOWN       112
#define EHOSTUNREACH    113
#define EALREADY        114
#define EINPROGRESS     115

#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

#include <iostream>
#include <string>
#include <list>

// Same def - different functions...

void showSocketError(std::string &out);

std::string socket_errorType(int err);
int sockaddr_cmp(struct sockaddr_in &addr1, struct sockaddr_in &addr2 );
int inaddr_cmp(struct sockaddr_in addr1, struct sockaddr_in addr2 );
int inaddr_cmp(struct sockaddr_in addr1, unsigned long);

bool    getPreferredInterface(struct in_addr &routeAddr, struct in_addr &prefAddr); // returns best addr.
bool getLocalInterfaces(in_addr &routeAddr, std::list<struct in_addr> &addrs); // returns all possible addrs.

	// checks (addr1 & 255.255.255.0) == (addr2 & 255.255.255.0)
bool    isSameSubnet(struct in_addr *addr1, struct in_addr *addr2);
bool	sameNet(const struct in_addr *addr, const struct in_addr *addr2); 

in_addr_t pqi_inet_netof(struct in_addr addr); // our implementation.

bool LookupDNSAddr(std::string name, struct sockaddr_in &addr);

/* universal socket interface */

int unix_close(int sockfd);
int unix_socket(int domain, int type, int protocol);
int unix_fcntl_nonblock(int sockfd);
int unix_connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);
int unix_getsockopt_error(int sockfd, int *err);

#ifdef WINDOWS_SYS // WINDOWS
/******************* WINDOWS SPECIFIC PART ******************/
int     WinToUnixError(int error);
#endif



#endif

