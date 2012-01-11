/*
 * "$Id: pqissl.h,v 1.18 2007-03-11 14:54:22 rmf24 Exp $"
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



#ifndef MRK_PQI_SSL_TUNNEL_HEADER
#define MRK_PQI_SSL_TUNNEL_HEADER

#include "util/rswin.h"

#include <openssl/ssl.h>

// operating system specific network header.
//#include "pqi/pqinetwork.h"

#include "pqi/pqi_base.h"

#include "services/p3tunnel.h"

#include "pqi/authssl.h"

/***************************** pqi Net SSL Interface *********************************
 * This provides the base SSL interface class,
 * and handles most of the required functionality.
 *
 * there are a series of small fn's that can be overloaded
 * to provide alternative behaviour....
 *
 * Classes expected to inherit from this are:
 *
 * pqissllistener 	-> pqissllistener  (tcp only)
 * 			-> pqixpgplistener (tcp only)
 *
 * pqissl	 	-> pqissltcp
 * 			-> pqissludp
 * 			-> pqixpgptcp
 * 			-> pqixpgpudp
 *
 */

class pqissl;
class cert;

class pqissltunnellistener;

class p3LinkMgr;

struct data_with_length {
    int length;
    void *data;
};

class pqissltunnel: public NetBinInterface
{
public:
        pqissltunnel(PQInterface *parent, p3LinkMgr *cm, p3tunnel *p3t);
virtual ~pqissltunnel();

	// NetInterface

//the addr is not used for the tunnel
virtual int connect(struct sockaddr_in raddr);
virtual int listen();
virtual int stoplistening();
virtual int reset();
virtual int disconnect();
virtual int getConnectAddress(struct sockaddr_in &raddr);

virtual bool connect_parameter(uint32_t type, uint32_t value);

	// BinInterface
virtual int	tick();
virtual int     status();

virtual int senddata(void*, int);
virtual int readdata(void*, int);
virtual int netstatus();
virtual int isactive();
virtual bool moretoread();
virtual bool cansend();

virtual int close(); /* BinInterface version of reset() */
virtual std::string gethash(); /* not used here */
virtual bool bandwidthLimited() { return true ; } // replace by !sameLAN to avoid bandwidth limiting on lAN

//called by the p3tunnel service to add incoming packets that will be read by the read data function.
void addIncomingPacket(void* encoded_data, int data_length);
void IncommingPingPacket();
void IncommingHanshakePacket(std::string incRelayPeerId);

private:
        //if no packet (last_time_packet_time) is received since PING_RECEIVE_TIME_OUT, let's assume the connection is broken
        int last_normal_connection_attempt_time;

        //if no packet (last_time_packet_time) is received since PING_RECEIVE_TIME_OUT, let's assume the connection is broken
        int last_packet_time;

        //send a ping on a regular basis
        int last_ping_send_time;

        int ConnectAttempt();
        void spam_handshake();
	int waiting;
	bool active;
        time_t resetTime;
	pqissltunnellistener *pqil;

	/* Need Certificate specific functions here! */
	time_t   mConnectTS;

	p3LinkMgr *mLinkMgr;

	p3tunnel *mP3tunnel;

	std::list<data_with_length> data_packet_queue;
	data_with_length curent_data_packet;
	int current_data_offset;

	//tunneling details
	std::string  relayPeerId;

};

#endif // MRK_PQI_SSL_HEADER
