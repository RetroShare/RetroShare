/*
 * libretroshare/src/dht: odhtmgr_test.cc
 *
 * Interface with OpenDHT for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#ifndef DHT_CHECK_PEERS_H
#define DHT_CHECK_PEERS_H


/***** Test for the new DHT system *****/

#include "pqi/p3dhtmgr.h"
#include "pqi/p3connmgr.h"
#include "pqi/pqimonitor.h"
#include "dht/opendhtmgr.h"

#include "util/rsnet.h"
#include "util/rsthreads.h"
#include "util/rsprint.h"

#include "tcponudp/tou_net.h"
#include "tcponudp/udpsorter.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <unistd.h>

#define BOOTSTRAP_DEBUG  1

extern "C" void* doStunPeer(void* p);

namespace DHT_CheckPears {


// Function Prototypes
void usage(char *name);
bool stunPeer(struct sockaddr_in toaddr, struct sockaddr_in &ansaddr);


class StunDetails
{
    StunDetails() :
                lastStatus(0),
                lastStunResult(0),
                stunAttemps(0),
                stunResults(0) {}

    std::string id;

    /* peerStatus details */
    struct sockaddr_in laddr, raddr;
    uint32_t type, mode, source;

    /* stun response */
    uint32_t stunAttempts;
    uint32_t stunResults;
    struct sockaddr_in stunaddr;

    /* timestamps */
    time_t lastStatus;
    time_t lastStunResult;
};

// CHANGED: CLASS_TO_STRUCT
struct dhtStunData
{
    pqiConnectCbStun *stunCb;
    std::string id;
    struct sockaddr_in toaddr;
    struct sockaddr_in ansaddr;
};


class StunDetails
{
public:
    StunDetails();
    std::string id;
    /* peerStatus details */
    struct sockaddr_in laddr, raddr;
    uint32_t type, mode, source;

    /* stun response */
    uint32_t stunAttempts;
    uint32_t stunResults;
    struct sockaddr_in stunaddr;

    /* timestamps */
    time_t lastStatus;
    time_t lastStunResult;
};

class pqiConnectCbStun: public pqiConnectCb
{
public:
    pqiConnectCbStun();

    virtual ~pqiConnectCbStun();

    void addPeer(std::string id);
    virtual void peerStatus(std::string id,
                            struct sockaddr_in laddr, struct sockaddr_in raddr,
                            uint32_t type, uint32_t mode, uint32_t source);
    void printPeerStatus();
    void stunPeer(std::string id, struct sockaddr_in peeraddr);

    virtual void peerConnectRequest(std::string id,
                                    struct sockaddr_in raddr, uint32_t source);

    virtual void stunStatus(std::string id, struct sockaddr_in raddr,
                            uint32_t type, uint32_t flags);

    virtual void stunSuccess(std::string id, struct sockaddr_in toaddr,
                             struct sockaddr_in ansaddr);
private:

    RsMutex peerMtx;
    std::map<std::string, StunDetails> peerMap;
};

}
#endif // DHT_CHECK_PEERS_H
