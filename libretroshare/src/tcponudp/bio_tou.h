/*******************************************************************************
 * libretroshare/src/tcponudp: bio_tou.h                                       *
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
