/*
 * "$Id: pqitunnelproxy.cc,v 1.4 2007-02-18 21:46:50 rmf24 Exp $"
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




#include "pqi/pqitunnelproxy.h"

PQTunnel *createPQTunnelProxy(void *d, int n)
{
	        return new PQTunnelProxy();
}


PQTunnelInit *createPQTunnelProxyInit(void *d, int n)
{
	        return new PQTunnelProxyInit();
}


PQTunnelProxy::PQTunnelProxy()
:PQTunnel(PQI_TUNNEL_PROXY_TYPE), seq(0), size(0), data(NULL)
{
	return;
}

PQTunnelProxy::~PQTunnelProxy()
{
	if (data)
		free(data);
}


	// copy functions.
PQTunnelProxy *PQTunnelProxy::clone()
{
	PQTunnelProxy *item = new PQTunnelProxy();
	item -> copy(this);
	return item;
}



void	PQTunnelProxy::copy(const PQTunnelProxy *item)
{
	// copy parents.
	PQTunnel::copy(item);

	if (data)
		free(data);
	size = 0;
	size = item -> PQTunnelProxy::getSize() - PQTunnelProxy::getSize();
	data = malloc(size);
			
	seq = item -> seq;
	src = item -> src;
	dest = item -> dest;
	//proxy = item -> proxy;

	memcpy(data, item -> data, size);
	memcpy(sign, item -> sign, PQI_TUNNEL_SIGN_SIZE);
}


int PQTunnelProxy::out(void *dta, const int n) const
{
	//std::cerr << "PQTunnelProxy::out()" << std::endl;
	if (n < PQTunnelProxy::getSize())
	{
		std::cerr << "PQTunnelProxy::out() Failed 1" << std::endl;
		return -1;
	}

	if (PQTunnel::getSize() != PQTunnel::out(dta, n))
	{
		std::cerr << "PQTunnelProxy::out() Failed 2" << std::endl;
		return -1;
	}

	if (n < PQTunnelProxy::getSize())
		return -1;
	char *ptr = (char *) dta;
	ptr += PQTunnel::getSize();


	memcpy(ptr, &seq, 4);
	ptr += 4;
	memcpy(ptr, src.data, CERTSIGNLEN);
	ptr += CERTSIGNLEN;
	memcpy(ptr, dest.data, CERTSIGNLEN);
	ptr += CERTSIGNLEN;

	memcpy(ptr, data, size);
	ptr += size;

	memcpy(ptr, sign, PQI_TUNNEL_SIGN_SIZE);
	ptr += PQI_TUNNEL_SIGN_SIZE;

	return PQTunnelProxy::getSize();
}

// The Size 
int PQTunnelProxy::in(const void *dta, const int n)
{
	//std::cerr << "PQTunnelProxy::in()" << std::endl;

	if ((dta == NULL) || (n < PQTunnelProxy::getSize()))
	{
		std::cerr << "PQTunnelProxy::in() Failed 1" << std::endl;
		if (dta == NULL)
		  std::cerr << "PQTunnelProxy::in() dta==NULL" << std::endl;
		std::cerr << "PQTunnelProxy::in() is?  n: " << n << " < ";
		std::cerr << PQTunnelProxy::getSize() << std::endl;
		if (n < PQTunnelProxy::getSize())
		  std::cerr << "PQTunnelProxy::in() not Enough Space" << std::endl;

		return -1;
	}
	
	if (PQTunnel::getSize() != PQTunnel::in(dta, n))
	{
		std::cerr << "PQTunnelProxy::in() Failed 2" << std::endl;
		return -1;
	}

	// range check size.
	if (data)
	{
		if (n != PQTunnelProxy::getSize())
		{
			free(data);
			size = 0;
			size = n - PQTunnelProxy::getSize();
			data = malloc(size);
		}
	}
	else
	{
		size = 0;
		size = n - PQTunnelProxy::getSize();
		data = malloc(size);
	}

	char *ptr = (char *) dta;
	ptr += PQTunnel::getSize();

	memcpy(&seq, ptr, 4);
	ptr += 4;
	memcpy(src.data, ptr, CERTSIGNLEN);
	ptr += CERTSIGNLEN;
	memcpy(dest.data, ptr, CERTSIGNLEN);
	ptr += CERTSIGNLEN;

	memcpy(data, ptr, size);
	ptr += size;

	memcpy(sign, ptr, PQI_TUNNEL_SIGN_SIZE);
	ptr += PQI_TUNNEL_SIGN_SIZE;

	return PQTunnelProxy::getSize();
}

const int PQTunnelProxy::getSize() const
{
	//std::cerr << "PQTunnelProxy::getSize()" << std::endl;
	return PQTunnel::getSize() + sizeof(long) + 
		CERTSIGNLEN * 2 +
		+ size + PQI_TUNNEL_SIGN_SIZE;
}


#include "pqipacket.h"

std::ostream &PQTunnelProxy::print(std::ostream &out)
{
	out << "-------- PQTunnelProxy" << std::endl;
	PQItem::print(out);

	out << "-------- -------- Tunnelling" << std::endl;
	PQItem *pkt = NULL;
	if ((data == NULL) || (size == 0))
	{
		out << "NULL Pkt" << std::endl;
	}
	else if (pqipkt_check(data, size))
	{
		pkt = pqipkt_create(data);
	}

	if (pkt == NULL)
	{
		out << "Unknown Packet Type" << std::endl;
	}
	else
	{
		pkt -> print(out);
		delete pkt;
	}
	out << "-------- -------- " << std::endl;
	return out << "-------- PQTunnelProxy" << std::endl;
}



PQTunnelProxyInit::PQTunnelProxyInit()
	:PQTunnelInit(PQI_TUNNEL_PROXY_TYPE), seq(0)
{
	return;
}

PQTunnelProxyInit::~PQTunnelProxyInit()
{
	return;
}

PQTunnelProxyInit *PQTunnelProxyInit::clone()
{
	PQTunnelProxyInit *ni = new PQTunnelProxyInit();
	ni -> copy(this);
	return ni;
}

void	PQTunnelProxyInit::copy(const PQTunnelProxyInit *item)
{
	PQTunnelInit::copy(item);

	mode = item -> mode;
	seq = item -> seq;
	src = item -> src;
	dest = item -> dest;
	proxy = item -> proxy;
	memcpy(challenge, item -> challenge, CERTSIGNLEN);
	memcpy(sign, item -> sign, PQI_TUNNEL_SIGN_SIZE);

}


std::ostream &PQTunnelProxyInit::print(std::ostream &out)
{
	out << "-----------------PQTunnelProxyInit" << std::endl;
	PQTunnelInit::print(out);
	out << "Mode: " << mode << std::endl;
	out << "Seq: " << seq << std::endl;

	out << std::hex;
	out << "Src: ";
	int i;
	for(i = 0; i < CERTSIGNLEN; i++)
	{
		out  << ":" << (unsigned int) src.data[i];
	}
	out << std::endl;

	out << "Proxy: ";
	for(i = 0; i < CERTSIGNLEN; i++)
	{
		out  << ":" << (unsigned int) proxy.data[i];
	}
	out << std::endl;

	out << "Dest: ";
	for(i = 0; i < CERTSIGNLEN; i++)
	{
		out  << ":" << (unsigned int) dest.data[i];
	}
	out << std::endl;
	out << std::dec;

	out << "-----------------PQTunnelProxyInit" << std::endl;

	return out;
}


int PQTunnelProxyInit::out(void *dta, const int n) const
{
	//std::cerr << "PQTunnelProxyInit::out()" << std::endl;
	if (n < PQTunnelProxyInit::getSize())
	{
		std::cerr << "PQTunnelProxyInit::out() Failed 1" << std::endl;
		return -1;
	}

	if (PQTunnelInit::getSize() != PQTunnelInit::out(dta, n))
	{
		std::cerr << "PQTunnelProxyInit::out() Failed 2" << std::endl;
		return -1;
	}


	char *ptr = (char *) dta;
	ptr += PQTunnelInit::getSize();

	memcpy(ptr, &seq, 4);
	ptr += 4;
	memcpy(ptr, &mode, 4);
	ptr += 4;
	memcpy(ptr, src.data, CERTSIGNLEN);
	ptr += CERTSIGNLEN;
	memcpy(ptr, proxy.data, CERTSIGNLEN);
	ptr += CERTSIGNLEN;
	memcpy(ptr, dest.data, CERTSIGNLEN);
	ptr += CERTSIGNLEN;

	memcpy(ptr, challenge, PQI_PROXY_CHALLENGE_SIZE);
	ptr += PQI_PROXY_CHALLENGE_SIZE;

	memcpy(ptr, sign, PQI_TUNNEL_SIGN_SIZE);
	ptr += PQI_TUNNEL_SIGN_SIZE;

	return PQTunnelProxyInit::getSize();
}


int PQTunnelProxyInit::in(const void *dta, const int n)
{
	//std::cerr << "PQTunnelProxyInit::in()" << std::endl;

	if ((dta == NULL) || (n < PQTunnelProxyInit::getSize()))
	{
		std::cerr << "PQTunnelProxyInit::in() Failed 1" << std::endl;

		return -1;
	}
	
	if (PQTunnelInit::getSize() != PQTunnelInit::in(dta, n))
	{
		std::cerr << "PQTunnelProxyInit::in() Failed 2" << std::endl;
		return -1;
	}

	char *ptr = (char *) dta;
	ptr += PQTunnelInit::getSize();

	memcpy(&seq, ptr, 4);
	ptr += 4;
	memcpy(&mode, ptr, 4);
	ptr += 4;
	memcpy(src.data, ptr, CERTSIGNLEN);
	ptr += CERTSIGNLEN;
	memcpy(proxy.data, ptr, CERTSIGNLEN);
	ptr += CERTSIGNLEN;
	memcpy(dest.data, ptr, CERTSIGNLEN);
	ptr += CERTSIGNLEN;

	memcpy(challenge, ptr, PQI_PROXY_CHALLENGE_SIZE);
	ptr += PQI_PROXY_CHALLENGE_SIZE;

	memcpy(sign, ptr, PQI_TUNNEL_SIGN_SIZE);
	ptr += PQI_TUNNEL_SIGN_SIZE;

	return PQTunnelProxyInit::getSize();
}



const int PQTunnelProxyInit::getSize() const
{
	//std::cerr << "PQTunnelProxyInit::getSize()" << std::endl;

	return PQTunnelInit::getSize() + 
		2 * sizeof(long) + 
		3 * CERTSIGNLEN + 
		PQI_PROXY_CHALLENGE_SIZE +
		PQI_TUNNEL_SIGN_SIZE;
}

