/*
 * "$Id: pqitunnel.cc,v 1.4 2007-02-18 21:46:50 rmf24 Exp $"
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




#include "pqi/pqitunnel.h"

const int pqitunnelzone = 60478;
#include "pqi/pqidebug.h"
#include <sstream>


PQTunnel::PQTunnel(int st)
	:PQItem(PQI_ITEM_TYPE_TUNNELITEM, st)
{
	pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
		"PQTunnel::PQTunnel() Creation...");

	return;
}

PQTunnel::~PQTunnel()
{
}

// This is commented out - because
// it should never be cloned at this level!
//
//PQTunnel *PQTunnel::clone()
//{
//	pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
//		"PQTunnel::clone");
//
//	PQTunnel *t = new PQTunnel(subtype);
//	t -> copy(this);
//	return t;
//}

void	PQTunnel::copy(const PQTunnel *src)
{
	pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
		"PQTunnel::copy() : Only the Base!");
	PQItem::copy(src);
}
					
std::ostream &PQTunnel::print(std::ostream &out)
{
	out << "-------- PQTunnel";
	PQItem::print(out);

	out << "PQTunnel size:" << getSize();
	out << "--------" << std::endl;
	return out;
}


//const int PQTunnel::getSize() const
//{
//	return size;
//}
//
//const void *PQTunnel::getData() const
//{
//	return data;
//}
//
//int PQTunnel::out(void *dta, const int n) const
//{
//	pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
//		"PQTunnel::out()");
//	
//	if ((dta == NULL) || (data == NULL))
//	{
//		return -1;
//	}
//	if (size == 0) 
//	{
//		return 0;
//	}
//
//	if (n < size)
//		return -1;
//	memcpy(dta, data, size);
//	return size;
//}
//
//int PQTunnel::in(const void *dta, const int n)
//{
//	pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
//		"PQTunnel::in()");
//
//	if (data)
//	{
//		if (n != size)
//		{
//			free(data);
//			data = malloc(n);
//			size = n;
//		}
//	}
//	else
//	{
//		data = malloc(n);
//		size = n;
//	}
//
//	memcpy(data, dta, size);
//	return 1;
//}

PQTunnelInit::PQTunnelInit(int st) 
	:PQItem(PQI_ITEM_TYPE_TUNNELINITITEM, st), mode(Request)
{
	pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
		"PQTunnelInit::PQTunnelInit()");
	return;
}

PQTunnelInit::~PQTunnelInit()
{
	return;
}


PQTunnelInit *PQTunnelInit::clone()
{
	pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
		"PQTunnelInit::clone()");

	PQTunnelInit *ni = new PQTunnelInit(subtype);
	ni -> copy(this);
	return ni;
}

void	PQTunnelInit::copy(const PQTunnelInit *item)
{
	pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
		"PQTunnelInit::copy()");

	PQItem::copy(item);

	mode = item -> mode;
}

std::ostream &PQTunnelInit::print(std::ostream &out)
{
	out << "---- ---- PQTunnelInit" << std::endl;
	PQItem::print(out);
	out << "mode: ";
	switch(mode)
	{
		case Request:
			out << "Request" << std::endl;
			break;
		case Connect:
			out << "Connect" << std::endl;
			break;
		case End:
			out << "End" << std::endl;
			break;
		default:
			out << "Unknown" << std::endl;
			break;
	}

	out << "---- ----" << std::endl;

	return out;
}

const int PQTunnelInit::getSize() const
{
	return 4;
}

int PQTunnelInit::out(void *data, const int size) const
{
	if (size < PQTunnelInit::getSize())
		return -1;
	((int *) data)[0] = (int) mode;
	return PQTunnelInit::getSize();
}


int PQTunnelInit::in(const void *data, const int size)
{
	if (size < PQTunnelInit::getSize())
		return -1;
	mode = (InitMsg) ((int *) data)[0];
	return PQTunnelInit::getSize();
}

PQTunnelServer::PQTunnelServer()
{
	pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
		"PQTunnelServer::PQTunnelServer()");

        rrit = services.begin();
	return;
}

int	PQTunnelServer::addService(PQTunnelService *ts)
{
	pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
		"PQTunnelServer::addService()");

	std::map<int, PQTunnelService *>::iterator it;
	it = services.find(ts -> getSubType());
	if (it != services.end())
	{
		// it exists already!
		return -1;
	}

	services[ts -> getSubType()] = ts;
        rrit = services.begin();
	return 1;
}

int	PQTunnelServer::incoming(PQItem *item)
{
	pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
		"PQTunnelServer::incoming()");

	{
		std::ostringstream out;
		out << "PQTunnelServer::incoming() Service: ";
		out << item -> subtype << std::endl;
		out << "Item:" << std::endl;
		item -> print(out);
		out << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, out.str());
	}

	std::map<int, PQTunnelService *>::iterator it;
	it = services.find(item -> subtype);
	if (it == services.end())
	{
		pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
		"PQTunnelServer::incoming() Service: No Service - deleting");

		// delete it.
		delete item;

		// it exists already!
		return -1;
	}

	switch(item -> type)
	{
		case PQI_ITEM_TYPE_TUNNELITEM:
		{
			std::ostringstream out;
			out << "PQTunnelServer::incoming() Sending to";
			out << it -> second << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, out.str());

			return (it->second) -> receive((PQTunnel *) item);
		}
		break;
		case PQI_ITEM_TYPE_TUNNELINITITEM:
		{
			std::ostringstream out;
			out << "PQTunnelServer::incoming() Sending to";
			out << it -> second << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, out.str());

			return (it->second) -> receive((PQTunnelInit *) item);
		}
		break;
		default:
		break;
	}
	delete item;
	return -1;
}



PQItem *PQTunnelServer::outgoing()
{
	pqioutput(PQL_DEBUG_ALL, pqitunnelzone, 
		"PQTunnelServer::outgoing()");

	if (rrit != services.end())
	{
		rrit++;
	}
	else
	{
		rrit = services.begin();
	}

	std::map<int, PQTunnelService *>::iterator sit = rrit;
	// run to the end.
	PQItem *item;

	// run through to the end,
	for(;rrit != services.end();rrit++)
	{
		// send out init items first.
		if (NULL != (item = (rrit -> second) -> sendInit()))
		{
			std::ostringstream out;
			out << "PQTunnelServer::outgoing() Got InitItem From:";
			out << rrit -> second << std::endl;

			item -> print(out);
			out << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, out.str());
			return item;
		}
		if (NULL != (item = (rrit -> second) -> send()))
		{
			std::ostringstream out;
			out << "PQTunnelServer::outgoing() Got Item From:";
			out << rrit -> second << std::endl;

			item -> print(out);
			out << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, out.str());
			return item;
		}
	}

	// from the beginning to where we started.
	for(rrit = services.begin();rrit != sit; rrit++)
	{
		if (NULL != (item = (rrit -> second) -> sendInit()))
		{
			std::ostringstream out;
			out << "PQTunnelServer::outgoing() Got InitItem From:";
			out << rrit -> second << std::endl;

			item -> print(out);
			out << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, out.str());
			return item;
		}
		if (NULL != (item = (rrit -> second) -> send()))
		{
			std::ostringstream out;
			out << "PQTunnelServer::outgoing() Got Item From:";
			out << rrit -> second << std::endl;

			item -> print(out);
			out << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, out.str());
			return item;
		}
	}
	return NULL;
}



int	PQTunnelServer::tick()
{
	pqioutput(PQL_DEBUG_ALL, pqitunnelzone, 
		"PQTunnelServer::tick()");

	std::map<int, PQTunnelService *>::iterator it;

	// from the beginning to where we started.
	for(it = services.begin();it != services.end(); it++)
	{
		std::ostringstream out;
		out << "PQTunnelServer::service id:" << it -> first;
		out << " -> Service: " << it -> second;
		out << std::endl;
		pqioutput(PQL_DEBUG_ALL, pqitunnelzone, out.str());

		// now we should actually tick the service.
		(it -> second) -> tick();
	}
	return 1;
}



