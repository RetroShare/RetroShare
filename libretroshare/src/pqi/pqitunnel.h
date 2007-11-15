/*
 * "$Id: pqitunnel.h,v 1.3 2007-02-18 21:46:50 rmf24 Exp $"
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



#ifndef PQI_TUNNEL_HEADER
#define PQI_TUNNEL_HEADER

#include "pqi/pqi_base.h"

// PQI Tunnel, this is a raw tunnel of data.
// The bottom layer on which other services
// can run on.
// 
// these packets get passed through the 
// server, to a service that is registered.
//
// example services:
// 	proxytunnel. -> p3proxy.
// 	sockettunnel
// 		-> either broadcast (attach to 
// 				open socket instead
// 				of broadcast address)
// 		-> or requested/signon.
//
// 	games,
// 	voice
// 	video
//

class PQTunnel: public PQItem
{
	// private copy constructor... prevent copying...
	PQTunnel(const PQTunnel &) {return; }
protected:
	PQTunnel(int st); // initialise with subtypes.
public:
virtual	~PQTunnel();

	// copy functions.
//virtual PQTunnel *clone() = 0;
void	copy(const PQTunnel *src);

// These will be made pure virtual.
virtual const int getSize() const { return 0; }
virtual int out(void *data, const int size) const { return 0;}
virtual int in(const void *data, const int size) { return 0;}
					// an overloaded 
					// fn that will
					// mean the building
					// fns will be attached.
					// to the class. and
					// dont have to 
					
virtual std::ostream &print(std::ostream &out);

//	int size;
//	void *data;
};




class PQTunnelInit: public PQItem
{
	// private copy constructor... prevent copying...
	PQTunnelInit(const PQTunnelInit &) {return; }
protected:
	PQTunnelInit(int st); // initialise with subtypes.
public:
virtual	~PQTunnelInit();

	// copy functions.
virtual PQTunnelInit *clone();
void	copy(const PQTunnelInit *src);

std::ostream &print(std::ostream &out);

        // These are overloaded from PQTunnelInit.
	// these should be able to construct + destruct 
	// the whole packet from here.
	virtual const int getSize() const;
	virtual int out(void *data, const int size) const; // write
	virtual int in(const void *data, const int size); // read,
	
	enum InitMsg { Request, Connect, End };
	InitMsg mode;
};


class PQTunnelService
{
	protected:

	PQTunnelService(int s) // our type of packets.
	:subtype(s) { return; }

virtual ~PQTunnelService() { return; }

	public:
	// 
virtual int		receive(PQTunnel *) = 0;
virtual PQTunnel *	send() = 0;

virtual int		receive(PQTunnelInit *) = 0;
virtual PQTunnelInit *	sendInit() = 0;

int	getSubType() { return subtype; }

virtual int	tick() { return 1; }

	private:
	int subtype;
};

#include <map>


class PQTunnelServer
{
public:
	PQTunnelServer();

int	addService(PQTunnelService *);

int	incoming(PQItem *);
PQItem *outgoing();

int	tick();

private:

std::map<int, PQTunnelService *> services;
std::map<int, PQTunnelService *>::iterator rrit;

};


	






#endif // PQI_TUNNEL_HEADER
