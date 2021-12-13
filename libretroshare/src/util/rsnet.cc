/*******************************************************************************
 * libretroshare/src/util: rsnet.cc                                            *
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

#include "util/rsnet.h"
#include "util/rsthreads.h"
#include "util/rsstring.h"
#include "util/rsmemory.h"

#ifdef WINDOWS_SYS
#else
#include <netdb.h>
#endif

#include <cstring>

/* enforce LITTLE_ENDIAN on Windows */
#ifdef WINDOWS_SYS
	#define BYTE_ORDER  1234
	#define LITTLE_ENDIAN 1234
	#define BIG_ENDIAN  4321
#endif

#ifndef ntohll
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
#endif
#ifndef htonll
uint64_t htonll(uint64_t x)
{
        return ntohll(x);
}
#endif

void sockaddr_clear(struct sockaddr_in *addr)
{
        memset(addr, 0, sizeof(struct sockaddr_in));
        addr->sin_family = AF_INET;
}

bool rsGetHostByName(const std::string& hostname, in_addr& returned_addr)
{
	addrinfo hint; memset(&hint, 0, sizeof(hint));
	hint.ai_family = AF_INET;
	addrinfo* info = nullptr;
	int res = getaddrinfo(hostname.c_str(), nullptr, &hint, &info);

	bool ok = true;
	if(res > 0 || !info || !info->ai_addr)
	{
		std::cerr << __PRETTY_FUNCTION__ << "(EE) getaddrinfo returned error "
		          << res << " on string \"" << hostname << "\"" << std::endl;
		returned_addr.s_addr = 0;
		ok = false;
	}
	else
		returned_addr.s_addr = ((sockaddr_in*)info->ai_addr)->sin_addr.s_addr;

	if(info) freeaddrinfo(info);

	return ok;
}

bool    isValidNet(const struct in_addr *addr)
{
        // invalid address.
	if((*addr).s_addr == INADDR_NONE)
		return false;
	if((*addr).s_addr == 0)
		return false;
	// should do more tests.
	return true;
}


bool    isLoopbackNet(const struct in_addr *addr)
{
	in_addr_t taddr = ntohl(addr->s_addr);
	return (taddr == (127 << 24 | 1));
}

bool isPrivateNet(const struct in_addr *addr)
{
	in_addr_t taddr = ntohl(addr->s_addr);

	if ( (taddr>>24 == 10) ||                     // 10.0.0.0/8
	     (taddr>>20 == (172<<4 | 16>>4)) ||       // 172.16.0.0/12
	     (taddr>>16 == (192<<8 | 168)) ||         // 192.168.0.0/16
	     (taddr>>16 == (169<<8 | 254)) )          // 169.254.0.0/16
		return true;

	return false;
}

bool isLinkLocalNet(const struct in_addr *addr)
{
	in_addr_t taddr = ntohl(addr->s_addr);
	if ( taddr>>16 == (169<<8 | 254) ) return true;  // 169.254.0.0/16

	return false;
}

bool    isExternalNet(const struct in_addr *addr)
{
	if (!isValidNet(addr))
	{
		return false;
	}
	if (isLoopbackNet(addr))
	{
		return false;
	}
	if (isPrivateNet(addr))
	{
		return false;
	}
	return true;
}

std::ostream &operator<<(std::ostream &out, const struct sockaddr_in &addr)
{
	out << "[" << rs_inet_ntoa(addr.sin_addr) << ":";
	out << htons(addr.sin_port) << "]";
	return out;
}

std::ostream& operator<<(std::ostream& o, const sockaddr_storage& addr)
{ return o << sockaddr_storage_tostring(addr); }

/* thread-safe version of inet_ntoa */

std::string rs_inet_ntoa(struct in_addr in)
{
	std::string str;
	uint8_t *bytes = (uint8_t *) &(in.s_addr);
	rs_sprintf(str, "%u.%u.%u.%u", (int) bytes[0], (int) bytes[1], (int) bytes[2], (int) bytes[3]);
	return str;
}

int rs_setSockTimeout( int sockfd, bool forReceive /*= true*/
                     , int timeout_Sec /*= 0*/, int timeout_uSec /*= 0*/)
{
#ifdef WINDOWS_SYS
		DWORD timeout = timeout_Sec * 1000 + timeout_uSec;
#else
		struct timeval timeout;
		timeout.tv_sec = timeout_Sec;
		timeout.tv_usec = timeout_uSec;
#endif
		return setsockopt( sockfd, SOL_SOCKET, forReceive ? SO_RCVTIMEO : SO_SNDTIMEO
		                 , (const char*)&timeout, sizeof timeout);
}
