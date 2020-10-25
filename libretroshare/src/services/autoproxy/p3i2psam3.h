#ifndef P3I2PSAM3_H
#define P3I2PSAM3_H

#include <queue>
#include <list>

#include "services/autoproxy/rsautoproxymonitor.h"
#include "pqi/p3cfgmgr.h"
#include "util/i2pcommon.h"
#include "util/rsthreads.h"

/*
 * This class implements I2P SAMv3 (Simple Anonymous Messaging) to allow RS
 * to automatically setup tunnel to and from I2P.
 * SAMv3 is a simple text-based interface: https://geti2p.net/de/docs/api/samv3
 *
 * For the actual SAM commands / low level stuff libsam3 (https://github.com/i2p/libsam3)
 * is used with some minor adjustments, for exmaple, the FORWARD session is always silent.
 *
 * SAM in a nutshell works like this:
 * 1) setup main/control session which configures everything (destination ID, tunnel number, hops number, and so on)
 * 2) setup a forward session, so that I2P will establish a connection to RS for each incoming connection to our i2p destination
 * 3a) query/lookup the destination (public key) for a given i2p address
 * 3b) connect to the given destination
 *
 * An established connection (both incoming or outgoing) are then handed over to RS.
 * The lifetime of a session (and its subordinates connections) is bound to their tcp socket. When the socket closes, the session is closed, too.
 *
 */

class p3PeerMgr;

// typedef samSession is used to unify access to the session independent of the underlying library
#ifdef RS_USE_I2P_SAM3_I2PSAM
namespace SAM {
class StreamSession;
class I2pSocket;
}

typedef SAM::StreamSession samSession;
#endif
#ifdef RS_USE_I2P_SAM3_LIBSAM3
class Sam3Session;
class Sam3Connection;

typedef Sam3Session samSession;
#endif

struct samSettings : i2p::settings {
	samSession *session;
};

struct samEstablishConnectionWrapper {
	i2p::address address;
#ifdef RS_USE_I2P_SAM3_I2PSAM
	int socket;
#endif
#ifdef RS_USE_I2P_SAM3_LIBSAM3
	Sam3Connection *connection;
#endif
};

struct samStatus {
	std::string sessionName;
	enum samState {
		offline,
		connectSession,
		connectForward,
		online
	} state; // the name is kinda redundant ...
};

class p3I2pSam3 : public RsTickingThread, public p3Config, public autoProxyService
{
public:
	p3I2pSam3(p3PeerMgr *peerMgr);

	// autoProxyService interface
public:
	bool isEnabled();
	bool initialSetup(std::string &addr, uint16_t &port);
	void processTaskAsync(taskTicket *ticket);
	void processTaskSync(taskTicket *ticket);

	// RsTickingThread interface
public:
	void threadTick();	/// @see RsTickingThread

	// p3Config interface
protected:
	RsSerialiser *setupSerialiser();
	bool saveList(bool &cleanup, std::list<RsItem *> &);
	bool loadList(std::list<RsItem *> &load);

private:
	bool startSession();
	bool startForwarding();
	void stopSession();
	void stopForwarding();

	bool generateKey(std::string &pub, std::string &priv);
	void lookupKey(taskTicket *ticket);
	void establishConnection(taskTicket *ticket);
	void closeConnection(taskTicket *ticket);
	void updateSettings_locked();

	bool mConfigLoaded;

	samSettings mSetting;
	p3PeerMgr *mPeerMgr;
	std::queue<taskTicket *> mPending;

	// Used to report the state to the gui
	// (Since the create session call/will can block and there is no easy way from outside the main thread to see
	// what is going on, it is easier to store the current state in an extra variable independen from the main thread)
	samStatus::samState mState;

	// used to keep track of connections, libsam3 does it internally but it can be unreliable since pointers are shared
	std::list<Sam3Connection *> mValidConnections, mInvalidConnections;

	// mutex
	RsMutex mLock;
#ifdef RS_USE_I2P_SAM3_LIBSAM3
	RsMutex mLockSam3Access; // libsam3 is not thread safe! (except for key lookup)
#endif
};

#endif // P3I2PSAM3_H
