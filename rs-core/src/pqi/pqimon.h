/*
 * "$Id: pqibin.h,v 1.2 2007-02-18 21:46:49 rmf24 Exp $"
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

#ifndef PQI_MONITOR_HEADER
#define PQI_MONITOR_HEADER

/**** Rough sketch of a Monitor class 
 * expect it to change significantly
 *
 */


const uint32_t PQI_PEER_STATE_OFFLINE = 0;
const uint32_t PQI_PEER_STATE_ONLINE  = 1;

class pqipeer
{
	public:
std::string id;
std::string name;
int	state;
};

class pqimonclient
{
	public:

	pqimonclient() { return; }
virtual ~pqimonclient() { return; }

virtual void monUpdate(const std::list<pqipeer> &plist) = 0;
};


class pqimonitor
{
public:
	pqimonitor();
	~pqimonitor();
int     tick();
int	addClient(pqimonclient *cli) { clients.push_back(cli); return 1; }

private:
	std::list<pqimonclient *> clients;
	std::list<pqipeer> plist;
};



#endif 
