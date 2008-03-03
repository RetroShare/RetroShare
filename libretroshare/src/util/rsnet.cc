/*
 * libretroshare/src/util: rsnet.cc
 *
 * Universal Networking Header for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#include "util/rsnet.h"
#include <string.h>

/* enforce LITTLE_ENDIAN on Windows */
#ifdef WINDOWS_SYS
	#define BYTE_ORDER  1234
	#define LITTLE_ENDIAN 1234
	#define BIG_ENDIAN  4321
#endif

uint64_t ntohll(uint64_t x)
{
#ifdef BYTE_ORDER
        #if BYTE_ORDER == BIG_ENDIAN
                return x;
        #elif BYTE_ORDER == LITTLE_ENDIAN

                uint32_t top = (uint32_t) (x >> 32);
                uint32_t bot = (uint32_t) (0x00000000ffffffffULL & x);

                uint64_t rev = ((uint64_t) ntohl(top)) | (((uint64_t) ntohl(bot)) << 32);

                return rev;
        #else
                #error "ENDIAN determination Failed"
        #endif
#else
        #error "ENDIAN determination Failed (BYTE_ORDER not defined)"
#endif

}

uint64_t htonll(uint64_t x)
{
        return ntohll(x);
}

void sockaddr_clear(struct sockaddr_in *addr)
{
        memset(addr, 0, sizeof(struct sockaddr_in));
        addr->sin_family = AF_INET;
}

