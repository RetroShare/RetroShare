/*******************************************************************************
 * libretroshare/src/util: rsnet.h                                             *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2006 Robert Fernie <retroshare@lunamutt.com>                 *
 * Copyright 2015-2018 Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

/**
 * Workaround for binary compatibility between Windows XP (which miss
 * IPV6_V6ONLY define), and newer Windows that has it.
 * @see http://lua-users.org/lists/lua-l/2013-04/msg00191.html
 */
#ifndef IPV6_V6ONLY
#	define IPV6_V6ONLY 27
#endif

/* 64 bit conversions */
#ifndef ntohll
uint64_t ntohll(uint64_t x);
#endif
#ifndef htonll
uint64_t htonll(uint64_t x);
#endif

/* blank a network address */
void sockaddr_clear(struct sockaddr_in *addr);

/* determine network type (moved from pqi/pqinetwork.cc) */
bool isValidNet(const struct in_addr *addr);
bool isLoopbackNet(const struct in_addr *addr);
bool isPrivateNet(const struct in_addr *addr);
bool isLinkLocalNet(const struct in_addr *addr);
bool isExternalNet(const struct in_addr *addr);

// uses a re-entrant version of gethostbyname
bool rsGetHostByName(const std::string& hostname, in_addr& returned_addr) ;

// Get hostName address using specific DNS server
// Using it allow to direct ask our Address to IP, so no need to have a DNS (IPv4 or IPv6 ???).
// If we ask to a IPv6 DNS Server, it respond for our IPv6 address.
bool rsGetHostByNameSpecDNS(const std::string& servername, const std::string& hostname, std::string& returned_addr, int timeout_s = -1);

std::ostream& operator<<(std::ostream& o, const sockaddr_in&);
std::ostream& operator<<(std::ostream& o, const sockaddr_storage&);

/* thread-safe version of inet_ntoa */
std::string rs_inet_ntoa(struct in_addr in);


/***************************/
// sockaddr_storage fns.

int rs_bind(int fd, const sockaddr_storage& addr);

void sockaddr_storage_clear(struct sockaddr_storage &addr);

// mods.
bool sockaddr_storage_zeroip(struct sockaddr_storage &addr);

/**
 * @brief Use this function to copy sockaddr_storage.
 *
 * POSIX does not require that objects of type sockaddr_storage can be copied
 * as aggregates thus it is unsafe to aggregate copy ( operator = )
 * sockaddr_storage and unexpected behaviors may happens due to padding
 * and alignment.
 *
 * @see https://sourceware.org/bugzilla/show_bug.cgi?id=20111
 * @see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71120
 */
bool sockaddr_storage_copy(const sockaddr_storage& src, sockaddr_storage& dst);

bool sockaddr_storage_copyip(struct sockaddr_storage &dst, const struct sockaddr_storage &src);
uint16_t sockaddr_storage_port(const struct sockaddr_storage &addr);
bool sockaddr_storage_setport(struct sockaddr_storage &addr, uint16_t port);

bool sockaddr_storage_setipv4(struct sockaddr_storage &addr, const sockaddr_in *addr_ipv4);
bool sockaddr_storage_setipv6(struct sockaddr_storage &addr, const sockaddr_in6 *addr_ipv6);

bool sockaddr_storage_inet_pton( sockaddr_storage &addr,
                                 const std::string& ipStr );
bool sockaddr_storage_ipv4_aton(struct sockaddr_storage &addr, const char *name);

bool sockaddr_storage_ipv4_setport(struct sockaddr_storage &addr, const uint16_t port);

bool sockaddr_storage_ipv4_to_ipv6(sockaddr_storage &addr);
bool sockaddr_storage_ipv6_to_ipv4(sockaddr_storage &addr);

// comparisons.
bool operator<(const struct sockaddr_storage &a, const struct sockaddr_storage &b);

bool sockaddr_storage_same(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2);
bool sockaddr_storage_samefamily(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2);
bool sockaddr_storage_sameip(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2);

// string,
std::string sockaddr_storage_tostring(const struct sockaddr_storage &addr);
bool sockaddr_storage_fromString(const std::string& str, sockaddr_storage &addr);
std::string sockaddr_storage_familytostring(const struct sockaddr_storage &addr);
std::string sockaddr_storage_iptostring(const struct sockaddr_storage &addr);
std::string sockaddr_storage_porttostring(const struct sockaddr_storage &addr);
void sockaddr_storage_dump(const sockaddr_storage & addr, std::string * outputString = NULL);

// net checks.
bool sockaddr_storage_isnull(const struct sockaddr_storage &addr);
bool sockaddr_storage_isValidNet(const struct sockaddr_storage &addr);
bool sockaddr_storage_isLoopbackNet(const struct sockaddr_storage &addr);
bool sockaddr_storage_isPrivateNet(const struct sockaddr_storage &addr);
bool sockaddr_storage_isLinkLocalNet(const struct sockaddr_storage &addr);
bool sockaddr_storage_ipv6_isLinkLocalNet(const sockaddr_storage &addr);
bool sockaddr_storage_isExternalNet(const struct sockaddr_storage &addr);

bool sockaddr_storage_inet_ntop(const sockaddr_storage &addr, std::string &dst);

int rs_setsockopt( int sockfd, int level, int optname,
                   const uint8_t *optval, uint32_t optlen );

#endif /* RS_UNIVERSAL_NETWORK_HEADER */
