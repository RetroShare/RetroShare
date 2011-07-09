/*
 * "$Id: pqissl.cc,v 1.28 2007-03-17 19:32:59 rmf24 Exp $"
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





#include "pqi/pqissltunnel.h"
#include "pqi/pqinetwork.h"
#include "pqi/p3linkmgr.h"

//#include "services/p3tunnel.h"

#include "util/rsnet.h"
#include "util/rsdebug.h"

#include <unistd.h>
#include <errno.h>
#include <openssl/err.h>

#include <sstream>

const int pqisslzone = 37714;

#define TUNNEL_WAITING_NOT		0
#define TUNNEL_WAITING_DELAY		1
#define TUNNEL_WAITING_SPAM_HANDSHAKE	2
#define TUNNEL_WAITING_RETURN_HANDSHAKE	3


#define TUNNEL_PASSIVE  0x00
#define TUNNEL_ACTIVE   0x01

#define TUNNEL_START_CONNECTION_DELAY	 1
#define TUNNEL_PING_TIMEOUT   6
#define TUNNEL_REPEAT_PING_TIME   2
#define TUNNEL_TIMEOUT_AFTER_RESET   30

#define TUNNEL_TRY_OTHER_CONNECTION_INTERVAL   190 //let's try a normal tcp or udp connection every 190 sec

//const int TUNNEL_LOCAL_FLAG = 0x01;
//const int TUNNEL_REMOTE_FLAG = 0x02;
//const int TUNNEL_UDP_FLAG = 0x02;
//
//static const int PQISSL_MAX_READ_ZERO_COUNT = 20;
//static const int PQISSL_SSL_CONNECT_TIMEOUT = 30;

/********** PQI SSL STUFF ******************************************
 *
 * A little note on the notifyEvent(FAILED)....
 *
 * this is called from 
 * (1) reset if needed!
 * (2) Determine_Remote_Address (when all options have failed).
 *
 * reset() is only called when a TCP/SSL connection has been
 * established, and there is an error. If there is a failed TCP 
 * connection, then an alternative address can be attempted.
 *
 * reset() is called from
 * (1) destruction.
 * (2) disconnect()
 * (3) bad waiting state.
 *
 * // TCP/or SSL connection already established....
 * (5) pqissltunnel::SSL_Connection_Complete() <- okay -> cos we made a TCP connection already.
 * (6) pqissltunnel::accept() <- okay cos something went wrong.
 * (7) moretoread()/cansend() <- okay cos 
 *
 */

pqissltunnel::pqissltunnel(PQInterface *parent, p3LinkMgr *cm, p3tunnel *p3t)
        :NetBinInterface(parent, parent->PeerId()), mLinkMgr(cm)
{
	active = false;
	waiting = TUNNEL_WAITING_NOT;

  	{
	  std::ostringstream out;
	  out << "pqissltunnel for PeerId: " << PeerId();
	  rslog(RSL_ALERT, pqisslzone, out.str());
	}

//        if (!(AuthSSL::getAuthSSL()->isAuthenticated(PeerId()))) {
//	  rslog(RSL_ALERT, pqisslzone,
//	    "pqissltunnel::Warning Certificate Not Approved!");
//	  rslog(RSL_ALERT, pqisslzone,
//	    "\t pqissltunnel will not initialise....");
//	}
	mP3tunnel = p3t;
	current_data_offset = 0;
	curent_data_packet.length = 0;

	return;
}

pqissltunnel::~pqissltunnel() {
  	rslog(RSL_ALERT, pqisslzone, 
	    "pqissltunnel::~pqissltunnel -> destroying pqissl");
        //stoplistening();
	reset(); 
	return;
}


/********** Implementation of NetInterface *************************/

int	pqissltunnel::connect(struct sockaddr_in raddr) {
#ifdef DEBUG_PQISSL_TUNNEL
	std::cerr << "pqissltunnel::connect() called : " << PeerId() << std::endl;
#endif
        last_normal_connection_attempt_time = time(NULL);
	mConnectTS = time(NULL);
        resetTime = time(NULL) - TUNNEL_TIMEOUT_AFTER_RESET;
	waiting = TUNNEL_WAITING_DELAY;
	return 0;
}

// tells pqilistener to listen for us.
int	pqissltunnel::listen()
{
	//no use
	return 0;
}

int 	pqissltunnel::stoplistening()
{
	//no use
	return 1;
}

int 	pqissltunnel::disconnect()
{
#ifdef DEBUG_PQISSL_TUNNEL
	std::cerr << "pqissltunnel::disconnect() called : " << PeerId() << std::endl;
#endif
	return reset();
}

/* BinInterface version of reset() for pqistreamer */
int 	pqissltunnel::close()
{
	return reset();
}

// put back on the listening queue.
int 	pqissltunnel::reset()
{
#ifdef DEBUG_PQISSL_TUNNEL
	std::cerr << "pqissltunnel::reset() called : " << PeerId()  << std::endl;
#endif

	if (active)
	{
#ifdef DEBUG_PQISSL_TUNNEL
		std::cerr << "pqissltunnel::reset() Reset Required because tunnel was activated !" << std::endl;
		std::cerr << "pqissltunnel::reset() Will Attempt notifyEvent(FAILED)" << std::endl;
#endif
		waiting = TUNNEL_WAITING_NOT;
		active = false;
                resetTime = time(NULL);
		// clean up the streamer
		if (parent()) {
#ifdef DEBUG_PQISSL_TUNNEL
		    std::cerr << "pqissltunnel::reset() notifyEvent(FAILED)" << std::endl;
#endif
		  parent() -> notifyEvent(this, NET_CONNECT_FAILED);
		}
	} else {
#ifdef DEBUG_PQISSL_TUNNEL
		std::cerr << "pqissltunnel::reset() Reset not required because tunnel was not activated !" << std::endl;
#endif
	}
	return 1;
}

int pqissltunnel::getConnectAddress(struct sockaddr_in &raddr) {
    sockaddr_clear(&raddr);
    return 0;
}

bool 	pqissltunnel::connect_parameter(uint32_t type, uint32_t value)
{
	{
                std::ostringstream out;
                out << "pqissltunnel::connect_parameter() (not used) Peer: " << PeerId();
		out << " type: " << type << "value: " << value;
		rslog(RSL_DEBUG_ALL, pqisslzone, out.str());
	}

        if (type == NET_PARAM_CONNECT_DELAY)
	{
                std::ostringstream out;
                out << "pqissltunnel::connect_parameter() (not used) Peer: " << PeerId();
		out << " DELAY: " << value;
		rslog(RSL_WARNING, pqisslzone, out.str());

		return true;
	}
        else if (type == NET_PARAM_CONNECT_TIMEOUT)
	{
                std::ostringstream out;
                out << "pqissltunnel::connect_parameter() (not used) Peer: " << PeerId();
		out << " TIMEOUT: " << value;
		rslog(RSL_WARNING, pqisslzone, out.str());

		return true;
	}
	return false;
}


/********** End of Implementation of NetInterface ******************/
/********** Implementation of BinInterface **************************
 * Only status() + tick() are here ... as they are really related
 * to the NetInterface, and not the BinInterface,
 *
 */

/* returns ...
 * -1 if inactive.
 *  0 if connecting.
 *  1 if connected.
 */

int 	pqissltunnel::status()
{
#ifdef DEBUG_PQISSL_TUNNEL
	std::cerr << "pqissltunnel::status() called." << std::endl;
#endif

	if (active) {
		std::cerr << " active: " << std::endl;
		// print out connection.
		std::cerr << "dest : " << PeerId();
		std::cerr << "relay : " << relayPeerId;
		std::cerr << std::endl;
	}
	else {
		std::cerr << " Waiting for connection!" << std::endl;
	}

	if (active) {
		return 1;
	} else if (waiting > 0) {
		return 0;
	}
	return -1;
}


int	pqissltunnel::tick()
{
        if (active && ((time(NULL) - last_normal_connection_attempt_time) > TUNNEL_TRY_OTHER_CONNECTION_INTERVAL)) {
        #ifdef DEBUG_PQISSL_TUNNEL
                std::cerr << "pqissltunnel::tick() attempt to connect through a normal tcp or udp connection." << std::endl;
        #endif
                last_normal_connection_attempt_time = time(NULL);
                mLinkMgr->retryConnect(parent()->PeerId());
        }

        if (active && ((time(NULL) - last_ping_send_time) > TUNNEL_REPEAT_PING_TIME)) {
        #ifdef DEBUG_PQISSL_TUNNEL
                std::cerr << "pqissltunnel::tick() sending a ping." << std::endl;
        #endif
                last_ping_send_time = time(NULL);;
                mP3tunnel->pingTunnelConnection(relayPeerId, parent()->PeerId());
        }

        if (active && ((time(NULL) - last_packet_time) > TUNNEL_PING_TIMEOUT)) {
        #ifdef DEBUG_PQISSL_TUNNEL
                std::cerr << "pqissltunnel::tick() no packet received since PING_RECEIVE_TIME_OUT. Connection is broken." << std::endl;
        #endif
                reset();
        }
	// continue existing connection attempt.
	if (!active)
	{
		// if we are waiting.. continue the connection (only)
		if (waiting > 0)
		{
                        #ifdef DEBUG_PQISSL_TUNNEL
                        //std::cerr << "pqissltunnel::tick() Continuing Connection Attempt!" << std::endl;
                        #endif
			ConnectAttempt();
			return 1;
		}
	}
	return 1;
}

/********** End of Implementation of BinInterface ******************/
/********** Internals of Tunnel Connection ****************************/
int 	pqissltunnel::ConnectAttempt()
{
        #ifdef DEBUG_PQISSL_TUNNEL
        //std::cerr << "pqissltunnel::ConnectAttempt() called." << std::endl;
        #endif
	switch(waiting)
	{
		case TUNNEL_WAITING_NOT:
			active = true; /* we're starting this one */
                        #ifdef DEBUG_PQISSL_TUNNEL
			std::cerr << "pqissltunnel::ConnectAttempt() STATE = Not Waiting." << std::endl;
                        #endif

		case TUNNEL_WAITING_DELAY:
                        if ((time(NULL) - mConnectTS) > TUNNEL_START_CONNECTION_DELAY) {
                            waiting = TUNNEL_WAITING_SPAM_HANDSHAKE;
			}

			break;

                case TUNNEL_WAITING_SPAM_HANDSHAKE:
                        #ifdef DEBUG_PQISSL_TUNNEL
                        std::cerr << "pqissltunnel::ConnectAttempt() STATE = Waiting for spamming handshake." << std::endl;
                        #endif

                        spam_handshake();
                        waiting = TUNNEL_WAITING_RETURN_HANDSHAKE;
			break;

                case TUNNEL_WAITING_RETURN_HANDSHAKE:
                        if ((time(NULL) - mConnectTS) < TUNNEL_PING_TIMEOUT) {
                            #ifdef DEBUG_PQISSL_TUNNEL
                            //std::cerr << "pqissltunnel::ConnectAttempt() STATE = Waiting for handshake reply." << std::endl;
                            #endif
			} else {
                            #ifdef DEBUG_PQISSL_TUNNEL
                            std::cerr << "pqissltunnel::ConnectAttempt() no handshake reply during imparing time. Connection failed." << std::endl;
                            #endif
			    waiting = TUNNEL_WAITING_NOT;
			    active = false;
			    // clean up the streamer
			    if (parent()) {
#ifdef DEBUG_PQISSL_TUNNEL
				std::cerr << "pqissltunnel::reset() Reset not required because tunnel was not activated !" << std::endl;
#endif
			      parent() -> notifyEvent(this, NET_CONNECT_FAILED);
			    }
			}
			break;


		default:
			std::cerr << "pqissltunnel::ConnectAttempt() STATE = Unknown - Reset" << std::endl;

			reset();
			break;
	}
	return -1;
}

void 	pqissltunnel::spam_handshake()
{
#ifdef DEBUG_PQISSL_TUNNEL
        std::cerr << "pqissltunnel::spam_handshake() starting to spam handshake tunnel packet." << std::endl;
#endif
	std::list<std::string> peers;
	mLinkMgr->getOnlineList(peers);
	std::list<std::string>::iterator it = peers.begin();
	while (it !=  peers.end()) {
            //send a handshake to the destination through the relay
            if (*it != parent()->PeerId()) {
                mP3tunnel->initiateHandshake(*it, parent()->PeerId());
	    }
	    ++it;
	}
}

void pqissltunnel::addIncomingPacket(void* encoded_data, int encoded_data_length) {
    #ifdef DEBUG_PQISSL_TUNNEL
    std::cerr << "pqissltunnel::addIncomingPacket() called." << std::endl;
    #endif
    last_packet_time = time(NULL);

    data_with_length data_w_l;
    data_w_l.data = (void*)malloc(encoded_data_length) ;
    memcpy(data_w_l.data, encoded_data, encoded_data_length);
    data_w_l.length = encoded_data_length;
    data_packet_queue.push_front(data_w_l);
}

void pqissltunnel::IncommingPingPacket() {
    #ifdef DEBUG_PQISSL_TUNNEL
    std::cerr << "pqissltunnel::IncommingPingPacket() called" << std::endl;
    #endif

    last_packet_time = time(NULL);
}

void pqissltunnel::IncommingHanshakePacket(std::string incRelayPeerId) {
    #ifdef DEBUG_PQISSL_TUNNEL
    std::cerr << "pqissltunnel::IncommingHanshakePacket() called with incRelayPeerId : " << incRelayPeerId << std::endl;
    #endif

    if ((time(NULL) - resetTime) <= TUNNEL_TIMEOUT_AFTER_RESET) {
#ifdef DEBUG_PQISSL_TUNNEL
        std::cerr << "pqissltunnel::IncommingHanshakePacket() a reset occured, don't activate the connection." << std::endl;
#endif
        return;
    }
    last_packet_time = time(NULL);

    std::string message = "pqissltunnel::IncommingHanshakePacket() mConnMgr->isOnline(parent()->PeerId() : ";
    if (mLinkMgr->isOnline(parent()->PeerId())) {
        message += "true";
    } else {
        message += "false";
    }
    rslog(RSL_DEBUG_BASIC, pqisslzone, message);

    if (active || mLinkMgr->isOnline(parent()->PeerId())) {
        //connection is already active, or peer is already online don't do nothing
        return;
    }
    //TODO : check if cert is in order before accepting

    //activate connection
    waiting = TUNNEL_WAITING_NOT;
    active = true;
    relayPeerId = incRelayPeerId;

    if (parent())
    {
        #ifdef DEBUG_PQISSL_TUNNEL
        std::cerr << "pqissltunnel::IncommingHanshakePacket() Notify the pqiperson.... (Both Connect/Receive)" <<  parent()->PeerId() <<std::endl;
        #endif
        rslog(RSL_DEBUG_BASIC, pqisslzone, "pqissltunnel::IncommingHanshakePacket() Notify the pqiperson.... (Both Connect/Receive)");
        parent() -> notifyEvent(this, NET_CONNECT_SUCCESS);
    }
}
/********** Implementation of BinInterface **************************
 * All the rest of the BinInterface.
 *
 */

int 	pqissltunnel::senddata(void *data, int len)
{
#ifdef DEBUG_PQISSL_TUNNEL
        std::cerr << "pqissltunnel::senddata() called" << std::endl ;
#endif
	if (!active) {
            #ifdef DEBUG_PQISSL_TUNNEL
            std::cerr << "pqissltunnel::senddata() connection is not active" << std::endl ;
            #endif
	    return -1;
	}

        int outlen = 0;
        void * out;
        if (!AuthSSL::getAuthSSL()->encrypt(out, outlen, data, len, parent()->PeerId())) {
            std::cerr << "pqissltunnel::senddata() problem while crypting packet, ignoring it." << std::endl;
            return -1;
        }

        #ifdef DEBUG_PQISSL_TUNNEL
        std::cerr << "pqissltunnel::senddata() sending item via p3tunnel" << std::endl ;
        #endif
        mP3tunnel->sendTunnelData(parent()->PeerId(), relayPeerId, out, outlen);

        return len;
}

int 	pqissltunnel::readdata(void *data, int len)
{
#ifdef DEBUG_PQISSL_TUNNEL
	std::cout << "pqissltunnel::readdata() called" << std::endl ;
#endif
	//let's see if we got a new packet to read
        if (current_data_offset >= curent_data_packet.length) {
	    //current packet has finished reading, let's pop out a new packet if available
	    if (data_packet_queue.size() ==0) {
		//no more to read
		return -1;
	    } else {
		//let's read a new packet
		current_data_offset = 0;
                //decrypt one packet from the queue and put it into the current data packet.
                if (!AuthSSL::getAuthSSL()->decrypt(curent_data_packet.data, curent_data_packet.length, data_packet_queue.back().data, data_packet_queue.back().length)) {
                    std::cerr << "pqissltunnel::readdata() problem while decrypting packet, ignoring it." << std::endl;
                    curent_data_packet.length = 0;
                    return -1;
                }
		data_packet_queue.pop_back();
	    }
	}

        if (current_data_offset < curent_data_packet.length) {
#ifdef DEBUG_PQISSL_TUNNEL
        std::cout << "pqissltunnel::readdata() reading..." << std::endl ;
        std::cout << "pqissltunnel::readdata() len : " << len << std::endl ;
        std::cout << "pqissltunnel::readdata() current_data_offset : " << current_data_offset << std::endl ;
        std::cout << "pqissltunnel::readdata() curent_data_packet.length : " << curent_data_packet.length << std::endl ;
        std::cerr << "pqissltunnel::readdata() getRsItemSize(curent_data_packet.data) : " << getRsItemSize(curent_data_packet.data) << std::endl;
#endif

            //read from packet
            memcpy(data, (void*)((unsigned long int)curent_data_packet.data+(unsigned long int)current_data_offset), len);
            current_data_offset += len;

            return len;
	}

	return -1;
}

// dummy function currently.
int 	pqissltunnel::netstatus()
{
	return 1;
}

int 	pqissltunnel::isactive()
{
	return active;
}

bool 	pqissltunnel::moretoread()
{
	//let's see if we got an old packet or a new packet to read
        if (current_data_offset >= curent_data_packet.length && data_packet_queue.size() ==0) {
	    return false;
	} else {
	    return true;
	}
}

bool 	pqissltunnel::cansend()
{
	if (!mLinkMgr->isOnline(relayPeerId)) {
	    reset();
	    return false;
	}
	return true;
}

std::string pqissltunnel::gethash()
{
	std::string dummyhash;
	return dummyhash;
}

/********** End of Implementation of BinInterface ******************/
