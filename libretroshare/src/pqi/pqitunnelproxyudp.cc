/*
 * "$Id: pqitunnelproxyudp.cc,v 1.3 2007-02-18 21:46:50 rmf24 Exp $"
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





#include "pqi/pqitunnelproxyudp.h"

PQProxyUdp::PQProxyUdp()
	:type(0), ksize(0), key(NULL), connectMode(0)
	{
		return;
	}

PQProxyUdp::PQProxyUdp(void *data, int n)
	:ksize(0), key(NULL), connectMode(0)
	{
		in(data, n);
	}

	// construct from info.
PQProxyUdp::PQProxyUdp(struct sockaddr_in *proxyaddr, void *ikey, int n)
	:type(PQI_TPUDP_TYPE_1), paddr(*proxyaddr), ksize(n), key(NULL),
	connectMode(0)

	{
		key = malloc(ksize);
		memcpy(key, ikey, ksize);
	}

PQProxyUdp::PQProxyUdp(struct sockaddr_in *neighaddr, 
			struct sockaddr_in *peeraddr, int cMode)
	:type(PQI_TPUDP_TYPE_2), 
		naddr(*neighaddr), 
		paddr(*peeraddr), 
		ksize(0), key(NULL), connectMode(cMode)
	{
		return;
	}
	
PQProxyUdp::~PQProxyUdp()
{
	if (key)
	{
		ksize = 0;
		free(key);
	}
}

const int PQProxyUdp::getSize() const
{
	return sizeof(int) + 2 * sizeof(struct sockaddr_in) 
		+ sizeof(int) + ksize;
}

int PQProxyUdp::out(void *dta, const int n) const // write
{
	if (n < getSize())
		return -1;

	char *loc = (char *) dta; // so we can count bytes
	int cs = sizeof(int);
	memcpy(loc, &type, cs);
	loc += cs;
	cs = sizeof(struct sockaddr_in);
	memcpy(loc, &naddr, cs);
	loc += cs;
	memcpy(loc, &paddr, cs);
	loc += cs;

	/* add in ConnectMode */
	cs = sizeof(int);
	memcpy(loc, &connectMode, cs);
	loc += cs;

	if (ksize)
	{
		memcpy(loc, key, ksize);
	}
	loc += ksize;

	return getSize();
}

int PQProxyUdp::in(const void *dta, const int n) // read,
{
	if (key)
	{
		ksize = 0;
		free(key);
		key = NULL;
	}

	if (n < getSize()) /* smaller than min ksize */
	{
		std::cerr << "PQTunnelProxy::in() Failed (n < getSize())" << std::endl;
		return -1;
	}

	char *loc = (char *) dta;
	int cs = sizeof(int);
	memcpy(&type, loc, cs);
	loc += cs;
	cs = sizeof(struct sockaddr_in);
	memcpy(&naddr, loc, cs);
	loc += cs;
	memcpy(&paddr, loc, cs);
	loc += cs;

	/* get in ConnectMode */
	cs = sizeof(int);
	memcpy(&connectMode,loc, cs);
	loc += cs;

	ksize = 0;
	ksize = n - getSize();
	if (ksize)
	{
		key = malloc(ksize);
		memcpy(key, loc, ksize);
	}
	return getSize(); /* should be equal to n */
}


std::ostream &PQProxyUdp::print(std::ostream &out)
{
	out << "-------- PQProxyUdp" << std::endl;
	out << "Type: " << type << std::endl;
	out << "naddr: " << inet_ntoa(naddr.sin_addr) << ":";
	out <<  ntohs(naddr.sin_port)  << std::endl;
	out << "paddr: " << inet_ntoa(paddr.sin_addr) << ":";
	out <<  ntohs(paddr.sin_port)  << std::endl;
	out << "ConnectMode: " << connectMode << std::endl;
	out << "KeySize: " << ksize << std::endl;
	return out << "-------- PQProxyUdp" << std::endl;
}


