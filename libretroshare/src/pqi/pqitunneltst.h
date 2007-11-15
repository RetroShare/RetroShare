/*
 * "$Id: pqitunneltst.h,v 1.4 2007-02-18 21:46:50 rmf24 Exp $"
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



#ifndef PQI_TUNNEL_TST_HEADER
#define PQI_TUNNEL_TST_HEADER

#include "pqi/pqitunnel.h"

#define PQI_TUNNEL_TST_TYPE 0x0265


#define pqitunnelzone  71849
#include "pqi/pqidebug.h"
#include <sstream>


PQTunnel *createPQTStst(void *d, int n);


class PQTunnelTst: public PQTunnel
{
public:
	PQTunnelTst()
	:PQTunnel(PQI_TUNNEL_TST_TYPE) 
	{
	  pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
	    "PQTunnelTst::PQTunnelTst()");
	  return; 
	}

virtual	~PQTunnelTst()
{
	pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
	  "PQTunnelTst::~PQTunnelTst()");
	return;
}

virtual PQTunnelTst *clone()
{
	PQTunnelTst *pqt = new PQTunnelTst();
	// getData() no longer exists.
	//pqt -> in(getData(), getSize());
	return pqt;
}


virtual std::ostream &print(std::ostream &out)
{
	out << "----------------" << std::endl;
	out << "PQTunnelTst" << std::endl;
	// getData() no longer exists.
	// out << "Msg:" << (char *) getData() << std::endl;
	PQTunnel::print(out);
	out << "----------------" << std::endl;
	return out;
}

};


class PQTStst: public PQTunnelService
{
public:
	PQTStst() :PQTunnelService(PQI_TUNNEL_TST_TYPE)
	{ 
	  pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
	    "PQTStst::PQTStst()");
		
	  ts = time(NULL); 
	  return; 
	}

int	receive(PQTunnel *item)
{
	pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
	  "PQTStst::receive()");

	PQTunnelTst *tst = (PQTunnelTst *) item;
	std::cerr << "PQTStst::receive()...." << std::endl;
	tst -> print(std::cerr);
	delete item;
	return 1;
}

PQTunnel *send()
{
	if (time(NULL) - ts > 100.0 * rand()/(RAND_MAX + 1.0) )
	{
		pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
	  	  "PQTStst::send() Sending Test Packet!");
		// make an item.
		PQTunnelTst *tst = new PQTunnelTst();
		std::string msg = "Hello";
		tst -> in(msg.c_str(), msg.length());
		ts = time(NULL);
		return tst;
	}
	return NULL;
}

int	receive(PQTunnelInit *item)
{
	pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
	   "PQTStst::receive() Cleaning PQTunnelInit");
	delete item;
	return 1;
}

PQTunnelInit *sendInit()
{
	pqioutput(PQL_DEBUG_BASIC, pqitunnelzone, 
	   "PQTStst::sendInit() returning NULL");
	return NULL;
}

	private:
	long ts;
};



#endif // PQI_TUNNEL_TST_HEADER
