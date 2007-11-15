#ifndef _RS_DHT_IFACE_H
#define _RS_DHT_IFACE_H

#include <string.h>

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


#include "util/rsthreads.h"
#include <string>
#include <map>

/* platform independent networking... */
#include "pqi/pqinetwork.h"
#include "pqi/pqiaddrstore.h"

class dhtentry
{
	public:
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
void    setOwnHash(std::string id);
void    setOwnPort(short port);
bool 	getExtAddr(sockaddr_in &addr, unsigned int &flags);

	/* at startup */
void    addFriend(std::string id);
void    removeFriend(std::string id);
int     dhtPeers();

	/* pqiAddrStore ... called prior to connect */
virtual bool    addrFriend(std::string id, struct sockaddr_in &addr, unsigned int &flags);

int 	init();
int 	shutdown();
int	print();

	/* must run thread */
virtual void run();

	private:

	int write_inifile();

	bool 	networkUp(); /* get status */

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
