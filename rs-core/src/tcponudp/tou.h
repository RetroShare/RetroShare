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


	/* creation/connections */
int	tou_socket(int domain, int type, int protocol);
int 	tou_bind(int sockfd, const struct sockaddr *my_addr, socklen_t addrlen);
int 	tou_connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);
int 	tou_listen(int sockfd, int backlog);
int 	tou_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

	/* udp interface */
	/* request an external udp port to use - returns sockfd */
int	tou_extudp(const struct sockaddr *ext, socklen_t tolen);
int	tou_extaddr(int sockfd, const struct sockaddr *to, socklen_t tolen);

ssize_t tou_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
ssize_t tou_sendto(int s, const void *buf, size_t len, int flags, const struct
	              sockaddr *to, socklen_t tolen);


/* non-standard bonuses */
int	tou_connected(int sockfd);
int 	tou_listenfor(int sockfd, const struct sockaddr *serv_addr, 
					socklen_t addrlen);

#ifdef  __cplusplus
}
#endif
#endif

