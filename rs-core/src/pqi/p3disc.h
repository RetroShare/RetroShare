/*
 * "$Id: p3disc.h,v 1.11 2007-02-18 21:46:49 rmf24 Exp $"
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



#ifndef MRK_PQI_AUTODISC_H
#define MRK_PQI_AUTODISC_H


// The AutoDiscovery Class

#include <string>
#include <list>

// system specific network headers
#include "pqi/pqinetwork.h"

#include "pqi/pqi.h"

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)

#include "pqi/xpgpcert.h"

#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#include "pqi/sslcert.h"

#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/


#include "pqi/discItem.h"
#include "pqi/pqitunnel.h"

class autoserver
{
	public:
		autoserver()
		:id(NULL), ca(NULL), connect(false), c_ts(0), 
		listen(false), l_ts(0), discFlags(0) { return;}

		Person *id;
		Person *ca;
		bool connect;
		unsigned int c_ts; // this is connect_tf converted to timestamp, 0 invalid.

		bool listen;
		unsigned int l_ts; // this is receive_tf converted to timestamp, 0 invalid.

		struct sockaddr_in local_addr;
		struct sockaddr_in server_addr;
		unsigned long discFlags;
};


class autoneighbour: public autoserver
{
	public:
		autoneighbour()
		:autoserver(), local(false), active(false) {}

		bool local;
		bool active; // meaning in ssl's list.
		std::list<autoserver *> neighbour_of;

};


class p3disc: public PQTunnelService
{
	public:
		bool local_disc;
		bool remote_disc;
		//sslroot *sslbase;

		p3disc(sslroot *r);
virtual		~p3disc();

		// PQTunnelService functions.
virtual int     receive(PQTunnel *);
virtual PQTunnel *send();
virtual PQTunnel *getObj();

virtual		int tick();

		// doesn't use the init functions.
virtual int     receive(PQTunnelInit *i) { delete i; return 1; }
virtual PQTunnelInit *sendInit() { return NULL; }
virtual PQTunnelInit *getObjInit() { return NULL; }

		// End of PQTunnelService functions.
		//
		// For Proxy Information.
std::list<sockaddr_in> requestStunServers();
std::list<cert *> potentialproxy(cert *target);

		// load and save configuration to sslroot.
		int save_configuration();
		int load_configuration();

		int ts_lastcheck;

		int idServers();

		// Handle Local Discovery.
		int localListen();
		int localSetup();
	
		int lsock; // local discovery socket.
		struct sockaddr_in laddr; // local addr
		struct sockaddr_in baddr; // local broadcast addr.
		struct sockaddr_in saddr; // pqi ssl server addr.

		// bonus configuration flags.
 		bool local_firewalled;
         	bool local_forwarded;


		// local message construction/destruction.
		void *ldata;
		int ldlen;
		int ldlenmax;


		bool std_port; // if we have bound to default.
		int  ts_nextlp; // -1 for never (if on default)

		// helper functions.
		int setLocalAddress(struct sockaddr_in srvaddr);
		int determineLocalNetAddr();
		int setupLocalPacket(int type, struct sockaddr_in *home,
					struct sockaddr_in *server);
		int localPing(struct sockaddr_in);
		int localReply(struct sockaddr_in);
		int addLocalNeighbour(struct sockaddr_in*, struct sockaddr_in*);

		// remote discovery function.
		int newRequests();
		int handleReplies();

		int handleDiscoveryData(DiscReplyItem *di);
		int handleDiscoveryPing(DiscItem *di);
		int sendDiscoveryReply(cert *);
		int collectCerts();
		int distillData();

		//cert *checkDuplicateX509(X509 *x509);
		std::list<cert *> &getDiscovered();

		// Main Storage
		std::list<autoneighbour *> neighbours;
		std::list<cert *> ad_init;

		std::list<cert *> discovered;
		sslroot *sroot;

		// Storage for incoming/outgoing.
		std::list<PQItem *> inpkts;
		std::list<PQItem *> outpkts;
};

#endif // MRK_PQI_AUTODISC_H
