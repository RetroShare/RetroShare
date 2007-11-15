/*
 * "$Id: pqitunnelproxyudp.h,v 1.3 2007-02-18 21:46:50 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
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



#ifndef PQI_TUNNEL_PROXY_UDP_HEADER
#define PQI_TUNNEL_PROXY_UDP_HEADER

#include "pqi/pqitunnel.h"

#define PQI_TPUDP_TYPE_1 0x0001
#define PQI_TPUDP_TYPE_2 0x0002

class PQProxyUdp
{
public:
	// construct from data.
	PQProxyUdp(void *data, int size);
	PQProxyUdp();

	// construct from info.
	PQProxyUdp(struct sockaddr_in *proxyaddr, void *key, int size);
	PQProxyUdp(struct sockaddr_in *neighaddr, 
			struct sockaddr_in *peeraddr, int cMode);
	
virtual	~PQProxyUdp();

	// no copy functions.

virtual const int getSize() const;
virtual int out(void *data, const int size) const; // write
virtual int in(const void *data, const int size); // read,

std::ostream &print(std::ostream &out);

	int type;
	struct sockaddr_in naddr, paddr;
	int     ksize;
	void    *key;
	int     connectMode;
};

#endif // PQI_TUNNEL_PROXY_UDP_HEADER
