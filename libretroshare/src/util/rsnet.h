/*
 * libretroshare/src/util: rsnet.h
 *
 * Universal Networking Header for RetroShare.
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

#ifndef RS_UNIVERSAL_NETWORK_HEADER
#define RS_UNIVERSAL_NETWORK_HEADER

#include <inttypes.h>
#include <stdlib.h>	/* Included because GCC4.4 wants it */
#include <string.h> 	/* Included because GCC4.4 wants it */
#include <iostream>

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <errno.h>

#else

#include <ws2tcpip.h>
typedef uint32_t in_addr_t;

int inet_aton(const char *name, struct in_addr *addr);

// Missing defines in MinGW
#ifndef MSG_WAITALL
#define MSG_WAITALL  8
#endif

#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

/* 64 bit conversions */
uint64_t ntohll(uint64_t x);
uint64_t htonll(uint64_t x);

/* blank a network address */
void sockaddr_clear(struct sockaddr_in *addr);

/* determine network type (moved from pqi/pqinetwork.cc) */
bool isValidNet(const struct in_addr *addr);
bool isLoopbackNet(const struct in_addr *addr);
bool isPrivateNet(const struct in_addr *addr);
bool isExternalNet(const struct in_addr *addr);

// uses a re-entrant version of gethostbyname
bool rsGetHostByName(const std::string& hostname, in_addr& returned_addr) ;

std::ostream& operator<<(std::ostream& o,const struct sockaddr_in&) ;

/* thread-safe version of inet_ntoa */
std::string rs_inet_ntoa(struct in_addr in);


/***************************/
// sockaddr_storage fns.

// Standard bind, on OSX anyway will not accept a longer socklen for IPv4.
// so hidding details behind function.
int     universal_bind(int fd, const struct sockaddr *addr, socklen_t socklen);

void sockaddr_storage_clear(struct sockaddr_storage &addr);

// mods.
bool sockaddr_storage_zeroip(struct sockaddr_storage &addr);
bool sockaddr_storage_copyip(struct sockaddr_storage &dst, const struct sockaddr_storage &src);
uint16_t sockaddr_storage_port(const struct sockaddr_storage &addr);
bool sockaddr_storage_setport(struct sockaddr_storage &addr, uint16_t port);

bool sockaddr_storage_setipv4(struct sockaddr_storage &addr, const sockaddr_in *addr_ipv4);
bool sockaddr_storage_setipv6(struct sockaddr_storage &addr, const sockaddr_in6 *addr_ipv6);

bool sockaddr_storage_ipv4_aton(struct sockaddr_storage &addr, const char *name);
bool sockaddr_storage_ipv4_setport(struct sockaddr_storage &addr, const uint16_t port);


// comparisons.
bool operator<(const struct sockaddr_storage &a, const struct sockaddr_storage &b);

bool sockaddr_storage_same(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2);
bool sockaddr_storage_samefamily(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2);
bool sockaddr_storage_sameip(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2);

// string,
std::string sockaddr_storage_tostring(const struct sockaddr_storage &addr);
std::string sockaddr_storage_familytostring(const struct sockaddr_storage &addr);
std::string sockaddr_storage_iptostring(const struct sockaddr_storage &addr);
std::string sockaddr_storage_porttostring(const struct sockaddr_storage &addr);

// output
//void sockaddr_storage_output(const struct sockaddr_storage &addr, std::ostream &out);
//void sockaddr_storage_ipoutput(const struct sockaddr_storage &addr, std::ostream &out);

// net checks.
bool sockaddr_storage_isnull(const struct sockaddr_storage &addr);
bool sockaddr_storage_isValidNet(const struct sockaddr_storage &addr);
bool sockaddr_storage_isLoopbackNet(const struct sockaddr_storage &addr);
bool sockaddr_storage_isPrivateNet(const struct sockaddr_storage &addr);
bool sockaddr_storage_isExternalNet(const struct sockaddr_storage &addr);



#endif /* RS_UNIVERSAL_NETWORK_HEADER */
