/*
 * "$Id: pqitunnelproxy.h,v 1.3 2007-02-18 21:46:50 rmf24 Exp $"
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



#ifndef PQI_TUNNEL_PROXY_HEADER
#define PQI_TUNNEL_PROXY_HEADER

#include "pqi/pqitunnel.h"

#define PQI_TUNNEL_PROXY_TYPE 0x0024

#define PQI_TUNNEL_SIGN_SIZE 256
#define PQI_TUNNEL_SHRT_SIGN_SIZE 16
#define PQI_PROXY_CHALLENGE_SIZE 1024

// mode types...
#define PQI_TI_REQUEST   0x01
#define PQI_TI_CONNECT   0x02
#define PQI_TI_END   0x03

PQTunnel *createPQTunnelProxy(void *d, int n);
PQTunnelInit *createPQTunnelProxyInit(void *d, int n);
// defined at the end of header.

class PQTunnelProxyInit: public PQTunnelInit
{
	// private copy constructor... prevent copying...
	PQTunnelProxyInit(const PQTunnelProxyInit &) 
	:PQTunnelInit(PQI_TUNNEL_PROXY_TYPE)
	{return; }

public:
	PQTunnelProxyInit(); 
virtual	~PQTunnelProxyInit();

	// copy functions.
virtual PQTunnelProxyInit *clone();
void	copy(const PQTunnelProxyInit *src);

	// These are overloaded from PQTunnelInit.
virtual const int getSize() const;
virtual int out(void *data, const int size) const; // write
virtual int in(const void *data, const int size); // read,

std::ostream &print(std::ostream &out);

	long seq;

	int mode;

	// source
	certsign src;
	certsign proxy;
	certsign dest;

	unsigned char challenge[PQI_PROXY_CHALLENGE_SIZE];
	unsigned char sign[PQI_TUNNEL_SIGN_SIZE];

};

// Data Via Proxy.

class PQTunnelProxy: public PQTunnel
{
	// private copy constructor... prevent copying...
	PQTunnelProxy(const PQTunnelProxy &) 
	:PQTunnel(PQI_TUNNEL_PROXY_TYPE)
	{return; }

public:
	PQTunnelProxy(); 
virtual	~PQTunnelProxy();

	// copy functions.
virtual PQTunnelProxy *clone();
void	copy(const PQTunnelProxy *src);

virtual const int getSize() const;
virtual int out(void *data, const int size) const; // write
virtual int in(const void *data, const int size); // read,

std::ostream &print(std::ostream &out);

//private:
	// full redefinition of the data space.
	
	long seq;

	certsign src, dest;

	int     size;
	void    *data;

	unsigned char sign[PQI_TUNNEL_SIGN_SIZE];
};

#endif // PQI_TUNNEL_HEADER
