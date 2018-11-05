/*******************************************************************************
 * libretroshare/src/tcponudp: tou.h                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef TOU_C_HEADER_H
#define TOU_C_HEADER_H


/* get OS-specific definitions for:
 * 	struct sockaddr, socklen_t, ssize_t
 */

#ifndef WINDOWS_SYS

	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>

#else
	#include <stdint.h>
	#include <winsock2.h>
	#include <stdio.h>
//	#include <stdint.h>
	typedef int socklen_t;
#endif


#ifdef  __cplusplus
extern "C" {
#endif

/* standard C interface (as Unix-like as possible)
 * for the tou (Tcp On Udp) library
 */
	/*
	 * Init: 
	 * The TOU library no longer references any UdpStack items.
	 * instead, two arrays should be passed to the init function.
	 * int tou_init( (void **) UdpSubReceiver **udpRecvers, int *udpType, int nUdps);
	 * 
	 * The UdpSubReceivers should be derived classes, with corresponding types:
	 *   UdpPeerReceiver	TOU_RECEIVER_TYPE_UDPPEER	
	 *   UdpRelayReceiver	TOU_RECEIVER_TYPE_UDPRELAY
	 *
	 */

#define MAX_TOU_RECEIVERS       16

#define TOU_RECEIVER_TYPE_NONE          0x0000
#define TOU_RECEIVER_TYPE_UDPPEER       0x0001
#define TOU_RECEIVER_TYPE_UDPRELAY      0x0002

// hack to avoid classes in C code. (MacOSX complaining)
int  	tou_init(void **udpSubRecvs, int *udpTypes, int nUdps);

	/* Connections are as similar to UNIX as possible 
	 * (1) create a socket: tou_socket() this reserves a socket id.
	 * (2) connect: active: tou_connect() or passive: tou_listenfor().
	 * (3) use as a normal socket.
	 *
	 * connect() now has a conn_period parameter - this is the
	 * estimate (in seconds) of how slowly the connection should proceed.
	 *
	 * tou_bind() is not valid. tou_init performs this role.
	 * tou_listen() is not valid. (must listen for a specific address) use tou_listenfor() instead.
	 * tou_accept() can still be used.
	 *
	 ****** THE ABOVE IS BECOMING LESS TRUE ********
	 *
	 * I have now added Multiple type of TOU Connections (Proxy, Relay), 
	 * and multiple UDP Receivers (meaning you can use different ports too).
	 *
	 * The UDP receivers must be specified at startup (new tou_init())
	 * and the Receiver, and Type of connection must be specified when you
	 * open the socket.
	 *
	 * The parameters to tou_socket, therefore mean something!
	 * some extra checking has been put in to try and catch bad usage.
	 */ 

	/* creation/connections */
int	tou_socket(uint32_t domain, uint32_t type, int protocol);
int 	tou_bind(int sockfd, const struct sockaddr *my_addr, socklen_t addrlen);	/* null op now */
int 	tou_listen(int sockfd, int backlog); 						/* null op now */
int 	tou_connect(int sockfd, const struct sockaddr *serv_addr, 
					socklen_t addrlen, uint32_t conn_period);
int 	tou_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);		

/* non-standard bonuses */
int	tou_connected(int sockfd);
int 	tou_listenfor(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);



	/* UNIX interface: minimum for the SSL BIO interface */
ssize_t tou_read(int sockfd, void *buf, size_t count);
ssize_t tou_write(int sockfd, const void *buf, size_t count);
int 	tou_close(int sockfd);

	/* non-standard */
int	tou_errno(int sockfd);
int	tou_clear_error(int sockfd);

	/* check stream */
int	tou_maxread(int sockfd);
int	tou_maxwrite(int sockfd);


#ifdef  __cplusplus
}

typedef struct sockaddr_in sockaddr_in;

/// for relay connections
int tou_connect_via_relay( int sockfd, const sockaddr_in& own_addr,
                           const sockaddr_in& proxy_addr,
                           const sockaddr_in& dest_addr );

#endif
#endif

