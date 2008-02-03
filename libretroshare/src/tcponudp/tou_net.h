/*
 * "$Id: tou_net.h,v 1.3 2007-02-18 21:46:50 rmf24 Exp $"
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



#ifndef TOU_UNIVERSAL_NETWORK_HEADER
#define TOU_UNIVERSAL_NETWORK_HEADER

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
 * int tounet_close(int fd);
 * int tounet_socket(int domain, int type, int protocol);
 * int tounet_bind(int  sockfd,  const  struct  sockaddr  *my_addr,  
 * 				socklen_t addrlen);
 * int tounet_fcntl(int fd, int cmd, long arg);
 * int tounet_setsockopt(int s, int level,  int  optname,  
 * 				const  void  *optval, socklen_t optlen);
 * ssize_t tounet_recvfrom(int s, void *buf, size_t len, int flags,
 *                              struct sockaddr *from, socklen_t *fromlen);
 * ssize_t tounet_sendto(int s, const void *buf, size_t len, int flags, 
 * 				const struct sockaddr *to, socklen_t tolen);
 *
 * There are some non-standard ones as well:
 * int tounet_errno();  	for internal networking errors 
 * int tounet_init();  		required for windows 
 * int tounet_checkTTL();  	a check if we can modify the ttl 
 */

/* Some Types need to be defined before the interface can be declared
 */
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <errno.h>

#else

#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h> /* for ssize_t */
typedef int socklen_t;
typedef unsigned long in_addr_t;

#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/


/* the universal interface */
int tounet_errno(); /* for internal networking errors */
int tounet_init(); /* required for windows */
int tounet_close(int fd);
int tounet_socket(int domain, int type, int protocol);
int tounet_bind(int  sockfd,  const  struct  sockaddr  *my_addr,  socklen_t addrlen);
int tounet_fcntl(int fd, int cmd, long arg);
int tounet_setsockopt(int s, int level,  int  optname,  
 				const  void  *optval, socklen_t optlen);
ssize_t tounet_recvfrom(int s, void *buf, size_t len, int flags,
                              struct sockaddr *from, socklen_t *fromlen);
ssize_t tounet_sendto(int s, const void *buf, size_t len, int flags, 
 				const struct sockaddr *to, socklen_t tolen);

/* address filling */
int tounet_inet_aton(const char *name, struct in_addr *addr);
/* check if we can modify the TTL on a UDP packet */
int tounet_checkTTL(int fd);



/* Extra stuff to declare for windows error handling (mimics unix errno)
 */

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifdef WINDOWS_SYS

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

#include "tou_errno.h"

int     tounet_w2u_errno(int error);

/* also put the sleep commands in here (where else to go)
 * ms uses millisecs.
 * void Sleep(int ms);
 */
void sleep(int sec); 
void usleep(int usec);

#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/


#ifdef  __cplusplus
} /* C Interface */
#endif

#endif /* TOU_UNIVERSAL_NETWORK_HEADER */
