/*
 * "$Id: p3disc.cc,v 1.23 2007-02-18 21:46:49 rmf24 Exp $"
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




#include "pqi/p3disc.h"

// for active local cert stuff.
#include "pqi/pqissl.h"

#include <iostream>
#include <errno.h>
#include <cmath>

const int PQI_DISC_ITEM_TYPE = 0x69532;

#include <sstream>
#include "pqi/pqidebug.h"

const int pqidisczone = 2482;


static int updateAutoServer(autoserver *as, DiscItem *di);
static int convertTDeltaToTRange(double tdelta);
static int convertTRangeToTDelta(int trange);
static int updateCertAvailabilityFlags(cert *c, unsigned long discFlags);
static unsigned long determineCertAvailabilityFlags(cert *c);

// Operating System specific includes.
#include "pqi/pqinetwork.h"

p3disc::p3disc(sslroot *r)
	:PQTunnelService(PQI_TUNNEL_DISC_ITEM_TYPE), sroot(r)
{
	ldata = NULL;
	ldlenmax = 1024;

	local_disc = false; //true;
	remote_disc = true;

	// set last check to current time, this prevents queued.
	// messages at the start! (actually shouldn't matter - as they aren't connected).
	ts_lastcheck = time(NULL); // 0;

        // configure...
	load_configuration();
	localSetup();

	return;
}

p3disc::~p3disc()
{
	return;
}

// The PQTunnelService interface.
int     p3disc::receive(PQTunnel *i)
{
	inpkts.push_back(i);

	std::ostringstream out;
	out << "p3disc::receive() InQueue:" << inpkts.size() << std::endl;
	out << "incoming packet: " << std::endl;
	i -> print(out);
	pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());

	return 1;
}

PQTunnel *p3disc::send()
{
	std::ostringstream out;
	out << "p3disc::send() To Send:" << outpkts.size() << std::endl;

	if (outpkts.size() < 1)
	{
		out << "No Packet" << std::endl;
		pqioutput(PQL_DEBUG_ALL, pqidisczone, out.str());
		return NULL;
	}
	PQTunnel *pkt = (PQTunnel *) outpkts.front();
	outpkts.pop_front();

	out << "outgoing packet: " << std::endl;
	pkt -> print(out);
	pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());

	return pkt;
}

PQTunnel *p3disc::getObj()
{
	return new DiscItem();
}


int p3disc::tick()
{
	pqioutput(PQL_DEBUG_ALL, pqidisczone,
		"p3disc::tick()");

	if (local_disc)
	{
		if (ts_nextlp == 0)
		{
			pqioutput(PQL_DEBUG_ALL, pqidisczone,
				"Local Discovery On!");
			localPing(baddr);
			localListen();
		}
	}
	else
	{
		pqioutput(PQL_DEBUG_ALL, pqidisczone,
				"Local Discovery Off!");
	}
	
	// ten minute counter.
	if (--ts_nextlp < 0)
	{
		ts_nextlp = 600;
	}

	if (ts_nextlp % 300 == 0)
		idServers();


	/* remote discovery can run infrequently....
	 * this is a good idea, as it ensures that 
	 * multiple Pings aren't sent to neighbours....
	 *
	 * only run every 5 seconds.
	 */

	if (ts_nextlp % 5 != 0)
	{
		return 1;
	}

	// important bit
	int nr = handleReplies(); // discards packets if not running.
	if (remote_disc)
	{
		pqioutput(PQL_DEBUG_ALL, pqidisczone,
				"Remote Discovery On!");
		newRequests();
		if ((sroot -> collectedCerts()) || (nr > 0))
		{
			distillData();
		}
	}
	else
	{
		pqioutput(PQL_DEBUG_ALL, pqidisczone,
				"Remote Discovery Off!");
	}
	return 1;
}

static int local_disc_def_port = 7770;
static int local_disc_secondary_port = 7870;

int p3disc::setLocalAddress(struct sockaddr_in srvaddr)
{
	saddr = srvaddr;
	return 1;
}


int p3disc::determineLocalNetAddr()
{
	// laddr filled in by load_configuration.
	laddr.sin_port = htons(local_disc_def_port);

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX
	// broadcast address.
	baddr.sin_family = AF_INET;
	inet_aton("0.0.0.0", &(baddr.sin_addr));
	baddr.sin_port = htons(local_disc_def_port);
#else // WIN

	baddr. sin_family = AF_INET;

	// So as recommended on this site.. will use |+& to calc it.
	unsigned long netmask = inet_addr("255.255.255.0");
	unsigned long netaddr = saddr.sin_addr.s_addr & netmask;
	baddr.sin_addr.s_addr =  netaddr | (~netmask);

	// direct works!
	//baddr.sin_addr.s_addr =  inet_addr("10.0.0.59"); 
	//baddr.sin_addr.s_addr =  inet_addr("127.0.0.1"); 
	// broadcast!
	baddr.sin_addr.s_addr =  INADDR_BROADCAST;
	baddr.sin_port = htons(local_disc_def_port);

	



#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

	{
		std::ostringstream out;
		out << "p3disc::determineLocalNetAddr() baddr: ";
		out << inet_ntoa(baddr.sin_addr) << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
	}

	return 1;
}

int p3disc::setupLocalPacket(int type, struct sockaddr_in *home, 
						struct sockaddr_in *server)
{
	if (ldata == NULL)
	{
		ldata = malloc(ldlenmax);
	}

	// setup packet.
	// 8 bytes - tag
	((char *) ldata)[0] = 'P';
	((char *) ldata)[1] = 'Q';
	((char *) ldata)[2] = 'I';
	((char *) ldata)[3] = 'L';
	if (type == AUTODISC_LDI_SUBTYPE_PING)
	{
		((char *) ldata)[4] = 'D';
		((char *) ldata)[5] = 'I';
		((char *) ldata)[6] = 'S';
		((char *) ldata)[7] = 'C';
	}
	else
	{
		((char *) ldata)[4] = 'R';
		((char *) ldata)[5] = 'P';
		((char *) ldata)[6] = 'L';
		((char *) ldata)[7] = 'Y';
	}

	// sockaddr copy.
	ldlen = 8;
	for(unsigned int i = 0; i < sizeof(*home); i++)
	{
		((char *) ldata)[ldlen + i] = ((char *) home)[i];
	}

	ldlen += sizeof(*home);
	for(unsigned int i = 0; i < sizeof(*server); i++)
	{
		((char *) ldata)[ldlen + i] = ((char *) server)[i];
	}
	ldlen += sizeof(*server);

	return 1;
}





int p3disc::localSetup()
{
		
	if (!local_disc)
	{
		pqioutput(PQL_DEBUG_BASIC, pqidisczone, 
			"p3disc::localSetup() Warning local_disc OFF!");
		return -1;
	}

	//First we must attempt to open the default socket
	determineLocalNetAddr();

	int err = 0;

	lsock = socket(PF_INET, SOCK_DGRAM, 0);
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX

	if (lsock < 0)
	{
		pqioutput(PQL_DEBUG_BASIC, pqidisczone, 
			"p3disc::localSetup() Cannot open UDP socket!");
		local_disc = false;
		return -1;
	}

	err = fcntl(lsock, F_SETFL, O_NONBLOCK);
        if (err < 0)
	{
		pqioutput(PQL_DEBUG_BASIC, pqidisczone, 
			"p3disc::localSetup() Error: Cannot make socket NON-Blocking: ");

		local_disc = false;
		return -1;
	}

	int on = 1;
	if(0 != (err =setsockopt(lsock, SOL_SOCKET, SO_BROADCAST,(void *) &on, sizeof(on))))
	{
		pqioutput(PQL_DEBUG_BASIC, pqidisczone, 
			"p3disc::localSetup() Error: Cannot make socket Broadcast: ");
		local_disc = false;
		return -1;
	}
	else
	{
		pqioutput(PQL_DEBUG_BASIC, pqidisczone, 
			"p3disc::localSetup() Broadcast Flag Set!");
	}
	
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else //WINDOWS_SYS

	if (lsock == INVALID_SOCKET)
	{
		pqioutput(PQL_DEBUG_BASIC, pqidisczone, 
			"p3disc::localSetup() Cannot open UDP socket!");
		local_disc = false;
		return -1;
	}

	unsigned long int on = 1;
	if (0 != (err = ioctlsocket(lsock, FIONBIO, &on)))
        {
		pqioutput(PQL_DEBUG_BASIC, pqidisczone, 
			"p3disc::localSetup() Error: Cannot make socket NON-Blocking: ");
		local_disc = false;
		return -1;
	}

	on = 1;
	if(0 != (err=setsockopt(lsock, SOL_SOCKET, SO_BROADCAST,(char *) &on, sizeof(on))))
	{
		pqioutput(PQL_DEBUG_BASIC, pqidisczone, 
			"p3disc::localSetup() Error: Cannot make socket Broadcast: ");

		local_disc = false;
		return -1;
	}
	else
	{
		pqioutput(PQL_DEBUG_BASIC, pqidisczone, 
			"p3disc::localSetup() Broadcast Flag Set!");
	}

#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

	{
	  std::ostringstream out;
	  out << "p3disc::localSetup()" << std::endl;
	  out << "\tSetup Family: " << laddr.sin_family;
	  out << std::endl;
	  out << "\tSetup Address: " << inet_ntoa(laddr.sin_addr);
	  out << std::endl;
	  out << "\tSetup Port: " << ntohs(laddr.sin_port) << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
	}

	if (0 != (err = bind(lsock, (struct sockaddr *) &laddr, sizeof(laddr))))
	{
	  	std::ostringstream out;
		out << "p3disc::localSetup()";
		out << " Cannot Bind to Default Address!" << std::endl;
		showSocketError(out);
		out << std::endl;
		out << " Trying Secondary Address." << std::endl;
	  	pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());

	}
	else
	{
		// ifsucessful then call localPing
		// set ts to -1 and don't worry about outgoing until
		// we receive a packet
	  	std::ostringstream out;
		out << "p3disc::localSetup()" << std::endl;
		out << " Bound to Address." << std::endl;
		out << "\tSetup Address: " << inet_ntoa(laddr.sin_addr);
		out << std::endl;
		out << "\tSetup Port: " << ntohs(laddr.sin_port) << std::endl;
	  	pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());

		ts_nextlp = -1;
		ts_nextlp = 10;
		localPing(baddr);
		return 1;
	}

	laddr.sin_port = htons(local_disc_secondary_port);
	if (0 != (err = bind(lsock, (struct sockaddr *) &laddr, sizeof(laddr))))
	{
	  	std::ostringstream out;
		out << "p3disc::localSetup()";
		out << " Cannot Bind to Secondary Address!" << std::endl;
		showSocketError(out);
		out << std::endl;
		out << " Giving Up!" << std::endl;
	  	pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
		local_disc = false;
		return -1;
	}
	else
	{
	  	std::ostringstream out;
		out << "p3disc::localSetup()" << std::endl;
		out << " Bound to Secondary Address." << std::endl;
		out << "\tSetup Address: " << inet_ntoa(laddr.sin_addr);
		out << std::endl;
		out << "\tSetup Port: " << ntohs(laddr.sin_port) << std::endl;
	  	pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());

		ts_nextlp = 10;
		localPing(baddr);
		return 1;
	}

	// else we open a random port and set the timer
	// ie - don't bind to a port.....
	ts_nextlp = 10; // ping every 10 minutes.
	localPing(baddr);
	return 1;
}

int p3disc::localPing(struct sockaddr_in reply_to)
{
	//This function sends a meessage out containing both cert
	// and server address, as well as the ping address (if not standard)
	
	// so we send a packet out to that address 
	// (most likely broadcast address).
	
	// setup up the data for connection.
	setupLocalPacket(AUTODISC_LDI_SUBTYPE_PING,&laddr, &saddr);
	
	// Cast to char for windows benefit.
	int len = sendto(lsock, (char *) ldata, ldlen, 0, (struct sockaddr *) &reply_to, sizeof(reply_to));
	if (len != ldlen)
	{
		std::ostringstream out;
		out << "p3disc::localPing()";
		out << " Failed to send Packet." << std::endl;
		out << "Sent (" << len << "/" << ldlen;
		out << std::endl;
		out << "Addr:" << inet_ntoa(reply_to.sin_addr) << std::endl;
		out << "Port:" << ntohs(reply_to.sin_port) << std::endl;
		out << std::endl;
	  	pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
	}
	else
	{
		std::ostringstream out;
		out << "p3disc::localPing() Success!" << std::endl;
		out << "Sent To Addr:" << inet_ntoa(reply_to.sin_addr) << std::endl;
		out << "Sent To Port:" << ntohs(reply_to.sin_port) << std::endl;
		out << "Message Size: " << len << std::endl;
	  	pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
	}
	return 1;
}

int p3disc::localReply(struct sockaddr_in reply_to)
{
	//This function sends a meessage out containing both cert
	// and server address, as well as the ping address (if not standard)
	
	// so we send a packet out to that address 
	// (most likely broadcast address).
	
	// setup up the data for connection.
	setupLocalPacket(AUTODISC_LDI_SUBTYPE_RPLY,&laddr, &saddr);

	// Cast to char for windows benefit.
	int len = sendto(lsock, (char *) ldata, ldlen, 0, (struct sockaddr *) &reply_to, sizeof(reply_to));
	if (len != ldlen)
	{
		std::ostringstream out;
		out << "p3disc::localPing()";
		out << " Failed to send Packet." << std::endl;
	  	pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
	}
	
	return 1;
}


int p3disc::localListen()
{
	//This function listens to the ping address.
	//For each reply, store the result in the structure and mark as local
	struct sockaddr_in addr;
	struct sockaddr_in neighbour;
	struct sockaddr_in server;
	socklen_t alen = sizeof(addr);
	int nlen = sizeof(neighbour);
	int len;
	int size = 0;

	while(0 < (size = recvfrom(lsock, (char *) ldata, ldlen, 0, 
				(struct sockaddr *) &addr, &alen)))
	{
		std::ostringstream out;
		out << "Recved Message" << std::endl;
		out << "From Addr:" << inet_ntoa(addr.sin_addr) << std::endl;
		out << "From Port:" << ntohs(addr.sin_port) << std::endl;
		out << "Message Size: " << size << std::endl;

		for(int i = 0; i < 8; i++)
		{
			out << ((char *) ldata)[i];
		}
		out << std::endl;

		len = 8;
		// sockaddr copy.
		for(int i = 0; i < nlen; i++)
		{
			((char *) &neighbour)[i] = ((char *) ldata)[len + i]; 
		}
		len += nlen;
		for(int i = 0; i < nlen; i++)
		{
			((char *) &server)[i] = ((char *) ldata)[len + i]; 
		}
		len += nlen;


		out << "Neighbour Addr:" << inet_ntoa(neighbour.sin_addr) << std::endl;
		out << "Neighbour Port:" << ntohs(neighbour.sin_port) << std::endl;
		out << "Server Addr:" << inet_ntoa(server.sin_addr) << std::endl;
		out << "Server Port:" << ntohs(server.sin_port) << std::endl;

		if ((laddr.sin_addr.s_addr == neighbour.sin_addr.s_addr) &&
			(laddr.sin_port == neighbour.sin_port))
		{
			// Then We Sent it!!!!
			// ignore..
			out << "Found Self! Addr - " << inet_ntoa(neighbour.sin_addr);
			out << ":" << ntohs(neighbour.sin_port) << std::endl;
		}
		else
		{
			if ('D' == (((char *) ldata)[4])) // Then Ping.
			{
				// reply.
				localReply(neighbour);
			}

			addLocalNeighbour(&neighbour, &server);
		}

	  	pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
	}
	
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX

	if ((size < 0) && (errno != EAGAIN))
	{
		std::ostringstream out;
		out << "Error Recieving Message" << std::endl;
		out << "Errno: " << errno << std::endl;
		out << socket_errorType(errno) << std::endl;
	  	pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
	}

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else // WINDOWS_SYS

	if (size == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			std::ostringstream out;
			out << "Error Recieving Message" << std::endl;
			out << "WSE: " << err << std::endl;
	  		pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
		}
	}

#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
		
	return 1;
}

// This needs to be fixed up....
// Local dicsovery disabled for the moment....

int p3disc::addLocalNeighbour(struct sockaddr_in *n, struct sockaddr_in *s)
{
	std::list<autoneighbour *>::iterator it;
	for(it = neighbours.begin(); it != neighbours.end(); it++)
	{
		// if the server address matches one already!
		// make sure its flags as local and return.
		if (0 == memcmp((char *) &((*it) -> server_addr), (char *) s, sizeof(*s)))
		{
			(*it) -> local = true;
			return 1;
		}
	}
	// else add it in!
	autoneighbour *ln = new autoneighbour();
	ln -> server_addr = (*s);
	ln -> local = true;
	ln -> id = NULL; // null cert

	std::string nname = "Local Neighbour (";
	nname += inet_ntoa(ln -> server_addr.sin_addr);
	nname += ")";

	//ln -> id -> Name(nname);

	// now we call the dummy connect...
	// this will only be done once per local neighbour.
	// connectForExchange(ln -> server_addr);

	neighbours.push_back(ln);
	// update dicovered.
	distillData();
	return 1;
}


// Code Fragment that might be useful when local is patched up....
/****************************************************
int 	p3disc::distillLocalData()
{
	// This transforms the autoneighbour tree into 
	// a list of certificates with the best guess settings.

	discovered.clear();

	pqioutput(PQL_DEBUG_BASIC, pqidisczone, "p3disc::distillData()");

	std::list<autoneighbour *>::iterator it;
	std::list<autoneighbour *>::iterator it2;

	// Now check for local -> remote duplicates....
	for(it = neighbours.begin(); it != neighbours.end();)
	{
		cert *c = (cert *) ((*it) -> id);
		if (((*it) -> local) && (c == NULL))
		{
			// potentially a duplicate.
			bool found = false;
			for(it2 = neighbours.begin(); it2 != neighbours.end(); it2++)
			{
				// if address is the same -> remove first version.
				if ((it != it2) && (0 == memcmp((char *) &((*it) -> addr), 
						(char *) &((*it2) -> addr), 
					 		sizeof(struct sockaddr))))
				{
					(*it2) -> local = true;
					found = true;
				}
			}
			if (found == true)
			{
				// remove the certless local.
				it = neighbours.erase(it);
			}
			else
			{
				it++;
			}
		}
		else
		{
			it++;
		}
	}

******************************************/


int p3disc::idServers()
{
	std::list<autoneighbour *>::iterator it;
	std::list<autoserver *>::iterator nit;
	int cts = time(NULL);

	std::ostringstream out;
	out << "::::AutoDiscovery Neighbours::::" << std::endl;
	for(it = neighbours.begin(); it != neighbours.end(); it++)
	{
		if ((*it) -> local)
		{
			out << "Local Neighbour: ";
		}
		else
		{
			out << "Friend of a friend: ";
		}
		cert *c = (cert *) ((*it) -> id);

		if (c != NULL)
		{
			if (c -> certificate != NULL)
			{
				out << c -> certificate -> name;
			}
			else
			{
				out << c -> Name();
			}
		}
		else
		{
			out << "UnIdentified";
		}

		out << std::endl;
		out << "BG LocalAddr: ";
		out <<  inet_ntoa((*it) -> local_addr.sin_addr);
		out << ":" << ntohs((*it) -> local_addr.sin_port) << std::endl;
		out << "BG Server: ";
		out <<  inet_ntoa((*it) -> server_addr.sin_addr);
		out << ":" << ntohs((*it) -> server_addr.sin_port) << std::endl;
		out << "  Listen TR: ";
		if (((*it) -> listen) && ((*it) -> l_ts))
		{
			out << cts - (*it) -> l_ts << " sec ago";
		}
		else
		{
			out << "Never";
		}
		out << "    ";

		out << "Connect TR: ";
		if (((*it) -> connect) && ((*it) -> c_ts))
		{
			out << cts - (*it) -> c_ts << " sec ago";
		}
		else
		{
			out << "Never";
		}

		if ((*it) -> active)
		{
			out << " Active!!!";
		}
		out << std::endl;

		out << " -->DiscFlags: 0x" << std::hex << (*it)->discFlags;
		out << std::dec << std::endl;

		for(nit = ((*it) -> neighbour_of).begin(); 
				nit != ((*it) -> neighbour_of).end(); nit++)
		{
			out << "\tConnected via: ";
			if ((*nit) -> id != NULL)
			{
			  out << ((*nit) ->id) -> Name() << "(";
			  out <<  inet_ntoa(((*nit) -> id) -> lastaddr.sin_addr);
			  out << ":" << ntohs(((*nit) -> id) -> lastaddr.sin_port);
			  out << ")";
			}
			out << std::endl;
			out << "\t\tServer: ";
			out <<  inet_ntoa((*nit) -> server_addr.sin_addr);
			out <<":"<< ntohs((*nit) -> server_addr.sin_port);
			out << std::endl;
			out << "\t\tLocalAddr: ";
			out <<  inet_ntoa((*nit) -> local_addr.sin_addr);
			out <<":"<< ntohs((*nit) -> local_addr.sin_port);

			out << std::endl;
			if ((*nit) -> listen)
			{
				out << "\t\tListen TR:";
				out << cts - (*nit) -> l_ts << " sec ago";
			}
			else
			{
				out << "\t\tNever Received!";
			}
			out  << std::endl;
			if ((*nit) -> connect)
			{
				out << "\t\tConnect TR:";
				out << cts - (*nit) -> c_ts << " sec ago";
			}
			else
			{
				out << "\t\tNever Connected!";
			}
			out << std::endl;
			out << "\t\tDiscFlags: 0x" << std::hex << (*nit)->discFlags;
			out << std::dec << std::endl;
		}
	}
	pqioutput(PQL_WARNING, pqidisczone, out.str());
	return 1;
}


		

int p3disc::newRequests()
{
	// Check the timestamp against the list of certs.
	// If any are newer and currently active, then
	// send out Discovery Request.
	// This initiates the p3disc procedure.

	if (!remote_disc)
	{
		pqioutput(PQL_DEBUG_ALL, pqidisczone, 
		  "p3disc::newRequests() Remote Discovery is turned off");
		return -1;
	}

	pqioutput(PQL_DEBUG_ALL, pqidisczone, 
		"p3disc::newRequests() checkin for new neighbours");

	// Perform operation on the cert list.
	std::list<cert *>::iterator it;
	// Temp variable
	std::list<cert *> &certlist = sroot -> getCertList();

	{
		std::ostringstream out;
		out << "Checking CertList!" << std::endl;
		out << "last_check: " << ts_lastcheck;
		out << " time(): " << time(NULL);
		pqioutput(PQL_DEBUG_ALL, pqidisczone, out.str());
	}

	for(it = certlist.begin(); it != certlist.end(); it++)
	{
		{
		  std::ostringstream out;
		  out << "Cert: " << (*it) -> Name();
		  out << " lc_ts: " << (*it) -> lc_timestamp;
		  out << " lr_ts: " << (*it) -> lr_timestamp;
		  pqioutput(PQL_DEBUG_ALL, pqidisczone, out.str());
		}

		// This should be Connected(), rather than Accepted().
		// should reply with all Accepted(), but only send to all connected().
		// if (((*it) -> Accepted()) && 
		//
		// need >= to ensure that it will happen, 
		// about 1 in 5 chance of multiple newRequests if called every 5 secs.
		// can live with this. (else switch to fractional seconds).

		if (((*it) -> Connected()) && 
				(((*it) -> lc_timestamp >= ts_lastcheck)
				|| ((*it) -> lr_timestamp >= ts_lastcheck)))
		{

		  // also must not have already sent message.
		  // (unless reconnection?)
		  // actually - this should occur, even if last 
		  // exchange not complete.
		  // reconnect 

/*****************************************************************************
 * No more need for ad_init silliness....
 *
		  //if (ad_init.end() == 
		  // find(ad_init.begin(),ad_init.end(),*it))
		  // infact - we need the opposite behaviour.
		  // remove if in the init list.
		  
		  std::list<cert *>::iterator it2;
		  if (ad_init.end() != 
			 (it2 = find(ad_init.begin(),ad_init.end(),*it)))
		  {
			  ad_init.erase(it2);
		  }
 *
 *
 *
 ****************************************************************************/

		  
		  {
			// Then send message.
			{
		  	  std::ostringstream out;
			  out << "p3disc::newRequests()";
			  out << "Constructing a Message!" << std::endl;
			  out << "Sending to: " << (*it) -> Name();
			  out << " lc_ts: " << (*it) -> lc_timestamp;
			  out << " lr_ts: " << (*it) -> lr_timestamp;
		 	  pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
			}

			// Construct a message
			DiscItem *di = new DiscItem();

			// get our details.....
			cert *own = sroot -> getOwnCert();

			// Fill the message
			di -> cid = (*it) -> cid;
			di -> laddr = own -> localaddr;
			di -> saddr = own -> serveraddr;

			// if we are firewalled..... (and no forwarding...)
			// set received as impossible.
			if (own -> Firewalled() && (!(own -> Forwarded())))
				di -> receive_tr = 0; /* invalid */
			else
				di -> receive_tr = 1; /* zero time */

			di -> connect_tr = 1; /* zero time */
			di -> discFlags = determineCertAvailabilityFlags(own);

			// Send off message
			outpkts.push_back(di);
			//p3i -> SendOtherPQItem(di);
		
/*****************************************************************************
 * No more need for ad_init silliness....
			// push onto init list.
			ad_init.push_back(*it);
 *
 ****************************************************************************/

			// Finally we should also advertise the
			// new connection to our neighbours????
			// SHOULD DO - NOT YET.

		  }
		}
	}
	ts_lastcheck = time(NULL);
	return 1;
}

bool isDiscItem(PQItem *item)
{
	if (item -> type == PQI_ITEM_TYPE_AUTODISCITEM)
		return true;
	return false;
}

int p3disc::handleReplies()
{
	DiscItem *di = NULL;
	pqioutput(PQL_DEBUG_ALL, pqidisczone, "p3disc::handleReplies()");

	// if off discard item.
	if (!remote_disc)
	{
		//while(NULL != (di = (DiscItem *) p3i -> SelectOtherPQItem(isDiscItem)))
		while(inpkts.size() > 0)
		{
			std::ostringstream out;
			out << "p3disc::handleReplies()";
			out << " Deleting - Cos RemoteDisc Off!" << std::endl;

			di = (DiscItem *) inpkts.front();
			inpkts.pop_front();

			di -> print(out);
			pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());

			delete di;
		}
		return 0;
	}

	int nhandled = 0;
	// While messages read
	//while(NULL != (di = (DiscItem *) p3i -> SelectOtherPQItem(isDiscItem)))
	while(inpkts.size() > 0)
	{
		PQItem *item = inpkts.front();
		inpkts.pop_front();

		DiscItem *di = NULL;
		DiscReplyItem *dri = NULL;

		if (NULL == (di = dynamic_cast<DiscItem *> (item)))
		{
			std::ostringstream out;
			out << "p3disc::handleReplies()";
			out << "Deleting Non DiscItem Msg" << std::endl;
			item -> print(out);
			pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());

			// delete and continue to next loop.
			delete item;

			continue;
		}
		nhandled++;

		{
		std::ostringstream out;
		out << "p3disc::handleReplies()";
		out << " Received Message!" << std::endl;
		di -> print(out);
		pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
		}


		// if discovery reply then respondif haven't already.
		if (NULL != (dri = dynamic_cast<DiscReplyItem *> (di)))
		{

/*********************************************************************************
 * Shouldn't Reply to a Reply - end up with silly traffic.....
 * only reply to a ping....
 *
			std::list<cert *>::iterator it;
			if (ad_init.end() != (it = find(ad_init.begin(), ad_init.end(),
							(cert *) (dri -> p))))
			{
				// Should send our answer!
				sendDiscoveryReply((cert *) dri -> p);
				pqioutput(PQL_DEBUG_BASIC, pqidisczone, 
					"After Reply to Reply");
				// remove from reply list.
				ad_init.erase(it);
			}
 *
 *
 ********************************************************************************/

			// add to data tree.
			handleDiscoveryData(dri);
		}
		else if (di -> discType == 0)
		{
			handleDiscoveryPing(di);
			sendDiscoveryReply((cert *) di -> p);
			pqioutput(PQL_DEBUG_BASIC, pqidisczone, 
				"After Reply to Ping");

		}
		delete di;
	}
	return nhandled;
}

int 	p3disc::sendDiscoveryReply(cert *p)
{
	if (!remote_disc)
		return -1;

	// So to send a discovery reply .... we need to....
	// 1) generate a list of our neighbours.....
	pqioutput(PQL_DEBUG_BASIC, pqidisczone, 
		"p3disc::sendDiscoveryReply() Generating Messages!");
	
	std::list<cert *>::iterator it;
	// Temp variable
	std::list<cert *> &certlist = sroot -> getCertList();
	int good_certs = 0;
	int cts = time(NULL);

	for(it = certlist.begin(); it != certlist.end(); it++)
	{
		// if accepted and has connected (soon)
		if ((*it) -> Accepted())
		{
			good_certs++;

			{
			std::ostringstream out;
			out << "p3disc::sendDiscoveryReply()";
			out << " Found Neighbour Cert!" << std::endl;
			out << "Encoding: "<<(*it)->Name() << std::endl;
			out << "Encoding(2): "<<(*it)->certificate->name << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
			}

			// Construct a message
			DiscReplyItem *di = new DiscReplyItem();

			// Fill the message
			// Set Target as input cert.
			di -> cid = p -> cid;

			// set the server address.
			di -> laddr = (*it) -> localaddr;
			di -> saddr = (*it) -> serveraddr;

			// set the timeframe since last connection.
			if ((*it) -> lr_timestamp <= 0)
			{
				di -> receive_tr = 0;
			}
			else
			{
				di -> receive_tr = convertTDeltaToTRange(cts - (*it) -> lr_timestamp);
			}

			if ((*it) -> lc_timestamp <= 0)
			{
				di -> connect_tr = 0;
			}
			else
			{
				di -> connect_tr = convertTDeltaToTRange(cts - (*it) -> lc_timestamp);
			}
			di -> discFlags = determineCertAvailabilityFlags(*it);

			// irrelevent
			di -> p = NULL;

			// actually ned to copy certificate to array
			// for proper cert stuff.

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
			int len = i2d_XPGP((*it) -> certificate, &(di -> certDER));
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
			int len = i2d_X509((*it) -> certificate, &(di -> certDER));
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
			if (len > 0)
			{
				di -> certLen = len;
				std::ostringstream out;
				out << "Cert Encoded(" << len << ")" << std::endl;
				pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
			}
			else
			{
				pqioutput(PQL_DEBUG_BASIC, pqidisczone, "Failed to Encode Cert");
				di -> certLen = 0;
			}

			// Send off message
			outpkts.push_back(di);
			pqioutput(PQL_DEBUG_BASIC, pqidisczone, "Sent DI Message");
		}
		else
		{
			std::ostringstream out;
			out << "p3disc::sendDiscoveryReply()";
			out << "Not Sending Cert: " << std::endl;
			out << (*it) -> Name() << std::endl;
			pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
		}

	}

	{
	std::ostringstream out;
	out << "p3disc::sendDiscoveryReply()";
	out << "Found " << good_certs << " Certs" << std::endl;
	pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
	}
	
	return 1;
}


int	p3disc::handleDiscoveryPing(DiscItem *di)
{
	std::list<autoneighbour *>::iterator it;

	// as already connected.... certificate available.
	cert *c = (cert *) di -> p;

	if (c == NULL)
		return -1;

	{
	  std::ostringstream out;
	  out << "p3disc::handleDiscoveryPing()" << std::endl;
	  di -> print(out);
	  out << "RECEIVED Self Describing DiscItem!";
	  out << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
	}

	// The first check is whether this packet came from 
	// the cert in the reply.

	// Local address is always right!
	// No User control anyway!
	c -> localaddr = di -> laddr;

	// The Rest of this should only be set
	// if we are in autoconnect mode.....
	// else should be done manually.
	// ----> Think we should do this always (is disc from then)
	/****************************
	if (!(c -> Manual()))
	****************************/
	{
	
		// if the connect addr isn't valid.
		if (!isValidNet(&(c -> lastaddr.sin_addr)))
		{
			// set it all
			c -> serveraddr = di -> saddr;
	  		pqioutput(PQL_WARNING, pqidisczone, 
			  "lastaddr !Valid -> serveraddr=di->saddr");
		}
		// if the connect addr == dispkt.local
		else if (0 == inaddr_cmp(c -> lastaddr, di -> laddr))
		{
			// set it all
			c -> serveraddr = di -> saddr;
			c -> Local(true);
	  		pqioutput(PQL_WARNING, pqidisczone, 
			  "lastaddr=di->laddr -> Local & serveraddr=di->saddr");
		}
		else if (0 == inaddr_cmp(c -> lastaddr, di -> saddr))
		{
	  		pqioutput(PQL_WARNING, pqidisczone, 
			  "lastaddr=di->saddr -> !Local & serveraddr=di->saddr");
			c -> serveraddr = di -> saddr;
			c -> Local(false);
		}
		else
		{
	  		pqioutput(PQL_WARNING, pqidisczone, 
			  "lastaddr!=(di->laddr|di->saddr) -> !Local,serveraddr left");
			c -> Local(false);
		}
	
		updateCertAvailabilityFlags(c, di->discFlags);

	}
	/****************************
	else
	{
	  	pqioutput(PQL_WARNING, pqidisczone, 
		  "peer is Manual -> leaving server settings");
		if (0 == inaddr_cmp(c -> lastaddr, di -> laddr))
		{
	  		pqioutput(PQL_WARNING, pqidisczone, 
		  		"c->lastaddr=di->laddr -> local");
			c -> Local(true);
		}
		else
		{
	  		pqioutput(PQL_WARNING, pqidisczone, 
		  		"c->lastaddr!=di->laddr -> !local");
			c -> Local(false);
		}

	}
	****************************/

	// Now add it into the system.
	// check if it exists already......
	for(it = neighbours.begin(); it != neighbours.end(); it++)
	{
		cert *c2 = (cert *) (*it) -> id;
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
		if ((c2 != NULL) && (0 == XPGP_cmp(
				c -> certificate, c2 -> certificate)))
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
		if ((c2 != NULL) && (0 == X509_cmp(
				c -> certificate, c2 -> certificate)))
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
		{
			// matching....
			// update it....;
	  		pqioutput(PQL_DEBUG_BASIC, pqidisczone, "Updating Certificate (AN)!");

			(*it)-> local = c -> Local();
			updateAutoServer((*it), di);

			/* now look through the neighbours_of */
			std::list<autoserver *>::iterator nit;
			for(nit = ((*it) -> neighbour_of).begin(); 
		  		  nit != ((*it) -> neighbour_of).end(); nit++)
			{

				/* check if we already have a autoserver.... */
				if ((*it)->id == (*nit)->id)
				{
					/* we already have one */
					// update it....;
	  				pqioutput(PQL_DEBUG_BASIC, pqidisczone, 
						"Updating Certificate (AS)!");

					updateAutoServer(*nit, di);
					return 0;
				}
			}

	  		pqioutput(PQL_DEBUG_BASIC, pqidisczone, 
						"Adding Certificate (AS)!");

			/* if we get here, we need to add an autoserver */
			autoserver *as = new autoserver();
			as -> id = c;
			updateAutoServer(as, di);
			(*it) -> neighbour_of.push_back(as);

			return 1;
		}
	}

	// if get here must add a autoneighbour + an autoserver.
	
	autoneighbour *an = new autoneighbour();
	an -> id = c;
	an -> local = c -> Local();
	updateAutoServer(an, di);

	// add autoserver to an.
	autoserver *as = new autoserver();
	as -> id = c;
	updateAutoServer(as, di);

	an -> neighbour_of.push_back(as);
	neighbours.push_back(an);

	return 1;
}


int	p3disc::handleDiscoveryData(DiscReplyItem *di)
{
	std::list<autoneighbour *>::iterator it;

	{
	  std::ostringstream out;
	  out << "p3disc::handleDiscoveryData()" << std::endl;
	  di -> print(out);
	  pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
	}

	/* WIN/LINUX Difference.
	 */
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // ie UNIX
	const unsigned char *certptr = di -> certDER; 
#else
	unsigned char *certptr = di -> certDER;
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/


	// load up the certificate.....
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)

	XPGP *tmp = NULL;
	XPGP *xpgp = d2i_XPGP(&tmp, (unsigned char **) &certptr, di -> certLen);
	if (xpgp == NULL)
		return -1;
	{
	  std::ostringstream out;
	  out << "p3disc::handleDiscoveryData()" << std::endl;
	  out << "certificate name: " << xpgp -> name << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
	}

	// if a duplicate with ad/or sslroot;
	cert *c = sroot -> makeCertificateXPGP(xpgp);
	if (c == NULL)
	{
	  	pqioutput(PQL_DEBUG_BASIC, pqidisczone,
			"Failed to Create Certificate");
		// delete the cert.
		XPGP_free(xpgp);
		return -1;
	}


#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	X509 *tmp = NULL;
	X509 *x509 = d2i_X509(&tmp, &certptr, di -> certLen);
	if (x509 == NULL)
		return -1;
	{
	  std::ostringstream out;
	  out << "p3disc::handleDiscoveryData()" << std::endl;
	  out << "certificate name: " << x509 -> name << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
	}

	// if a duplicate with ad/or sslroot;
	cert *c = sroot -> makeCertificate(x509);
	if (c == NULL)
	{
	  	pqioutput(PQL_DEBUG_BASIC, pqidisczone,
			"Failed to Create Certificate");
		// delete the cert.
		X509_free(x509);
		return -1;
	}

#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/


	// So have new/existing cert;
	// check if it exists already......
	for(it = neighbours.begin(); it != neighbours.end(); it++)
	{
		cert *c2 = (cert *) (*it) -> id;

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
		if ((c2 != NULL) && (0 == XPGP_cmp(
				c -> certificate, c2 -> certificate)))
#else  /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
		if ((c2 != NULL) && (0 == X509_cmp(
				c -> certificate, c2 -> certificate)))
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
		{
			// matching.... check neighbours of....
			// for the source of the message.

			std::list<autoserver *>::iterator nit;
			for(nit = ((*it) -> neighbour_of).begin(); 
		  		  nit != ((*it) -> neighbour_of).end(); nit++)
			{
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
			  if (0 == XPGP_cmp(
#else  /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
			  if (0 == X509_cmp(
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
				((cert *) (*nit) -> id) -> certificate, 
				((cert *) (di -> p)) -> certificate))
			  {
				
				// update it....;
	  			pqioutput(PQL_DEBUG_BASIC, pqidisczone, 
					"Updating Certificate!");

				updateAutoServer(*nit, di);

				return 0;
			  }
			}

			// if we get to here - add neighbour of info.
			autoserver *as = new autoserver();
			as -> id = (cert *) (di -> p);

			// add in some more ....as -> addr = (di -> );
			
			updateAutoServer(as, di);

			(*it) -> neighbour_of.push_back(as);
				
			return 1;
		}
	}

	// if get here must add a autoneighbour + autoserver.

	{
	  std::ostringstream out;
	  out << "p3disc::handleDiscoveryData()" << std::endl;
	  out << "Adding New AutoNeighbour:" << c -> Name() << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
	}

	autoneighbour *an = new autoneighbour();
	an -> id = c;
	// initial guess.
	an -> local_addr = di -> laddr;
	an -> server_addr = di -> saddr;
	an -> local = false;

	autoserver *as = new autoserver();
	as -> id = (cert *) di -> p;

	updateAutoServer(as, di);

	an -> neighbour_of.push_back(as);

	neighbours.push_back(an);

	return 1;
}

int 	p3disc::collectCerts()
{

	// First get any extras from the CollectedCerts Queue.
	// if the cert matches an existing one.... update + discard
	// else add in....
	
	std::list<autoneighbour *>::iterator it;
	std::list<autoneighbour *>::iterator it2;

	cert *nc;
	while(NULL != (nc = sroot -> getCollectedCert()))
	{
		// check for matching certs.
		bool found = false;

		{
		std::ostringstream out;
		out << "p3disc::collectCert: " << std::endl;
		out << "Name: " << nc -> Name() << std::endl;
		out << "CN: " << nc -> certificate -> name << std::endl;

		out << "    From: ";
		out <<  inet_ntoa(nc -> lastaddr.sin_addr);
		out << ":" << ntohs(nc -> lastaddr.sin_port) << std::endl;
		out << "    Local: ";
		out <<  inet_ntoa(nc -> localaddr.sin_addr);
		out << ":" << ntohs(nc -> localaddr.sin_port) << std::endl;
		out << "    Server: ";
		out <<  inet_ntoa(nc -> serveraddr.sin_addr);
		out << ":" << ntohs(nc -> serveraddr.sin_port) << std::endl;
		out << "    Listen TS:";
		out << nc -> lr_timestamp <<  "    ";
		out << "Connect TR:";
		out << nc -> lc_timestamp << std::endl;
		out << std::endl;
	  	pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
		}

		
		for(it = neighbours.begin(); (!found) && (it != neighbours.end()); it++)
		{
			cert *c = (cert *) ((*it) -> id);
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
			if ((c != NULL) && 
				(0 == XPGP_cmp(c -> certificate, nc -> certificate)))
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
			if ((c != NULL) && 
				(0 == X509_cmp(c -> certificate, nc -> certificate)))
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
			{
				/* addresses handled already ....
				 * by sslroot. (more intelligent decisions).
				 * update timestamps so we don't overwrite
				 * the uptodate cert data.
				 */

				found = true;
				if ((nc -> lc_timestamp > 0) && 
					((unsigned) nc -> lc_timestamp > (*it) -> c_ts))
				{
					// connect.... timestamp 
					(*it) -> connect = true;
					(*it) -> c_ts = nc -> lc_timestamp;
					// don't make this decision here.
					//(*it) -> server_addr = nc -> lastaddr;
				}

				if ((nc -> lr_timestamp > 0) &&
					((unsigned) nc -> lr_timestamp > (*it) -> l_ts))
				{
					// received.... timestamp 
					(*it) -> listen = true;
					(*it) -> l_ts = nc -> lr_timestamp;
				}

				if ((c != nc) || 
				    (c -> certificate != nc -> certificate))
				{
					std::ostringstream out;
					out << "Warning Dup/Diff Mem ";
					out << " Found in p3Disc!";
					out << std::endl;
	  				pqioutput(PQL_ALERT, pqidisczone, out.str());
					exit(1);
				}
			}
		}
		if (!found)
		{
			// add into the list.....
			autoneighbour *an = new autoneighbour();
			an -> id = nc;

			// initial guess.
			an -> local_addr = nc -> localaddr;
			an -> server_addr = nc -> serveraddr;
			an -> local = false;

			if (nc -> lc_timestamp > 0)
			{
				an -> c_ts = nc -> lc_timestamp;
				an -> connect = true;
			}
			else
			{
				an -> c_ts = 0;
				an -> connect = false;
			}

			if (nc -> lr_timestamp > 0)
			{
				an -> l_ts = nc -> lr_timestamp;
				an -> listen = true;
			}
			else
			{
				an -> l_ts = 0;
				an -> listen = false;
			}

			neighbours.push_back(an);
		}
	}
	return 1;
}

int 	p3disc::distillData()
{
	// This transforms the autoneighbour tree into 
	// a list of certificates with the best guess settings.

	// get any extra. from sslroot.
	collectCerts();

	discovered.clear();

	std::ostringstream out;
	out << "p3disc::distillData()" << std::endl;

	std::list<autoneighbour *>::iterator it;
	std::list<autoneighbour *>::iterator it2;
	std::list<autoserver *>::iterator nit;
	cert *own = sroot -> getOwnCert();

	for(it = neighbours.begin(); it != neighbours.end(); it++)
	{
		/* for the moment this is going to be a simplistic
		 * (and non-fault tolerent design)....
		 * we will take the most up-to-date values.... from the friends of neighbours.
		 *
		 * if these are more up-to-date than both the
		 * (1) neighbour (*it) and
		 * (2) the actual certificate and
		 * (3) we are not connected... then 
		 *
		 * (a) we update the addresses and timestamps on the neighbour.
		 * (b) addresses on the certificate.
		 *
		 * Therefore 
		 * 	cert has (1) our connect times, (2) best guess server.
		 * 	neighbour has uptodate times/servers from last distill.
		 * 
		 * NOTE this requires a better algorithm.
		 *
		 */

		unsigned int mr_connect = 0;
		unsigned int mr_listen  = 0;

		unsigned int mr_both = 0; /* connect or receive */
		/* three fields below match most recent (of either) */
		struct sockaddr_in mr_server; 
		struct sockaddr_in mr_local;  
		unsigned int mr_flags = 0;

		/* if we find a neighbour_of, which is the same cert.
		 * then we have the definitive answer already 
		 * (and it has been installed)
		 */

		bool haveDefinitive = false;

		cert *c = (cert *) (*it) -> id;
		for(nit = ((*it) -> neighbour_of).begin(); 
				nit != ((*it) -> neighbour_of).end(); nit++)
		{
			out << "\tDistill Connected via: ";
			if ((*nit) -> id != NULL)
			{
				out << ((*nit) ->id) -> Name();
			}
			out << std::endl;
			out << "\t\tServer: ";
			out <<  inet_ntoa((*nit)->server_addr.sin_addr);
			out << ":" << ntohs((*nit)->server_addr.sin_port);
			out << std::endl;
			if ((*nit)->id == (*it)->id)
			{
				haveDefinitive = true;
				out << "\t\tIs Definitive Answer!";
				out << std::endl;
			}

			if ((*nit) -> listen)
			{
				if ((*nit)->l_ts > mr_listen)
				{
				    mr_listen = (*nit)->l_ts;
				    if (mr_listen > mr_both)
				    {
				    	mr_both = mr_listen;
					mr_server = (*nit) -> server_addr;
					mr_local  = (*nit) -> local_addr;
					mr_flags  = (*nit) -> discFlags;
				    }
				}
			}

			if ((*nit) -> connect)
			{
				if ((*nit) -> c_ts > mr_connect)
				{
				  mr_connect = (*nit)->c_ts;
				  if (mr_connect > mr_both)
				  {
				    	mr_both = mr_connect;
					mr_server = (*nit) -> server_addr;
					mr_local  = (*nit) -> local_addr;
					mr_flags  = (*nit) -> discFlags;
				  }
				}
			}
		}
		
		if ((c == own) || (haveDefinitive))
		{
			out << c -> Name();
			out << ": Is Own or Definitive: no Update...";
			out << std::endl;

			discovered.push_back(c);
			continue;
		}

		if ((mr_both > (*it)-> c_ts) && (mr_both > (*it)-> l_ts))
		{
			(*it) -> server_addr = mr_server;
			(*it) -> local_addr  = mr_local;
			(*it) -> discFlags   = mr_flags;

		}

		/* now we can check against (*it) */
		if ((!(*it)->listen) || ((*it)-> l_ts < mr_listen))
		{
			(*it) -> listen = true;
			(*it)-> l_ts = mr_listen;
		}

		if ((!(*it)->connect) || ((*it)-> c_ts < mr_connect))
		{
			(*it) -> connect = true;
			(*it)-> c_ts = mr_connect;
		}

			/* XXX fixme ***/
		// Finally we can update the certificate, if auto
		// is selected.... or not in use.
		if (!(c -> Connected()))
		{
			out << "Checking: " << c -> Name() << std::endl;

			// if empty local 
			if (0 == inaddr_cmp(c -> localaddr, INADDR_ANY)) 
			{
			  out << "\tUpdating NULL Local Addr:" << std::endl;
			  out << "\t\tOld: ";
			  out <<  inet_ntoa(c->localaddr.sin_addr);
			  out << ":" << ntohs(c->localaddr.sin_port);
			  c -> localaddr = (*it) -> local_addr;
			  out << "\t\tNew: ";
			  out <<  inet_ntoa(c->localaddr.sin_addr);
			  out << ":" << ntohs(c->localaddr.sin_port);
			}

			// if empty server .....
			if (0 == inaddr_cmp(c -> serveraddr, INADDR_ANY)) 
			{
			  out << "\tUpdating NULL Serv Addr:" << std::endl;
			  out << "\t\tOld: ";
			  out <<  inet_ntoa(c->serveraddr.sin_addr);
			  out << ":" << ntohs(c->serveraddr.sin_port);
			  c -> serveraddr = (*it) -> server_addr;
			  out << "\t\tNew: ";
			  out <<  inet_ntoa(c->serveraddr.sin_addr);
			  out << ":" << ntohs(c->serveraddr.sin_port);
			}
			// if local (second as should catch empty)
			else if ((0 == inaddr_cmp((*it) -> server_addr, 
						c -> localaddr)))
				 //&& (inaddr_local(c -> localaddr))
			{
				out << "\tMaking Local..." << std::endl;
				c -> Local(true);
			}

			// Finally the key update .... 
			// check only against the latest data....
			
			if (mr_both) 
			{
				// 
				unsigned int cert_both = c -> lc_timestamp;
				if (cert_both < (unsigned) c -> lr_timestamp)
				{
					cert_both = c -> lr_timestamp;
				}

				int log_delta = -1; /* invalid log */
				if (mr_both > cert_both)
				{
					log_delta = (int) log10((double) (mr_both - cert_both));
				}

				/* if a peer has connected more recently than us */
				if (log_delta > 3) // or > 10000 (secs), or ~3 hours.
				{
			  		out << "\tUpdating OLD Addresses:" << std::endl;
			  		out << "\t\tOld Local: ";
			  		out <<  inet_ntoa(c->serveraddr.sin_addr);
			  		out << ":" << ntohs(c->serveraddr.sin_port);
					out << std::endl;
			  		out << "\t\tOld Server: ";
			  		out <<  inet_ntoa(c->serveraddr.sin_addr);
			  		out << ":" << ntohs(c->serveraddr.sin_port);
					out << std::endl;
					if (c->Firewalled())
					{
			  			out << "\t\tFireWalled/";
					}
					else
					{
			  			out << "\t\tNot FireWalled/";
					}
					if (c->Forwarded())
					{
			  			out << "Forwarded";
					}
					else
					{
			  			out << "Not Forwarded";
					}
					out << std::endl;

					if (0!=inaddr_cmp(mr_server, INADDR_ANY))
					{
			  			c -> serveraddr = mr_server;
					}

					if (0!=inaddr_cmp(mr_local, INADDR_ANY))
					{
			  			c -> localaddr = mr_local;
					}

					updateCertAvailabilityFlags(c, mr_flags);

			  		out << "\t\tNew: ";
			  		out <<  inet_ntoa(c->serveraddr.sin_addr);
			  		out << ":" << ntohs(c->serveraddr.sin_port);
			  		out << "\t\tNew Local: ";
			  		out <<  inet_ntoa(c->serveraddr.sin_addr);
			  		out << ":" << ntohs(c->serveraddr.sin_port);
					out << std::endl;
			  		out << "\t\tNew Server: ";
			  		out <<  inet_ntoa(c->serveraddr.sin_addr);
			  		out << ":" << ntohs(c->serveraddr.sin_port);
					out << std::endl;
					if (c->Firewalled())
					{
			  			out << "\t\tFireWalled/";
					}
					else
					{
			  			out << "\t\tNot FireWalled/";
					}
					if (c->Forwarded())
					{
			  			out << "Forwarded";
					}
					else
					{
			  			out << "Not Forwarded";
					}
					out << std::endl;
			 	}
			}
		}
		discovered.push_back(c);
	}
	pqioutput(PQL_DEBUG_BASIC, pqidisczone, out.str());
	idServers();
	return 1;
}

std::list<cert *> &p3disc::getDiscovered()
{
	return discovered;
}

static const std::string pqi_adflags("PQI_ADFLAGS");

int	p3disc::save_configuration()
{
	if (sroot == NULL)
		return -1;

	std::string localflags;
	if (local_disc)
		localflags += "L";
	if (remote_disc)
		localflags += "R";
	sroot -> setSetting(pqi_adflags, localflags);
	return 1;
}


// load configuration from sslcert -> owncert()
// instead of from the configuration files.

int	p3disc::load_configuration()
{
	unsigned int i = 0;

	if (sroot == NULL)
		return -1;

	Person *p = sroot -> getOwnCert();
	if (p == NULL)
		return -1;
	laddr = p -> localaddr;
	//laddr.sin_family = AF_INET;

	saddr = p -> serveraddr;
	local_firewalled = p -> Firewalled();
	local_forwarded = p -> Forwarded();

	std::string localflags = sroot -> getSetting(pqi_adflags);
	// initially drop out gracefully.
	if (localflags.length() == 0)
		return 1;
	if (i < localflags.length()) 
		if (local_disc = ('L' == localflags[i]))
			i++;
	if (i < localflags.length()) 
		if (remote_disc = ('R' == localflags[i]))
			i++;
	// temp turn on!
	local_disc = false; // true;
	remote_disc = true;
	return 1;
}


std::list<cert *> p3disc::potentialproxy(cert *target)
{
	std::list<cert *> certs;
	// search the discovery tree for proxies for target.

	std::list<autoneighbour *>::iterator it;
	std::list<autoserver *>::iterator nit;

	pqioutput(PQL_DEBUG_BASIC, pqidisczone, 
		"p3disc::potentialproxy()");

	for(it = neighbours.begin(); it != neighbours.end(); it++)
	{
		cert *c = (cert *) (*it) -> id;
		if (c == target)
		{
			// found target.
			for(nit = ((*it) -> neighbour_of).begin(); 
				nit != ((*it) -> neighbour_of).end(); nit++)
			{
			  /* can't use target as proxy */
			  cert *pp = (cert *) (*nit)->id;
			  if ((pp -> Connected()) &&  (target != pp))
			  {
				std::ostringstream out;
				out << "Potential Proxy: ";
				out << pp -> Name();
				certs.push_back(pp);
				pqioutput(PQL_DEBUG_BASIC, pqidisczone, 
						out.str());
			  }
			}

			return certs;
		}
	}

	pqioutput(PQL_DEBUG_BASIC, pqidisczone, 
		"p3disc::potentialproxy() No proxies found");
	// empty list.
	return certs;
}

std::list<struct sockaddr_in> p3disc::requestStunServers()
{

	/* loop through all the possibilities 
	 *
	 * find the ones which aren't firewalled.
	 *
	 * get their addresses.
	 */
	cert *own = sroot -> getOwnCert();

	std::list<struct sockaddr_in> stunList;

	std::list<autoneighbour *>::iterator it;
	for(it = neighbours.begin(); it != neighbours.end(); it++)
	{
		cert *c = (cert *) (*it) -> id;

		/* if flags are correct, and the address looks
		 * valid.
		 */

/* switch on Local Stun for testing */
/*
 * #define STUN_ALLOW_LOCAL_NET 1
 */


#ifdef STUN_ALLOW_LOCAL_NET
		bool isExtern = true;
#else
		bool isExtern = (!c->Firewalled()) || 
			(c->Firewalled() && c->Forwarded());
#endif

		if (isExtern)
		{
			// second level of checks.
			// if we will connect, and haven't -> they are probably
			// offline.
			if (c->Accepted() && (!c->Connected()))
			{
				std::ostringstream out;
				out << "Offline Friend: ";
				out << c -> Name();
				out << " not available for Stun";
				pqioutput(PQL_DEBUG_ALERT, pqidisczone, out.str());
				isExtern = false;
			}

			// and address looks good.
			//
			// and not in our subnet (external to us)
		}


		if (isExtern)
		{
			std::ostringstream out;
			out << "Potential Stun Server: ";
			out << c -> Name();
			out << std::endl;
			out << " ServerAddr: " << inet_ntoa(c->serveraddr.sin_addr);
			out << " : " << ntohs(c->serveraddr.sin_port);
			out << std::endl;
			out << " LocalAddr: " << inet_ntoa(c->localaddr.sin_addr);
			out << " : " << ntohs(c->localaddr.sin_port);
			out << std::endl;

#ifdef STUN_ALLOW_LOCAL_NET
			if (isValidNet(&(c->serveraddr.sin_addr)) &&
				 (!sameNet(&(own->serveraddr.sin_addr), &(c->serveraddr.sin_addr))))
#else
			if ((isValidNet(&(c->serveraddr.sin_addr))) &&
				(!isPrivateNet(&(c->serveraddr.sin_addr))) &&
				 (!sameNet(&(own->localaddr.sin_addr), &(c->serveraddr.sin_addr))) &&
				 (!sameNet(&(own->serveraddr.sin_addr), &(c->serveraddr.sin_addr))))
#endif
			{
				out << " -- Chose Server Address";
				out << std::endl;
				stunList.push_back(c->serveraddr);
			} 
#ifdef STUN_ALLOW_LOCAL_NET
			else if (isValidNet(&(c->localaddr.sin_addr)))
#else
			else if ((!c->Firewalled()) && 
				 (isValidNet(&(c->localaddr.sin_addr))) &&
				 (!isPrivateNet(&(c->localaddr.sin_addr))) &&
				 (!sameNet(&(own->localaddr.sin_addr), &(c->localaddr.sin_addr))))
#endif
			{
				out << " -- Chose Local Address";
				out << std::endl;
				stunList.push_back(c->localaddr);
			}
			else
			{
				out << "<=> Invalid / Private Addresses";
				out << std::endl;
			}
			pqioutput(PQL_DEBUG_ALERT, pqidisczone, out.str());
		}
		else
		{
			std::ostringstream out;
			out << "Non-Stun Neighbour: ";
			out << c -> Name();
			pqioutput(PQL_DEBUG_ALERT, pqidisczone, out.str());
		}
	}

	return stunList;
}





// tdelta     -> trange.
// -inf...<0	   0 (invalid)
//    0.. <9       1
//    9...<99      2
//   99...<999	   3
//  999...<9999    4
//  etc...

int convertTDeltaToTRange(double tdelta)
{
	if (tdelta < 0)
		return 0;
	int trange = 1 + (int) log10(tdelta + 1.0);
	return trange;

}

// trange     -> tdelta
// -inf...0	  -1 (invalid)
//    1            8
//    2           98
//    3          998
//    4         9998
//  etc...

int convertTRangeToTDelta(int trange)
{
	if (trange <= 0)
		return -1;
	
	return (int) (pow(10.0, trange) - 1.5); // (int) xxx98.5 -> xxx98
}

// fn which updates: connect, c_ts, 
// 			listen, l_ts, 
// 			local_addr, server_addr, 
// 			and discFlags.
int updateAutoServer(autoserver *as, DiscItem *di)
{
	int cts = time(NULL);


	as->listen = (di->receive_tr != 0);
	as->connect= (di->connect_tr != 0);

	/* convert [r|c]_tf to timestamps.... 
	 *
	 * Conversion to a _tf....
	 *
	 *
	 * */
	if (as->listen)
	{
		as->l_ts = cts - convertTRangeToTDelta(di->receive_tr);
	}

	if (as->connect)
	{
		as->c_ts = cts - convertTRangeToTDelta(di->connect_tr);

	}
	as->local_addr = di->laddr;
	as->server_addr = di->saddr;
	as->discFlags = di->discFlags;

	return 1;
}


static const int PQI_DISC_FLAGS_FIREWALLED = 0x0001;
static const int PQI_DISC_FLAGS_FORWARDED  = 0x0002;
static const int PQI_DISC_FLAGS_LOCAL	     = 0x0004;

int updateCertAvailabilityFlags(cert *c, unsigned long discFlags)
{
	if (c)
	{
		c->Firewalled(discFlags & PQI_DISC_FLAGS_FIREWALLED);
		c->Forwarded(discFlags & PQI_DISC_FLAGS_FORWARDED);
 
		if (discFlags & PQI_DISC_FLAGS_FIREWALLED)
		{
	  		pqioutput(PQL_WARNING, pqidisczone, 
				"updateCertAvailabilityFlags() Setting Firewalled Flag = true");
		}
		else
		{
	  		pqioutput(PQL_WARNING, pqidisczone, 
				"updateCertAvailabilityFlags() Setting Firewalled Flag = false");
		}

		if (discFlags & PQI_DISC_FLAGS_FORWARDED)
		{
	  		pqioutput(PQL_WARNING, pqidisczone, 
				"updateCertAvailabilityFlags() Setting Forwarded Flag = true");
		}
		else
		{
	  		pqioutput(PQL_WARNING, pqidisczone, 
				"updateCertAvailabilityFlags() Setting Forwarded Flag = false");
		}

		return 1;
	}
	return 0;
}


unsigned long determineCertAvailabilityFlags(cert *c)
{
	unsigned long flags = 0;
	if (c->Firewalled())
	{
		flags |= PQI_DISC_FLAGS_FIREWALLED;
	}

	if (c->Forwarded())
	{
		flags |= PQI_DISC_FLAGS_FORWARDED;
	}

	if (c->Local())
	{
		flags |= PQI_DISC_FLAGS_LOCAL;
	}
	
	return flags;
}

