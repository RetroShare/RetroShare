/*******************************************************************************
 * libretroshare/src/util: rsnet_ss.cc                                         *
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

#include "util/rsurl.h"

#include <sstream>
#include <iomanip>
#include <cstdlib>

#ifdef WINDOWS_SYS
#	include <Winsock2.h>
/** Provides Linux like accessor for in6_addr.s6_addr16 for Windows.
 * Yet Windows doesn't provide 32 bits accessors so there is no way to use
 * in6_addr.s6_addr32 crossplatform.
 */
#	define s6_addr16 u.Word
#else
#	include <netinet/in.h>
#	include <sys/socket.h>
#	include <sys/types.h>
#	ifdef __APPLE__
/// Provides Linux like accessor for in6_addr.s6_addr16 for Mac.
#		define	s6_addr16 __u6_addr.__u6_addr16
#endif // __APPLE__
#endif // WINDOWS_SYS

#include "util/rsnet.h"
#include "util/rsstring.h"
#include "pqi/pqinetwork.h"
#include "util/stacktrace.h"

/***************************** Internal Helper Fns ******************************/

/******************************** Casting  **************************************/
struct sockaddr_in *to_ipv4_ptr(struct sockaddr_storage &addr);
struct sockaddr_in6 *to_ipv6_ptr(struct sockaddr_storage &addr);

const struct sockaddr_in *to_const_ipv4_ptr(const struct sockaddr_storage &addr);
const struct sockaddr_in6 *to_const_ipv6_ptr(const struct sockaddr_storage &addr);


/******************************** Set / Clear ***********************************/

bool sockaddr_storage_ipv4_zeroip(struct sockaddr_storage &addr);
bool sockaddr_storage_ipv4_copyip(struct sockaddr_storage &dst, const struct sockaddr_storage &src);

uint16_t sockaddr_storage_ipv4_port(const struct sockaddr_storage &addr);
bool sockaddr_storage_ipv4_setport(struct sockaddr_storage &addr, uint16_t port);

bool sockaddr_storage_ipv6_zeroip(struct sockaddr_storage &addr);
bool sockaddr_storage_ipv6_copyip(struct sockaddr_storage &dst, const struct sockaddr_storage &src);

uint16_t sockaddr_storage_ipv6_port(const struct sockaddr_storage &addr);
bool sockaddr_storage_ipv6_setport(struct sockaddr_storage &addr, uint16_t port);

/******************************** Comparisions **********************************/

bool sockaddr_storage_ipv4_lessthan(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2);
bool sockaddr_storage_ipv4_same(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2);
bool sockaddr_storage_ipv4_sameip(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2);

bool sockaddr_storage_ipv6_lessthan(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2);
bool sockaddr_storage_ipv6_same(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2);
bool sockaddr_storage_ipv6_sameip(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2);


/********************************* Output     ***********************************/
std::string sockaddr_storage_ipv4_iptostring(const struct sockaddr_storage &addr);
std::string sockaddr_storage_ipv6_iptostring(const struct sockaddr_storage &addr);

/********************************* Net Checks ***********************************/
bool sockaddr_storage_ipv4_isnull(const struct sockaddr_storage &addr);
bool sockaddr_storage_ipv4_isValidNet(const struct sockaddr_storage &addr);
bool sockaddr_storage_ipv4_isLoopbackNet(const struct sockaddr_storage &addr);
bool sockaddr_storage_ipv4_isPrivateNet(const struct sockaddr_storage &addr);
bool sockaddr_storage_ipv4_isExternalNet(const struct sockaddr_storage &addr);

bool sockaddr_storage_ipv6_isnull(const struct sockaddr_storage &addr);
bool sockaddr_storage_ipv6_isValidNet(const struct sockaddr_storage &addr);
bool sockaddr_storage_ipv6_isLoopbackNet(const struct sockaddr_storage &addr);
bool sockaddr_storage_ipv6_isPrivateNet(const struct sockaddr_storage &addr);
bool sockaddr_storage_ipv6_isExternalNet(const struct sockaddr_storage &addr);


/***************************/

/******************************** Socket Fns  ***********************************/
// Standard bind, on OSX anyway will not accept a longer socklen for IPv4.
// so hidding details behind function.
int rs_bind(int fd, const sockaddr_storage& addr)
{
#ifdef SS_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif

	socklen_t len = 0;
	switch (addr.ss_family)
	{
		case AF_INET:
			len = sizeof(struct sockaddr_in);
			break;
		case AF_INET6:
			len = sizeof(struct sockaddr_in6);
			break;
	}

	return bind(fd, reinterpret_cast<const struct sockaddr *>(&addr), len);
}




/******************************** Set / Clear ***********************************/

void sockaddr_storage_clear(struct sockaddr_storage &addr)
{
	memset(&addr, 0, sizeof(addr));
}

// mods.
bool sockaddr_storage_zeroip(struct sockaddr_storage &addr)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_zeroip()";
	std::cerr << std::endl;
#endif

	switch(addr.ss_family)
	{
		case AF_INET:
			return sockaddr_storage_ipv4_zeroip(addr);
			break;
		case AF_INET6:
			return sockaddr_storage_ipv6_zeroip(addr);
			break;
		default:
			std::cerr << "sockaddr_storage_zeroip() invalid addr.ss_family clearing whole address";
			std::cerr << std::endl;
			sockaddr_storage_clear(addr);
			break;
	}
	return false;
}

bool sockaddr_storage_copyip(struct sockaddr_storage &dst, const struct sockaddr_storage &src)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_copyip()" << std::endl;
#endif

	switch(src.ss_family)
	{
		case AF_INET:
			return sockaddr_storage_ipv4_copyip(dst, src);
			break;
		case AF_INET6:
			return sockaddr_storage_ipv6_copyip(dst, src);
			break;
		default:
#ifdef SS_DEBUG
			std::cerr << "sockaddr_storage_copyip() Unknown ss_family: " << src.ss_family << std::endl;
#endif
			break;
	}
	return false;
}


uint16_t sockaddr_storage_port(const struct sockaddr_storage &addr)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_port()" << std::endl;
#endif
	switch(addr.ss_family)
	{
		case AF_INET:
			return sockaddr_storage_ipv4_port(addr);
		case AF_INET6:
			return sockaddr_storage_ipv6_port(addr);
		default:
			std::cerr << "sockaddr_storage_port() invalid addr.ss_family" << std::endl;
#ifdef SS_DEBUG
			sockaddr_storage_dump(addr);
			print_stacktrace();
#endif
			break;
	}
	return 0;
}

bool sockaddr_storage_setport(struct sockaddr_storage &addr, uint16_t port)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_setport()" << std::endl;
#endif

	switch(addr.ss_family)
	{
		case AF_INET:
			return sockaddr_storage_ipv4_setport(addr, port);
			break;
		case AF_INET6:
			return sockaddr_storage_ipv6_setport(addr, port);
			break;
		default:
			std::cerr << "sockaddr_storage_setport() invalid addr.ss_family" << std::endl;
			break;
	}
	return false;
}


bool sockaddr_storage_setipv4(struct sockaddr_storage &addr, const sockaddr_in *addr_ipv4)
{
#ifdef SS_DEBUG
	RS_ERR();
#endif

	sockaddr_storage_clear(addr);
	struct sockaddr_in *ipv4_ptr = to_ipv4_ptr(addr);

	ipv4_ptr->sin_family = AF_INET;
	ipv4_ptr->sin_addr = addr_ipv4->sin_addr;
	ipv4_ptr->sin_port = addr_ipv4->sin_port;

	return true;
}

bool sockaddr_storage_setipv6(struct sockaddr_storage &addr, const sockaddr_in6 *addr_ipv6)
{
#ifdef SS_DEBUG
	RS_ERR();
#endif

	sockaddr_storage_clear(addr);
	struct sockaddr_in6 *ipv6_ptr = to_ipv6_ptr(addr);

	ipv6_ptr->sin6_family = AF_INET6;
	ipv6_ptr->sin6_addr = addr_ipv6->sin6_addr;
	ipv6_ptr->sin6_port = addr_ipv6->sin6_port;

	return true;
}

#ifdef WINDOWS_SYS
#ifndef InetPtonA
int inet_pton(int af, const char *src, void *dst)
{
	sockaddr_storage ss;
	int size = sizeof(ss);
	char src_copy[INET6_ADDRSTRLEN+1];

	ZeroMemory(&ss, sizeof(ss));
	/* stupid non-const API */
	strncpy (src_copy, src, INET6_ADDRSTRLEN+1);
	src_copy[INET6_ADDRSTRLEN] = 0;

	if (WSAStringToAddressA(src_copy, af, NULL, (sockaddr *)&ss, &size) == 0)
	{
		switch(af)
		{
		case AF_INET:
			*(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
			return 1;
		case AF_INET6:
			*(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
			return 1;
		}
	}
	return 0;
}
#endif
#endif

bool sockaddr_storage_inet_pton( sockaddr_storage &addr,
                                 const std::string& ipStr )
{
#ifdef SS_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif

	struct sockaddr_in6 * addrv6p = (struct sockaddr_in6 *) &addr;
	struct sockaddr_in * addrv4p = (struct sockaddr_in *) &addr;

	if ( 1 == inet_pton(AF_INET6, ipStr.c_str(), &(addrv6p->sin6_addr)) )
	{
		addr.ss_family = AF_INET6;
		return true;
	}
	else if ( 1 == inet_pton(AF_INET, ipStr.c_str(), &(addrv4p->sin_addr)) )
	{
		addr.ss_family = AF_INET;
		return sockaddr_storage_ipv4_to_ipv6(addr);
	}

	return false;
}


bool sockaddr_storage_ipv4_aton(struct sockaddr_storage &addr, const char *name)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_ipv4_aton()";
	std::cerr << std::endl;
#endif

	struct sockaddr_in *ipv4_ptr = to_ipv4_ptr(addr);
	ipv4_ptr->sin_family = AF_INET;
	return (1 == inet_aton(name, &(ipv4_ptr->sin_addr)));
}

bool sockaddr_storage_ipv4_to_ipv6(sockaddr_storage &addr)
{
#ifdef SS_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif

	if ( addr.ss_family == AF_INET6 ) return true;

	if ( addr.ss_family == AF_INET )
	{
		sockaddr_in & addr_ipv4 = (sockaddr_in &) addr;
		sockaddr_in6 & addr_ipv6 = (sockaddr_in6 &) addr;

		uint32_t ip = addr_ipv4.sin_addr.s_addr;
		uint16_t port = addr_ipv4.sin_port;

		sockaddr_storage_clear(addr);
		addr_ipv6.sin6_family = AF_INET6;
		addr_ipv6.sin6_port = port;
		addr_ipv6.sin6_addr.s6_addr16[5] = htons(0xffff);
		memmove( reinterpret_cast<void*>(&(addr_ipv6.sin6_addr.s6_addr16[6])),
		         reinterpret_cast<void*>(&ip), 4 );
		return true;
	}

	return false;
}

bool sockaddr_storage_ipv6_to_ipv4(sockaddr_storage &addr)
{
#ifdef SS_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif

	if ( addr.ss_family == AF_INET ) return true;

	if ( addr.ss_family == AF_INET6 )
	{
		sockaddr_in6 & addr_ipv6 =  (sockaddr_in6 &) addr;
		bool ipv4m = addr_ipv6.sin6_addr.s6_addr16[5] == htons(0xffff);
		for ( int i = 0; ipv4m && i < 5 ; ++i )
			ipv4m &= addr_ipv6.sin6_addr.s6_addr16[i] == htons(0x0000);

		if(ipv4m)
		{
			uint32_t ip;
			memmove( reinterpret_cast<void*>(&ip),
			         reinterpret_cast<void*>(&(addr_ipv6.sin6_addr.s6_addr16[6])),
			         4 );
			uint16_t port = addr_ipv6.sin6_port;

			sockaddr_in & addr_ipv4 = (sockaddr_in &) addr;

			sockaddr_storage_clear(addr);
			addr_ipv4.sin_family = AF_INET;
			addr_ipv4.sin_port = port;
			addr_ipv4.sin_addr.s_addr = ip;

			return true;
		}
	}

	return false;
}


/******************************** Comparisions **********************************/

bool operator<(const struct sockaddr_storage &a, const struct sockaddr_storage &b)
{
	if (!sockaddr_storage_samefamily(a, b))
	{
		return (a.ss_family < b.ss_family);
	}

	switch(a.ss_family)
	{
		case AF_INET:
			return sockaddr_storage_ipv4_lessthan(a, b);
			break;
		case AF_INET6:
			return sockaddr_storage_ipv6_lessthan(a, b);
			break;
		default:
#ifdef SS_DEBUG
            std::cerr << "sockaddr_storage_operator<() INVALID Family - error";
            std::cerr << std::endl;
#endif
			break;
	}
	return false;
}


bool sockaddr_storage_same(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_same()";
	std::cerr << std::endl;
#endif

	if (!sockaddr_storage_samefamily(addr, addr2))
		return false;

	switch(addr.ss_family)
	{
		case AF_INET:
			return sockaddr_storage_ipv4_same(addr, addr2);
			break;
		case AF_INET6:
			return sockaddr_storage_ipv6_same(addr, addr2);
			break;
		default:
#ifdef SS_DEBUG
            std::cerr << "sockaddr_storage_same() INVALID Family - error";
            std::cerr << std::endl;
#endif
			break;
	}
	return false;
}


bool sockaddr_storage_samefamily(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_samefamily()";
	std::cerr << std::endl;
#endif

	return (addr.ss_family == addr2.ss_family);
}

bool sockaddr_storage_sameip(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_sameip()";
	std::cerr << std::endl;
#endif

	if (!sockaddr_storage_samefamily(addr, addr2))
		return false;

	switch(addr.ss_family)
	{
		case AF_INET:
			return sockaddr_storage_ipv4_sameip(addr, addr2);
			break;
		case AF_INET6:
			return sockaddr_storage_ipv6_sameip(addr, addr2);
			break;
		default:
#ifdef SS_DEBUG
            std::cerr << "sockaddr_storage_sameip() INVALID Family - error";
            std::cerr << std::endl;
#endif
			break;
	}
	return false;
}

/********************************* Output     ***********************************/

std::string sockaddr_storage_tostring(const struct sockaddr_storage &addr)
{
	RsUrl url;

	switch(addr.ss_family)
	{
	case AF_INET:
		url.setScheme("ipv4");
		break;
	case AF_INET6:
		url.setScheme("ipv6");
		break;
	default:
		return "AF_INVALID";
	}

	url.setHost(sockaddr_storage_iptostring(addr))
	   .setPort(sockaddr_storage_port(addr));

	return url.toString();
}

bool sockaddr_storage_fromString(const std::string& str, sockaddr_storage &addr)
{
	RsUrl url(str);
	bool valid = sockaddr_storage_inet_pton(addr, url.host());
	if(url.hasPort()) sockaddr_storage_setport(addr, url.port());
	return valid;
}

void sockaddr_storage_dump(const sockaddr_storage & addr, std::string * outputString)
{
	// This function must not rely on others sockaddr_storage_*

	std::stringstream output;
	output << "sockaddr_storage_dump(addr) ";

	switch (addr.ss_family)
	{
	case AF_INET:
	{
		const sockaddr_in * in = (const sockaddr_in *) & addr;
		output << "addr.ss_family = AF_INET";
		output << " in->sin_addr = ";
		output << inet_ntoa(in->sin_addr);
		output << " in->sin_port = ";
		output << in->sin_port;
		break;
	}
	case AF_INET6:
	{
		const sockaddr_in6 * in6 = (const sockaddr_in6 *) & addr;
		std::string addrStr = "INVALID_IPV6";
		sockaddr_storage_inet_ntop(addr, addrStr);
		output << "addr.ss_family = AF_INET6";
		output << " in6->sin6_addr = ";
		output << addrStr;
		output << " in6->sin6_scope_id = ";
		output << in6->sin6_scope_id;
		output << " in6->sin6_port = ";
		output << in6->sin6_port;
		break;
	}
	default:
	{
		output << "unknown addr.ss_family ";
		const uint8_t * buf = reinterpret_cast<const uint8_t *>(&addr);
		for( uint32_t i = 0; i < sizeof(addr); ++i )
			output << std::setw(2) << std::setfill('0') << std::hex << +buf[i];
		/* The unary +buf[i] operation forces a no-op type conversion to an int
		 * with the correct sign */
	}
	}

	if(outputString)
	{
		outputString->append(output.str() + "\n");
#ifdef SS_DEBUG
		std::cerr << output.str() << std::endl;
#endif
	}
	else
		std::cerr << output.str() << std::endl;
}

std::string sockaddr_storage_familytostring(const struct sockaddr_storage &addr)
{
	std::string output;
	switch(addr.ss_family)
	{
		case AF_INET:
			output = "IPv4";
			break;
		case AF_INET6:
			output = "IPv6";
			break;
		default:
			output = "AF_INVALID";
#ifdef SS_DEBUG
			std::cerr << __PRETTY_FUNCTION__ << " Got invalid address!"
			          << std::endl;
			sockaddr_storage_dump(addr);
			print_stacktrace();
#endif
			break;
	}
	return output;
}

std::string sockaddr_storage_iptostring(const struct sockaddr_storage &addr)
{
	std::string output;
	switch(addr.ss_family)
	{
	case AF_INET:
		output = sockaddr_storage_ipv4_iptostring(addr);
		break;
	case AF_INET6:
		output = sockaddr_storage_ipv6_iptostring(addr);
		break;
	default:
		output = "INVALID_IP";
		std::cerr << __PRETTY_FUNCTION__ << " Got invalid IP!" << std::endl;
#ifdef SS_DEBUG
		sockaddr_storage_dump(addr);
		print_stacktrace();
#endif
		break;
	}
	return output;
}

std::string sockaddr_storage_porttostring(const struct sockaddr_storage &addr)
{
	std::string output;
	uint16_t port = sockaddr_storage_port(addr);
	rs_sprintf(output, "%u", port);
	return output;
}


/********************************* Net Checks ***********************************/
bool sockaddr_storage_isnull(const struct sockaddr_storage &addr)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_isnull()";
	std::cerr << std::endl;
#endif

	if (addr.ss_family == 0)
		return true;

	switch(addr.ss_family)
	{
		case AF_INET:
			return sockaddr_storage_ipv4_isnull(addr);
			break;
		case AF_INET6:
			return sockaddr_storage_ipv6_isnull(addr);
			break;
		default:
			return true;
			break;
	}
	return true;
}

bool sockaddr_storage_isValidNet(const struct sockaddr_storage &addr)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_isValidNet()" << std::endl;
#endif

	switch(addr.ss_family)
	{
		case AF_INET:
			return sockaddr_storage_ipv4_isValidNet(addr);
		case AF_INET6:
			return sockaddr_storage_ipv6_isValidNet(addr);
		default:
#ifdef SS_DEBUG
			std::cerr << "sockaddr_storage_isValidNet() INVALID Family" << std::endl;
			sockaddr_storage_dump(addr);
#endif
			break;
	}
	return false;
}

bool sockaddr_storage_isLoopbackNet(const struct sockaddr_storage &addr)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_isLoopbackNet()";
	std::cerr << std::endl;
#endif

	switch(addr.ss_family)
	{
		case AF_INET:
			return sockaddr_storage_ipv4_isLoopbackNet(addr);
			break;
		case AF_INET6:
			return sockaddr_storage_ipv6_isLoopbackNet(addr);
			break;
		default:
#ifdef SS_DEBUG
            std::cerr << "sockaddr_storage_isLoopbackNet() INVALID Family - error: " << sockaddr_storage_iptostring(addr);
            std::cerr << std::endl;
#endif
			break;
	}
	return false;
}


bool sockaddr_storage_isPrivateNet(const struct sockaddr_storage &addr)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_isPrivateNet()";
	std::cerr << std::endl;
#endif

	switch(addr.ss_family)
	{
		case AF_INET:
			return sockaddr_storage_ipv4_isPrivateNet(addr);
			break;
		case AF_INET6:
			return sockaddr_storage_ipv6_isPrivateNet(addr);
			break;
		default:
#ifdef SS_DEBUG
            std::cerr << "sockaddr_storage_isPrivateNet() INVALID Family - error: " << sockaddr_storage_iptostring(addr);
            std::cerr << std::endl;
#endif
			break;
	}
	return false;
}

bool sockaddr_storage_isLinkLocalNet(const struct sockaddr_storage &addr)
{
#ifdef SS_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif

	switch(addr.ss_family)
	{
	case AF_INET:
		return isLinkLocalNet(&(to_const_ipv4_ptr(addr)->sin_addr));
	case AF_INET6:
		return sockaddr_storage_ipv6_isLinkLocalNet(addr);
	default:
#ifdef SS_DEBUG
		std::cerr << __PRETTY_FUNCTION__ <<" INVALID Family:" << std::endl;
		sockaddr_storage_dump(addr);
#endif
		break;
	}

	return false;
}

bool sockaddr_storage_ipv6_isLinkLocalNet(const sockaddr_storage &addr)
{
	if(addr.ss_family != AF_INET6) return false;

	const sockaddr_in6 * addr6 = (const sockaddr_in6 *) &addr;
	uint16_t mask = htons(0xffc0);
	uint16_t llPrefix = htons(0xfe80);
	return ((addr6->sin6_addr.s6_addr16[0] & mask ) == llPrefix);
}


bool sockaddr_storage_isExternalNet(const struct sockaddr_storage &addr)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_isExternalNet()";
	std::cerr << std::endl;
#endif

	switch(addr.ss_family)
	{
		case AF_INET:
			return sockaddr_storage_ipv4_isExternalNet(addr);
			break;
		case AF_INET6:
			return sockaddr_storage_ipv6_isExternalNet(addr);
			break;
		default:
#ifdef SS_DEBUG
            std::cerr << "sockaddr_storage_isExternalNet() INVALID Family - error";
            std::cerr << std::endl;
#endif
			break;
	}
	return false;
}





/***************************** Internal Helper Fns ******************************/


/******************************** Casting  **************************************/

struct sockaddr_in *to_ipv4_ptr(struct sockaddr_storage &addr)
{
	struct sockaddr_in *ipv4_ptr = (struct sockaddr_in *) &addr;
	return ipv4_ptr;
}

struct sockaddr_in6 *to_ipv6_ptr(struct sockaddr_storage &addr)
{
	struct sockaddr_in6 *ipv6_ptr = (struct sockaddr_in6 *) &addr;
	return ipv6_ptr;
}

const struct sockaddr_in *to_const_ipv4_ptr(const struct sockaddr_storage &addr)
{
	const struct sockaddr_in *ipv4_ptr = (const struct sockaddr_in *) &addr;
	return ipv4_ptr;
}

const struct sockaddr_in6 *to_const_ipv6_ptr(const struct sockaddr_storage &addr)
{
	const struct sockaddr_in6 *ipv6_ptr = (const struct sockaddr_in6 *) &addr;
	return ipv6_ptr;
}


/******************************** Set / Clear ***********************************/

bool sockaddr_storage_copy(const sockaddr_storage& src, sockaddr_storage& dst)
{
	if(&src == &dst) return true;

	switch(src.ss_family)
	{
	case AF_INET:
	{
		sockaddr_storage_clear(dst);
		const sockaddr_in& ins(reinterpret_cast<const sockaddr_in&>(src));
		sockaddr_in& ind(reinterpret_cast<sockaddr_in&>(dst));

		ind.sin_family = AF_INET;
		ind.sin_addr.s_addr = ins.sin_addr.s_addr;
		ind.sin_port = ins.sin_port;

		return true;
	}
	case AF_INET6:
	{
		sockaddr_storage_clear(dst);
		const sockaddr_in6& ins6(reinterpret_cast<const sockaddr_in6&>(src));
		sockaddr_in6& ind6(reinterpret_cast<sockaddr_in6&>(dst));

		ind6.sin6_family = AF_INET6;
		for(int i=0; i<8; ++i)
			ind6.sin6_addr.s6_addr16[i] = ins6.sin6_addr.s6_addr16[i];
		ind6.sin6_flowinfo = ins6.sin6_flowinfo;
		ind6.sin6_port = ins6.sin6_port;
		ind6.sin6_scope_id = ins6.sin6_scope_id;

		return true;
	}
	default:
#ifdef SS_DEBUG
		std::cerr << __PRETTY_FUNCTION__ << " Attempt to copy unknown family! "
		          << src.ss_family << " defaulting to memmove!" << std::endl;
		sockaddr_storage_dump(src);
		print_stacktrace();
#endif // SS_DEBUG
		memmove(&dst, &src, sizeof(sockaddr_storage));
		return true;
	}
}

bool sockaddr_storage_ipv4_zeroip(struct sockaddr_storage &addr)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_ipv4_zeroip()";
	std::cerr << std::endl;
#endif

	struct sockaddr_in *ipv4_ptr = to_ipv4_ptr(addr);
	memset(&(ipv4_ptr->sin_addr), 0, sizeof(ipv4_ptr->sin_addr));
	return true;
}


bool sockaddr_storage_ipv4_copyip(struct sockaddr_storage &dst, const struct sockaddr_storage &src)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_ipv4_copyip()";
	std::cerr << std::endl;
#endif

	struct sockaddr_in *dst_ptr = to_ipv4_ptr(dst);
	const struct sockaddr_in *src_ptr = to_const_ipv4_ptr(src);

	dst_ptr->sin_family = AF_INET;
	memcpy(&(dst_ptr->sin_addr), &(src_ptr->sin_addr), sizeof(src_ptr->sin_addr));
	return true;
}

uint16_t sockaddr_storage_ipv4_port(const struct sockaddr_storage &addr)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_ipv4_port()";
	std::cerr << std::endl;
#endif

	const struct sockaddr_in *ipv4_ptr = to_const_ipv4_ptr(addr);
	uint16_t port = ntohs(ipv4_ptr->sin_port);
	return port;
}

bool sockaddr_storage_ipv4_setport(struct sockaddr_storage &addr, uint16_t port)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_ipv4_setport()";
	std::cerr << std::endl;
#endif

	struct sockaddr_in *ipv4_ptr = to_ipv4_ptr(addr);
	ipv4_ptr->sin_port = htons(port);
	return true;
}

bool sockaddr_storage_ipv6_zeroip(struct sockaddr_storage &addr)
{
#ifdef SS_DEBUG
    std::cerr << "sockaddr_storage_ipv6_zeroip()";
    std::cerr << std::endl;
#endif

	struct sockaddr_in6 *ipv6_ptr = to_ipv6_ptr(addr);
	memset(&(ipv6_ptr->sin6_addr), 0, sizeof(ipv6_ptr->sin6_addr));
	return true;
}

bool sockaddr_storage_ipv6_copyip(struct sockaddr_storage &dst, const struct sockaddr_storage &src)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_ipv6_copyip()" << std::endl;
#endif

	struct sockaddr_in6 *dst_ptr = to_ipv6_ptr(dst);
	const struct sockaddr_in6 *src_ptr = to_const_ipv6_ptr(src);

	dst_ptr->sin6_family = AF_INET6;
	memcpy(&(dst_ptr->sin6_addr), &(src_ptr->sin6_addr), sizeof(src_ptr->sin6_addr));
	return true;
}

uint16_t sockaddr_storage_ipv6_port(const struct sockaddr_storage &addr)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_ipv6_port()";
    std::cerr << std::endl;
#endif

	const struct sockaddr_in6 *ipv6_ptr = to_const_ipv6_ptr(addr);
	uint16_t port = ntohs(ipv6_ptr->sin6_port);
	return port;
}

bool sockaddr_storage_ipv6_setport(struct sockaddr_storage &addr, uint16_t port)
{
#ifdef SS_DEBUG
    std::cerr << "sockaddr_storage_ipv6_setport()";
    std::cerr << std::endl;
#endif

	struct sockaddr_in6 *ipv6_ptr = to_ipv6_ptr(addr);
	ipv6_ptr->sin6_port = htons(port);
	return true;
}


/******************************** Comparisions **********************************/

bool sockaddr_storage_ipv4_lessthan(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_ipv4_lessthan()";
	std::cerr << std::endl;
#endif

	const struct sockaddr_in *ptr1 = to_const_ipv4_ptr(addr);
	const struct sockaddr_in *ptr2 = to_const_ipv4_ptr(addr2);

	if (ptr1->sin_addr.s_addr == ptr2->sin_addr.s_addr) 
	{
		return	ptr1->sin_port < ptr2->sin_port;
	}
	return (ptr1->sin_addr.s_addr < ptr2->sin_addr.s_addr);
}

bool sockaddr_storage_ipv4_same(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_ipv4_same()";
	std::cerr << std::endl;
#endif

	const struct sockaddr_in *ptr1 = to_const_ipv4_ptr(addr);
	const struct sockaddr_in *ptr2 = to_const_ipv4_ptr(addr2);

	return (ptr1->sin_addr.s_addr == ptr2->sin_addr.s_addr) &&
		(ptr1->sin_port == ptr2->sin_port);
}

bool sockaddr_storage_ipv4_sameip(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_ipv4_sameip()";
	std::cerr << std::endl;
#endif

	const struct sockaddr_in *ptr1 = to_const_ipv4_ptr(addr);
	const struct sockaddr_in *ptr2 = to_const_ipv4_ptr(addr2);

	return (ptr1->sin_addr.s_addr == ptr2->sin_addr.s_addr);
}

// IPV6
bool sockaddr_storage_ipv6_lessthan(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2)
{
#ifdef SS_DEBUG
    std::cerr << "sockaddr_storage_ipv6_lessthan()";
    std::cerr << std::endl;
#endif

	const struct sockaddr_in6 *ptr1 = to_const_ipv6_ptr(addr);
	const struct sockaddr_in6 *ptr2 = to_const_ipv6_ptr(addr2);

        uint32_t *ip6addr1 = (uint32_t *) ptr1->sin6_addr.s6_addr;
        uint32_t *ip6addr2 = (uint32_t *) ptr2->sin6_addr.s6_addr;
        for(int i = 0; i < 4; i++)
        {
		if (ip6addr1[i] == ip6addr2[i])
		{
			continue;
		}

		return (ip6addr1[i] < ip6addr2[i]);
        }

	return (ptr1->sin6_port < ptr2->sin6_port);
}

bool sockaddr_storage_ipv6_same(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2)
{
#ifdef SS_DEBUG
    std::cerr << "sockaddr_storage_ipv6_same()";
    std::cerr << std::endl;
#endif
	const struct sockaddr_in6 *ptr1 = to_const_ipv6_ptr(addr);
	const struct sockaddr_in6 *ptr2 = to_const_ipv6_ptr(addr2);

	return sockaddr_storage_ipv6_sameip(addr, addr2) && (ptr1->sin6_port == ptr2->sin6_port);
}

bool sockaddr_storage_ipv6_sameip(const struct sockaddr_storage &addr, const struct sockaddr_storage &addr2)
{
#ifdef SS_DEBUG
    std::cerr << "sockaddr_storage_ipv6_sameip()";
    std::cerr << std::endl;
#endif

	const struct sockaddr_in6 *ptr1 = to_const_ipv6_ptr(addr);
	const struct sockaddr_in6 *ptr2 = to_const_ipv6_ptr(addr2);

        uint32_t *ip6addr1 = (uint32_t *) ptr1->sin6_addr.s6_addr;
        uint32_t *ip6addr2 = (uint32_t *) ptr2->sin6_addr.s6_addr;

        for(int i = 0; i < 4; i++)
        {
		if (ip6addr1[i] != ip6addr2[i])
		{
			return false;
		}
	}
	return true;
}

/********************************* Output     ***********************************/
std::string sockaddr_storage_ipv4_iptostring(const struct sockaddr_storage &addr)
{
	const struct sockaddr_in *ptr = to_const_ipv4_ptr(addr);
	std::string output;
	output = rs_inet_ntoa(ptr->sin_addr);
	return output;
}

std::string sockaddr_storage_ipv6_iptostring(const struct sockaddr_storage & addr)
{
	std::string addrStr;
	sockaddr_storage_inet_ntop(addr, addrStr);
	return addrStr;
}


/********************************* Net Checks ***********************************/
bool sockaddr_storage_ipv4_isnull(const struct sockaddr_storage &addr)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_ipv4_isnull()";
	std::cerr << std::endl;
#endif

	const struct sockaddr_in *ptr1 = to_const_ipv4_ptr(addr);
	if (ptr1->sin_family != AF_INET)
	{
		return true;
	}
	if ((ptr1->sin_addr.s_addr == 0) || (ptr1->sin_addr.s_addr == 1))
	{
		return true;
	}
	return false;
}

bool sockaddr_storage_ipv4_isValidNet(const struct sockaddr_storage &addr)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_ipv4_isValidNet()";
	std::cerr << std::endl;
#endif

	const struct sockaddr_in *ptr1 = to_const_ipv4_ptr(addr);
	if (ptr1->sin_family != AF_INET)
	{
		return false;
	}
	return isValidNet(&(ptr1->sin_addr));
}


bool sockaddr_storage_ipv4_isLoopbackNet(const struct sockaddr_storage &addr)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_ipv4_isLoopbackNet()";
	std::cerr << std::endl;
#endif


	const struct sockaddr_in *ptr1 = to_const_ipv4_ptr(addr);
	if (ptr1->sin_family != AF_INET)
	{
		return false;
	}
	return isLoopbackNet(&(ptr1->sin_addr));
}

bool sockaddr_storage_ipv4_isPrivateNet(const struct sockaddr_storage &addr)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_ipv4_isPrivateNet()";
	std::cerr << std::endl;
#endif


	const struct sockaddr_in *ptr1 = to_const_ipv4_ptr(addr);
	if (ptr1->sin_family != AF_INET)
	{
		return false;
	}
	return isPrivateNet(&(ptr1->sin_addr));
}

bool sockaddr_storage_ipv4_isExternalNet(const struct sockaddr_storage &addr)
{
#ifdef SS_DEBUG
	std::cerr << "sockaddr_storage_ipv4_isExternalNet()";
	std::cerr << std::endl;
#endif

	const struct sockaddr_in *ptr1 = to_const_ipv4_ptr(addr);
	if (ptr1->sin_family != AF_INET)
	{
		return false;
	}
	return isExternalNet(&(ptr1->sin_addr));
}


bool sockaddr_storage_ipv6_isnull(const struct sockaddr_storage& addr)
{
	const sockaddr_in6& addr6 = reinterpret_cast<const sockaddr_in6&>(addr);

	uint16_t nZero = htons(0); // anyway 0 should be the same in host and net
	bool isZero = (addr6.sin6_addr.s6_addr16[7] == nZero);
	for (int i=0; isZero && i<7; ++i)
		isZero &= (addr6.sin6_addr.s6_addr16[i] == nZero);

	return nZero;
}

bool sockaddr_storage_ipv6_isValidNet(const struct sockaddr_storage& addr)
{
	return !sockaddr_storage_ipv6_isnull(addr);
}

bool sockaddr_storage_ipv6_isLoopbackNet(const struct sockaddr_storage& addr)
{
	const sockaddr_in6& addr6 = reinterpret_cast<const sockaddr_in6&>(addr);
	bool isLoopBack = (addr6.sin6_addr.s6_addr16[7] == htons(0x0001));
	uint16_t nZero = htons(0); // anyway 0 should be the same in host and net
	for (int i=0; isLoopBack && i<7; ++i)
		isLoopBack &= (addr6.sin6_addr.s6_addr16[i] == nZero);

#ifdef SS_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << " " << sockaddr_storage_tostring(addr)
	          << " " << isLoopBack << std::endl;
#endif

	return isLoopBack;
}

bool sockaddr_storage_ipv6_isPrivateNet(const struct sockaddr_storage &/*addr*/)
{
	/* It is unlikely that we end up connecting to an IPv6 address behind NAT
	 * W.R.T. RS it is probably better to consider all IPv6 as internal/local
	 * addresses as direct connection should be always possible. */

	return true;
}

bool sockaddr_storage_ipv6_isExternalNet(const struct sockaddr_storage &/*addr*/)
{
	/* It is unlikely that we end up connecting to an IPv6 address behind NAT
	 * W.R.T. RS it is probably better to consider all IPv6 as internal/local
	 * addresses as direct connection should be always possible. */

	return false;
}

bool sockaddr_storage_inet_ntop (const sockaddr_storage &addr, std::string &dst)
{
	bool success = false;
	char ipStr[255];

#ifdef WINDOWS_SYS
	// Use WSAAddressToString instead of InetNtop because the latter is missing
	// on XP and is present only on Vista and newers
	wchar_t wIpStr[255];
	long unsigned int len = 255;
	sockaddr_storage tmp;
	sockaddr_storage_clear(tmp);
	sockaddr_storage_copyip(tmp, addr);
	sockaddr * sptr = (sockaddr *) &tmp;
	success = (0 == WSAAddressToString( sptr, sizeof(sockaddr_storage), NULL, wIpStr, &len ));
	wcstombs(ipStr, wIpStr, len);
#else // WINDOWS_SYS
	switch(addr.ss_family)
	{
	case AF_INET:
	{
		const struct sockaddr_in * addrv4p = (const struct sockaddr_in *) &addr;
		success = inet_ntop( addr.ss_family, (const void *) &(addrv4p->sin_addr), ipStr, INET_ADDRSTRLEN );
	}
	break;
	case AF_INET6:
	{
		const struct sockaddr_in6 * addrv6p = (const struct sockaddr_in6 *) &addr;
		success = inet_ntop( addr.ss_family, (const void *) &(addrv6p->sin6_addr), ipStr, INET6_ADDRSTRLEN );
	}
	break;
	}
#endif // WINDOWS_SYS

	dst = ipStr;
	return success;
}

int rs_setsockopt( int sockfd, int level, int optname,
                   const uint8_t *optval, uint32_t optlen )
{
#ifdef WINDOWS_SYS
	return setsockopt( sockfd, level, optname,
	                   reinterpret_cast<const char*>(optval), optlen );
#else
	return setsockopt( sockfd, level, optname,
	                   reinterpret_cast<const void*>(optval), optlen );
#endif // WINDOWS_SYS
}
