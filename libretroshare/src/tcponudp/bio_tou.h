/*
 * "$Id: bio_tou.h,v 1.2 2007-02-18 21:46:50 rmf24 Exp $"
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

#ifndef BIO_TCPONUDP_H
#define BIO_TCPONUDP_H

#include <openssl/bio.h>

#ifdef  __cplusplus
extern "C" {
#endif


int BIO_tou_socket_should_retry(int s, int e);
int BIO_tou_socket_non_fatal_error(int error);

#define BIO_TYPE_TOU_SOCKET     (30|0x0400|0x0100)      /* NEW rmfern type */

BIO_METHOD *BIO_s_tou_socket(void);

#ifdef  __cplusplus
}
#endif
#endif
