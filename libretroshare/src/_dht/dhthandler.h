#ifndef _RS_DHT_IFACE_H
#define _RS_DHT_IFACE_H


/* This stuff is actually C */

#ifdef  __cplusplus
extern "C" {
#endif

#include <int128.h>
#include <KadCapi.h>

#ifdef  __cplusplus
} /* extern C */
#endif
/* This stuff is actually C */

#include <string>
#include <map>
#include <string.h>
#include <iostream>
#include <sstream>

/* platform independent networking... */
#include "pqi/pqinetwork.h"
#include "pqi/pqiaddrstore.h"
#include "util/rsthreads.h"

/* HACK TO SWITCH THIS OFF during testing */
/*define  NO_DHT_RUNNING  1*/


std::ostream &operator<<(std::ostream &out, dhtentry &ent);
void cleardhtentry(dhtentry *ent, std::string id);
void initdhtentry(dhtentry *ent);
void founddhtentry(dhtentry *ent, struct sockaddr_in inaddr, unsigned int flags);

// CHANGED: CLASS_TO_STRUCT
struct dhtentry
{
    std::string name;
    std::string id;
    struct sockaddr_in addr;
    unsigned int flags;
    int status;
    int lastTs;
};
	
class dhthandler: public RsThread, public pqiAddrStore
{
public:

    dhthandler(std::string inifile);
    ~dhthandler();

    /* RsIface */
    /* set own tag */
    void setOwnHash(std::string id);
    void setOwnPort(short port);
    bool getExtAddr(sockaddr_in &addr, unsigned int &flags);

    /* at startup */
    void addFriend(std::string id);
    void removeFriend(std::string id);
    int dhtPeers();

    /* pqiAddrStore ... called prior to connect */
    virtual bool    addrFriend(std::string id, struct sockaddr_in &addr, unsigned int &flags);

    int init();
    int shutdown();
    int	print();

    /* must run thread */
    virtual void run();

private:

    int write_inifile();

    bool networkUp(); /* get status */

    int checkOwnStatus();
    int checkPeerIds();
    int publishOwnId();
    int searchId(std::string id);

    dhtentry *finddht(std::string id);

    /* Mutex for data below */
    RsMutex dataMtx;

    dhtentry ownId;
    std::map<std::string, dhtentry> addrs;

    KadCcontext kcc, *pkcc;
    KadC_status kcs;
    std::string kadcFile;
    bool mShutdown;

    bool dhtOk;
};



#endif /* _RS_DHT_IFACE_H */
