#ifndef DHT_BOOTSTRAP_H
#define DHT_BOOTSTRAP_H

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

namespace DHT_Bootstrap {

// Function Prototypes
void usage(char *name);
void	addPeer(std::string id);
extern "C" void* doStunPeer(void* p);

class pqiConnectCbStun;

// CHANGED: REMOVED : Reason -> not implemented
//void loadBootStrapIds(std::list<std::string> &peerIds);
//bool stunPeer(struct sockaddr_in toaddr, struct sockaddr_in &ansaddr);

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


};
#endif // DHT_BOOTSTRAP_H
