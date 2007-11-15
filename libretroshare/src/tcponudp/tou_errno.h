/*
 * "$Id: tou_errno.h,v 1.3 2007-02-18 21:46:50 rmf24 Exp $"
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



#ifndef TOU_ERRNO_HEADER
#define TOU_ERRNO_HEADER

/* C Interface */
#ifdef  __cplusplus
extern "C" {
#endif

/*******
 * This defines the unix errno's for windows, these are
 * needed to determine error types, these are defined 
 * to be the same as the unix ones. 
 */

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifdef WINDOWS_SYS

#define EAGAIN		11
#define EWOULDBLOCK 	EAGAIN

#define EUSERS 		87
#define ENOTSOCK 	88

#define EOPNOTSUPP 	95 

#define EADDRINUSE 	98
#define EADDRNOTAVAIL	99
#define ENETDOWN 	100
#define ENETUNREACH 	101

#define ECONNRESET	104

#define ETIMEDOUT 	110
#define ECONNREFUSED 	111
#define EHOSTDOWN 	112
#define EHOSTUNREACH 	113
#define EALREADY 	114
#define EINPROGRESS 	115


#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

#ifdef  __cplusplus
} /* C Interface */
#endif

#endif /* TOU_ERRNO_HEADER */
