#ifndef P3I2PBOB_H
#define P3I2PBOB_H

#include <map>
#include <queue>
#include <sys/types.h>
#include <sys/socket.h>

#include "services/autoproxy/rsautoproxymonitor.h"
#include "util/rsthreads.h"
#include "pqi/p3cfgmgr.h"

/*
 * This class implements I2P BOB (BASIC OPEN BRIDGE) communication to allow RS
 * to automatically remote control I2P to setup the needed tunnel.
 * BOB is a simple text-based interface: https://geti2p.net/en/docs/api/bob
 *
 * Note 1:
 *	One tunnel is enough even for hidden locations since it can be used
 *	bidirectional. (In contrast to what RS I2P users had to set up manually.)
 *
 * Note 2:
 *	BOB tunnels are no SOCKS tunnel. Therefore pqissli2pbob implements a simplified
 *	proxy specially for BOB tunnels.
 *
 * Note 3:
 *	BOB needs a unique name as an ID for each tunnel.
 *	We use 'RetroShare-' + 8 base32 characters.
 *
 * This service works autonomous and only minimal control is exposed.
 * To interact with this service three function are available (blocking and
 * non-blocking - the blocking ones are wrapper):
 *		bool startUpBOBConnection();
 *		bool startUpBOBConnectionBlocking();
 *		bool shutdownBOBConnection();
 *		bool shutdownBOBConnectionBlocking();
 *		bool getNewKeys();
 *		bool getNewKeysBlocking();
 *
 * startUpBOBConnection:
 *	Instructs the service to establish the BOB tunnel. When server keys are available
 *	they are automatically used. Otherwise a client tunnel is configured and started.
 *
 * shutdownBOBConnection:
 *	Instructs the service to close the BOB tunnel.
 *
 * getNewKeys:
 *	Instructs the service to retrieve new server keys. This is a one-shot and can
 *	(currently) only be used when no bob tunnel is set up.
 *	Server keys are only needed for hidden nodes.
 *	They can be set manually too (e.g. when migrating from manual set up to BOB)
 *
 * Note 4:
 *	Only use the blocking functions /after/ the service thread was started!
 *
 * Design:
 *	The service uses two state machines to manage its task:
 *		int stateMachineBOB();
 *		int stateMachineController();
 *
 * stateMachineBOB:
 *	This state machine manages the low level communication with BOB. It basically has a linked
 *	list (currently a implemented as a std::map) that contains a command and the next
 *	state.
 *	Each high level operation (start up / shut down / get keys) is represented by a
 *	chain (or linked list) of states. E.g. the chain to retrieve new keys:
 *		mCommands[bobState::setnickN] = {setnick, bobState::newkeysN};
 *		mCommands[bobState::newkeysN] = {newkeys, bobState::getkeys};
 *		mCommands[bobState::getkeys]  = {getkeys, bobState::clear};
 *		mCommands[bobState::clear]    = {clear,   bobState::quit};
 *		mCommands[bobState::quit]     = {quit,    bobState::cleared};
 *
 * stateMachineController:
 *	This state machone manages the high level tasks.
 *	It starts and controlls stateMachineBOB.
 *
 * How a taks looks like:
 *	1) RS sets task using the tree functions (see above)
 *	2) stateMachineController connects to BOBs control port, sets mBobState to a chain/list
 *		head and updates mState
 *	3) stateMachineBOB processes chain
 *	4) stateMachineBOB is done and sets mBobState to cleared signaling that the connection
 *		is cleared and can be closed
 *	5) stateMachineController disconnects from BOBs control port and updates mState
 *	6) (optional) blocking functions returns
 *
 * TODO / What is missing:
 *	- Currently it is expected that I2P is not closed or restarted while RS is running.
 *	- Currently issuing a new task while another is running may cause unwanted behaviour.
 *
 */

///
/// \brief The controllerState enum
/// States for the controller to keep track of what he is currently doing
enum controllerState {
	csStarting,
	csStarted,
	csClosing,
	csClosed,
	csGettingKeys,
	csError
};

///
/// \brief The controllerTask enum
/// This state tracks whether the controller should try to setup a BOB tunnel or shut down
/// an existing one.
/// E.g. BOB can be enabled (see bobSettings) while the controller won't set up a BOB tunnel
enum controllerTask {
	ctIdle,
	ctRun,
	ctGetKeys
};

///
/// \brief The bobState enum
/// One state for each message
///
enum bobState {
	bsCleared,
	bsSetnickC, // chain head for only client tunnel
	bsSetnickN, // chain head for getting new (server) keys
	bsSetnickS, // chain head for client and server tunnel
	bsGetnick,
	bsNewkeysC, // part of chain for only client tunnel
	bsNewkeysN, // part of chain for getting new (server) keys
	bsGetkeys,
	bsSetkeys,
	bsInhost,
	bsOuthost,
	bsInport,
	bsOutport,
	bsInlength,
	bsOutlength,
	bsInvariance,
	bsOutvariance,
	bsInquantity,
	bsOutquantity,
	bsQuiet,
	bsStart,
	bsStop,
	bsClear,
	bsQuit
};

///
/// \brief The bobStateInfo struct
/// State machine with commands
/// \todo This could be replaced by a linked list instead of a map
struct bobStateInfo {
	std::string command;
	bobState    nextState;
};

struct bobSettings {
	bool enableBob;		///< This field is used by the pqi subsystem to determinine whether SOCKS proxy or BOB is used for I2P connections
	std::string keys;	///< (optional) server keys
	std::string addr;	///< (optional) hidden service addr. in base32 form

	int8_t inLength;
	int8_t inQuantity;
	int8_t inVariance;

	int8_t outLength;
	int8_t outQuantity;
	int8_t outVariance;
};

///
/// \brief The bobStates struct
/// This container struct is used to pass all states.
/// Additionally, the tunnel name is included to to show it in the GUI.
/// The advantage of a struct is that it can be forward declared.
struct bobStates {
	bobState bs;
	controllerState cs;
	controllerTask ct;

	std::string tunnelName;
};

class p3PeerMgr;

class p3I2pBob : public RsTickingThread, public p3Config, public autoProxyService
{
public:
	p3I2pBob(p3PeerMgr *peerMgr);

	// autoProxyService interface
public:
	bool isEnabled();
	bool initialSetup(std::string &addr, uint16_t &);
	void processTaskAsync(taskTicket *ticket);
	void processTaskSync(taskTicket *ticket);

	static std::string keyToBase32Addr(const std::string &key);

	// RsTickingThread interface
public:
	void data_tick();

private:
	int stateMachineBOB();
	int stateMachineController();

	// p3Config interface
protected:
	RsSerialiser *setupSerialiser();
	bool saveList(bool &cleanup, std::list<RsItem *> &lst);
	bool loadList(std::list<RsItem *> &load);

private:
	// helpers
	void getBOBSettings(bobSettings *settings);
	void setBOBSettings(const bobSettings *settings);
	void getStates(bobStates *bs);

	std::string executeCommand(const std::string &command);
	bool connectI2P();
	bool disconnectI2P();

	void finalizeSettings_locked();
	void updateSettings_locked();

	std::string recv();

	// states for state machines
	controllerState mState;
	controllerTask	mTask;
	bobSettings mSetting;
	bobState	mBOBState;

	// used variables
	p3PeerMgr *mPeerMgr;
	bool mConfigLoaded;
	int mSocket;
	sockaddr_storage mI2PProxyAddr;
	std::map<bobState, bobStateInfo> mCommands;
	std::string mErrorMsg;

	std::queue<taskTicket *> mPending;
	taskTicket *mProcessing;

	// mutex
	RsMutex mLock;
};

#endif // P3I2PBOB_H
