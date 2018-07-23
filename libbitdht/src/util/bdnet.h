/*******************************************************************************
 * util/bdnet.h                                                                *
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
#ifndef BITDHT_UNIVERSAL_NETWORK_HEADER
#define BITDHT_UNIVERSAL_NETWORK_HEADER

#include <inttypes.h>
#include <string>

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#if defined(_WIN32) || defined(__MINGW32__)

#include <ws2tcpip.h>

#include <stdio.h> /* for ssize_t */
typedef uint32_t in_addr_t;

#else // UNIX

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <errno.h>

#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/



/* C Interface */
#ifdef  __cplusplus
extern "C" {
#endif

/*******
 * This defines a (unix-like) universal networking layer
 * that should function on both windows and unix. (C - interface)
 *
 * This is of course only a subset of the full interface.
 * functions required are:
 *
 * int bdnet_close(int fd);
 * int bdnet_socket(int domain, int type, int protocol);
 * int bdnet_bind(int  sockfd,  const  struct  sockaddr  *my_addr,  
 * 				socklen_t addrlen);
 * int bdnet_fcntl(int fd, int cmd, long arg);
 * int bdnet_setsockopt(int s, int level,  int  optname,  
 * 				const  void  *optval, socklen_t optlen);
 * ssize_t bdnet_recvfrom(int s, void *buf, size_t len, int flags,
 *                              struct sockaddr *from, socklen_t *fromlen);
 * ssize_t bdnet_sendto(int s, const void *buf, size_t len, int flags, 
 * 				const struct sockaddr *to, socklen_t tolen);
 *
 * There are some non-standard ones as well:
 * int bdnet_errno();  	for internal networking errors 
 * int bdnet_init();  		required for windows 
 * int bdnet_checkTTL();  	a check if we can modify the ttl 
 */


/* the universal interface */
int bdnet_errno(); /* for internal networking errors */
int bdnet_init(); /* required for windows */
int bdnet_close(int fd);
int bdnet_socket(int domain, int type, int protocol);
int bdnet_bind(int  sockfd,  const  struct  sockaddr  *my_addr,  socklen_t addrlen);
int bdnet_fcntl(int fd, int cmd, long arg);
int bdnet_setsockopt(int s, int level,  int  optname,  
 				const  void  *optval, socklen_t optlen);
ssize_t bdnet_recvfrom(int s, void *buf, size_t len, int flags,
                              struct sockaddr *from, socklen_t *fromlen);
ssize_t bdnet_sendto(int s, const void *buf, size_t len, int flags, 
 				const struct sockaddr *to, socklen_t tolen);

/* address filling */
int bdnet_inet_aton(const char *name, struct in_addr *addr);
/* check if we can modify the TTL on a UDP packet */
int bdnet_checkTTL(int fd);

void	bdsockaddr_clear(struct sockaddr_in *addr);

/* Extra stuff to declare for windows error handling (mimics unix errno)
 */

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#if defined(_WIN32) || defined(__MINGW32__)

// Some Network functions that are missing from windows.
//in_addr_t inet_netof(struct in_addr addr);
//in_addr_t inet_network(char *inet_name);
//int inet_aton(const char *name, struct in_addr *addr);


// definitions for fcntl (NON_BLOCK) (random?)
#define F_SETFL  	0x1010
#define O_NONBLOCK 	0x0100

// definitions for setsockopt (TTL) (random?)
//#define IPPROTO_IP 	0x0011
//#define IP_TTL		0x0110

/* define the Unix Error Codes that we use...
 * NB. we should make the same, but not necessary
 */

#define EAGAIN          11
#define EUSERS          87

#define EHOSTDOWN       112

#ifndef __MINGW64_VERSION_MAJOR
#define EWOULDBLOCK     EAGAIN

#define ENOTSOCK        88

#define EOPNOTSUPP      95 

#define EADDRINUSE      98
#define EADDRNOTAVAIL   99
#define ENETDOWN        100
#define ENETUNREACH     101

#define ECONNRESET      104

#define ETIMEDOUT       10060 // value from pthread.h
#define ECONNREFUSED    111
#define EHOSTUNREACH    113
#define EALREADY        114
#define EINPROGRESS     115
#endif

int     bdnet_w2u_errno(int error);

/* also put the sleep commands in here (where else to go)
 * ms uses millisecs.
 * void Sleep(int ms);
 */
#ifndef __MINGW64_VERSION_MAJOR
int sleep(unsigned int sec);
#endif
int usleep(unsigned int usec);

#endif // END of WINDOWS defines.
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/


#ifdef  __cplusplus
} /* C Interface */
#endif

/* thread-safe version of inet_ntoa */
std::string bdnet_inet_ntoa(struct in_addr in);

#endif /* BITDHT_UNIVERSAL_NETWORK_HEADER */
