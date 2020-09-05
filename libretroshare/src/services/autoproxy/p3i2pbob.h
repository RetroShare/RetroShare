/*******************************************************************************
 * libretroshare/src/services/autoproxy: p3i2pbob.h                            *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2016 by Sehraf                                                    *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#ifndef P3I2PBOB_H
#define P3I2PBOB_H

#include <map>
#include <queue>
#include <sys/types.h>
#include "util/rstime.h"
#ifndef WINDOWS_SYS
	#include <sys/socket.h>
#endif

#include "pqi/p3cfgmgr.h"
#include "services/autoproxy/rsautoproxymonitor.h"
#include "util/rsthreads.h"
#include "util/i2pcommon.h"

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
 *	We use 'RetroShare-' + 8 random base32 characters.
 *
 * Design:
 *	The service uses three state machines to manage its task:
 *		int stateMachineBOB();
 *			mBOBState
 *		int stateMachineController();
 *			mState
 *			mTask
 *
 * stateMachineBOB:
 *	This state machine manages the low level communication with BOB. It basically has a linked
 *	list (currently a implemented as a std::map) that contains a command and the next
 *	state.
 *	Each high level operation (start up / shut down / get keys) is represented by a
 *	chain of states. E.g. the chain to retrieve new keys:
 *		mCommands[bobState::setnickN] = {setnick, bobState::newkeysN};
 *		mCommands[bobState::newkeysN] = {newkeys, bobState::getkeys};
 *		mCommands[bobState::getkeys]  = {getkeys, bobState::clear};
 *		mCommands[bobState::clear]    = {clear,   bobState::quit};
 *		mCommands[bobState::quit]     = {quit,    bobState::cleared};
 *
 * stateMachineController:
 *	This state machine manages the high level tasks.
 *	It is controlled by mState and mTask.
 *
 *		mTast:
 *			Tracks the high level operation (like start up).
 *			It will keep its value even when a task is done to track
 *			the requested BOB state.
 *			When other operations are performed like a conection check
 *			the last task gets backed up and is later restored again
 *
 *		mState:
 *			This state lives only for one operation an manages the communication
 *			with the BOB instance. This is basically connecting, starting BOB
 *			protocol and disconnecting
 *
 * How a task looks like:
 *	1) RS sets task using the ticket system
 *	2) stateMachineController connects to BOBs control port, sets mBobState to a lists head
 *	3) stateMachineBOB processes command chain
 *	4) stateMachineBOB is done and sets mBobState to cleared signaling that the connection
 *		is cleared and can be closed
 *	5) stateMachineController disconnects from BOBs control port and updates mState
 */

///
/// \brief The controllerState enum
/// States for the controller to keep track of what he is currently doing
enum controllerState {
	csIdel,
	csDoConnect,
	csConnected,
	csWaitForBob,
	csDoDisconnect,
	csDisconnected,
	csError
};

///
/// \brief The controllerTask enum
/// This state tracks the controllers tast (e.g. setup a BOB tunnel or shut down
/// an existing one).
enum controllerTask {
	ctIdle,
	ctRunSetUp,
	ctRunShutDown,
	ctRunGetKeys,
	ctRunCheck
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
	bsList, // chain head for 'list' command
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

struct bobSettings : i2p::settings {};

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
	explicit p3I2pBob(p3PeerMgr *peerMgr);

	// autoProxyService interface
public:
	bool isEnabled();
	bool initialSetup(std::string &addr, uint16_t &);
	void processTaskAsync(taskTicket *ticket);
	void processTaskSync(taskTicket *ticket);

	void threadTick() override; /// @see RsTickingThread

private:
	int stateMachineBOB();
	int stateMachineBOB_locked_failure(const std::string &answer, const bobStateInfo &currentState);

	int stateMachineController();
	int stateMachineController_locked_idle();
	int stateMachineController_locked_connected();
	int stateMachineController_locked_disconnected();
	int stateMachineController_locked_error();

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
	// used to store old state when in error state
	// mStateOld is also used as a flag when an error occured in BOB protocol
	controllerState mStateOld;
	// mTaskOld is used to keep the previous task (start up / shut down) when requesting keys or checking the connection
	controllerTask	mTaskOld;
	bobSettings mSetting;
	bobState	mBOBState;

	// used variables
	p3PeerMgr *mPeerMgr;
	bool mConfigLoaded;
	int mSocket;
	rstime_t mLastProxyCheck;
	sockaddr_storage mI2PProxyAddr;
	std::map<bobState, bobStateInfo> mCommands;
	std::string mErrorMsg;
	std::string mTunnelName;

	std::queue<taskTicket *> mPending;
	taskTicket *mProcessing;

	// mutex
	RsMutex mLock;
};

#endif // P3I2PBOB_H
