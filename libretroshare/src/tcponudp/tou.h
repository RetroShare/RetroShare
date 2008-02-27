/*
 * "$Id: tou.h,v 1.4 2007-02-18 21:46:50 rmf24 Exp $"
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
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <stdio.h>
	typedef int socklen_t;
#endif


/* standard C interface (as Unix-like as possible)
 * for the tou (Tcp On Udp) library
 */

#ifdef  __cplusplus
extern "C" {
#endif

	/* The modification to a single UDP socket means
 	 * that the structure of the TOU interface must be changed.
	 *
	 * Init: 
	 * (1) choose our local address. (a universal bind)
	 * int  tou_init(const struct sockaddr *my_addr);
	 * (2) query if we have determined our external address.
         * int	tou_extaddr(struct sockaddr *ext_addr, socklen_t *addrlen);
	 * (3) offer more stunpeers, for external address determination.
         * int	tou_stunpeer(const struct sockaddr *ext_addr, socklen_t addrlen, const char *id);
	 * (4) repeat (2)+(3) until a valid extaddr is returned.
	 *
	 */

int  	tou_init(const struct sockaddr *my_addr, socklen_t addrlen);
int	tou_extaddr(struct sockaddr *ext_addr, socklen_t *addrlen);
int	tou_stunpeer(const struct sockaddr *ext_addr, socklen_t addrlen, const char *id);

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
	 */ 

	/* creation/connections */
int	tou_socket(int domain, int type, int protocol);
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
#endif
#endif

